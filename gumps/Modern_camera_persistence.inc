// Feature-specific settings persistence; included by GameDisplayOptions_gump.cc.
	if (page == Page::movement) {

	gwin->set_modern_movement_tau_ms(modern_tau_values[std::clamp(modern_tau, 0, static_cast<int>(std::size(modern_tau_values)) - 1)]);
	gwin->set_smooth_scrolling_enabled(modern_smooth != 0);
	gwin->set_smooth_avatar_walk_enabled(smooth_avatar_walk != 0);

	}
