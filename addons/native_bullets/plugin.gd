tool
extends EditorPlugin


var theme
var bullets_inspector_plugin
var bullet_kit_inspector_plugin


func _enter_tree():
	theme = get_editor_interface().get_base_control().theme
	bullets_inspector_plugin = preload("inspector/bullets_inspector.gd").new()
	bullet_kit_inspector_plugin = preload("inspector/bullet_kit_inspector.gd").new()
	bullet_kit_inspector_plugin.init(theme)
	add_inspector_plugin(bullets_inspector_plugin)
	add_inspector_plugin(bullet_kit_inspector_plugin)

func _exit_tree():
	remove_inspector_plugin(bullets_inspector_plugin)
	remove_inspector_plugin(bullet_kit_inspector_plugin)
	bullets_inspector_plugin = null
	bullet_kit_inspector_plugin = null
