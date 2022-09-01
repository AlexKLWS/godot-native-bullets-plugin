#include <Godot.hpp>
#include <VisualServer.hpp>
#include <PhysicsServer.hpp>
#include <Viewport.hpp>
#include <World.hpp>
#include <Font.hpp>

#include "unit_pool.h"

using namespace godot;

//-- START Default "standard" implementations.

template <class Kit, class UnitType>
void AbstractUnitPool<Kit, UnitType>::_init_unit(UnitType *bullet) {}

template <class Kit, class UnitType>
void AbstractUnitPool<Kit, UnitType>::_enable_unit(UnitType *bullet)
{
	bullet->lifetime = 0.0f;

	VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, true);
}

template <class Kit, class UnitType>
void AbstractUnitPool<Kit, UnitType>::_disable_unit(UnitType *bullet)
{
	VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, false);
}

template <class Kit, class UnitType>
bool AbstractUnitPool<Kit, UnitType>::_process_unit(UnitType *bullet, float delta)
{
	bullet->transform.set_origin(bullet->transform.get_origin() + bullet->velocity * delta);

	if (bullet->lifetime >= kit->max_lifetime)
	{
		return true;
	}

	bullet->lifetime += delta;
	return false;
}

//-- END Default "standard" implementation.

template <class Kit, class UnitType>
AbstractUnitPool<Kit, UnitType>::~AbstractUnitPool()
{
	// Bullets node is responsible for clearing all the area and area shapes
	for (int32_t i = 0; i < pool_size; i++)
	{
		VisualServer::get_singleton()->free_rid(bullets[i]->item_rid);
		bullets[i]->free();
	}

	delete[] bullets;
	delete[] shapes_to_indices;
}

template <class Kit, class UnitType>
void AbstractUnitPool<Kit, UnitType>::_init(Spatial *bullet_parent, RID shared_area, int32_t starting_shape_index,
																						int32_t set_index, Ref<UnitKit> kit, int32_t pool_size, int32_t z_index)
{

	// Check if collisions are enabled and if layer or mask are != 0,
	// otherwise the bullets would not collide with anything anyways.
	this->collisions_enabled = kit->collisions_enabled && kit->collision_shape.is_valid() &&
														 ((int64_t)kit->collision_layer + (int64_t)kit->collision_mask) != 0;
	this->bullet_parent = bullet_parent;
	this->shared_area = shared_area;
	this->starting_shape_index = starting_shape_index;
	this->kit = kit;
	this->pool_size = pool_size;
	this->set_index = set_index;

	available_units = pool_size;
	active_units = 0;

	bullets = new UnitType *[pool_size];
	shapes_to_indices = new int32_t[pool_size];

	Transform default_transform = Transform(Basis(), Vector3(0.0f, 0.0f, 0.0f));

	for (int32_t i = 0; i < pool_size; i++)
	{
		UnitType *bullet = UnitType::_new();
		bullets[i] = bullet;

		bullet->item_rid = VisualServer::get_singleton()->instance_create();
		VisualServer::get_singleton()->instance_set_scenario(bullet->item_rid, bullet_parent->get_world()->get_scenario());
		VisualServer::get_singleton()->instance_set_base(bullet->item_rid, kit->mesh->get_rid());
		VisualServer::get_singleton()->instance_geometry_set_material_override(bullet->item_rid, kit->material->get_rid());
		VisualServer::get_singleton()->instance_set_transform(bullet->item_rid, default_transform);
		VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, false);

		if (collisions_enabled)
		{
			RID shared_shape_rid = kit->collision_shape->get_rid();

			PhysicsServer::get_singleton()->area_add_shape(shared_area, shared_shape_rid, Transform(), true);
			bullet->shape_index = starting_shape_index + i;
			shapes_to_indices[i] = i;
		}

		_init_unit(bullet);
	}
}

template <class Kit, class UnitType>
int32_t AbstractUnitPool<Kit, UnitType>::_process(float delta)
{

	int32_t amount_variation = 0;

	if (collisions_enabled)
	{
		for (int32_t i = pool_size - 1; i >= available_units; i--)
		{
			UnitType *bullet = bullets[i];

			if (_process_unit(bullet, delta))
			{
				_release_unit(i);
				amount_variation -= 1;
				i += 1;
				continue;
			}

			VisualServer::get_singleton()->instance_set_transform(bullet->item_rid, bullet->transform);
			PhysicsServer::get_singleton()->area_set_shape_transform(shared_area, bullet->shape_index, bullet->transform);
		}
	}
	else
	{
		for (int32_t i = pool_size - 1; i >= available_units; i--)
		{
			UnitType *bullet = bullets[i];

			if (_process_unit(bullet, delta))
			{
				_release_unit(i);
				amount_variation -= 1;
				i += 1;
				continue;
			}

			VisualServer::get_singleton()->instance_set_transform(bullet->item_rid, bullet->transform);
		}
	}
	return amount_variation;
}

template <class Kit, class UnitType>
void AbstractUnitPool<Kit, UnitType>::spawn_unit(Dictionary properties)
{
	if (available_units > 0)
	{
		available_units -= 1;
		active_units += 1;

		UnitType *bullet = bullets[available_units];

		if (collisions_enabled)
			PhysicsServer::get_singleton()->area_set_shape_disabled(shared_area, bullet->shape_index, false);

		Array keys = properties.keys();
		for (int32_t i = 0; i < keys.size(); i++)
		{
			bullet->set(keys[i], properties[keys[i]]);
		}

		VisualServer::get_singleton()->instance_set_transform(bullet->item_rid, bullet->transform);
		if (collisions_enabled)
			PhysicsServer::get_singleton()->area_set_shape_transform(shared_area, bullet->shape_index, bullet->transform);

		_enable_unit(bullet);
	}
}

template <class Kit, class UnitType>
BulletID AbstractUnitPool<Kit, UnitType>::obtain_unit()
{
	if (available_units > 0)
	{
		available_units -= 1;
		active_units += 1;

		UnitType *bullet = bullets[available_units];

		if (collisions_enabled)
			PhysicsServer::get_singleton()->area_set_shape_disabled(shared_area, bullet->shape_index, false);

		_enable_unit(bullet);

		return BulletID(bullet->shape_index, bullet->cycle, set_index);
	}
	return BulletID(-1, -1, -1);
}

template <class Kit, class UnitType>
bool AbstractUnitPool<Kit, UnitType>::release_unit(BulletID id)
{
	if (id.index >= starting_shape_index && id.index < starting_shape_index + pool_size && id.set == set_index)
	{
		int32_t bullet_index = shapes_to_indices[id.index - starting_shape_index];
		if (bullet_index >= available_units && bullet_index < pool_size && id.cycle == bullets[bullet_index]->cycle)
		{
			_release_unit(bullet_index);
			return true;
		}
	}
	return false;
}

template <class Kit, class UnitType>
PoolVector3Array AbstractUnitPool<Kit, UnitType>::release_all_units()
{
	PoolVector3Array result = PoolVector3Array();
	if (kit->is_clearable)
	{
		for (int32_t i = pool_size - 1; i >= available_units; i--)
		{
			Vector3 released_unit_pos = _release_unit(i);
			result.append(released_unit_pos);
			i += 1;
		}
	}
	return result;
}

template <class Kit, class UnitType>
PoolVector3Array AbstractUnitPool<Kit, UnitType>::release_all_units_in_radius(Vector3 from, float distance_squared)
{
	PoolVector3Array result = PoolVector3Array();
	if (kit->is_clearable)
	{
		for (int32_t i = pool_size - 1; i >= available_units; i--)
		{
			UnitType *bullet = bullets[i];
			Transform bullet_transform = bullet->transform;
			if (bullet_transform.get_origin().distance_squared_to(from) < distance_squared)
			{
				Vector3 released_unit_pos = _release_unit(i);
				result.append(released_unit_pos);
				i += 1;
			}
		}
	}
	return result;
}

template <class Kit, class UnitType>
Vector3 AbstractUnitPool<Kit, UnitType>::_release_unit(int32_t index)
{
	UnitType *bullet = bullets[index];
	Transform bullet_transform = bullet->transform;

	if (collisions_enabled)
		PhysicsServer::get_singleton()->area_set_shape_disabled(shared_area, bullet->shape_index, true);

	_disable_unit(bullet);
	bullet->cycle += 1;

	_swap(shapes_to_indices[bullet->shape_index - starting_shape_index], shapes_to_indices[bullets[available_units]->shape_index - starting_shape_index]);
	_swap(bullets[index], bullets[available_units]);

	available_units += 1;
	active_units -= 1;

	return bullet_transform.get_origin();
}

template <class Kit, class UnitType>
bool AbstractUnitPool<Kit, UnitType>::is_unit_valid(BulletID id)
{
	if (id.index >= starting_shape_index && id.index < starting_shape_index + pool_size && id.set == set_index)
	{
		int32_t bullet_index = shapes_to_indices[id.index - starting_shape_index];
		if (bullet_index >= available_units && bullet_index < pool_size && id.cycle == bullets[bullet_index]->cycle)
		{
			return true;
		}
	}
	return false;
}

template <class Kit, class UnitType>
bool AbstractUnitPool<Kit, UnitType>::does_unit_exist(int32_t shape_index)
{
	if (shape_index >= starting_shape_index && shape_index < starting_shape_index + pool_size)
	{
		int32_t bullet_index = shapes_to_indices[shape_index - starting_shape_index];
		if (bullet_index >= available_units)
		{
			return true;
		}
	}
	return false;
}

template <class Kit, class UnitType>
BulletID AbstractUnitPool<Kit, UnitType>::get_unit_from_shape(int32_t shape_index)
{
	if (shape_index >= starting_shape_index && shape_index < starting_shape_index + pool_size)
	{
		int32_t bullet_index = shapes_to_indices[shape_index - starting_shape_index];
		if (bullet_index >= available_units)
		{
			return BulletID(shape_index, bullets[bullet_index]->cycle, set_index);
		}
	}
	return BulletID(-1, -1, -1);
}

template <class Kit, class UnitType>
void AbstractUnitPool<Kit, UnitType>::set_unit_property(BulletID id, String property, Variant value)
{
	if (is_unit_valid(id))
	{
		int32_t bullet_index = shapes_to_indices[id.index - starting_shape_index];
		bullets[bullet_index]->set(property, value);

		if (property == "transform")
		{
			UnitType *bullet = bullets[bullet_index];
			VisualServer::get_singleton()->instance_set_transform(bullet->item_rid, bullet->transform);
			if (collisions_enabled)
				PhysicsServer::get_singleton()->area_set_shape_transform(shared_area, bullet->shape_index, bullet->transform);
		}
	}
}

template <class Kit, class UnitType>
Variant AbstractUnitPool<Kit, UnitType>::get_unit_property(BulletID id, String property)
{
	if (is_unit_valid(id))
	{
		int32_t bullet_index = shapes_to_indices[id.index - starting_shape_index];

		return bullets[bullet_index]->get(property);
	}
	return Variant();
}