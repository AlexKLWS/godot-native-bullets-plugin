#ifndef UNIT_H
#define UNIT_H

#include <Godot.hpp>
#include <Transform2D.hpp>

using namespace godot;

struct BulletID
{
	int32_t index;
	int32_t cycle;
	int32_t set;

	BulletID(int32_t index, int32_t cycle, int32_t set) : index(index), cycle(cycle), set(set) {}
};

class Unit : public Object
{
	GODOT_CLASS(Unit, Object)

public:
	RID item_rid;
	int32_t cycle = 0;
	int32_t shape_index = -1;
	Transform2D transform;
	Vector2 velocity;
	float lifetime;
	Variant data;

	void _init() {}

	RID get_item_rid() { return item_rid; }
	void set_item_rid(RID value) { ERR_PRINT("Can't edit the item rid of bullets!"); }

	int32_t get_cycle() { return cycle; }
	void set_cycle(int32_t value) { ERR_PRINT("Can't edit the cycle of bullets!"); }

	int32_t get_shape_index() { return shape_index; }
	void set_shape_index(int32_t value) { ERR_PRINT("Can't edit the shape index of bullets!"); }

	static void _register_methods()
	{
		register_property<Unit, RID>("item_rid", &Unit::set_item_rid, &Unit::get_item_rid, RID());
		register_property<Unit, int32_t>("cycle", &Unit::set_cycle, &Unit::get_cycle, 0);
		register_property<Unit, int32_t>("shape_index", &Unit::set_shape_index, &Unit::get_shape_index, 0);

		register_property<Unit, Transform2D>("transform", &Unit::transform, Transform2D());
		register_property<Unit, Vector2>("velocity", &Unit::velocity, Vector2());
		register_property<Unit, float>("lifetime", &Unit::lifetime, 0.0f);
		register_property<Unit, Variant>("data", &Unit::data, Variant());
	}
};

#endif