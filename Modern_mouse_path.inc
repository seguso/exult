// A* mouse target follows the highest reachable 3D surface under the cursor.
	if (modern_mouse_steering && modern_mouse_target_enabled) {
		const Tile_coord current = main_actor->get_tile();
		const std::vector<Tile_coord> candidates = modern_mouse_surface_targets(winx, winy);
		Actor_action* current_action = main_actor->get_action();
		const bool replan = current_action && current_action->following_smart_path();
		const bool mmt_log = mmt_profiling_enabled();
		const auto mmt_begin = std::chrono::steady_clock::now();
		int attempt = 0;
		bool routed = false;
		for (const Tile_coord& target : candidates) {
			if (target == current) {
				stop_actor();
				return;
			}
			begin_mmt_path_search();
			if (mmt_log) begin_mmt_astar_diagnostics();
			bool accepted = false;
			if (replan) {
				// A failed replan leaves the old A* action, time queue, walk
				// phase and path intact. Try the next lower surface.
				accepted = current_action->retarget_smart_path(main_actor, target);
				if (accepted) main_actor->set_frame_time(speed);
			} else {
				accepted = main_actor->walk_path_to_tile(target, speed);
			}
			if (mmt_log) {
				const auto diag = end_mmt_astar_diagnostics();
				std::ostringstream mmt_line;
				mmt_line << "[MMT_ASTAR] attempt=" << ++attempt
					<< " start=(" << current.tx << "," << current.ty << "," << current.tz << ")"
					<< " goal=(" << target.tx << "," << target.ty << "," << target.tz << ")"
					<< " replan=" << replan << " success=" << accepted
					<< " search_found=" << diag.found
					<< " expanded=" << diag.nodes_expanded
					<< " generated=" << diag.nodes_generated
					<< " max_open=" << diag.maximum_open
					<< " neighbor_checks=" << diag.neighbor_checks
					<< " steps=" << diag.result_steps
					<< " ms=" << diag.elapsed_ms;
				append_mmt_profile_line(mmt_line.str());
			}
			end_mmt_path_search();
			if (!accepted) continue;
			routed = true;
			break;
		}
		if (mmt_log) {
			const auto elapsed = std::chrono::duration<double, std::milli>(
				std::chrono::steady_clock::now() - mmt_begin).count();
			std::ostringstream mmt_line;
			mmt_line << "[MMT_RETARGET] start=(" << current.tx << ","
				<< current.ty << "," << current.tz << ")"
				<< " candidates=" << candidates.size() << " attempts=" << attempt
				<< " success=" << routed << " ms=" << elapsed;
			append_mmt_profile_line(mmt_line.str());
		}
		if (!routed) {
			show_modern_mouse_unreachable();
			return;
		}
		clear_modern_mouse_unreachable();
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
