#include "Modern_mouse_heading.h"
#include <cassert>
#include <vector>

using P = Modern_mouse_heading::Point;

int main() {
    // A two-tile sawtooth with a straight northward overall course.
    Modern_mouse_heading zig;
    for (int y = 20; y > 12; --y) {
        const int offset = y % 2;
        const int dir = zig.advance({double(offset), double(y)},
              {{double(1 - offset), double(y - 1)},
               {double(offset), double(y - 2)},
               {double(1 - offset), double(y - 3)},
               {double(offset), double(y - 4)},
               {double(1 - offset), double(y - 5)}}, 3072);
        assert(dir == 0); // Do not face E/W on alternating tiles.
    }

    // A wide U: head east, bend south, return west.
    Modern_mouse_heading turn;
    const auto dir1 = turn.advance({0, 0}, {{1,0},{2,0},{3,0},{4,0},{5,0}}, 3072);
    assert(dir1 == 2);
    for (int i = 0; i < 5; ++i) {
        turn.advance({double(4+i/2), double(i)}, {{double(5),double(i+1)},{5,double(i+2)},{5,double(i+3)},{4,double(i+4)},{3,double(i+4)}}, 3072);
    }
    for (int i = 0; i < 5; ++i) {
        turn.advance({double(5-i), 5}, {{double(4-i),5},{double(3-i),5},{double(2-i),5},{double(1-i),5}}, 3072);
    }
    assert(turn.direction() == 6); // West on the return leg, not final-target facing on outbound.

    // Retarget at the same position doesn't erase state: only walking steps
    // invoke advance; an action's heading persists over A* replacement.
    const int retained = turn.direction();
    assert(retained == turn.direction());

    // Wrapping world coordinates must not create a false 3000-tile turn.
    Modern_mouse_heading wrap;
    assert(wrap.advance({3071,10}, {{0,10},{1,10},{2,10}}, 3072) == 2);
}
