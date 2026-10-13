#include "Modern_mouse_heading.h"
#include <cassert>
#include <initializer_list>
#include <vector>
using P = Modern_mouse_heading::Point;
static int at(Modern_mouse_heading& h, P here, std::initializer_list<P> rest) {
    h.remember(here, 3072);
    const int dir = h.calculate(here, std::vector<P>(rest), 3072);
    h.set_direction(dir);
    return dir;
}
int main() {
    // The already-travelled part of a U is retained and joined to the future.
    Modern_mouse_heading u;
    assert(at(u, {0,0}, {{1,0},{2,0},{3,0},{4,0},{5,0}}) == 2);
    assert(at(u, {1,0}, {{2,0},{3,0},{4,0},{5,0}}) == 2);
    assert(at(u, {2,0}, {{3,0},{4,0},{5,0},{5,1},{5,2},{5,3},{4,3},{3,3},{2,3}}) == 2);
    assert(at(u, {3,0}, {{4,0},{5,0},{5,1},{5,2},{5,3},{4,3},{3,3}}) == 2);
    assert(at(u, {5,1}, {{5,2},{5,3},{4,3},{3,3},{2,3}}) == 4);
    assert(at(u, {4,3}, {{3,3},{2,3},{1,3},{0,3}}) == 6);
    assert(at(u, {3,3}, {{2,3},{1,3},{0,3}}) == 6);
    // Alternating E/W tile steps: overall northern tangent, without a
    // temporal filter or delaying any turn by a prescribed number of steps.
    Modern_mouse_heading zig;
    for (int y = 20; y >= 12; --y) {
        const int x = y % 2;
        assert(at(zig, {double(x),double(y)},
            {{double(1-x),double(y-1)},{double(x),double(y-2)},
             {double(1-x),double(y-3)},{double(x),double(y-4)},
             {double(1-x),double(y-5)}}) == 0);
    }
    Modern_mouse_heading wrapped;
    assert(at(wrapped, {3071,10}, {{0,10},{1,10},{2,10}}) == 2);
    // A replan changes the direction immediately even without a walking
    // step and retains the actual past in the action's history.
    Modern_mouse_heading replanned;
    assert(at(replanned, {0,0}, {{1,0},{2,0}}) == 2);
    assert(at(replanned, {0,0}, {{0,1},{0,2},{0,3}}) == 4);
}
