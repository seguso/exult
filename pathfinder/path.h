/*
 *  Copyright (C) 2000-2022  The Exult Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PATH_H
#define PATH_H

#include "tiles.h"

#include <utility>
#include <vector>

class Pathfinder_client;

// Opt-in, synchronous MMT A* diagnostics; no effect on route selection.
struct Mmt_astar_diagnostics {
    bool found = false;
    int nodes_generated = 0;
    int nodes_expanded = 0;
    int maximum_open = 0;
    int neighbor_checks = 0;
    int result_steps = 0;
    double elapsed_ms = 0.0;
};
void begin_mmt_astar_diagnostics();
Mmt_astar_diagnostics end_mmt_astar_diagnostics();

std::pair<std::vector<Tile_coord>, bool> Find_path(const Tile_coord&, const Tile_coord&, const Pathfinder_client* client);

#endif
