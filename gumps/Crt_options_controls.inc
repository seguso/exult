// Optional setting callbacks, included by GameDisplayOptions_gump.cc.
void GameDisplayOptions_gump::preview_crt() {
	gwin->preview_crt_filter_settings(
			crt_enabled != 0, crt_horizontal_strength, crt_vertical_strength,
			crt_brightness_compensation, crt_horizontal_width, crt_vertical_width, crt_beam_sigma);
}

void GameDisplayOptions_gump::choose_crt_sigma() {
	bool escaped = false;
	const int previous = crt_beam_sigma;
	const int value = gwin->get_gump_man()->prompt_for_number(
			10, 100, 1, previous, this, &escaped,
			[this](int v) { crt_beam_sigma = v; preview_crt(); });
	crt_beam_sigma = escaped ? previous : value;
	preview_crt();
	update_crt_sigma_button();
}

void GameDisplayOptions_gump::update_crt_sigma_button() {
	constexpr int small_size = 44;
	const int button_y = buttons[id_crt_beam_sigma]->get_y();
	buttons[id_crt_beam_sigma] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_sigma,
			std::to_string(crt_beam_sigma / 100.0f).substr(0, 4),
			get_button_pos_for_label("CRT beam sigma:"), button_y, small_size);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

void GameDisplayOptions_gump::choose_crt_brightness_compensation() {
	bool escaped = false;
	const int original = crt_brightness_compensation;
	const int value = gwin->get_gump_man()->prompt_for_number(
			50, 150, 1, original, this, &escaped,
			[this](int v) {
				crt_brightness_compensation = v;
				preview_crt();
			});
	crt_brightness_compensation = escaped ? original : value;
	preview_crt();
	update_crt_compensation_button();
}

void GameDisplayOptions_gump::update_crt_compensation_button() {
	constexpr int small_size = 44;
	const int button_y = buttons[id_crt_brightness_compensation]->get_y();
	buttons[id_crt_brightness_compensation] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_brightness_compensation,
			std::to_string(crt_brightness_compensation) + "%",
			get_button_pos_for_label("CRT brightness comp:"), button_y, small_size);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

void GameDisplayOptions_gump::choose_crt_horizontal_strength() {
	bool escaped = false;
	const int original = crt_horizontal_strength;
	const int value = gwin->get_gump_man()->prompt_for_number(
			0, 20, 1, crt_horizontal_strength, this, &escaped,
			[this](int v) { crt_horizontal_strength = v; preview_crt(); });
	if (!escaped) {
		crt_horizontal_strength = value;
		update_crt_strength_buttons();
		gwin->set_all_dirty();
	} else {
		crt_horizontal_strength = original;
		preview_crt();
	}
}

void GameDisplayOptions_gump::choose_crt_vertical_strength() {
	bool escaped = false;
	const int original = crt_vertical_strength;
	const int value = gwin->get_gump_man()->prompt_for_number(
			0, 40, 1, crt_vertical_strength, this, &escaped,
			[this](int v) { crt_vertical_strength = v; preview_crt(); });
	if (!escaped) {
		crt_vertical_strength = value;
		update_crt_strength_buttons();
		gwin->set_all_dirty();
	} else {
		crt_vertical_strength = original;
		preview_crt();
	}
}

void GameDisplayOptions_gump::update_crt_strength_buttons() {
	constexpr int small_size = 44;
	if (buttons[id_crt_horizontal_strength]) {
		const int button_y = buttons[id_crt_horizontal_strength]->get_y();
		buttons[id_crt_horizontal_strength] = std::make_unique<GameDisplayOptions_button>(
				this, &GameDisplayOptions_gump::choose_crt_horizontal_strength,
				std::to_string(crt_horizontal_strength) + "%",
				get_button_pos_for_label("CRT horizontal scanlines:"), button_y, small_size);
	}
	if (buttons[id_crt_vertical_strength]) {
		const int button_y = buttons[id_crt_vertical_strength]->get_y();
		buttons[id_crt_vertical_strength] = std::make_unique<GameDisplayOptions_button>(
				this, &GameDisplayOptions_gump::choose_crt_vertical_strength,
				(std::to_string(crt_vertical_strength / 2) + (crt_vertical_strength % 2 ? ".5%" : "%")),
				get_button_pos_for_label("CRT vertical mask:"), button_y, small_size);
	}
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

