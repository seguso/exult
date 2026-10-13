#include "Modern_mouse_heading.h"
#include <cassert>
#include <cstddef>
#include <vector>

using P = Modern_mouse_heading::Point;

static int at(Modern_mouse_heading& heading, const std::vector<P>& route, std::size_t index) {
    heading.remember(route[index], 3072);
    const int dir = heading.calculate(
            route[index], std::vector<P>(route.begin() + index + 1, route.end()), 3072);
    heading.set_direction(dir);
    return dir;
}

int main() {
    // Use contiguous, actually walked tiles so the retained history is real.
    Modern_mouse_heading u;
    const std::vector<P> route = {
        {0,0},{1,0},{2,0},{3,0},{4,0},{5,0},
        {5,1},{5,2},{5,3},{4,3},{3,3},{2,3},{1,3},{0,3}
    };
    std::vector<int> heading;
    for (std::size_t i = 0; i < route.size(); ++i)
        heading.push_back(at(u, route, i));
    assert(heading[2] == 2);           // East along the outgoing leg.
    assert(heading[6] == 4);           // South in the U's first elbow.
    assert(heading[9] == 6);           // West in the return leg.
    assert(heading.back() == 6);

    // A genuine northward route that zigzags east/west at every tile.
    Modern_mouse_heading zig;
    std::vector<P> saw;
    for (int y = 20; y >= 8; --y)
        saw.push_back({static_cast<double>(y % 2), static_cast<double>(y)});
    for (std::size_t i = 0; i < saw.size(); ++i)
        assert(at(zig, saw, i) == 0);

    // Tile wrapping never creates a false 3072-tile course change.
    Modern_mouse_heading wrapped;
    const std::vector<P> wrap = {{3071,10},{0,10},{1,10},{2,10}};
    for (std::size_t i = 0; i < wrap.size(); ++i)
        assert(at(wrapped, wrap, i) == 2);

    // New A* plan replaces only the future, retaining traversed points.
    // Orientation updates immediately at the junction; no 500ms inertia.
    Modern_mouse_heading replanned;
    const std::vector<P> old_plan = {{0,0},{1,0},{2,0},{3,0},{4,0}};
    assert(at(replanned, old_plan, 0) == 2);
    assert(at(replanned, old_plan, 1) == 2);
    const std::vector<P> new_plan = {{1,0},{1,1},{1,2},{1,3},{1,4}};
    assert(at(replanned, new_plan, 0) == 4);
}
