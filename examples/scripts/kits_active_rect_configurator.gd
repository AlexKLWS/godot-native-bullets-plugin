extends Node


export(float, -512.0, 512.0) var bullets_active_rect_grow = 32.0 setget _set_bullets_active_rect_grow

var environment: BulletsEnvironment


func _ready():
	get_viewport().connect("size_changed", self, "_update_active_rect")


func _on_bullets_environment_tree_entering(node):
	environment = node
	_set_bullets_active_rect_grow(bullets_active_rect_grow)


func _update_active_rect():
	_set_bullets_active_rect_grow(bullets_active_rect_grow)


func _set_bullets_active_rect_grow(value):
	bullets_active_rect_grow = value
	if environment:
		# Set every kit active rect to the viewport visible rect grown by `bullets_active_rect_grow`.
		for kit in environment.unit_kits:
			kit.active_rect = get_viewport().get_visible_rect().grow(bullets_active_rect_grow)
