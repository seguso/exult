#include "Modern_mouse_surface_geometry.h"

#include <cassert>
#include <vector>

static bool same(const Tile_coord& p, int x, int y, int z) {
    return p.tx == x && p.ty == y && p.tz == z;
}

int main() {
    using G = Modern_mouse_surface_geometry;
    // Stair top, mid-landing and ground, sorted from higher to lower.
    const auto stair = G::descending(32, 48, 500, 600, 4,
        [](const Tile_coord& p) { return p.tz == 4 || p.tz == 2; });
    assert(stair.size() == 3);
    assert(same(stair[0], 506, 608, 4));   // (32+4*4)/8, (48+4*4)/8
    assert(same(stair[1], 505, 607, 2));
    assert(same(stair[2], 504, 606, 0));

    // A clicked roof can fail pathfinding; the subsequent candidate is on
    // the ground underneath the SAME SCREEN PIXEL, not the roof's world X/Y.
    const auto roof = G::descending(32, 48, 500, 600, 5,
        [](const Tile_coord&) { return false; });
    assert(roof.size() == 1);
    assert(same(roof[0], 504, 606, 0));

    // Even a top visible surface with no cache support is offered first;
    // collision and A* determine if the Avatar can actually stand there.
    const Tile_coord projected_top(507, 609, 5);
    const auto top_then_floor = G::descending(32, 48, 500, 600, 5,
        [](const Tile_coord&) { return false; }, &projected_top);
    assert(top_then_floor.size() == 2);
    assert(top_then_floor[0] == projected_top);
    assert(same(top_then_floor[1], 504, 606, 0));

    // Negative scene pixels and map edges wrap correctly at every elevation.
    assert(same(G::project(-4, -4, 0, 0, 0), 3071, 3071, 0));
    assert(same(G::project(-4, -4, 0, 0, 2), 0, 0, 2));

    // An upper floor unreachable by A* is discarded by the CALLER, so the
    // next candidate is precisely the next supported lower surface.
    const auto floors = G::descending(1, 1, 0, 0, 6,
        [](const Tile_coord& t) { return t.tz == 6 || t.tz == 3; });
    assert(floors.size() == 3);
    assert(floors[0].tz == 6 && floors[1].tz == 3 && floors[2].tz == 0);
}
