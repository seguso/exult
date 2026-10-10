#include "Modern_keyboard_direction.h"
#include <cassert>
int main() {
    Modern_keyboard_keys keys;
    auto dir = modern_keyboard_direction(keys, true);
    assert(dir.dx == 0 && dir.dy == 0);
    keys.w = true; keys.d = true;
    dir = modern_keyboard_direction(keys, true);
    assert(dir.dx == 1 && dir.dy == -1);
    dir = modern_keyboard_direction(keys, false);
    assert(dir.dx == 0 && dir.dy == 0);
    keys.s = true; keys.a = true;
    dir = modern_keyboard_direction(keys, true);
    assert(dir.dx == 0 && dir.dy == 0);
    keys.w = false; keys.d = false; keys.s = false; keys.a = false;
    keys.left = true; keys.up = true;
    dir = modern_keyboard_direction(keys, true);
    assert(dir.dx == -1 && dir.dy == -1);
    keys.right = true;
    dir = modern_keyboard_direction(keys, true);
    assert(dir.dx == 0 && dir.dy == -1);
    keys.up = false; keys.left = false; keys.right = false;
    keys.down = true;
    dir = modern_keyboard_direction(keys, true);
    assert(dir.dx == 0 && dir.dy == 1);
}
