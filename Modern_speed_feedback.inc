// The upstream snapshot does not provide the newer overlay-layer API used by
// devmix. Keep this feature independent: use Exult's existing on-screen text
// effect until/unless that API is proposed as a separate common prerequisite.
static void show_keyboard_speed_feedback() {
	if (gwin && gwin->get_effects()) {
		gwin->get_effects()->center_text(
				modern_movement_speed.is_medium() ? "Speed: medium" : "Speed: fast");
	}
}

// This branch intentionally predates devmix's overlay-layer API. Keep the
// movement backport independent of unrelated renderer changes.
static bool modern_mouse_unreachable_on_down = false;
void set_modern_mouse_unreachable_feedback_enabled(bool on) {
    modern_mouse_unreachable_on_down = on;
}
void show_modern_mouse_unreachable() {
    if (modern_mouse_unreachable_on_down && gwin && gwin->get_effects())
        gwin->get_effects()->center_text("Can't reach");
}
void clear_modern_mouse_unreachable() {
    // The legacy text effect manages its own short lifetime.
}
