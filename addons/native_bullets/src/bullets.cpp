#include <VisualServer.hpp>
#include <Physics2DServer.hpp>
#include <World2D.hpp>
#include <Viewport.hpp>
#include <OS.hpp>
#include <Engine.hpp>
#include <Font.hpp>
#include <RegExMatch.hpp>

#include "bullets.h"

using namespace godot;


void Bullets::_register_methods() {
	register_method("_ready", &Bullets::_ready);
	register_method("_physics_process", &Bullets::_physics_process);

	register_method("_get", &Bullets::_get);
	register_method("_set", &Bullets::_set);
	register_method("_get_property_list", &Bullets::_get_property_list);

	register_method("spawn_bullet", &Bullets::spawn_bullet);
	register_method("obtain_bullet", &Bullets::obtain_bullet);
	register_method("release_bullet", &Bullets::release_bullet);

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

	register_property<Bullets, Array>("bullet_kits", &Bullets::bullet_kits, Array(),
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "BulletKit");
	register_property<Bullets, Array>("pools_sizes", &Bullets::pools_sizes, Array(),
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "1,100000");
	register_property<Bullets, Array>("z_indices", &Bullets::z_indices, Array(),
		GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "-128,128");
}

Bullets::Bullets() { }

Bullets::~Bullets() {
	if(Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	for(int32_t i = 0; i < shared_areas.size(); i++) {
		Physics2DServer::get_singleton()->area_clear_shapes(shared_areas[i]);
		Physics2DServer::get_singleton()->free_rid(shared_areas[i]);
	}
}

void Bullets::_init() {
	available_bullets = 0;
	active_bullets = 0;
	invalid_id = PoolIntArray();
	invalid_id.resize(3);
	invalid_id.set(0, -1);
	invalid_id.set(1, -1);
	invalid_id.set(2, -1);

	prop_regex = Ref<RegEx>(RegEx::_new());
	prop_regex->compile("bullet_types/\\d*/\\w*");
}

void Bullets::_ready() {
	if(Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	areas_to_pool_set_indices.clear();
	kits_to_set_pool_indices.clear();
	shared_areas.clear();

	available_bullets = 0;
	active_bullets = 0;

	Dictionary collision_layers_masks_to_kits;
	
	for(int32_t i = 0; i < bullet_kits.size(); i++) {
		Ref<BulletKit> kit = Object::cast_to<BulletKit>(bullet_kits[i]);
		// By default add the the BulletKit to a no-collisions list. (layer and mask = 0)
		int64_t layer_mask = 0;
		if(kit->collisions_enabled) {
			// If collisions are enabled, add the BulletKit to another list.
			layer_mask = (int64_t)kit->collision_layer + ((int64_t)kit->collision_mask << 32);
		}
		if(collision_layers_masks_to_kits.has(layer_mask)) {
			collision_layers_masks_to_kits[layer_mask].operator Array().append(kit);
		}
		else {
			Array array = Array();
			array.append(kit);
			collision_layers_masks_to_kits[layer_mask] = array;
		}
	}
	// Create the PoolKitSets array. If they exist, a set will be allocated for no-collisions pools.
	pool_sets.resize(collision_layers_masks_to_kits.size());
	
	Array layer_mask_keys = collision_layers_masks_to_kits.keys();
	for(int32_t i = 0; i < layer_mask_keys.size(); i++) {
		Array kits = collision_layers_masks_to_kits[layer_mask_keys[i]];
		Ref<BulletKit> first_kit = Object::cast_to<BulletKit>(kits[0]);

		pool_sets[i].pools.resize(kits.size());

		RID shared_area = RID();
		if(layer_mask_keys[i].operator signed int() != 0) {
			// This is a collisions-enabled set, create the shared area.
			shared_area = Physics2DServer::get_singleton()->area_create();
			Physics2DServer::get_singleton()->area_set_collision_layer(shared_area, first_kit->collision_layer);
			Physics2DServer::get_singleton()->area_set_collision_mask(shared_area, first_kit->collision_mask);
			Physics2DServer::get_singleton()->area_set_monitorable(shared_area, true);
			Physics2DServer::get_singleton()->area_set_space(shared_area, get_world_2d()->get_space());

			shared_areas.append(shared_area);
			areas_to_pool_set_indices[shared_area] = i;
		}
		int32_t pool_set_available_bullets = 0;

		for(int32_t j = 0; j < kits.size(); j++) {
			Ref<BulletKit> kit = Object::cast_to<BulletKit>(kits[j]);

			PoolIntArray set_pool_indices = PoolIntArray();
			set_pool_indices.resize(2);
			set_pool_indices.set(0, i);
			set_pool_indices.set(1, j);
			kits_to_set_pool_indices[kit] = set_pool_indices;
			
			int32_t kit_index_in_node = bullet_kits.find(kit);
			int32_t pool_size = pools_sizes[kit_index_in_node];

			pool_sets[i].pools[j].pool = kit->_create_pool();
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

void Bullets::_physics_process(float delta) {
	if(Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	int32_t bullets_variation = 0;

	for(int32_t i = 0; i < pool_sets.size(); i++) {
		for(int32_t j = 0; j < pool_sets[i].pools.size(); j++) {
			bullets_variation = pool_sets[i].pools[j].pool->_process(delta);
			available_bullets -= bullets_variation;
			active_bullets += bullets_variation;
		}
	}
}

Variant Bullets::_get(String property) {
	if(property == "bullet_types_amount") {
		return bullet_kits.size();
	}
	Ref<RegExMatch> result = prop_regex->search(property);

	if(result.is_valid()) {
		PoolStringArray strings = result->get_string().split("/");
		int prop_index = (int)strings[1].to_int();
		if(bullet_kits.size() > prop_index) {
			if(strings[2] == "bullet_kit"){
				return bullet_kits[prop_index];
			}
			else if(strings[2] == "pool_size") {
				return pools_sizes[prop_index];
			} 
			else if(strings[2] == "z_index") {
				return z_indices[prop_index];
			}
		}
	}

	return Variant((Object*)nullptr);
}

bool Bullets::_set(String property, Variant value) {
	if(property == "bullet_types_amount") {
		bullet_kits.resize(value);
		pools_sizes.resize(value);
		z_indices.resize(value);
		property_list_changed_notify();
		return true;
	}
	Ref<RegExMatch> result = prop_regex->search(property);

	if(result.is_valid()) {
		PoolStringArray strings = result->get_string().split("/");
		int prop_index = (int)strings[1].to_int();
		if(bullet_kits.size() > prop_index) {
			if(strings[2] == "bullet_kit"){
				bullet_kits[prop_index] = value;
				return true;
			}
			else if(strings[2] == "pool_size") {
				pools_sizes[prop_index] = value;
				return true;
			} 
			else if(strings[2] == "z_index") {
				z_indices[prop_index] = value;
				return true;
			}
		}
	}

	return false;
}

Array Bullets::_get_property_list() {
	Array properties = Array();

	Dictionary kits_amount = Dictionary();
	kits_amount["name"] = "bullet_types_amount";
	kits_amount["type"] = GODOT_VARIANT_TYPE_INT;
	kits_amount["usage"] = GODOT_PROPERTY_USAGE_DEFAULT;
	kits_amount["hint"] = GODOT_PROPERTY_HINT_RANGE;
	kits_amount["hint_string"] = "0,64";

	properties.append(kits_amount);

	for(int32_t i = 0; i < bullet_kits.size(); i++) {
		Array index = Array();
		index.append(i);

		Dictionary kit = Dictionary();
		kit["name"] = String("bullet_types/{0}/bullet_kit").format(index);
		kit["type"] = GODOT_VARIANT_TYPE_OBJECT;
		kit["usage"] = GODOT_PROPERTY_USAGE_DEFAULT;
		kit["hint"] = GODOT_PROPERTY_HINT_RESOURCE_TYPE;
		kit["hint_string"] = "Resource";

		Dictionary size = Dictionary();
		size["name"] = String("bullet_types/{0}/pool_size").format(index);
		size["type"] = GODOT_VARIANT_TYPE_INT;
		size["usage"] = GODOT_PROPERTY_USAGE_DEFAULT;
		size["hint"] = GODOT_PROPERTY_HINT_RANGE;
		size["hint_string"] = "1,65536";

		Dictionary z_index = Dictionary();
		z_index["name"] = String("bullet_types/{0}/z_index").format(index);
		z_index["type"] = GODOT_VARIANT_TYPE_INT;
		z_index["usage"] = GODOT_PROPERTY_USAGE_DEFAULT;
		z_index["hint"] = GODOT_PROPERTY_HINT_RANGE;
		z_index["hint_string"] = "-4096,4096";

		properties.append(kit);
		properties.append(size);
		properties.append(z_index);
	}

	return properties;
}

int32_t Bullets::_get_pool_index(int32_t set_index, int32_t bullet_index) {
	if(bullet_index >= 0 && set_index >= 0 && set_index < pool_sets.size() && bullet_index < pool_sets[set_index].bullets_amount) {
		int32_t pool_threshold = pool_sets[set_index].pools[0].size;
		int32_t pool_index = 0;

		while(bullet_index >= pool_threshold) {
			pool_index++;
			pool_threshold += pool_sets[set_index].pools[pool_index].size;
		}
		if(pool_index < pool_sets[set_index].pools.size()) {
			return pool_index;
		}
	}
	return -1;
}

bool Bullets::spawn_bullet(Ref<BulletKit> kit, Dictionary properties) {
	if(available_bullets > 0 && kits_to_set_pool_indices.has(kit)) {
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit].operator PoolIntArray();
		BulletsPool* pool = pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool.get();

		if(pool->get_available_bullets() > 0) {
			available_bullets -= 1;
			active_bullets += 1;

			pool->spawn_bullet(properties);
			return true;
		}
	}
	return false;
}

Variant Bullets::obtain_bullet(Ref<BulletKit> kit) {
	if(available_bullets > 0 && kits_to_set_pool_indices.has(kit)) {
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit].operator PoolIntArray();
		BulletsPool* pool = pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool.get();

		if(pool->get_available_bullets() > 0) {
			available_bullets -= 1;
			active_bullets += 1;

			BulletID bullet_id = pool->obtain_bullet();
			PoolIntArray to_return = invalid_id;
			to_return.set(0, bullet_id.index);
			to_return.set(1, bullet_id.cycle);
			to_return.set(2, bullet_id.set);
			return to_return;
		}
	}
	return invalid_id;
}

bool Bullets::release_bullet(Variant id) {
	PoolIntArray bullet_id = id.operator PoolIntArray();
	bool result = false;

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if(pool_index >= 0) {
		result = pool_sets[bullet_id[2]].pools[pool_index].pool->release_bullet(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]));
		if(result) {
			available_bullets += 1;
			active_bullets -= 1;
		}
	}
	return result;
}

bool Bullets::is_bullet_valid(Variant id) {
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if(pool_index >= 0) {
		return pool_sets[bullet_id[2]].pools[pool_index].pool->is_bullet_valid(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]));
	}
	return false;
}

bool Bullets::is_kit_valid(Ref<BulletKit> kit) {
	return kits_to_set_pool_indices.has(kit);
}

int32_t Bullets::get_available_bullets(Ref<BulletKit> kit) {
	if(kits_to_set_pool_indices.has(kit)) {
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool->get_available_bullets();
	}
	return 0;
}

int32_t Bullets::get_active_bullets(Ref<BulletKit> kit) {
	if(kits_to_set_pool_indices.has(kit)) {
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].pool->get_active_bullets();
	}
	return 0;
}

int32_t Bullets::get_pool_size(Ref<BulletKit> kit) {
	if(kits_to_set_pool_indices.has(kit)) {
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].size;
	}
	return 0;
}

int32_t Bullets::get_z_index(Ref<BulletKit> kit) {
	if(kits_to_set_pool_indices.has(kit)) {
		PoolIntArray set_pool_indices = kits_to_set_pool_indices[kit];
		return pool_sets[set_pool_indices[0]].pools[set_pool_indices[1]].z_index;
	}
	return 0;
}

int32_t Bullets::get_total_available_bullets() {
	return available_bullets;
}

int32_t Bullets::get_total_active_bullets() {
	return active_bullets;
}

bool Bullets::is_bullet_existing(RID area_rid, int32_t shape_index) {
	if(!areas_to_pool_set_indices.has(area_rid)) {
		return false;
	}
	int32_t set_index = areas_to_pool_set_indices[area_rid];
	int32_t pool_index = _get_pool_index(set_index, shape_index);
	if(pool_index >= 0) {
		return pool_sets[set_index].pools[pool_index].pool->is_bullet_existing(shape_index);
	}
	return false;
}

Variant Bullets::get_bullet_from_shape(RID area_rid, int32_t shape_index) {
	if(!areas_to_pool_set_indices.has(area_rid)) {
		return false;
	}
	int32_t set_index = areas_to_pool_set_indices[area_rid];
	int32_t pool_index = _get_pool_index(set_index, shape_index);
	if(pool_index >= 0) {
		BulletID result = pool_sets[set_index].pools[pool_index].pool->get_bullet_from_shape(shape_index);

		PoolIntArray to_return = invalid_id;
		to_return.set(0, result.index);
		to_return.set(1, result.cycle);
		to_return.set(2, result.set);
		return to_return;
	}
	return invalid_id;
}

Ref<BulletKit> Bullets::get_kit_from_bullet(Variant id) {
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if(pool_index >= 0 && pool_sets[bullet_id[2]].pools[pool_index].pool->is_bullet_valid(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]))) {
		return pool_sets[bullet_id[2]].pools[pool_index].bullet_kit;
	}
	return Ref<BulletKit>();
}

void Bullets::set_bullet_property(Variant id, String property, Variant value) {
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if(pool_index >= 0) {
		pool_sets[bullet_id[2]].pools[pool_index].pool->set_bullet_property(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]), property, value);
	}
}

Variant Bullets::get_bullet_property(Variant id, String property) {
	PoolIntArray bullet_id = id.operator PoolIntArray();

	int32_t pool_index = _get_pool_index(bullet_id[2], bullet_id[0]);
	if(pool_index >= 0) {
		pool_sets[bullet_id[2]].pools[pool_index].pool->get_bullet_property(BulletID(bullet_id[0], bullet_id[1], bullet_id[2]), property);
	}
	return Variant();
}