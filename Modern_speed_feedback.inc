// Shared notification overlays for keyboard speed and Modern Mouse Target.
static int keyboard_speed_layer = -1;
static Uint32 keyboard_speed_feedback_until = 0;
static int mouse_unreachable_layer = -1;
static Uint32 mouse_unreachable_until = 0;
static bool mouse_unreachable_feedback_enabled = false;

static void hide_feedback_layer(int& layer, Uint32& until) {
    if (layer >= 0 && gwin) gwin->destroy_layer(layer);
    layer = -1;
    until = 0;
}

static void paint_feedback_layer(int& layer, Uint32& until,
        const char* label, const char* text, bool bottom_left) {
    if (!gwin) return;
    hide_feedback_layer(layer, until);
    Shape_manager* sman = Shape_manager::get_instance();
    if (!sman) return;
    constexpr int pad = 2;
    const int w = sman->get_text_width(0, text) + 2 * pad;
    const int h = sman->get_text_height(0) + 2 * pad;
    layer = gwin->create_layer(label, w, h, 255, 0, (1 << 20) - 2);
    if (layer < 0) return;
    gwin->layer_set_ui_kind(layer, Image_window::UiLayerTextEffects);
    Image_buffer8* buf = gwin->get_layer_ibuf(layer);
    if (!buf) {
        hide_feedback_layer(layer, until);
        return;
    }
    buf->clear_clip();
    buf->fill8(255);
    Image_buffer8* prev = gwin->push_render_target(buf);
    sman->paint_text(0, text, pad, pad);
    gwin->pop_render_target(prev);
    gwin->layer_set_dirty(layer);

    Image_window* iwin = gwin->get_win();
    const float scale = iwin->get_ui_scale_factor(Image_window::UiLayerTextEffects);
    const int sw = static_cast<int>(std::lround(static_cast<double>(w) * scale));
    const int sh = static_cast<int>(std::lround(static_cast<double>(h) * scale));
    int sx = 0, sy = 0;
    // In-game pixel coordinates before mapping to the scaled display.
    const int gx = 6;
    const int gy = bottom_left ? std::max(6, gwin->get_height() - h - 6) : 6;
    iwin->game_to_screen(gx, gy, false, sx, sy);
    gwin->layer_set_dest(layer, sx, sy, sw, sh);
    gwin->layer_set_visible(layer, true);
    until = SDL_GetTicks() + 1500;
}

static void hide_keyboard_speed_feedback() {
    hide_feedback_layer(keyboard_speed_layer, keyboard_speed_feedback_until);
}

static void show_keyboard_speed_feedback() {
    paint_feedback_layer(keyboard_speed_layer, keyboard_speed_feedback_until,
        "Keyboard speed", modern_movement_speed.is_medium() ? "Speed: medium" : "Speed: fast", false);
}

// Exported to the MMT path code in gamewin.cc.
void set_modern_mouse_unreachable_feedback_enabled(bool enabled) {
    mouse_unreachable_feedback_enabled = enabled;
}

void show_modern_mouse_unreachable() {
    // Display only for an explicit RMB down/up probe, never for held
    // mouse retargets. The route computation itself is unchanged.
    if (!mouse_unreachable_feedback_enabled) return;
    paint_feedback_layer(mouse_unreachable_layer, mouse_unreachable_until,
                         "Mouse target unreachable", "Can't reach", true);
}

void clear_modern_mouse_unreachable() {
    // Any successful path, including during RMB hold, dismisses it immediately.
    hide_feedback_layer(mouse_unreachable_layer, mouse_unreachable_until);
}
