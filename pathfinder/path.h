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

#include <string>
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
bool mmt_profiling_enabled();
bool mmt_limited_search_enabled();
bool mmt_final_retry_enabled();
void set_mmt_final_retry_enabled(bool enabled);
int mmt_offlift_node_limit();
void set_mmt_offlift_node_limit(int value);
void set_mmt_limited_search_enabled(bool enabled);
bool automatic_path_doors_enabled();
void set_automatic_path_doors_enabled(bool enabled);
void begin_mmt_path_search(int expansion_limit = 0);
bool mmt_path_search_budget_exhausted();
void end_mmt_path_search();
void set_mmt_profiling_enabled(bool enabled);
void append_mmt_profile_line(const std::string& line);
std::string save_mmt_profile_log();
void reset_mmt_profile_log();
void begin_mmt_astar_diagnostics();
Mmt_astar_diagnostics end_mmt_astar_diagnostics();

std::pair<std::vector<Tile_coord>, bool> Find_path(const Tile_coord&, const Tile_coord&, const Pathfinder_client* client);

#endif
