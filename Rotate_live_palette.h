/*
 * Copyright (C) 2026 The Exult Team
 * GPL-2.0-or-later
 *
 * Snapshot the palette actually displayed by Image_window8. The engine's
 * Palette::pal1 is not necessarily the palette being shown: transitions
 * can apply intermediate palettes, gamma and colour cycling update the
 * window palette independently.
 */
#ifndef EXULT_ROTATE_LIVE_PALETTE_H
#define EXULT_ROTATE_LIVE_PALETTE_H

#include <array>
#include <cstddef>

class Rotate_live_palette {
    std::array<unsigned char, 768> colors{};

public:
    explicit Rotate_live_palette(const unsigned char* displayed_rgb) {
        // The existing rotated resamplers and weight LUTs use 6-bit RGB.
        for (std::size_t i = 0; i < colors.size(); ++i)
            colors[i] = static_cast<unsigned char>(displayed_rgb[i] >> 2);
    }

    int get_red(unsigned char index) const { return colors[3 * index]; }
    int get_green(unsigned char index) const { return colors[3 * index + 1]; }
    int get_blue(unsigned char index) const { return colors[3 * index + 2]; }

    int find_color(int r, int g, int b) const {
        // Match Exult's policy: do not quantize a NEW RGB blend into an
        // animated colour slot (0xe0..0xff).
        int best = 0;
        int best_distance = 0x7fffffff;
        for (int i = 0; i < 0xe0; ++i) {
            const int dr = r - colors[3 * i];
            const int dg = g - colors[3 * i + 1];
            const int db = b - colors[3 * i + 2];
            const int distance = dr * dr + dg * dg + db * db;
            if (distance < best_distance) {
                best = i;
                best_distance = distance;
                if (distance == 0) break;
            }
        }
        return best;
    }
};

#endif
