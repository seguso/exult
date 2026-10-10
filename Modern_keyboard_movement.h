/*
 * Copyright (C) 2026 The Exult Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef EXULT_MODERN_KEYBOARD_MOVEMENT_H
#define EXULT_MODERN_KEYBOARD_MOVEMENT_H

#include "Modern_keyboard_direction.h"
#include "gamewin.h"
#include "keyactions.h"
#include <SDL3/SDL.h>

// Called once per event-loop iteration. Modern keyboard owns its held-key
// direction and release transitions independently of mouse A* and camera.
// When disabled it leaves the original Exult keybinder as sole key owner.
inline void update_modern_keyboard_movement(Game_window& win, bool medium_speed) {
    static int previous_dx = 0;
    static int previous_dy = 0;
    int dx = 0;
    int dy = 0;
    if (win.is_modern_keyboard_enabled()) {
        const SDL_Keymod movement_mods =
                SDL_KMOD_SHIFT | SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI;
        const bool plain = (SDL_GetModState() & movement_mods) == 0;
        const bool* state = SDL_GetKeyboardState(nullptr);
        const Modern_keyboard_keys keys{
                state[SDL_SCANCODE_W] != 0,
                state[SDL_SCANCODE_A] != 0,
                state[SDL_SCANCODE_S] != 0,
                state[SDL_SCANCODE_D] != 0,
                state[SDL_SCANCODE_UP] != 0,
                state[SDL_SCANCODE_LEFT] != 0,
                state[SDL_SCANCODE_DOWN] != 0,
                state[SDL_SCANCODE_RIGHT] != 0};
        const auto direction = modern_keyboard_direction(keys, plain);
        dx = direction.dx;
        dy = direction.dy;
        if (dx != 0 || dy != 0) {
            if (dx != previous_dx || dy != previous_dy || !win.is_moving()) {
                const int params[] = {medium_speed ? 1 : 0};
                if (dy < 0 && dx < 0) ActionWalkNorthWest(params);
                else if (dy < 0 && dx > 0) ActionWalkNorthEast(params);
                else if (dy > 0 && dx < 0) ActionWalkSouthWest(params);
                else if (dy > 0 && dx > 0) ActionWalkSouthEast(params);
                else if (dy < 0) ActionWalkNorth(params);
                else if (dy > 0) ActionWalkSouth(params);
                else if (dx < 0) ActionWalkWest(params);
                else ActionWalkEast(params);
            }
        }
    }
    // Preserve held right mouse movement when the keyboard releases or the
    // option is switched off while movement is in progress.
    if (dx == 0 && dy == 0 && (previous_dx != 0 || previous_dy != 0)
        && !(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK)) {
        win.stop_actor();
    }
    previous_dx = dx;
    previous_dy = dy;
}

#endif
