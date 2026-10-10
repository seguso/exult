#include "Modern_mouse_target.h"
#include <cassert>

// The classic mouse path remains unthrottled. A* hold retargets every 500 ms.
// Speed selection is shared with WASD but the two features do not depend on
// each other's input handling.
int main() {
    Modern_mouse_target target;
    assert(target.should_retarget(false, 0));
    assert(!target.should_retarget(true, 0));
    assert(target.should_retarget(true, 500));
    target.retargeted(500);
    assert(target.should_retarget(false, 501));
    assert(!target.should_retarget(true, 999));
    assert(target.should_retarget(true, 1000));
    // Unsigned clock subtraction correctly handles the tick wraparound.
    target.retargeted(0xfffffff0u);
    assert(!target.should_retarget(true, 100u));
    assert(target.should_retarget(true, 500u));
    assert(Modern_mouse_target::walk_speed(false, 50, 200, 100) == 100);
    assert(Modern_mouse_target::walk_speed(true, 50, 200, 100) == 50);
}
