// Optional movement settings page; included in GameDisplayOptions_gump::build_buttons.
	if (page == Page::movement) {

	buttons[id_modern_smooth] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_modern_smooth, yesNo, modern_smooth,
			get_button_pos_for_label(Strings::Modernsmoothscrolling_()), yForRow(++y_index), small_size);
	buttons[id_smooth_avatar_walk] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_smooth_avatar_walk,
			yesNo, smooth_avatar_walk,
			get_button_pos_for_label("Smooth avatar walk:"), yForRow(++y_index), small_size);

	std::vector<std::string> tau_text;
	for (int value : modern_tau_values) {
		tau_text.emplace_back(std::to_string(value) + " ms");
	}
	buttons[id_modern_tau] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_modern_tau,
			std::move(tau_text), modern_tau,
			get_button_pos_for_label(Strings::Smoothcameratau_()), yForRow(++y_index), small_size);


	}
