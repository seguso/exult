// A* mouse target follows the highest reachable 3D surface under the cursor.
	if (modern_mouse_steering && modern_mouse_target_enabled) {
		const Tile_coord current = main_actor->get_tile();
		const std::vector<Tile_coord> candidates = modern_mouse_surface_targets(winx, winy);
		Actor_action* current_action = main_actor->get_action();
		const bool replan = current_action && current_action->following_smart_path();
		bool routed = false;
		Tile_coord chosen;
		std::size_t attempts = 0;
		for (const Tile_coord& target : candidates) {
			++attempts;
			if (target == current) {
				stop_actor();
				return;
			}
			if (replan) {
				// A failed replan leaves the old A* action, time queue, walk
				// phase and path intact. Try the next lower surface.
				if (!current_action->retarget_smart_path(main_actor, target))
					continue;
				main_actor->set_frame_time(speed);
			} else {
				if (!main_actor->walk_path_to_tile(target, speed))
					continue;
			}
			routed = true;
			chosen = target;
			break;
		}
		if (!routed) {
			show_modern_mouse_unreachable();
			std::cerr << "[MMT/A*] no reachable clicked surface: from=("
				<< current.tx << ',' << current.ty << ',' << current.tz
				<< ") screen=(" << winx << ',' << winy
				<< ") tested=" << attempts
				<< "; old route kept if retargeting.\n";
			return;
		}
		clear_modern_mouse_unreachable();
		if (attempts > 1) {
			std::cerr << "[MMT/A*] surface fallback: from=("
				<< current.tx << ',' << current.ty << ',' << current.tz
				<< ") selected=(" << chosen.tx << ',' << chosen.ty << ','
				<< chosen.tz << ") after " << attempts << " candidates.\n";
		}
		// The route-heading state is owned by the ACTION, so it survives
		// 500-ms A* recalculations without resetting the walking frame.
		if (main_actor->get_action())
			main_actor->get_action()->enable_modern_mouse_heading();
		if (walk_in_formation && main_actor->get_action())
			main_actor->get_action()->set_get_party(true);
		else
			main_actor->get_followers();
		return;
	}
