// Optional setting callbacks, included by GameDisplayOptions_gump.cc.
void GameDisplayOptions_gump::choose_conversation_font_size() {
	bool escaped = false;
	const int value = gwin->get_gump_man()->prompt_for_number(
			5, 32, 1, conversation_font_size, this, &escaped);
	if (!escaped) {
		conversation_font_size = value;
		update_conversation_font_size_button();
		gwin->set_all_dirty();
	}
}

void GameDisplayOptions_gump::update_conversation_font_size_button() {
	constexpr int small_size = 44;
	// The exact row depends on which game/platform-specific options are present.
	// Reuse the current button's y coordinate instead of assuming a fixed row.
	const int button_y = buttons[id_conversation_font_size]
			? buttons[id_conversation_font_size]->get_y()
			: yForRow(16);
	buttons[id_conversation_font_size] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_conversation_font_size,
			std::to_string(conversation_font_size) + " px",
			get_button_pos_for_label(Strings::Conversationfontsize_()), button_y, small_size);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

