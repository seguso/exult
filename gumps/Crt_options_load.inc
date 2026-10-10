// Feature option loading; included inside GameDisplayOptions_gump::load_settings.
	crt_enabled = gwin->is_crt_filter_enabled() ? 1 : 0;

	const auto nearest_index = [](int value, const int* values, size_t count) {
		size_t best = 0;
		for (size_t i = 1; i < count; ++i) {
			if (std::abs(values[i] - value) < std::abs(values[best] - value)) {
				best = i;
			}
		}
		return static_cast<int>(best);
	};
	crt_horizontal_strength = std::clamp(gwin->get_crt_horizontal_strength(), 0, 20);
	crt_vertical_strength = std::clamp(gwin->get_crt_vertical_strength(), 0, 40);
	crt_brightness_compensation = std::clamp(gwin->get_crt_horizontal_compensation(), 50, 150);
	crt_beam_sigma = std::clamp(gwin->get_crt_beam_sigma(), 10, 100);
	crt_horizontal_width = nearest_index(
			gwin->get_crt_horizontal_width(), crt_width_values, std::size(crt_width_values));
	crt_vertical_width = nearest_index(
			gwin->get_crt_vertical_width(), crt_width_values, std::size(crt_width_values));

