// Feature-specific settings persistence; included by GameDisplayOptions_gump.cc.
	if (page == Page::crt) {

	gwin->set_crt_filter_settings(
			crt_enabled != 0,
			std::clamp(crt_horizontal_strength, 0, 20),
			std::clamp(crt_vertical_strength, 0, 40),
			crt_brightness_compensation,
			crt_brightness_compensation,
			crt_width_values[std::clamp(crt_horizontal_width, 0, static_cast<int>(std::size(crt_width_values)) - 1)],
			crt_width_values[std::clamp(crt_vertical_width, 0, static_cast<int>(std::size(crt_width_values)) - 1)], crt_beam_sigma);
	}
