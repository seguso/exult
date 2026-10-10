/*
 * Standalone unit checks for Modern_movement_speed.
 * Build without SDL or game assets:
 *   c++ -std=c++17 -Wall -Wextra -Werror -I. tests/modern_movement_speed_test.cc -o modern_movement_speed_test
 *   ./modern_movement_speed_test
 */
#include "Modern_movement_speed.h"
#include <cassert>

using Key = Modern_movement_speed::Key;
static void tap(Modern_movement_speed& s, Key k) {
    assert(!s.key_down(k, false));
    assert(s.key_up(k));
}

int main() {
    Modern_movement_speed s;
    assert(!s.is_medium()); // default fast
    tap(s, Key::left_shift);
    assert(s.is_medium());
    tap(s, Key::right_shift);
    assert(!s.is_medium());

    // A held Shift plus another keyboard key is not a standalone tap.
    s.key_down(Key::left_shift, false);
    s.key_down(Key::other, false);
    assert(!s.key_up(Key::left_shift));
    assert(!s.is_medium());

    // Repeated down events and unmatched releases cannot toggle.
    s.key_down(Key::right_shift, false);
    s.key_down(Key::right_shift, true);
    assert(s.key_up(Key::right_shift));
    assert(s.is_medium());
    assert(!s.key_up(Key::right_shift));
    assert(s.is_medium());

    // Two simultaneously held Shift keys are not two separate taps.
    s.key_down(Key::left_shift, false);
    s.key_down(Key::right_shift, false);
    assert(!s.key_up(Key::left_shift));
    assert(!s.key_up(Key::right_shift));
    assert(s.is_medium());

    // Focus loss or disabling both consumers must cancel an incomplete tap.
    s.key_down(Key::left_shift, false);
    s.cancel_pending_taps();
    assert(!s.key_up(Key::left_shift));
    assert(s.is_medium());

    // Mode survives cancellation; next legitimate tap switches it.
    tap(s, Key::left_shift);
    assert(!s.is_medium());
}
