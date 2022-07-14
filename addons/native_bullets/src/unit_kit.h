#ifndef UNIT_KIT_H
#define UNIT_KIT_H

#include <Godot.hpp>
#include <Resource.hpp>
#include <Shape2D.hpp>
#include <Material.hpp>
#include <Texture.hpp>
#include <PackedScene.hpp>
#include <Script.hpp>

#include <memory>

#include "unit.h"

using namespace godot;

class UnitPool;

class UnitKit : public Resource
{

public:
	// The material used to render each bullet.
	Ref<Material> material;
	// Controls whether collisions with other objects are enabled. Turning it off increases performance.
	bool collisions_enabled = true;
	// Collisions related properties.
	int32_t collision_layer = 0;
	int32_t collision_mask = 0;
	Ref<Shape2D> collision_shape;
	// Controls whether the active rect is automatically set as the viewport visible rect.
	bool use_viewport_as_active_rect = true;
	// Controls where the bullets can live, if a bullet exits this rect, it will be removed.
	Rect2 active_rect;
	// Additional data the user can set via the editor.
	Variant data;

	void _init() {}

	void _property_setter(String value) {}
	String _property_getter() { return ""; }

	virtual bool is_valid() { return material.is_valid(); }

	virtual std::unique_ptr<UnitPool> _create_pool() { return std::unique_ptr<UnitPool>(); }
};

#include "unit_pool.h"

#endif