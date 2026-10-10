// Smooth movement cursor-angle update, included by Mouse::set_speed_cursor.
		if (gwin->is_modern_movement_enabled() && (dx != 0 || dy != 0)) {
			constexpr double pi = 3.14159265358979323846;
			const double exact_screen_angle = std::atan2(static_cast<double>(mousey - ay), static_cast<double>(mousex - ax));
			const double base_screen_angle = (-90.0 + 45.0 * static_cast<int>(dir)) * pi / 180.0;
			double residual = exact_screen_angle - base_screen_angle;
			while (residual > pi) {
				residual -= 2.0 * pi;
			}
			while (residual < -pi) {
				residual += 2.0 * pi;
			}
			smooth_arrow_residual_rad = residual;
			// 256 angular steps over a full turn (~1.4 degrees). Quantizing keeps
			// redraw/cache behavior stable while remaining visually continuous.
			const double exact_turn = exact_screen_angle / (2.0 * pi);
			int bucket = static_cast<int>(std::lround(exact_turn * 256.0));
			bucket %= 256;
			if (bucket < 0) {
				bucket += 256;
			}
			smooth_arrow_angle_bucket = bucket;
			smooth_arrow_active = true;
		}

