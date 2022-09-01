#ifndef FOLLOWING_BULLET_KIT_H
#define FOLLOWING_BULLET_KIT_H

#include <PackedScene.hpp>
#include <Spatial.hpp>
#include <SceneTree.hpp>
#include <cmath>

#include "../unit_kit.h"

using namespace godot;

// Bullet definition.
class FollowingBullet : public Unit
{
	GODOT_CLASS(FollowingBullet, Unit)
public:
	Spatial *target_node = nullptr;

	void _init() {}

	void set_target_node(Spatial *node)
	{
		target_node = node;
	}

	Spatial *get_target_node()
	{
		return target_node;
	}

	static void _register_methods()
	{
		// Registering an Object reference property with GODOT_PROPERTY_HINT_RESOURCE_TYPE and hint_string is just
		// a way to tell the editor plugin the type of the property, so that it can be viewed in the UnitKit inspector.
		register_property<FollowingBullet, Spatial *>("target_node",
																									&FollowingBullet::set_target_node,
																									&FollowingBullet::get_target_node, nullptr,
																									GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_NO_INSTANCE_STATE, GODOT_PROPERTY_HINT_RESOURCE_TYPE, "Spatial");
	}
};

// Bullet kit definition.
class FollowingBulletKit : public UnitKit
{
	GODOT_CLASS(FollowingBulletKit, UnitKit)
public:
	UNIT_KIT(FollowingBulletsPool)

	float bullets_turning_speed = 1.0f;

	static void _register_methods()
	{
		register_property<FollowingBulletKit, float>("bullets_turning_speed", &FollowingBulletKit::bullets_turning_speed, 1.0f,
																								 GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "0.0,128.0");

		UNIT_KIT_REGISTRATION(FollowingBulletKit, FollowingBullet)
	}
};

// Bullets pool definition.
class FollowingBulletsPool : public AbstractUnitPool<FollowingBulletKit, FollowingBullet>
{

	// void _init_bullet(FollowingBullet* bullet); Use default implementation.

	void _enable_unit(FollowingBullet *bullet)
	{
		// Reset the bullet lifetime.
		bullet->lifetime = 0.0f;

		VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, true);
	}

	// void _disable_unit(FollowingBullet* bullet); Use default implementation.

	bool _process_unit(FollowingBullet *bullet, float delta)
	{
		if (bullet->target_node != nullptr)
		{
			// Find the rotation to the target node.
			Vector3 to_target = bullet->target_node->get_global_transform().get_origin() - bullet->transform.get_origin();
			float rotation_to_target = bullet->velocity.angle_to(to_target);
			float rotation_value = Math::min(kit->bullets_turning_speed * delta, std::abs(rotation_to_target));

			// Apply the rotation, capped to the max turning speed.
			bullet->velocity = bullet->velocity.rotated(bullet->transform.get_basis().get_euler().normalized(), Math::sign(rotation_to_target) * rotation_value);
		}
		// Apply velocity.
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

UNIT_KIT_IMPLEMENTATION(FollowingBulletKit, FollowingBulletsPool)

#endif