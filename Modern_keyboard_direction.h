/*
 * Copyright (C) 2026 The Exult Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef EXULT_MODERN_KEYBOARD_DIRECTION_H
#define EXULT_MODERN_KEYBOARD_DIRECTION_H

// SDL-independent direction resolution for optional WASD + arrow movement.
// The caller decides whether keyboard movement is enabled and whether any
// modifier is held. No camera, A* mouse, or Shift-speed state is referenced.
struct Modern_keyboard_keys {
    bool w = false, a = false, s = false, d = false;
    bool up = false, left = false, down = false, right = false;
};

struct Modern_keyboard_direction {
    int dx = 0;
    int dy = 0;
};

inline Modern_keyboard_direction modern_keyboard_direction(
        const Modern_keyboard_keys& keys, bool plain_movement_keys) {
    if (!plain_movement_keys) return {};
    const int east = keys.d || keys.right ? 1 : 0;
    const int west = keys.a || keys.left ? 1 : 0;
    const int south = keys.s || keys.down ? 1 : 0;
    const int north = keys.w || keys.up ? 1 : 0;
    return {east - west, south - north};
}
#endif
