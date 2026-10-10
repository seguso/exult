/*
 * Copyright (C) 2026 The Exult Team
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef EXULT_MODERN_MOUSE_TARGET_H
#define EXULT_MODERN_MOUSE_TARGET_H

// Mouse A* retarget cadence and shared movement speed are independent of the
// optional WASD implementation. The caller owns SDL polling and actor control.
class Modern_mouse_target {
public:
    // Retarget at most every 500 ms while A* steering is active. The classic
    // mouse path is deliberately never throttled.
    bool should_retarget(bool enabled, unsigned int ticks) const {
        return !enabled || static_cast<unsigned int>(ticks - last_target_ticks) >= 500;
    }

    void retargeted(unsigned int ticks) {
        last_target_ticks = ticks;
    }

    // Same fast/medium speed choice used by the optional modern keyboard.
    static int walk_speed(bool medium, int standard_delay,
                          int medium_factor, int fast_factor) {
        const int factor = medium ? medium_factor : fast_factor;
        return 200 * standard_delay / factor;
    }

private:
    unsigned int last_target_ticks = 0;
};
#endif
