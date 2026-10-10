// Keyboard movement targets, isolated from camera and rotation.
static void start_modern_keyboard_walk(Game_window* gwin, int dx, int dy, int speed) {
	if (!gwin->is_modern_keyboard_enabled()) {
		// Preserve Exult's original centered keyboard target when disabled.
		gwin->start_actor(gwin->get_width() / 2 + dx * 50,
				gwin->get_height() / 2 + dy * 50, speed);
		return;
	}
	// Held WASD movement must aim from the Avatar rather than a fixed
	// screen center: the classic camera need not keep the Avatar centered.
	int ax = 0, ay = 0;
	gwin->get_shape_location(gwin->get_main_actor(), ax, ay);
	gwin->start_actor(ax + dx * 50, ay + dy * 50, speed);
}
