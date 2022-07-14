#include "unit_pool.h"

using namespace godot;

UnitPool::UnitPool() {}

UnitPool::~UnitPool() {}

int32_t UnitPool::get_available_units()
{
	return available_units;
}

int32_t UnitPool::get_active_units()
{
	return active_units;
}