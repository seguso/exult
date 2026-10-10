// Feature-specific settings persistence; included by GameDisplayOptions_gump.cc.
	if (page == Page::rotation) {

	gwin->set_rotate_sampling_mode(rotate_sampling_mode);
	gwin->set_rotate_world_enabled(rotate_world != 0);
	}
