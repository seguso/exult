// Shared modern movement speed notification UI; compiled within exult.cc.
static int    keyboard_speed_layer          = -1;
static Uint32 keyboard_speed_feedback_until = 0;

static void hide_keyboard_speed_feedback() {
	if (keyboard_speed_layer >= 0 && gwin) {
		gwin->destroy_layer(keyboard_speed_layer);
	}
	keyboard_speed_layer          = -1;
	keyboard_speed_feedback_until = 0;
}

static void show_keyboard_speed_feedback() {
	if (!gwin) {
		return;
	}
	hide_keyboard_speed_feedback();

	const char* text = modern_movement_speed.is_medium() ? "Speed: medium" : "Speed: fast";
	Shape_manager* sman = Shape_manager::get_instance();
	if (!sman) {
		return;
	}

	const int pad = 2;
	const int w   = sman->get_text_width(0, text) + 2 * pad;
	const int h   = sman->get_text_height(0) + 2 * pad;
	keyboard_speed_layer = gwin->create_layer("Keyboard speed", w, h, 255, 0, (1 << 20) - 2);
	if (keyboard_speed_layer < 0) {
		return;
	}
	gwin->layer_set_ui_kind(keyboard_speed_layer, Image_window::UiLayerTextEffects);

	Image_buffer8* buf = gwin->get_layer_ibuf(keyboard_speed_layer);
	if (!buf) {
		hide_keyboard_speed_feedback();
		return;
	}
	buf->clear_clip();
	buf->fill8(255);
	Image_buffer8* prev = gwin->push_render_target(buf);
	sman->paint_text(0, text, pad, pad);
	gwin->pop_render_target(prev);
	gwin->layer_set_dirty(keyboard_speed_layer);

	Image_window* iwin = gwin->get_win();
	int sx = 0;
	int sy = 0;
	iwin->game_to_screen(6, 6, false, sx, sy);
	const float scale = iwin->get_ui_scale_factor(Image_window::UiLayerTextEffects);
	gwin->layer_set_dest(
			keyboard_speed_layer, sx, sy,
			static_cast<int>(std::lround(static_cast<double>(w) * scale)),
			static_cast<int>(std::lround(static_cast<double>(h) * scale)));
	gwin->layer_set_visible(keyboard_speed_layer, true);
	keyboard_speed_feedback_until = SDL_GetTicks() + 1500;
}

