#include <VisualServer.hpp>
#include <PhysicsServer.hpp>
#include <World.hpp>
#include <Viewport.hpp>
#include <OS.hpp>
#include <Engine.hpp>
#include <Font.hpp>
#include <RegExMatch.hpp>

#include "bullets.h"

using namespace godot;

void Bullets::_register_methods()
{
	register_method("_physics_process", &Bullets::_physics_process);

	register_method("mount", &Bullets::mount);
	register_method("unmount", &Bullets::unmount);
	register_method("get_bullets_environment", &Bullets::get_bullets_environment);

	register_method("spawn_bullet", &Bullets::spawn_bullet);
	register_method("spawn_multiple_units", &Bullets::spawn_multiple_units);
	register_method("obtain_bullet", &Bullets::obtain_bullet);
	register_method("release_bullet", &Bullets::release_bullet);
	register_method("release_all_units", &Bullets::release_all_units);
	register_method("release_all_units_in_radius", &Bullets::release_all_units_in_radius);

	register_method("is_bullet_valid", &Bullets::is_bullet_valid);
	register_method("is_kit_valid", &Bullets::is_kit_valid);

	register_method("get_available_bullets", &Bullets::get_available_bullets);
	register_method("get_active_bullets", &Bullets::get_active_bullets);
	register_method("get_pool_size", &Bullets::get_pool_size);
	register_method("get_z_index", &Bullets::get_z_index);

	register_method("get_total_available_bullets", &Bullets::get_total_available_bullets);
	register_method("get_total_active_bullets", &Bullets::get_total_active_bullets);

	register_method("is_bullet_existing", &Bullets::is_bullet_existing);
	register_method("get_bullet_from_shape", &Bullets::get_bullet_from_shape);
	register_method("get_kit_from_bullet", &Bullets::get_kit_from_bullet);

	register_method("set_bullet_property", &Bullets::set_bullet_property);
	register_method("get_bullet_property", &Bullets::get_bullet_property);
}

Bullets::Bullets() {}

Bullets::~Bullets()
{
	_clear_rids();
}

void Bullets::_init()
{
	available_bullets = 0;
	active_bullets = 0;
	total_bullets = 0;
	invalid_id = PoolIntArray();
	invalid_id.resize(3);
	invalid_id.set(0, -1);
	invalid_id.set(1, -1);
	invalid_id.set(2, -1);
}

void Bullets::_physics_process(float delta)
{
	if (Engine::get_singleton()->is_editor_hint())
	{
		return;
	}
	int32_t bullets_variation = 0;

	for (int32_t i = 0; i < pool_sets.size(); i++)
	{
		for (int32_t j = 0; j < pool_sets[i].pools.size(); j++)
		{
			bullets_variation = pool_sets[i].pools[j].pool->_process(delta);
			available_bullets -= bullets_variation;
			active_bullets += bullets_variation;
		}
	}
}

void Bullets::_clear_rids()
{
	for (int32_t i = 0; i < shared_areas.size(); i++)
	{
		PhysicsServer::get_singleton()->area_clear_shapes(shared_areas[i]);
		PhysicsServer::get_singleton()->free_rid(shared_areas[i]);
	}
}

int32_t Bullets::_get_pool_index(int32_t set_index, int32_t bullet_index)
{
	if (bullet_index >= 0 && set_index >= 0 && set_index < pool_sets.size() && bullet_index < pool_sets[set_index].bullets_amount)
	{
		int32_t pool_threshold = pool_sets[set_index].pools[0].size;
		int32_t pool_index = 0;

		while (bullet_index >= pool_threshold)
		{
			pool_index++;
			pool_threshold += pool_sets[set_index].pools[pool_index].size;
		}
		if (pool_index < pool_sets[set_index].pools.size())
		{
			return pool_index;
		}
	}
	return -1;
}

void Bullets::mount(Spatial *bullets_environment)
{
	if (bullets_environment == nullptr || this->bullets_environment == bullets_environment)
	{
		return;
	}
	if (this->bullets_environment != nullptr)
	{
		this->bullets_environment->set("current", false);
	}

	this->bullets_environment = bullets_environment;
	this->bullets_environment->set("current", true);

	VisualServer::get_singleton()->instance_set_scenario(this, bullets_environment->get_world()->get_scenario());

	Array unit_kits = bullets_environment->get("unit_kits");
	Array pools_sizes = bullets_environment->get("pools_sizes");
	Array z_indices = bullets_environment->get("z_indices");

	pool_sets.clear();
	areas_to_pool_set_indices.clear();
	kits_to_set_pool_indices.clear();
	_clear_rids();
	shared_areas.clear();

	available_bullets = 0;
	active_bullets = 0;

	Dictionary collision_layers_masks_to_kits;

	for (int32_t i = 0; i < unit_kits.size(); i++)
	{
		Ref<UnitKit> kit = unit_kits[i];
		if (!kit->is_valid())
		{
			ERR_PRINT("UnitKit is not valid!");
			continue;
		}
		// By default add the the UnitKit to a no-collisions list. (layer and mask = 0)
		int64_t layer_mask = 0;
		if (kit->collisions_enabled && kit->collision_shape.is_valid())
		{
			// If collisions are enabled, add the UnitKit to another list.
			layer_mask = (int64_t)kit->collision_layer + ((int64_t)kit->collision_mask << 32);
		}
		if (collision_layers_masks_to_kits.has(layer_mask))
		{
			collision_layers_masks_to_kits[layer_mask].operator Array().append(kit);
		}
		else
		{
			Array array = Array();
			array.append(kit);
			collision_layers_masks_to_kits[layer_mask] = array;
		}
	}
	// Create the PoolKitSets array. If they exist, a set will be allocated for no-collisions pools.
	pool_sets.resize(collision_layers_masks_to_kits.size());

	Array layer_mask_keys = collision_layers_masks_to_kits.keys();
	for (int32_t i = 0; i < layer_mask_keys.size(); i++)
	{
		Array kits = collision_layers_masks_to_kits[layer_mask_keys[i]];
		Ref<UnitKit> first_kit = kits[0];

		pool_sets[i].pools.resize(kits.size());

		RID shared_area = RID();
		if (layer_mask_keys[i].operator int64_t() != 0)
		{
			// This is a collisions-enabled set, create the shared area.
			shared_area = PhysicsServer::get_singleton()->area_create();
			PhysicsServer::get_singleton()->area_set_collision_layer(shared_area, first_kit->collision_layer);
			PhysicsServer::get_singleton()->area_set_collision_mask(shared_area, first_kit->collision_mask);
			PhysicsServer::get_singleton()->area_set_monitorable(shared_area, true);
			PhysicsServer::get_singleton()->area_set_space(shared_area, bullets_environment->get_world()->get_space());

			shared_areas.append(shared_area);
			areas_to_pool_set_indices[shared_area] = i;
		}
		int32_t pool_set_available_bullets = 0;

		for (int32_t j = 0; j < kits.size(); j++)
		{
			Ref<UnitKit> kit = kits[j];

			PoolIntArray set_pool_indices = PoolIntArray();
			set_pool_indices.resize(2);
			set_pool_indices.set(0, i);
			set_pool_indices.set(1, j);
			kits_to_set_pool_indices[kit] = set_pool_indices;

			int32_t kit_index_in_node = unit_kits.find(kit);
			int32_t pool_size = pools_sizes[kit_index_in_node];

			pool_sets[i].pools[j].pool = kit->_create_pool();
			pool_sets[i].pools[j].unit_kit = kit;
			pool_sets[i].pools[j].size = pool_size;
			pool_sets[i].pools[j].z_index = z_indices[kit_index_in_node];

			pool_sets[i].pools[j].pool->_init(this, shared_area, pool_set_available_bullets,
																				i, kit, pool_size, z_indices[kit_index_in_node]);

			pool_set_available_bullets += pool_size;
		}
		pool_sets[i].bullets_amount = pool_set_available_bullets;
		available_bullets += pool_set_available_bullets;
	}
	total_bullets = available_bullets;
}

void Bullets::unmount(Spatial *bullets_environment)
{
	if (this->bullets_environment == bullets_environment)
	{
		pool_sets.clear();
		areas_to_pool_set_indices.clear();
		kits_to_set_pool_indices.clear();
		_clear_rids();
		shared_areas.clear();

		available_bullets = 0;
		active_bullets = 0;
		total_bullets = 0;

		this->bullets_environment = nullptr;
	}
	if (bullets_environment != nullptr)
	{
		bullets_environment->set("current", false);
	}
}

Spatial *Bullets::get_bullets_environment()
{
	return bullets_environment;
}

bool Bullets::spawn_bullet(Ref<UnitKit> kit, Dictionary properties)
{
	if (available_bullets > 0 && kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit].operator PoolIntArray();
		UnitPool *pool = pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool.get();

		if (pool->get_available_units() > 0)
		{
			available_bullets -= 1;
			active_bullets += 1;

			pool->spawn_unit(properties);
			return true;
		}
	}
	return false;
}

int32_t Bullets::spawn_multiple_units(Ref<UnitKit> kit, Array set_of_dictionaries)
{
	int32_t successful_spawn_count = 0;
	if (available_bullets > 0 && kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit].operator PoolIntArray();
		UnitPool *pool = pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool.get();
		for (int32_t i = 0; i < set_of_dictionaries.size(); i++)
		{
			if (pool->get_available_units() > 0)
			{
				available_bullets -= 1;
				active_bullets += 1;

				pool->spawn_unit(set_of_dictionaries[i]);
				successful_spawn_count++;
			}
		}
	}
	return successful_spawn_count;
}

Variant Bullets::obtain_bullet(Ref<UnitKit> kit)
{
	if (available_bullets > 0 && kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit].operator PoolIntArray();
		UnitPool *pool = pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool.get();

		if (pool->get_available_units() > 0)
		{
			available_bullets -= 1;
			active_bullets += 1;

			BulletID bullet_id = pool->obtain_unit();
			PoolIntArray to_return = invalid_id;
			to_return.set(0, bullet_id.index);
			to_return.set(1, bullet_id.cycle);
			to_return.set(2, bullet_id.set);
			return to_return;
		}
	}
	return invalid_id;
}

bool Bullets::release_bullet(Variant id)
{
	PoolIntArray bullet_id = id.operator PoolIntArray();
	bool result = false;

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if (pool_index >= 0)
	{
		result = pool_sets[bullet_id[2]].pools[pool_index].pool->release_unit(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]));
		if (result)
		{
			available_bullets += 1;
			active_bullets -= 1;
		}
	}
	return result;
}

PoolVector3Array Bullets::release_all_units()
{
	PoolVector3Array result = PoolVector3Array();
	if (available_bullets > 0)
	{
		for (int32_t x = 0; x < pool_sets.size(); ++x)
		{
			for (int32_t y = 0; y < pool_sets[x].pools.size(); ++y)
			{
				UnitPool *pool = pool_sets[x].pools[y].pool.get();
				if (pool->get_available_units() > 0)
				{
					PoolVector3Array released_units_positions = pool->release_all_units();
					int size = released_units_positions.size();
					if (size > 0)
					{
						available_bullets += size;
						active_bullets -= size;
						result.append_array(released_units_positions);
					}
				}
			}
		}
	}
	return result;
}

PoolVector3Array Bullets::release_all_units_in_radius(Vector3 from, float distance)
{
	PoolVector3Array result = PoolVector3Array();
	if (available_bullets > 0)
	{
		float distance_squared = distance * distance;
		for (int32_t x = 0; x < pool_sets.size(); ++x)
		{
			for (int32_t y = 0; y < pool_sets[x].pools.size(); ++y)
			{
				UnitPool *pool = pool_sets[x].pools[y].pool.get();
				if (pool->get_available_units() > 0)
				{
					PoolVector3Array released_units_positions = pool->release_all_units_in_radius(from, distance_squared);
					int size = released_units_positions.size();
					if (size > 0)
					{
						available_bullets += size;
						active_bullets -= size;
						result.append_array(released_units_positions);
					}
				}
			}
		}
	}
	return result;
}

bool Bullets::is_bullet_valid(Variant id)
{
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if (pool_index >= 0)
	{
		return pool_sets[bullet_id[2]].pools[pool_index].pool->is_unit_valid(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]));
	}
	return false;
}

bool Bullets::is_kit_valid(Ref<UnitKit> kit)
{
	return kits_to_set_pool_indices.has(kit);
}

int32_t Bullets::get_available_bullets(Ref<UnitKit> kit)
{
	if (kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool->get_available_units();
	}
	return 0;
}

int32_t Bullets::get_active_bullets(Ref<UnitKit> kit)
{
	if (kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool->get_active_units();
	}
	return 0;
}

int32_t Bullets::get_pool_size(Ref<UnitKit> kit)
{
	if (kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].size;
	}
	return 0;
}

int32_t Bullets::get_z_index(Ref<UnitKit> kit)
{
	if (kits_to_set_pool_indices.has(kit))
	{
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].z_index;
	}
	return 0;
}

int32_t Bullets::get_total_available_bullets()
{
	return available_bullets;
}

int32_t Bullets::get_total_active_bullets()
{
	return active_bullets;
}

bool Bullets::is_bullet_existing(RID area_rid, int32_t shape_index)
{
	if (!areas_to_pool_set_indices.has(area_rid))
	{
		return false;
	}
	int32_t set_index = areas_to_pool_set_indices[area_rid];
	int32_t pool_index = _get_pool_index(set_index, shape_index);
	if (pool_index >= 0)
	{
		return pool_sets[set_index].pools[pool_index].pool->does_unit_exist(shape_index);
	}
	return false;
}

Variant Bullets::get_bullet_from_shape(RID area_rid, int32_t shape_index)
{
	if (!areas_to_pool_set_indices.has(area_rid))
	{
		return invalid_id;
	}
	int32_t set_index = areas_to_pool_set_indices[area_rid];
	int32_t pool_index = _get_pool_index(set_index, shape_index);
	if (pool_index >= 0)
	{
		BulletID result = pool_sets[set_index].pools[pool_index].pool->get_unit_from_shape(shape_index);

		PoolIntArray to_return = invalid_id;
		to_return.set(0, result.index);
		to_return.set(1, result.cycle);
		to_return.set(2, result.set);
		return to_return;
	}
	return invalid_id;
}

Ref<UnitKit> Bullets::get_kit_from_bullet(Variant id)
{
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if (pool_index >= 0 && pool_sets[bullet_id[2]].pools[pool_index].pool->is_unit_valid(BulletID(bullet_id[0], bullet_id[1], bullet_id[2])))
	{
		return pool_sets[bullet_id[2]].pools[pool_index].unit_kit;
	}
	return Ref<UnitKit>();
}

void Bullets::set_bullet_property(Variant id, String property, Variant value)
{
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if (pool_index >= 0)
	{
		pool_sets[bullet_id[2]].pools[pool_index].pool->set_unit_property(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]), property, value);
	}
}

Variant Bullets::get_bullet_property(Variant id, String property)
{
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if (pool_index >= 0)
	{
		return pool_sets[bullet_id[2]].pools[pool_index].pool->get_unit_property(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]), property);
	}
	return Variant();
}