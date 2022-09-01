extends Area2D


export(float, 0, 1000) var speed = 300.0


func _process(delta):
	var velocity = Vector2()
	
	if(Input.is_action_pressed("ui_up")):
		velocity.y -= 1
	if(Input.is_action_pressed("ui_down")):
		velocity.y += 1
	if(Input.is_action_pressed("ui_left")):
		velocity.x -= 1
	if(Input.is_action_pressed("ui_right")):
		velocity.x += 1
		
	if(Input.is_action_just_pressed("ui_select")):
		call_deferred("release_surrounding_bullets", position)
	
	if(velocity.length() > 0):
		velocity = velocity.normalized() * speed
	
	position += velocity * delta
	global_position.x = clamp(global_position.x, 12, get_viewport_rect().size.x - 12)
	global_position.y = clamp(global_position.y, 12, get_viewport_rect().size.y - 12)
	
	
func release_surrounding_bullets(pos):
	Bullets.release_all_units_in_radius(pos, 100.0);

func _on_area_shape_entered(area_id, _area, area_shape, _local_shape):
	var bullet_id = Bullets.get_bullet_from_shape(area_id, area_shape)
	
	var kit = Bullets.get_kit_from_bullet(bullet_id)
	var bullet_hit = kit.data.hit_scene.instance()
	add_child(bullet_hit)
	bullet_hit.global_position = Bullets.get_bullet_property(bullet_id, "transform").get_origin()
	
	call_deferred("_handle_bullet", bullet_id)	


func _handle_bullet(bullet_id):
	Bullets.release_bullet(bullet_id)
