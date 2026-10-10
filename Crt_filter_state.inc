// Optional feature state methods. Included by gamewin.cc to retain internal access.
void Game_window::preview_crt_filter_settings(
		bool enabled, int horizontal_strength, int vertical_strength,
		int compensation, int horizontal_width, int vertical_width, int beam_sigma) {
	win->set_crt_filter(
			enabled, horizontal_strength, vertical_strength,
			compensation, compensation, horizontal_width, vertical_width, beam_sigma);
	set_all_dirty();
}

void Game_window::set_crt_filter_settings(
		bool enabled, int horizontal_strength, int vertical_strength,
		int horizontal_compensation, int vertical_compensation,
		int horizontal_width, int vertical_width, int beam_sigma) {
	crt_filter_enabled = enabled;
	crt_horizontal_strength = std::clamp(horizontal_strength, 0, 20);
	crt_vertical_strength = std::clamp(vertical_strength, 0, 40);
	crt_horizontal_compensation = std::clamp(horizontal_compensation, 0, 200);
	crt_vertical_compensation = std::clamp(vertical_compensation, 0, 200);
	crt_horizontal_width = std::clamp(horizontal_width, 1, 6);
	crt_vertical_width = std::clamp(vertical_width, 1, 6);
	crt_beam_sigma = std::clamp(beam_sigma, 10, 100);

	config->set("config/video/crt/enabled", crt_filter_enabled ? "yes" : "no", false);
	config->set("config/video/crt/horizontal_strength", crt_horizontal_strength, false);
	config->set("config/video/crt/vertical_strength_half", crt_vertical_strength, false);
	config->set("config/video/crt/horizontal_compensation", crt_horizontal_compensation, false);
	config->set("config/video/crt/vertical_compensation", crt_vertical_compensation, false);
	config->set("config/video/crt/horizontal_width", crt_horizontal_width, false);
	config->set("config/video/crt/vertical_width", crt_vertical_width, false);
	config->set("config/video/crt/beam_sigma", crt_beam_sigma, false);

	win->set_crt_filter(
			crt_filter_enabled,
			crt_horizontal_strength,
			crt_vertical_strength,
			crt_horizontal_compensation,
			crt_vertical_compensation,
			crt_horizontal_width,
			crt_vertical_width, crt_beam_sigma);
	set_all_dirty();
	paint();
}

