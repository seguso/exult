// Optional feature state methods. Included by gamewin.cc to retain internal access.
void Game_window::set_modern_movement_tau_ms(int ms) {
	modern_movement_tau_ms = std::clamp(ms, 10, 500);
	config->set("config/gameplay/modern_movement_tau_ms", modern_movement_tau_ms, true);
	reset_velocity_camera();
}

void Game_window::set_modern_keyboard_enabled(bool enabled) {
	modern_keyboard_enabled = enabled;
	config->set("config/gameplay/modern_keyboard", enabled ? "yes" : "no", true);
}

void Game_window::set_modern_mouse_target_enabled(bool enabled) {
	modern_mouse_target_enabled = enabled;
	config->set("config/gameplay/modern_mouse_target", enabled ? "yes" : "no", true);
}

void Game_window::set_smooth_avatar_walk_enabled(bool enabled) {
	smooth_avatar_walk_enabled = enabled;
	config->set("config/gameplay/smooth_avatar_walk", enabled ? "yes" : "no", true);
	avatar_walk_last_tx = avatar_walk_last_ty = -1;
	avatar_walk_from_dx = avatar_walk_from_dy = 0;
	party_walk_visuals.clear();
	avposx_ld = avposy_ld = 0;
	set_all_dirty();
	paint_current_view();
}

void Game_window::set_smooth_scrolling_enabled(bool enabled) {
	modern_movement_enabled = enabled;
	config->set("config/gameplay/modern_movement", enabled ? "yes" : "no", true);
	if (!enabled) {
		mouse_walk_visual_dir = -1;
	}
	if (Mouse::mouse()) {
		Mouse::mouse()->set_speed_cursor();
	}
	// Throw away any visual-camera lag immediately when switching modes so
	// rendering and mouse hit-testing use the same origin from this frame on.
	reset_velocity_camera();
	scrolltx_lo = 0;
	scrollty_lo = 0;
	avposx_ld = 0;
	avposy_ld = 0;
	set_all_dirty();
	paint();
}

