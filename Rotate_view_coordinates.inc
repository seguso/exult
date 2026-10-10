// World/display coordinate bridge for the optional 45-degree rotation.
// The visual camera is intentionally not a dependency of this module.
void Game_window::display_to_world(int& x, int& y) const {
	const World_view_point p = world_view.display_to_scene({double(x), double(y)});
	x = static_cast<int>(std::lround(p.x));
	y = static_cast<int>(std::lround(p.y));
}

void Game_window::world_to_display(int& x, int& y) const {
	const World_view_point p = world_view.scene_to_display({double(x), double(y)});
	x = static_cast<int>(std::lround(p.x));
	y = static_cast<int>(std::lround(p.y));
}

void Game_window::get_world_scene_bounds(int& x, int& y, int& w, int& h) const {
	if (!rotate_world) {
		x = y = 0;
		w = get_width(); h = get_height();
		return;
	}
	x = world_view.get_scene_x();
	y = world_view.get_scene_y();
	w = h = world_view.get_scene_size();
}

void Game_window::start_actor_from_display(int x, int y, int speed) {
	display_to_world(x, y);
	start_actor(x, y, speed);
}
void Game_window::start_actor_along_path_from_display(int x, int y, int speed) {
	display_to_world(x, y);
	start_actor_along_path(x, y, speed);
}
Game_object* Game_window::find_object_from_display(int x, int y) {
	display_to_world(x, y);
	return find_object(x, y);
}
