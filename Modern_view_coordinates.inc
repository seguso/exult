// Display/world coordinate integration for camera and world rotation.
void Game_window::display_to_world(int& x, int& y) const {
	const World_view_point p = world_view.display_to_scene({static_cast<double>(x), static_cast<double>(y)});
	x = static_cast<int>(std::lround(p.x));
	y = static_cast<int>(std::lround(p.y));

	// The velocity camera is purely visual: paint_velocity_camera() temporarily
	// renders with a different scroll origin and then restores the logical
	// tile scroll. Mouse hit-testing happens after that restore, so compensate
	// for the difference between the origin the user is actually looking at
	// and the current logical origin. During painting both origins coincide,
	// making this adjustment naturally zero.
	if (modern_movement_enabled && smooth_cam_valid) {
		const int world_pixels = c_num_tiles * c_tilesize;
		const auto wrapped_delta = [&](int visual, int logical) {
			int d = visual - logical;
			const int half = world_pixels / 2;
			while (d > half) {
				d -= world_pixels;
			}
			while (d < -half) {
				d += world_pixels;
			}
			return d;
		};
		const int visual_x = static_cast<int>(std::lround(smooth_cam_x));
		const int visual_y = static_cast<int>(std::lround(smooth_cam_y));
		const int logical_x = scrolltx * c_tilesize + scrolltx_lo;
		const int logical_y = scrollty * c_tilesize + scrollty_lo;
		x += wrapped_delta(visual_x, logical_x);
		y += wrapped_delta(visual_y, logical_y);
	}
}

void Game_window::world_to_display(int& x, int& y) const {
	// Inverse of display_to_world(): convert coordinates expressed relative to
	// the current logical scroll origin into the visually rendered camera
	// origin before applying the optional 45-degree world transform.
	if (modern_movement_enabled && smooth_cam_valid) {
		const int world_pixels = c_num_tiles * c_tilesize;
		const auto wrapped_delta = [&](int visual, int logical) {
			int d = visual - logical;
			const int half = world_pixels / 2;
			while (d > half) {
				d -= world_pixels;
			}
			while (d < -half) {
				d += world_pixels;
			}
			return d;
		};
		const int visual_x = static_cast<int>(std::lround(smooth_cam_x));
		const int visual_y = static_cast<int>(std::lround(smooth_cam_y));
		const int logical_x = scrolltx * c_tilesize + scrolltx_lo;
		const int logical_y = scrollty * c_tilesize + scrollty_lo;
		x -= wrapped_delta(visual_x, logical_x);
		y -= wrapped_delta(visual_y, logical_y);
	}

	const World_view_point p = world_view.scene_to_display({static_cast<double>(x), static_cast<double>(y)});
	x = static_cast<int>(std::lround(p.x));
	y = static_cast<int>(std::lround(p.y));
}

void Game_window::get_world_scene_bounds(int& x, int& y, int& w, int& h) const {
	if (!rotate_world) {
		x = 0;
		y = 0;
		w = get_width();
		h = get_height();
		return;
	}
	x = world_view.get_scene_x();
	y = world_view.get_scene_y();
	w = world_view.get_scene_size();
	h = world_view.get_scene_size();
}

void Game_window::start_actor_from_display(int x, int y, int speed) {
	display_to_world(x, y);
	start_actor(x, y, speed, modern_movement_enabled || modern_mouse_target_enabled);
}

void Game_window::start_actor_along_path_from_display(int x, int y, int speed) {
	display_to_world(x, y);
	start_actor_along_path(x, y, speed);
}

Game_object* Game_window::find_object_from_display(int x, int y) {
	display_to_world(x, y);
	return find_object(x, y);
}
