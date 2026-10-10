// Display-relative keyboard movement for an optionally rotated world.
// Classic Exult actor targeting is retained byte-for-byte when disabled.
static void start_rotation_keyboard_walk(Game_window* gwin, int screen_dx, int screen_dy, int speed) {
	if (!gwin->is_rotate_world_enabled()) {
		gwin->start_actor(gwin->get_width() / 2 + screen_dx * 50,
				gwin->get_height() / 2 + screen_dy * 50, speed);
		return;
	}
	const int world_dx = screen_dx + screen_dy;
	const int world_dy = -screen_dx + screen_dy;
	int ax = 0, ay = 0;
	gwin->get_shape_location(gwin->get_main_actor(), ax, ay);
	gwin->start_actor(ax + world_dx * 50, ay + world_dy * 50, speed);
}
