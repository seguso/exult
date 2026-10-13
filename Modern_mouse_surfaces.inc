/*
 * Modern Mouse Target: resolve a mouse click into real 3-D walk destinations.
 *
 * Exult's rendering projects each lift 4 pixels towards the upper left in
 * BOTH axes. The same screen pixel therefore denotes a different (x,y)
 * for every possible z. Unlike the old target(x,y,-1), these goals require
 * A* to end at a particular WALKABLE HEIGHT.
 *
 * Find the visible solid 3-D sprite under the cursor, even if it is not
 * "findable" by normal item interaction. Try its upper walking surface first.
 * Then descend through supported horizontal surfaces to terrain z=0,
 * reprojecting x/y at EACH lift. This is a target lookup: it never removes
 * objects or changes the game's actual collision/pathfinding rules.
 */
std::vector<Tile_coord> Game_window::modern_mouse_surface_targets(int winx, int winy) {
    const auto wrap = Modern_mouse_surface_geometry::wrap;
    const auto floor_div = Modern_mouse_surface_geometry::floor_div;
    const auto project = [&](int z) {
        return Modern_mouse_surface_geometry::project(
                winx, winy, get_scrolltx(), get_scrollty(), z);
    };

    const int visible_ceiling = std::clamp(get_render_skip_lift() - 1, 0, 254);
    // Bounded sprite search: do not scan 255 levels of chunks on every
    // 500-ms retarget. A high-floor Avatar can still click floors above it.
    const int scan_ceiling = std::min(visible_ceiling,
            std::max(64, static_cast<int>(main_actor->get_lift()) + 24));
    const int min_tx = get_scrolltx() + floor_div(winx, c_tilesize);
    const int min_ty = get_scrollty() + floor_div(winy, c_tilesize);
    const int max_tx = get_scrolltx() + floor_div(winx + 4 * scan_ceiling, c_tilesize)
            + c_tiles_per_chunk * 2;
    const int max_ty = get_scrollty() + floor_div(winy + 4 * scan_ceiling, c_tilesize)
            + c_tiles_per_chunk * 2;

    Game_object* clicked = nullptr;
    bool clicked_transparent = true;
    // Same candidate region and sprite-pixel check as find_object(), but
    // solid stair/roof pieces may intentionally be non-interactive.
    for (int cy = floor_div(min_ty, c_tiles_per_chunk);
         cy <= floor_div(max_ty, c_tiles_per_chunk); ++cy) {
        for (int cx = floor_div(min_tx, c_tiles_per_chunk);
             cx <= floor_div(max_tx, c_tiles_per_chunk); ++cx) {
            Map_chunk* chunk = map->get_chunk(
                    wrap(cx, c_num_chunks), wrap(cy, c_num_chunks));
            if (!chunk) continue;
            Object_iterator it(chunk->get_objects());
            Game_object* obj;
            while ((obj = it.get_next()) != nullptr) {
                if (obj->as_actor() || obj->get_lift() >= get_render_skip_lift())
                    continue;
                const Shape_info& info = obj->get_info();
                const int top = obj->get_lift() + info.get_3d_height();
                if (top <= 0 || top > scan_ceiling) continue;
                // Only physically elevated scenery can define a walkable
                // surface; never treat small items or NPCs as the floor.
                if (!info.is_solid() && info.get_shape_class() != Shape_info::building)
                    continue;
                Shape_frame* sprite = obj->get_shape();
                if (!sprite) continue;
                int ox, oy;
                get_shape_location(obj, ox, oy);
                if (!get_shape_rect(obj).has_world_point(winx, winy)
                    || !sprite->has_point(winx - ox, winy - oy))
                    continue;
                const bool transparent = info.is_transparent();
                if (!clicked || clicked->lt(*obj) != 0 || clicked_transparent) {
                    if (!transparent || clicked_transparent) {
                        clicked = obj;
                        clicked_transparent = transparent;
                    }
                }
            }
        }
    }

    const Tile_coord avatar = main_actor->get_tile();
    const int highest = clicked
        ? std::min(visible_ceiling,
                   static_cast<int>(clicked->get_lift()) + clicked->get_info().get_3d_height())
        : std::min(visible_ceiling, static_cast<int>(avatar.tz));

    Tile_coord top_of_clicked;
    bool has_clicked_top = false;

    if (clicked) {
        Tile_coord top = project(highest);
        const TileRect footprint = clicked->get_footprint();
        // Clicking a vertical wall/step face may project outside the object's
        // top footprint. Target the closest tile of its TOP face instead of
        // accidentally selecting the ground behind the sprite.
        if (footprint.w > 0 && footprint.h > 0
            && !footprint.has_world_point(top.tx, top.ty)) {
            const auto closest = [&](int coord, int origin, int count) {
                int best = coord;
                int best_distance = c_num_tiles + 1;
                for (int i = 0; i < count; ++i) {
                    const int candidate = wrap(origin + i, c_num_tiles);
                    const int direct = std::abs(candidate - coord);
                    const int dist = std::min(direct, c_num_tiles - direct);
                    if (dist < best_distance) {
                        best_distance = dist;
                        best = candidate;
                    }
                }
                return best;
            };
            top.tx = closest(top.tx, footprint.x, footprint.w);
            top.ty = closest(top.ty, footprint.y, footprint.h);
        }
        top_of_clicked = top;
        has_clicked_top = true;
    }

    // Every lower lift uses a fresh x/y projection of the same clicked pixel.
    // Only genuinely supported surfaces are eligible for A*.
    return Modern_mouse_surface_geometry::descending(
            winx, winy, get_scrolltx(), get_scrollty(), highest,
            [this](const Tile_coord& p) {
                Map_chunk* chunk = map->get_chunk_safely(
                        p.tx / c_tiles_per_chunk, p.ty / c_tiles_per_chunk);
                return chunk && chunk->get_highest_blocked(
                        p.tz, p.tx % c_tiles_per_chunk,
                        p.ty % c_tiles_per_chunk) == p.tz - 1;
            },
            has_clicked_top ? &top_of_clicked : nullptr);
}
