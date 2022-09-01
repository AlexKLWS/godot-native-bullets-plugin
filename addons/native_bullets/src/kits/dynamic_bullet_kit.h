#ifndef DYNAMIC_BULLET_KIT_H
#define DYNAMIC_BULLET_KIT_H

#include <PackedScene.hpp>
#include <Curve.hpp>

#include "../unit_kit.h"

using namespace godot;

// Bullet definition.
class DynamicBullet : public Unit
{
	GODOT_CLASS(DynamicBullet, Unit)
public:
	Transform starting_trasform;
	float starting_speed;

	void set_transform(Transform transform)
	{
		starting_trasform = transform;
		this->transform = transform;
	}

	Transform get_transform()
	{
		return transform;
	}

	void set_velocity(Vector3 velocity)
	{
		starting_speed = velocity.length();
		this->velocity = velocity;
	}

	Vector3 get_velocity()
	{
		return velocity;
	}

	void _init() {}

	static void _register_methods()
	{
		register_property<DynamicBullet, Transform>("transform",
																								&DynamicBullet::set_transform,
																								&DynamicBullet::get_transform, Transform());
		register_property<DynamicBullet, Transform>("starting_trasform",
																								&DynamicBullet::starting_trasform, Transform());
		register_property<DynamicBullet, Vector3>("velocity",
																							&DynamicBullet::set_velocity,
																							&DynamicBullet::get_velocity, Vector3());
		register_property<DynamicBullet, float>("starting_speed",
																						&DynamicBullet::starting_speed, 0.0f);
	}
};

// Bullet kit definition.
class DynamicBulletKit : public UnitKit
{
	GODOT_CLASS(DynamicBulletKit, UnitKit)
public:
	UNIT_KIT(DynamicBulletsPool)

	float lifetime_curves_span = 1.0f;
	bool lifetime_curves_loop = true;
	Ref<Curve> speed_multiplier_over_lifetime;
	Ref<Curve> rotation_offset_over_lifetime;

	static void _register_methods()
	{
		register_property<DynamicBulletKit, float>("lifetime_curves_span", &DynamicBulletKit::lifetime_curves_span, 1.0f,
																							 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "0.001,256.0");
		register_property<DynamicBulletKit, bool>("lifetime_curves_loop", &DynamicBulletKit::lifetime_curves_loop, true,
																							GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT);
		register_property<DynamicBulletKit, Ref<Curve>>("speed_multiplier_over_lifetime", &DynamicBulletKit::speed_multiplier_over_lifetime, Ref<Curve>(),
																										GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Curve");
		register_property<DynamicBulletKit, Ref<Curve>>("rotation_offset_over_lifetime", &DynamicBulletKit::rotation_offset_over_lifetime, Ref<Curve>(),
																										GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Curve");

		UNIT_KIT_REGISTRATION(DynamicBulletKit, DynamicBullet)
	}
};

// Bullets pool definition.
class DynamicBulletsPool : public AbstractUnitPool<DynamicBulletKit, DynamicBullet>
{

	// void _init_bullet(Bullet* bullet); Use default implementation.

	void _enable_unit(DynamicBullet *bullet)
	{
		// Reset the bullet lifetime.
		bullet->lifetime = 0.0f;

		VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, true);
	}

	// void _disable_unit(Bullet* bullet); Use default implementation.

	bool _process_unit(DynamicBullet *bullet, float delta)
	{
		float adjusted_lifetime = bullet->lifetime / kit->lifetime_curves_span;
		if (kit->lifetime_curves_loop)
		{
			adjusted_lifetime = fmod(adjusted_lifetime, 1.0f);
		}

		if (kit->speed_multiplier_over_lifetime.is_valid())
		{
			float speed_multiplier = kit->speed_multiplier_over_lifetime->interpolate(adjusted_lifetime);
			bullet->velocity = bullet->velocity.normalized() * bullet->starting_speed * speed_multiplier;
		}
		if (kit->rotation_offset_over_lifetime.is_valid())
		{
			float rotation_offset = kit->rotation_offset_over_lifetime->interpolate(adjusted_lifetime);

			bullet->velocity = bullet->velocity.rotated(bullet->transform.get_basis().get_euler().normalized(), rotation_offset);
		}

		bullet->transform.set_origin(bullet->transform.get_origin() + bullet->velocity * delta);

		if (bullet->lifetime >= kit->max_lifetime)
		{
			// Return true if the bullet should be deleted.
			return true;
		}
		// Bullet is still alive, increase its lifetime.
		bullet->lifetime += delta;
		// Return false if the bullet should not be deleted yet.
		return false;
	}
};

UNIT_KIT_IMPLEMENTATION(DynamicBulletKit, DynamicBulletsPool)

#endif