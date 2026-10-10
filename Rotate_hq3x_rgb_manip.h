/*
 * Copyright (C) 2026 The Exult Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef EXULT_ROTATE_HQ3X_RGB_MANIP_H
#define EXULT_ROTATE_HQ3X_RGB_MANIP_H

#include "palette.h"
#include <cstdint>

// Pixel-channel adapter for rotated HQ3x sampling.
// Independent of the camera, dialog options and world coordinate transforms.
class Rotate_hq3x_rgb_manip {
    const Palette* palette;

public:
    explicit Rotate_hq3x_rgb_manip(const Palette* p) : palette(p) {}

    void split_source(unsigned char pix, unsigned int& r, unsigned int& g, unsigned int& b) const {
        // Exult uses six-bit palette channels; HQ3x interpolation expects
        // a full eight-bit-like channel range.
        r = static_cast<unsigned int>(palette->get_red(pix)) << 2;
        g = static_cast<unsigned int>(palette->get_green(pix)) << 2;
        b = static_cast<unsigned int>(palette->get_blue(pix)) << 2;
    }

    std::uint32_t rgb(unsigned int r, unsigned int g, unsigned int b) const {
        return ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
    }
};
#endif
