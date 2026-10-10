// Optional fonts settings page; included in GameDisplayOptions_gump::build_buttons.
	if (page == Page::fonts) {
buttons[id_conversation_font] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_conversation_font, yesNo, conversation_font,
			get_button_pos_for_label(Strings::Readableconversationfont_()), yForRow(++y_index), small_size);

	buttons[id_conversation_font_size] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_conversation_font_size,
			std::to_string(conversation_font_size) + " px",
			get_button_pos_for_label(Strings::Conversationfontsize_()), yForRow(++y_index), small_size);
	buttons[id_conversation_font_tracking] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_conversation_font_tracking,
			std::vector<std::string>{"Normal", "+1 px"},
			conversation_font_tracking, get_button_pos_for_label("Character spacing:"),
			yForRow(++y_index), small_size);
	buttons[id_conversation_font_file] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_conversation_font_file,
			"Browse...", get_button_pos_for_label("TTF / OTF file:"), yForRow(++y_index), 65);
	buttons[id_conversation_system_font] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_installed_conversation_font,
			"Browse...", get_button_pos_for_label("Installed fonts:"), yForRow(++y_index), 65);
	buttons[id_conversation_font_reset] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::reset_conversation_font,
			"Reset", get_button_pos_for_label("Restore automatic font:"), yForRow(++y_index), 65);
	}
