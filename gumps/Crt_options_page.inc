// Optional crt settings page; included in GameDisplayOptions_gump::build_buttons.
	if (page == Page::crt) {
buttons[id_crt_enabled] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_crt_enabled, yesNo, crt_enabled,
			get_button_pos_for_label("CRT filter:"), yForRow(++y_index), small_size);

	buttons[id_crt_horizontal_strength] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_horizontal_strength,
			std::to_string(crt_horizontal_strength) + "%",
			get_button_pos_for_label("CRT horizontal scanlines:"), yForRow(++y_index), small_size);
	buttons[id_crt_vertical_strength] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_vertical_strength,
			(std::to_string(crt_vertical_strength / 2) + (crt_vertical_strength % 2 ? ".5%" : "%")),
			get_button_pos_for_label("CRT vertical mask:"), yForRow(++y_index), small_size);

	buttons[id_crt_brightness_compensation] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_brightness_compensation,
			std::to_string(crt_brightness_compensation) + "%",
			get_button_pos_for_label("CRT brightness comp:"), yForRow(++y_index), small_size);

	std::vector<std::string> crt_width_text;
	for (const int value : crt_width_values) {
		crt_width_text.emplace_back(std::to_string(value) + " px");
	}
	buttons[id_crt_horizontal_width] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_crt_horizontal_width, crt_width_text, crt_horizontal_width,
			get_button_pos_for_label("CRT H line width:"), yForRow(++y_index), small_size);
	buttons[id_crt_vertical_width] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_crt_vertical_width, std::move(crt_width_text), crt_vertical_width,
			get_button_pos_for_label("CRT V mask width:"), yForRow(++y_index), small_size);

	buttons[id_crt_beam_sigma] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_sigma,
			std::to_string(crt_beam_sigma / 100.0f).substr(0, 4),
			get_button_pos_for_label("CRT beam sigma:"), yForRow(++y_index), small_size);
	}
