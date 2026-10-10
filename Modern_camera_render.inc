// Modern visual camera and avatar smoothing. Included by gamerend.cc.
bool Game_window::update_smooth_avatar_walk(uint32 ticks) {
	if (!smooth_avatar_walk_enabled || !modern_movement_enabled || !camera_actor) {
		const bool changed = avposx_ld != 0 || avposy_ld != 0;
		avposx_ld = avposy_ld = 0;
		avatar_walk_last_tx = avatar_walk_last_ty = -1;
		const bool followers_changed = !party_walk_visuals.empty();
		party_walk_visuals.clear();
		return changed || followers_changed;
	}
	const Tile_coord tile = camera_actor->get_tile();
	if (avatar_walk_last_tx < 0) {
		avatar_walk_last_tx = tile.tx;
		avatar_walk_last_ty = tile.ty;
		avatar_walk_step_ticks = ticks;
		return false;
	}
	int dx = avatar_walk_last_tx - tile.tx;
	int dy = avatar_walk_last_ty - tile.ty;
	while (dx < -c_num_tiles / 2) dx += c_num_tiles;
	while (dx > c_num_tiles / 2) dx -= c_num_tiles;
	while (dy < -c_num_tiles / 2) dy += c_num_tiles;
	while (dy > c_num_tiles / 2) dy -= c_num_tiles;
	if (dx || dy) {
		avatar_walk_last_tx = tile.tx;
		avatar_walk_last_ty = tile.ty;
		avatar_walk_step_ticks = ticks;
		// Only interpolate adjacent logical steps; teleports remain immediate.
		avatar_walk_from_dx = (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) ? dx * c_tilesize : 0;
		avatar_walk_from_dy = (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) ? dy * c_tilesize : 0;
	}
	const int oldx = avposx_ld, oldy = avposy_ld;
	const int duration = std::max(1, camera_actor->get_frame_time());
	const float progress = std::min(1.0f, static_cast<float>(ticks - avatar_walk_step_ticks) / duration);
	avposx_ld = static_cast<int>(std::lround(avatar_walk_from_dx * (1.0f - progress)));
	avposy_ld = static_cast<int>(std::lround(avatar_walk_from_dy * (1.0f - progress)));
	bool changed = avposx_ld != oldx || avposy_ld != oldy;
	// Party member interpolation is visual only. Use each NPC's own last tile
	// and frame time; their AI schedules, formation, collision and movement
	// decisions remain untouched.
	std::map<int, Party_walk_visual> next_party;
	for (int i = 0; i < party_man->get_count(); ++i) {
		Actor* member = get_npc(party_man->get_member(i));
		if (!member || member == camera_actor || !member->is_in_party()) {
			continue;
		}
		const int npc_num = member->get_npc_num();
		Party_walk_visual state;
		const auto previous = party_walk_visuals.find(npc_num);
		if (previous != party_walk_visuals.end() && previous->second.actor == member) {
			state = previous->second;
		}
		const Tile_coord pos = member->get_tile();
		if (state.last_tx < 0) {
			state.actor = member;
			state.last_tx = pos.tx;
			state.last_ty = pos.ty;
			state.step_ticks = ticks;
		} else {
			int step_x = state.last_tx - pos.tx;
			int step_y = state.last_ty - pos.ty;
			while (step_x < -c_num_tiles / 2) step_x += c_num_tiles;
			while (step_x > c_num_tiles / 2) step_x -= c_num_tiles;
			while (step_y < -c_num_tiles / 2) step_y += c_num_tiles;
			while (step_y > c_num_tiles / 2) step_y -= c_num_tiles;
			if (step_x || step_y) {
				// Formation can move a follower by up to two tiles per axis in
				// one logical step. Start from its *current visual position*,
				// not the preceding tile, so consecutive steps cannot rewind
				// the sprite or make it oscillate.
				const bool formation_step = std::abs(step_x) <= 2 && std::abs(step_y) <= 2;
				const int carry_x = state.offset_x;
				const int carry_y = state.offset_y;
				state.last_tx = pos.tx;
				state.last_ty = pos.ty;
				state.step_ticks = ticks;
				state.from_dx = formation_step ? carry_x + step_x * c_tilesize : 0;
				state.from_dy = formation_step ? carry_y + step_y * c_tilesize : 0;
			}
		}
		// Formation steps are driven directly by Party_manager::step(), not
		// necessarily by an Actor action: frame_time can be zero. Use the
		// Avatar's actual walk cadence, then the standard delay as fallback.
		int step_ms = member->get_frame_time();
		if (step_ms <= 0) {
			step_ms = camera_actor->get_frame_time();
		}
		if (step_ms <= 0) {
			step_ms = get_std_delay();
		}
		const float duration = static_cast<float>(std::max(1, step_ms));
		const float progress = std::min(1.0f, static_cast<float>(ticks - state.step_ticks) / duration);
		state.offset_x = static_cast<int>(std::lround(state.from_dx * (1.0f - progress)));
		state.offset_y = static_cast<int>(std::lround(state.from_dy * (1.0f - progress)));
		if (previous == party_walk_visuals.end()
			|| previous->second.offset_x != state.offset_x || previous->second.offset_y != state.offset_y) {
			changed = true;
		}
		next_party.emplace(npc_num, state);
	}
	if (next_party.size() != party_walk_visuals.size()) {
		changed = true;
	}
	party_walk_visuals.swap(next_party);
	return changed;
}

void Game_window::paint_current_view() {
	if (!modern_movement_enabled || !smooth_cam_valid || painting_smooth_view) {
		paint();
		return;
	}
	painting_smooth_view = true;

	const int world_pixels = c_num_tiles * c_tilesize;
	const auto wrap_pixel = [&](int p) {
		p %= world_pixels;
		if (p < 0) {
			p += world_pixels;
		}
		return p;
	};

	const int saved_scrolltx = scrolltx;
	const int saved_scrollty = scrollty;
	const int saved_scrolltx_lo = scrolltx_lo;
	const int saved_scrollty_lo = scrollty_lo;
	const int saved_avposx_ld = avposx_ld;
	const int saved_avposy_ld = avposy_ld;

	const int render_x = static_cast<int>(std::lround(smooth_cam_x));
	const int render_y = static_cast<int>(std::lround(smooth_cam_y));
	const int wrapped_x = wrap_pixel(render_x);
	const int wrapped_y = wrap_pixel(render_y);
	scrolltx = wrapped_x / c_tilesize;
	scrollty = wrapped_y / c_tilesize;
	scrolltx_lo = wrapped_x % c_tilesize;
	scrollty_lo = wrapped_y % c_tilesize;
	// Preserve the independently interpolated avatar offset while rendering.

	paint();

	scrolltx = saved_scrolltx;
	scrollty = saved_scrollty;
	scrolltx_lo = saved_scrolltx_lo;
	scrollty_lo = saved_scrollty_lo;
	avposx_ld = saved_avposx_ld;
	avposy_ld = saved_avposy_ld;
	painting_smooth_view = false;
}

void Game_window::reset_velocity_camera() {
	smooth_cam_stage1_x = 0.0;
	smooth_cam_stage1_y = 0.0;
	smooth_cam_stage2_x = 0.0;
	smooth_cam_stage2_y = 0.0;
	smooth_cam_x = 0.0;
	smooth_cam_y = 0.0;
	smooth_cam_last_ticks = 0;
	smooth_cam_valid = false;
}

	bool Game_window::paint_velocity_camera(uint32 ticks) {
	if (!camera_actor) {
		reset_velocity_camera();
		return false;
	}
	const bool avatar_changed = update_smooth_avatar_walk(ticks);

	const int world_pixels = c_num_tiles * c_tilesize;
	const double half_world = static_cast<double>(world_pixels) * 0.5;

	// The logical target still moves in discrete tile-sized steps. Feed it
	// through three identical first-order low-pass filters in cascade:
	//
	//   target -> stage 1 -> stage 2 -> rendered camera
	//
	// Each stage is a convex combination of its input and previous value, so a
	// stationary step target is approached monotonically and can never be
	// overshot. In the continuous-time equivalent H(s)=1/(1+tau*s)^3, camera
	// position, velocity and acceleration are continuous even when the input
	// target itself jumps.
	const Tile_coord actor_pos = camera_actor->get_tile();
	const int tw = get_width() / c_tilesize;
	const int th = get_height() / c_tilesize;
	double target_x = static_cast<double>((actor_pos.tx - tw / 2) * c_tilesize);
	double target_y = static_cast<double>((actor_pos.ty - th / 2) * c_tilesize);

	if (!smooth_cam_valid) {
		// Start exactly at the currently rendered logical camera. Initializing all
		// three stages to the same point avoids any startup transient.
		const double initial_x = static_cast<double>(scrolltx * c_tilesize + scrolltx_lo);
		const double initial_y = static_cast<double>(scrollty * c_tilesize + scrollty_lo);
		smooth_cam_stage1_x = initial_x;
		smooth_cam_stage1_y = initial_y;
		smooth_cam_stage2_x = initial_x;
		smooth_cam_stage2_y = initial_y;
		smooth_cam_x = initial_x;
		smooth_cam_y = initial_y;
		smooth_cam_last_ticks = ticks;
		smooth_cam_valid = true;
		return false;
	}

	// Choose the wrapped target image nearest to the current unwrapped camera.
	// Use the rendered stage as the reference so all three stages remain on one
	// continuous unwrapped world image while crossing the map seam.
	const auto nearest_wrapped = [&](double target, double current) {
		double delta = target - current;
		while (delta > half_world) {
			delta -= world_pixels;
		}
		while (delta < -half_world) {
			delta += world_pixels;
		}
		return current + delta;
	};
	target_x = nearest_wrapped(target_x, smooth_cam_x);
	target_y = nearest_wrapped(target_y, smooth_cam_y);

	uint32 elapsed_ms = ticks - smooth_cam_last_ticks;
	smooth_cam_last_ticks = ticks;
	if (elapsed_ms > 50) {
		elapsed_ms = 50;
	}
	const double dt = static_cast<double>(elapsed_ms) / 1000.0;
	if (dt <= 0.0) {
		return false;
	}

	// Map/teleport discontinuities should not spend seconds flowing across the
	// world. Normal walking produces only tile-sized target steps, nowhere near
	// this threshold, so snap all stages together only for genuine jumps.
	const double error_x = target_x - smooth_cam_x;
	const double error_y = target_y - smooth_cam_y;
	if (std::abs(error_x) > 128.0 || std::abs(error_y) > 128.0) {
		smooth_cam_stage1_x = target_x;
		smooth_cam_stage1_y = target_y;
		smooth_cam_stage2_x = target_x;
		smooth_cam_stage2_y = target_y;
		smooth_cam_x = target_x;
		smooth_cam_y = target_y;
	}

	const int old_render_x = static_cast<int>(std::lround(smooth_cam_x));
	const int old_render_y = static_cast<int>(std::lround(smooth_cam_y));

	// Time constant of each of the three cascaded stages. The effective group
	// delay while following steady motion is roughly 3*tau (165 ms here).
	// alpha is the exact frame-rate-independent update of one first-order stage.
	const double tau = static_cast<double>(modern_movement_tau_ms) / 1000.0;
	const double alpha = 1.0 - std::exp(-dt / tau);

	// IMPORTANT: update all stages from the *previous* frame's stage values.
	// Doing the assignments sequentially with freshly updated inputs would
	// partially collapse the cascade and lose the intended third-order
	// smoothness.
	const double old_stage1_x = smooth_cam_stage1_x;
	const double old_stage1_y = smooth_cam_stage1_y;
	const double old_stage2_x = smooth_cam_stage2_x;
	const double old_stage2_y = smooth_cam_stage2_y;
	const double old_cam_x = smooth_cam_x;
	const double old_cam_y = smooth_cam_y;

	smooth_cam_stage1_x = old_stage1_x + alpha * (target_x - old_stage1_x);
	smooth_cam_stage1_y = old_stage1_y + alpha * (target_y - old_stage1_y);
	smooth_cam_stage2_x = old_stage2_x + alpha * (old_stage1_x - old_stage2_x);
	smooth_cam_stage2_y = old_stage2_y + alpha * (old_stage1_y - old_stage2_y);
	smooth_cam_x = old_cam_x + alpha * (old_stage2_x - old_cam_x);
	smooth_cam_y = old_cam_y + alpha * (old_stage2_y - old_cam_y);

	const int render_x = static_cast<int>(std::lround(smooth_cam_x));
	const int render_y = static_cast<int>(std::lround(smooth_cam_y));
	const bool camera_changed = render_x != old_render_x || render_y != old_render_y;
	if (!camera_changed && !avatar_changed && !is_dirty()) {
		return false;
	}

	const int saved_scrolltx = scrolltx;
	const int saved_scrollty = scrollty;

	const auto wrap_pixel = [&](int p) {
		p %= world_pixels;
		if (p < 0) {
			p += world_pixels;
		}
		return p;
	};
	const int wrapped_x = wrap_pixel(render_x);
	const int wrapped_y = wrap_pixel(render_y);
	scrolltx = wrapped_x / c_tilesize;
	scrollty = wrapped_y / c_tilesize;
	scrolltx_lo = wrapped_x % c_tilesize;
	scrollty_lo = wrapped_y % c_tilesize;

	// The independent avatar walk offset is retained while the camera follows
	// its own three-stage trajectory. Neither alters the logical tile path.

	painting_smooth_view = true;
	paint();
	painting_smooth_view = false;

	scrolltx = saved_scrolltx;
	scrollty = saved_scrollty;
	scrolltx_lo = 0;
	scrollty_lo = 0;
	// Keep avatar offsets for out-of-band smooth-view repaints.
	return true;
}

/*
 *  Paint the flat (non-rle) shapes in a chunk.
 */

void Game_render::paint_chunk_flats(
		int cx, int cy,       // Chunk coords (0 - 12*16).
		int xoff, int yoff    // Pixel offset of top-of-screen.
) {
	Game_window* gwin  = Game_window::get_instance();
	Map_chunk*   olist = gwin->map->get_chunk(cx, cy);
	// Paint flat tiles.
	Image_buffer8* cflats = olist->get_rendered_flats();
	if (cflats) {
		gwin->win->copy8(cflats->get_bits(), c_chunksize, c_chunksize, xoff, yoff);
	}
}

/*
 *  Paint the flat RLE (terrain) shapes in a chunk.
 */

void Game_render::paint_chunk_flat_rles(
		int cx, int cy,       // Chunk coords (0 - 12*16).
		int xoff, int yoff    // Pixel offset of top-of-screen.
) {
	ignore_unused_variable_warning(xoff, yoff);
	Game_window*         gwin  = Game_window::get_instance();
	Map_chunk*           olist = gwin->map->get_chunk(cx, cy);
	Flat_object_iterator next(olist);    // Do flat RLE objects.
	Game_object*         obj;
	while ((obj = next.get_next()) != nullptr) {
		obj->paint();
	}
}

/*
 *  Paint a chunk's objects, left-to-right, top-to-bottom.
 *
 *  Output: # light sources found.
 */

int Game_render::paint_chunk_objects(
		int cx, int cy    // Chunk coords (0 - 12*16).
) {
	Game_object*      obj;
	Game_window*      gwin          = Game_window::get_instance();
	Map_chunk*        olist         = gwin->map->get_chunk(cx, cy);
	int               light_sources = 0;    // Also check for light sources.
	Main_actor* const main_actor    = gwin->get_main_actor();
	if (main_actor != nullptr) {
		const auto& lights = gwin->is_in_dungeon() ? olist->get_dungeon_lights() : olist->get_non_dungeon_lights();
		for (const auto& light_obj : lights) {
			const Shape_info& info = light_obj->get_info();
			if (info.get_object_light(light_obj->get_framenum()) > 0) {
				// Count light sources.
				light_sources += get_light_strength(light_obj, main_actor);
			}
		}
	}
	skip = gwin->get_render_skip_lift();
	Nonflat_object_iterator next(olist);

	while ((obj = next.get_next()) != nullptr) {
		if (obj->render_seq != render_seq) {
			paint_object(obj);
		}
	}

	skip = 255;    // Back to a safe #.
	return light_sources;
}

/*
 *  Render an object after first rendering any that it depends on.
 */

void Game_render::paint_object(Game_object* obj) {
	const int lift = obj->get_lift();
	if (lift >= skip) {
		return;
	}
	obj->render_seq                          = render_seq;
	const Game_object::Game_object_set& deps = obj->get_dependencies();
	for (auto* dep : deps) {
		if (dep && dep->render_seq != render_seq) {
			paint_object(dep);
		}
	}
	int bbox_x, bbox_y;
	Game_window::get_instance()->get_shape_location(obj, bbox_x, bbox_y);

	// paint bbox back
	if (bbox_palindex != -1) {
		obj->get_info().paint_bbox(
				bbox_x, bbox_y, obj->get_framenum(), Game_window::get_instance()->get_win()->get_ib8(), bbox_palindex, 2);
	}
	obj->paint();    // Finally, paint this one.
	// paint bbox front
	if (bbox_palindex != -1) {
		obj->get_info().paint_bbox(
				bbox_x, bbox_y, obj->get_framenum(), Game_window::get_instance()->get_win()->get_ib8(), bbox_palindex, 1);
	}
}

/*
 *  Paint 'dirty' rectangle.
 */
