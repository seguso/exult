// Optional feature state methods. Included by gamewin.cc to retain internal access.
void Game_window::resize_rotate_scene() {
	world_view.configure(win->get_game_width(), win->get_game_height());
	if (!rotate_world) {
		// Keep the default path allocation-free: the enlarged scene buffers only
		// exist while the optional rotated-world view is enabled.
		rotate_scene.reset();
		rotate_scene_2x.reset();
		rotate_scene_3x.reset();
		rotate_scene_hq3x.clear();
		return;
	}

	const int scene_size = world_view.get_scene_size();
	auto buffer = win->create_buffer(scene_size, scene_size);
	rotate_scene.reset(static_cast<Image_buffer8*>(buffer.release()));
	rotate_scene->set_offset(world_view.get_scene_offset_x(), world_view.get_scene_offset_y());

	// Allocate only the intermediate required by the selected A/B mode so the
	// clean2 baseline keeps its original memory/per-frame characteristics.
	rotate_scene_hq3x.clear();
	if (rotate_sampling_mode == 0) {
		auto buffer_2x = win->create_buffer(scene_size * 2, scene_size * 2);
		rotate_scene_2x.reset(static_cast<Image_buffer8*>(buffer_2x.release()));
		rotate_scene_3x.reset();
	} else if (rotate_sampling_mode <= 2) {
		auto buffer_3x = win->create_buffer(scene_size * 3, scene_size * 3);
		rotate_scene_3x.reset(static_cast<Image_buffer8*>(buffer_3x.release()));
		rotate_scene_2x.reset();
	} else if (rotate_sampling_mode <= 4) {
		rotate_scene_2x.reset();
		rotate_scene_3x.reset();
		rotate_scene_hq3x.resize(
				static_cast<size_t>(scene_size) *
				static_cast<size_t>(scene_size) * 9u);
	} else {
		// Weighted forward modes work directly from the original scene.
		rotate_scene_2x.reset();
		rotate_scene_3x.reset();
	}
}

void Game_window::set_rotate_world_enabled(bool enabled) {
	rotate_world = enabled;
	world_view.set_enabled(enabled);
	resize_rotate_scene();
	config->set("config/gameplay/rotate_world", enabled ? "yes" : "no", true);
	set_all_dirty();
	paint();
}

void Game_window::set_rotate_sampling_mode(int mode) {
#ifdef USE_HQ3X_SCALER
	mode = std::clamp(mode, 0, 7);
#else
	mode = std::clamp(mode, 0, 2);
#endif
	if (rotate_sampling_mode == mode) {
		return;
	}
	rotate_sampling_mode = mode;
	config->set("config/gameplay/rotate_sampling_mode", rotate_sampling_mode, true);
	static constexpr const char* names[] = {
			"Scale2x + 4 samples",
			"Scale3x + 4 samples",
			"Scale3x + 9 samples",
			"HQ3x RGB + 9 samples",
			"HQ3x RGB + 4 samples",
			"Forward weighted diagonal triplets",
			"Forward weighted diagonal pairs",
			"Forward weighted diagonal quadruplets"};
	cout << "Rotate quality: " << names[rotate_sampling_mode] << std::endl;
	if (rotate_world) {
		resize_rotate_scene();
		set_all_dirty();
		paint();
	}
}

