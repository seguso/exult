/*
 * Copyright (C) 2026 The Exult Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef EXULT_OPTION_ROW_FADER_H
#define EXULT_OPTION_ROW_FADER_H

#include "ibuf8.h"
#include "palette.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Indexed-palette backdrop compositor for visually disabled option rows.
// This helper deliberately knows nothing about any particular settings,
// widgets, events, or feature dependencies. Capture before painting the row,
// then call fade_after_paint after painting its label and control.
class Option_row_fader {
public:
    Option_row_fader(Image_buffer8& target, int x, int y, int w, int h)
            : buffer(target) {
        const int left = std::clamp(x, 0, static_cast<int>(buffer.get_width()));
        const int top = std::clamp(y, 0, static_cast<int>(buffer.get_height()));
        const int right = std::clamp(static_cast<long long>(x) + std::max(0, w),
                                    0LL, static_cast<long long>(buffer.get_width()));
        const int bottom = std::clamp(static_cast<long long>(y) + std::max(0, h),
                                     0LL, static_cast<long long>(buffer.get_height()));
        origin_x = left;
        origin_y = top;
        width = std::max(0, right - left);
        height = std::max(0, bottom - top);
        backdrop.reserve(static_cast<std::size_t>(width) * height);
        for (int yy = 0; yy < height; ++yy)
            for (int xx = 0; xx < width; ++xx)
                backdrop.push_back(buffer.get_pixel8(origin_x + xx, origin_y + yy));
    }

    // Draw at half opacity against the captured pre-paint background.
    // Identical foreground/background palette indices are left untouched.
    void fade_after_paint(const Palette& palette) {
        if (backdrop.empty()) return;
        std::array<int, 65536> cache;
        cache.fill(-1);
        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                const auto bg = backdrop[static_cast<std::size_t>(yy) * width + xx];
                const auto fg = buffer.get_pixel8(origin_x + xx, origin_y + yy);
                if (fg == bg) continue;
                const unsigned key = (static_cast<unsigned>(fg) << 8) | bg;
                int& mixed = cache[key];
                if (mixed < 0) {
                    mixed = palette.find_color(
                            (palette.get_red(fg) + palette.get_red(bg) + 1) / 2,
                            (palette.get_green(fg) + palette.get_green(bg) + 1) / 2,
                            (palette.get_blue(fg) + palette.get_blue(bg) + 1) / 2);
                }
                buffer.put_pixel8(static_cast<unsigned char>(mixed),
                                  origin_x + xx, origin_y + yy);
            }
        }
    }

private:
    Image_buffer8& buffer;
    int origin_x = 0, origin_y = 0, width = 0, height = 0;
    std::vector<unsigned char> backdrop;
};
#endif
