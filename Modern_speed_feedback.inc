// The upstream snapshot does not provide the newer overlay-layer API used by
// devmix. Keep this feature independent: use Exult's existing on-screen text
// effect until/unless that API is proposed as a separate common prerequisite.
static void show_keyboard_speed_feedback() {
	if (gwin && gwin->get_effects()) {
		gwin->get_effects()->center_text(
				modern_movement_speed.is_medium() ? "Speed: medium" : "Speed: fast");
	}
}
