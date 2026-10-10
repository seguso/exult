// Optional rotation settings page; included in GameDisplayOptions_gump::build_buttons.
	if (page == Page::rotation) {
buttons[id_rotate_world] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_rotate_world, yesNo, rotate_world,
			get_button_pos_for_label(Strings::Rotateworld45deg_()), yForRow(++y_index), small_size);

	std::vector<std::string> rotate_quality_text = {
			"2x / 4 samples", "3x / 4 samples", "3x / 9 samples"};
#ifdef USE_HQ3X_SCALER
	rotate_quality_text.emplace_back("HQ3x / 9 samples");
	rotate_quality_text.emplace_back("HQ3x / 4 samples");
	rotate_quality_text.emplace_back("Forward weighted triplets");
	rotate_quality_text.emplace_back("Forward weighted pairs");
	rotate_quality_text.emplace_back("Forward weighted quadruplets");
#endif
	buttons[id_rotate_sampling_mode] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_rotate_sampling_mode,
			std::move(rotate_quality_text), rotate_sampling_mode,
			get_button_pos_for_label(Strings::Rotatequality_()), yForRow(++y_index), large_size);
	}
