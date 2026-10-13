/*
 * Copyright (C) 2026 The Exult Team
 * GPL-2.0-or-later
 *
 * Geometry-only Modern Mouse Target surface selection.
 * Pure and independently testable: no mutations of world objects, tiles
 * or A* paths. A click at different lifts projects to DIFFERENT world tiles.
 */
#ifndef EXULT_MODERN_MOUSE_SURFACE_GEOMETRY_H
#define EXULT_MODERN_MOUSE_SURFACE_GEOMETRY_H

#include "tiles.h"

#include <algorithm>
#include <vector>

class Modern_mouse_surface_geometry {
public:
    static int wrap(int coord, int extent) {
        coord %= extent;
        return coord < 0 ? coord + extent : coord;
    }

    static int floor_div(int value, int divisor) {
        return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
    }

    static Tile_coord project(int screen_x, int screen_y,
            int scrolltx, int scrollty, int lift) {
        return Tile_coord(
                wrap(scrolltx + floor_div(screen_x + 4 * lift, c_tilesize), c_num_tiles),
                wrap(scrollty + floor_div(screen_y + 4 * lift, c_tilesize), c_num_tiles),
                lift);
    }

    template<class HasSupport>
    static std::vector<Tile_coord> descending(
            int screen_x, int screen_y, int scrolltx, int scrollty,
            int highest, HasSupport&& supported, const Tile_coord* visible_top = nullptr) {
        highest = std::clamp(highest, 0, 254);
        std::vector<Tile_coord> goals;
        goals.reserve(static_cast<std::size_t>(highest) + 2);
        const auto offer = [&](const Tile_coord& candidate) {
            for (const Tile_coord& existing : goals)
                if (existing == candidate) return;
            goals.push_back(candidate);
        };
        // The top face actually hit by the user is preferred even if the
        // corresponding projected plane is just beyond that object's edge.
        if (visible_top) offer(*visible_top);
        for (int z = highest; z >= 0; --z) {
            const Tile_coord target = project(screen_x, screen_y, scrolltx, scrollty, z);
            // Ground is the final fallback. Upper levels need physical support.
            if (z == 0 || supported(target)) offer(target);
        }
        return goals;
    }
};

#endif
