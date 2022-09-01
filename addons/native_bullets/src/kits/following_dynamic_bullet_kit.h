#ifndef FOLLOWING_DYNAMIC_BULLET_KIT_H
#define FOLLOWING_DYNAMIC_BULLET_KIT_H

#include <PackedScene.hpp>
#include <Curve.hpp>
#include <Spatial.hpp>

#include "../unit_kit.h"

using namespace godot;

// Bullet definition.
class FollowingDynamicBullet : public Unit
{
	GODOT_CLASS(FollowingDynamicBullet, Unit)
public:
	Spatial *target_node = nullptr;
	float starting_speed;

	void set_target_node(Spatial *node)
	{
		target_node = node;
	}

	Spatial *get_target_node()
	{
		return target_node;
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
		// Registering an Object reference property with GODOT_PROPERTY_HINT_RESOURCE_TYPE and hint_string is just
		// a way to tell the editor plugin the type of the property, so that it can be viewed in the UnitKit inspector.
		register_property<FollowingDynamicBullet, Spatial *>("target_node",
																												 &FollowingDynamicBullet::set_target_node,
																												 &FollowingDynamicBullet::get_target_node, nullptr,
																												 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NO_INSTANCE_STATE, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Spatial");
		register_property<FollowingDynamicBullet, Vector3>("velocity",
																											 &FollowingDynamicBullet::set_velocity,
																											 &FollowingDynamicBullet::get_velocity, Vector3());
		register_property<FollowingDynamicBullet, float>("starting_speed",
																										 &FollowingDynamicBullet::starting_speed, 0.0f);
	}
};

// Bullet kit definition.
class FollowingDynamicBulletKit : public UnitKit
{
	GODOT_CLASS(FollowingDynamicBulletKit, UnitKit)
public:
	UNIT_KIT(FollowingDynamicBulletsPool)

	float lifetime_curves_span = 1.0f;
	float distance_curves_span = 128.0f;
	bool lifetime_curves_loop = true;
	int32_t speed_control_mode = 0;
	Ref<Curve> speed_multiplier;
	int32_t turning_speed_control_mode = 0;
	Ref<Curve> turning_speed;

	static void _register_methods()
	{
		register_property<FollowingDynamicBulletKit, float>("lifetime_curves_span", &FollowingDynamicBulletKit::lifetime_curves_span, 1.0f,
																												GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "0.001,256.0");
		register_property<FollowingDynamicBulletKit, float>("distance_curves_span", &FollowingDynamicBulletKit::distance_curves_span, 128.0f,
																												GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "0.001,65536.0");
		register_property<FollowingDynamicBulletKit, bool>("lifetime_curves_loop", &FollowingDynamicBulletKit::lifetime_curves_loop, true,
																											 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT);
		register_property<FollowingDynamicBulletKit, int32_t>("speed_control_mode", &FollowingDynamicBulletKit::speed_control_mode, 0,
																													GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM,
																													"Based On Lifetime,Based On Target Distance,Based On Angle To Target");
		register_property<FollowingDynamicBulletKit, Ref<Curve>>("speed_multiplier",
																														 &FollowingDynamicBulletKit::speed_multiplier, Ref<Curve>(),
																														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Curve");
		register_property<FollowingDynamicBulletKit, int32_t>("turning_speed_control_mode",
																													&FollowingDynamicBulletKit::turning_speed_control_mode, 0,
																													GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_ENUM,
																													"Based On Lifetime,Based On Target Distance,Based On Angle To Target");
		register_property<FollowingDynamicBulletKit, Ref<Curve>>("turning_speed",
																														 &FollowingDynamicBulletKit::turning_speed, Ref<Curve>(),
																														 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Curve");

		UNIT_KIT_REGISTRATION(FollowingDynamicBulletKit, FollowingDynamicBullet)
	}
};

// Bullets pool definition.
class FollowingDynamicBulletsPool : public AbstractUnitPool<FollowingDynamicBulletKit, FollowingDynamicBullet>
{

	// void _init_bullet(FollowingDynamicBullet* bullet); Use default implementation.

	void _enable_unit(FollowingDynamicBullet *bullet)
	{
		// Reset the bullet lifetime.
		bullet->lifetime = 0.0f;

		VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, true);
	}

	// void _disable_unit(FollowingDynamicBullet* bullet); Use default implementation.

	bool _process_unit(FollowingDynamicBullet *bullet, float delta)
	{
		float adjusted_lifetime = bullet->lifetime / kit->lifetime_curves_span;
		if (kit->lifetime_curves_loop)
		{
			adjusted_lifetime = fmod(adjusted_lifetime, 1.0f);
		}
		float bullet_turning_speed = 0.0f;
		float speed_multiplier = 1.0f;

		if (kit->turning_speed.is_valid() && bullet->target_node != nullptr)
		{
			Vector3 to_target = bullet->target_node->get_transform().get_origin() - bullet->transform.get_origin();
			// If based on lifetime.
			if (kit->turning_speed_control_mode == 0)
			{
				bullet_turning_speed = kit->turning_speed->interpolate(adjusted_lifetime);
			}
			// If based on distance to target.
			else if (kit->turning_speed_control_mode == 1)
			{
				float distance_to_target = to_target.length();
				bullet_turning_speed = kit->turning_speed->interpolate(distance_to_target / kit->distance_curves_span);
			}
			// If based on angle to target.
			else if (kit->turning_speed_control_mode == 2)
			{
				float angle_to_target = bullet->velocity.angle_to(to_target);
				bullet_turning_speed = kit->turning_speed->interpolate(std::abs(angle_to_target) / (float)Math_PI);
			}
		}
		if (kit->speed_multiplier.is_valid())
		{
			// If based on lifetime.
			if (kit->speed_control_mode <= 0)
			{
				speed_multiplier = kit->speed_multiplier->interpolate(adjusted_lifetime);
			}
			// If based on target node: 1 or 2.
			else if (kit->speed_control_mode < 3 && bullet->target_node != nullptr)
			{
				Vector3 to_target = bullet->target_node->get_transform().get_origin() - bullet->transform.get_origin();
				// If based on distance to target.
				if (kit->speed_control_mode == 1)
				{
					float distance_to_target = to_target.length();
					speed_multiplier = kit->speed_multiplier->interpolate(distance_to_target / kit->distance_curves_span);
				}
				// If based on angle to target.
				else if (kit->speed_control_mode == 2)
				{
					float angle_to_target = bullet->velocity.angle_to(to_target);
					speed_multiplier = kit->speed_multiplier->interpolate(std::abs(angle_to_target) / (float)Math_PI);
				}
			}
		}

		if (speed_multiplier != 1.0f)
		{
			bullet->velocity = bullet->velocity.normalized() * bullet->starting_speed * speed_multiplier;
		}
		if (bullet_turning_speed != 0.0 && bullet->target_node != nullptr)
		{
			// Find the rotation to the target node.
			Vector3 to_target = bullet->target_node->get_transform().get_origin() - bullet->transform.get_origin();
			float rotation_to_target = bullet->velocity.angle_to(to_target);
			float rotation_value = Math::min(bullet_turning_speed * delta, std::abs(rotation_to_target));
			// Apply the rotation, capped to the max turning speed.
			bullet->velocity = bullet->velocity.rotated(bullet->transform.get_basis().get_euler().normalized(), Math::sign(rotation_to_target) * rotation_value);
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

UNIT_KIT_IMPLEMENTATION(FollowingDynamicBulletKit, FollowingDynamicBulletsPool)

#endif