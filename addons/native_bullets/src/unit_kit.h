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

#define UNIT_KIT(UnitPoolType) \
	std::unique_ptr<UnitPool> _create_pool() override;

#define UNIT_KIT_REGISTRATION(UnitKitType, UnitType)                                                                \
	register_property<UnitKitType, String>("unit_class_name",                                                         \
																				 &UnitKitType::_property_setter, &UnitKitType::_property_getter, #UnitType, \
																				 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);

#define UNIT_KIT_IMPLEMENTATION(UnitKitType, UnitPoolType) \
	std::unique_ptr<UnitPool> UnitKitType::_create_pool()    \
	{                                                        \
		return std::unique_ptr<UnitPool>(new UnitPoolType());  \
	}

using namespace godot;

class UnitPool;

class UnitKit : public Resource
{
	GODOT_CLASS(UnitKit, Resource)

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
	// If enabled, bullets will auto-rotate based on their direction of travel.
	bool rotate = false;
	// Allows the ability to have a unique-ish value in each instance of the bullet material.
	// Can be used to offset the bullets animation by a unique amount to avoid having them animate in sync.
	int32_t unique_modulate_component = 0;
	// Additional data the user can set via the editor.
	Variant data;
	// Controls whether the units of this kit are clearable in bulk
	bool is_clearable = true;

	void _init() {}

	void _property_setter(String value) {}
	String _property_getter() { return ""; }

	static void _register_methods()
	{
		register_property<UnitKit, Ref<Material>>("material", &UnitKit::material,
																							Ref<Material>(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT,
																							GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Material");
		register_property<UnitKit, bool>("collisions_enabled", &UnitKit::collisions_enabled, true,
																		 GODOT_METHOD_RPC_MODE_DISABLED, (godot_property_usage_flags)(GODOT_PROPERTY_USAGE_DEFAULT | GODOT_PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED),
																		 GODOT_PROPERTY_HINT_NONE);
		register_property<UnitKit, int32_t>("collision_layer", &UnitKit::collision_layer, 0,
																				GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_2D_PHYSICS);
		register_property<UnitKit, int32_t>("collision_mask", &UnitKit::collision_mask, 0,
																				GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_LAYERS_2D_PHYSICS);
		register_property<UnitKit, Ref<Shape2D>>("collision_shape", &UnitKit::collision_shape,
																						 Ref<Shape2D>(), GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT,
																						 GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Shape2D");
		register_property<UnitKit, bool>("use_viewport_as_active_rect", &UnitKit::use_viewport_as_active_rect, true,
																		 GODOT_METHOD_RPC_MODE_DISABLED, (godot_property_usage_flags)(GODOT_PROPERTY_USAGE_DEFAULT | GODOT_PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED),
																		 GODOT_PROPERTY_HINT_NONE);
		register_property<UnitKit, Rect2>("active_rect", &UnitKit::active_rect, Rect2(),
																			GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_NONE);
		register_property<UnitKit, bool>("rotate", &UnitKit::rotate, false,
																		 GODOT_METHOD_RPC_MODE_DISABLED, (godot_property_usage_flags)(GODOT_PROPERTY_USAGE_DEFAULT | GODOT_PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED),
																		 GODOT_PROPERTY_HINT_NONE);
		register_property<UnitKit, int32_t>("unique_modulate_component", &UnitKit::unique_modulate_component, 0,
																				GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM, "None,Red,Green,Blue,Alpha");
		register_property<UnitKit, Variant>("data", &UnitKit::data, Dictionary(),
																				GODOT_METHOD_RPC_MODE_DISABLED, (godot_property_usage_flags)(GODOT_PROPERTY_USAGE_DEFAULT | GODOT_PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED),
																				GODOT_PROPERTY_HINT_NONE);

		register_property<UnitKit, String>("unit_class_name",
																			 &UnitKit::_property_setter, &UnitKit::_property_getter, "",
																			 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NOEDITOR);
		register_property<UnitKit, String>("bullet_properties",
																			 &UnitKit::_property_setter, &UnitKit::_property_getter, "",
																			 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_EDITOR);
	}

	virtual bool is_valid() { return material.is_valid(); }

	virtual std::unique_ptr<UnitPool> _create_pool() { return std::unique_ptr<UnitPool>(); }
};

#include "unit_pool.h"

#endif