// A* mouse-target replan path inside Game_window::start_actor_alt.
	if (modern_mouse_steering && modern_mouse_target_enabled) {
		const int lift = main_actor->get_lift();
		const int liftpixels = 4 * lift;
		const int tx = (get_scrolltx() + (winx + liftpixels) / c_tilesize + c_num_tiles) % c_num_tiles;
		const int ty = (get_scrollty() + (winy + liftpixels) / c_tilesize + c_num_tiles) % c_num_tiles;
		const Tile_coord current = main_actor->get_tile();
		if (tx == current.tx && ty == current.ty) {
			stop_actor();
			return;
		}
		// Astar's Actor_pathfinder_client adjusts tz as each step climbs or
		// descends. A fixed destination lift would incorrectly require the
		// clicked tile to be reachable at the Avatar's starting elevation.
		// -1 means accept the naturally reached elevation at these x/y coords.
		const Tile_coord target(tx, ty, -1);
		Actor_action* current_action = main_actor->get_action();
		bool routed = false;
		if (current_action && current_action->following_smart_path()) {
			// Match the currently selected Shift speed without restarting the
			// walking action, animation frame or time-queue entry.
			main_actor->set_frame_time(speed);
			// A failed replan leaves the existing path and animation intact.
			current_action->retarget_smart_path(main_actor, target);
			routed = true;
		} else {
			// Initial press: create a walking action only once.
			routed = main_actor->walk_path_to_tile(target, speed) != 0;
		}
		if (!routed) {
			return;
		}
		if (walk_in_formation && main_actor->get_action()) {
			main_actor->get_action()->set_get_party(true);
		} else {
			main_actor->get_followers();
		}
		return;
	}

