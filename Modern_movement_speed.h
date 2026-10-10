/*
 * Copyright (C) 2026 The Exult Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef EXULT_MODERN_MOVEMENT_SPEED_H
#define EXULT_MODERN_MOVEMENT_SPEED_H

// Common state for independent optional modern keyboard and A* mouse movement.
// SDL and event dispatch are deliberately outside this class, so neither feature
// must depend on the other feature's implementation.
class Modern_movement_speed {
public:
    enum class Key {
        left_shift,
        right_shift,
        other
    };

    bool is_medium() const { return medium; }

    // Called only while at least one consumer is enabled. Returns true when a
    // standalone Shift tap changes the shared fast/medium movement mode.
    bool key_down(Key key, bool repeat) {
        if (key == Key::other) {
            left_candidate = right_candidate = false;
            return false;
        }
        if (repeat) return false;
        if (key == Key::left_shift) {
            // Pressing the second Shift key is not a standalone single-key tap.
            if (right_down) right_candidate = false;
            left_candidate = !left_down && !right_down;
            left_down = true;
        } else {
            if (left_down) left_candidate = false;
            right_candidate = !right_down && !left_down;
            right_down = true;
        }
        return false;
    }

    bool key_up(Key key) {
        if (key == Key::other) return false;
        const bool left = key == Key::left_shift;
        const bool eligible = left ? (left_down && left_candidate)
                                   : (right_down && right_candidate);
        if (left) {
            left_down = left_candidate = false;
        } else {
            right_down = right_candidate = false;
        }
        if (eligible) medium = !medium;
        return eligible;
    }

    // Call on focus loss and while disabling all modern speed consumers.
    // Preserves the chosen speed across temporary changes in focus.
    void cancel_pending_taps() {
        left_down = right_down = false;
        left_candidate = right_candidate = false;
    }

private:
    bool medium = false;
    bool left_down = false;
    bool right_down = false;
    bool left_candidate = false;
    bool right_candidate = false;
};

#endif
