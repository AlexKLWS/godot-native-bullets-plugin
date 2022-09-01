#ifndef BASIC_BULLET_KIT_H
#define BASIC_BULLET_KIT_H

#include <PackedScene.hpp>

#include "../unit_kit.h"

using namespace godot;

// Bullet kit definition.
class BasicBulletKit : public UnitKit
{
	GODOT_CLASS(BasicBulletKit, UnitKit)
public:
	UNIT_KIT(BasicBulletsPool)

	static void _register_methods()
	{
		UNIT_KIT_REGISTRATION(BasicBulletKit, Unit)
	}
};

// Bullets pool definition.
class BasicBulletsPool : public AbstractUnitPool<BasicBulletKit, Unit>
{

	// void _init_bullet(Bullet* bullet); Use default implementation.

	void _enable_unit(Unit *bullet)
	{
		// Reset the bullet lifetime.
		bullet->lifetime = 0.0f;

		VisualServer::get_singleton()->instance_set_visible(bullet->item_rid, true);
	}

	// void _disable_unit(Bullet* bullet); Use default implementation.

	bool _process_unit(Unit *bullet, float delta)
	{
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

UNIT_KIT_IMPLEMENTATION(BasicBulletKit, BasicBulletsPool)

#endif