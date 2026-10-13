#include "Rotate_live_palette.h"
#include <array>
#include <cassert>

int main() {
    std::array<unsigned char, 768> day{};
    day[3 * 10 + 0] = 240;
    day[3 * 10 + 1] = 20;
    day[3 * 11 + 0] = 20;
    day[3 * 11 + 1] = 240;
    const Rotate_live_palette daylight(day.data());
    assert(daylight.get_red(10) == 60);
    assert(daylight.get_green(10) == 5);
    assert(daylight.find_color(60, 5, 0) == 10);

    auto night = day;
    for (int component = 0; component < 3; ++component) {
        night[3 * 10 + component] = day[3 * 11 + component];
        night[3 * 11 + component] = day[3 * 10 + component];
    }
    const Rotate_live_palette nighttime(night.data());
    assert(nighttime.get_red(10) == 5);
    assert(nighttime.get_green(10) == 60);
    assert(nighttime.find_color(60, 5, 0) == 11);

    // The palette snapshot does not change during a partially painted frame.
    night[3 * 11 + 0] = 0;
    assert(nighttime.get_red(11) == 60);

    // Animated colours remain valid sources but are never NEW blend indices.
    std::array<unsigned char, 768> animated{};
    animated[3 * 230 + 2] = 252;
    const Rotate_live_palette cycling(animated.data());
    assert(cycling.get_blue(230) == 63);
    assert(cycling.find_color(0, 0, 63) < 224);
}
