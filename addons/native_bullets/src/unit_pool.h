#ifndef UNIT_POOL_H
#define UNIT_POOL_H

#include <Godot.hpp>
#include <CanvasItem.hpp>
#include <AtlasTexture.hpp>
#include <Material.hpp>
#include <Color.hpp>

#include "unit.h"
#include "unit_kit.h"

using namespace godot;

class UnitPool
{

protected:
	int32_t *shapes_to_indices = nullptr;
	int32_t available_units = 0;
	int32_t active_units = 0;
	int32_t units_to_handle = 0;
	bool collisions_enabled;

	CanvasItem *canvas_parent;
	RID canvas_item;
	RID shared_area;
	int32_t starting_shape_index;

	float max_lifetime = 1.0f;

	template <typename T>
	void _swap(T &a, T &b)
	{
		T t = a;
		a = b;
		b = t;
	}

public:
	int32_t pool_size = 0;
	int32_t set_index = -1;

	UnitPool();
	virtual ~UnitPool();

	virtual void _init(CanvasItem *canvas_parent, RID shared_area, int32_t starting_shape_index,
										 int32_t set_index, Ref<UnitKit> kit, int32_t pool_size, int32_t z_index) = 0;

	int32_t get_available_units();
	int32_t get_active_units();

	virtual int32_t _process(float delta) = 0;

	virtual void spawn_unit(Dictionary properties) = 0;
	virtual BulletID obtain_unit() = 0;
	virtual bool release_unit(BulletID id) = 0;
	virtual PoolVector2Array release_all_units() = 0;
	virtual PoolVector2Array release_all_units_in_radius(Vector2 from, float distance_squared) = 0;
	virtual bool is_unit_valid(BulletID id) = 0;

	virtual bool does_unit_exist(int32_t shape_index) = 0;
	virtual BulletID get_unit_from_shape(int32_t shape_index) = 0;

	virtual void set_unit_property(BulletID id, String property, Variant value) = 0;
	virtual Variant get_unit_property(BulletID id, String property) = 0;
};

template <class Kit, class UnitType>
class AbstractUnitPool : public UnitPool
{

protected:
	Ref<Kit> kit;
	UnitType **bullets = nullptr;

	virtual inline void _init_unit(UnitType *bullet);
	virtual inline void _enable_unit(UnitType *bullet);
	virtual inline void _disable_unit(UnitType *bullet);
	virtual inline bool _process_unit(UnitType *bullet, float delta);

	inline Vector2 _release_unit(int32_t index);

public:
	AbstractUnitPool() {}
	virtual ~AbstractUnitPool();

	virtual void _init(CanvasItem *canvas_parent, RID shared_area, int32_t starting_shape_index,
										 int32_t set_index, Ref<UnitKit> kit, int32_t pool_size, int32_t z_index) override;

	virtual int32_t _process(float delta) override;

	virtual void spawn_unit(Dictionary properties) override;
	virtual BulletID obtain_unit() override;
	virtual bool release_unit(BulletID id) override;
	virtual PoolVector2Array release_all_units() override;
	virtual PoolVector2Array release_all_units_in_radius(Vector2 from, float distance_squared) override;
	virtual bool is_unit_valid(BulletID id) override;

	virtual bool does_unit_exist(int32_t shape_index) override;
	virtual BulletID get_unit_from_shape(int32_t shape_index) override;

	virtual void set_unit_property(BulletID id, String property, Variant value) override;
	virtual Variant get_unit_property(BulletID id, String property) override;
};

#include "unit_pool.inl"

#endif