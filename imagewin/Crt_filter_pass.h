/*
 * Copyright (C) 2026 The Exult Team
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 */
#ifndef EXULT_CRT_FILTER_PASS_H
#define EXULT_CRT_FILTER_PASS_H

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// CRT processing owns only its renderer pass, not image-window lifecycle.
struct Crt_filter_settings {
    bool enabled;
    int width, height, scale;
    int horizontal_strength, vertical_strength;
    int horizontal_compensation, vertical_compensation;
    int horizontal_width, vertical_width, beam_sigma;
};

inline void render_crt_filter_pass(SDL_Renderer* screen_renderer, const SDL_FRect& content_rect,
        const Crt_filter_settings& settings) {
    const bool crt_enabled = settings.enabled;
    const int display_width = settings.width, display_height = settings.height;
    const int crt_horizontal_strength = settings.horizontal_strength;
    const int crt_vertical_strength = settings.vertical_strength;
    const int crt_horizontal_compensation = settings.horizontal_compensation;
    const int crt_vertical_compensation = settings.vertical_compensation;
    const int crt_horizontal_width = settings.horizontal_width;
    const int crt_vertical_width = settings.vertical_width;
    const int crt_beam_sigma = settings.beam_sigma;
    const int scale = settings.scale;
	if (!crt_enabled || !screen_renderer || display_width <= 1 || display_height <= 1) {
		return;
	}

	Uint8 old_r = 0;
	Uint8 old_g = 0;
	Uint8 old_b = 0;
	Uint8 old_a = 255;
	SDL_BlendMode old_blend = SDL_BLENDMODE_NONE;
	SDL_GetRenderDrawColor(screen_renderer, &old_r, &old_g, &old_b, &old_a);
	SDL_GetRenderDrawBlendMode(screen_renderer, &old_blend);

	// Darkening is multiplicative: dst = dst * source_colour.
	const SDL_BlendMode darken_mode = SDL_ComposeCustomBlendMode(
			SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
	// Bright compensation is proportional to the existing pixel rather than an
	// additive white lift: dst = dst + source_colour * dst.
	const SDL_BlendMode brighten_mode = SDL_ComposeCustomBlendMode(
			SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
			SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);

	// Custom blend support depends on the active SDL renderer. Probe both modes
	// before drawing anything so unsupported backends don't produce a half-filter.
	if (!SDL_SetRenderDrawBlendMode(screen_renderer, darken_mode)
			|| !SDL_SetRenderDrawBlendMode(screen_renderer, brighten_mode)) {
		SDL_SetRenderDrawBlendMode(screen_renderer, old_blend);
		SDL_SetRenderDrawColor(screen_renderer, old_r, old_g, old_b, old_a);
		SDL_ClearError();
		return;
	}

	static thread_local std::vector<SDL_FRect> cols_dark;
	static thread_local std::vector<SDL_FRect> cols_bright;
	cols_dark.clear();
	cols_bright.clear();

	// Lock scanline phase to the content's final scaled/cropped origin.
	const int row_origin = static_cast<int>(std::lround(
			-content_rect.y * static_cast<float>(display_height) / content_rect.h));
	cols_dark.reserve(static_cast<size_t>(display_width / std::max(1, crt_vertical_width) + 2));
	cols_bright.reserve(static_cast<size_t>(display_width / std::max(1, crt_vertical_width) + 2));
	for (int x = 0; x < display_width;) {
		const int phase = (x / crt_vertical_width) & 1;
		const int band_end = std::min(display_width, ((x / crt_vertical_width) + 1) * crt_vertical_width);
		SDL_FRect r{static_cast<float>(x), 0.0f, static_cast<float>(band_end - x), static_cast<float>(display_height)};
		(phase ? cols_dark : cols_bright).push_back(r);
		x = band_end;
	}

	const auto render_axis = [&](float strength, int compensation,
			const std::vector<SDL_FRect>& dark_rects,
			const std::vector<SDL_FRect>& bright_rects) {
		if (strength <= 0) {
			return;
		}

		// Dark line multiplier: 1-strength. For 30%, alternate lines are 0.70.
		const float s = strength / 100.0f;
		const int dark_rgb = std::clamp(
				static_cast<int>(std::lround((1.0f - s) * 255.0f)), 0, 255);
		SDL_SetRenderDrawBlendMode(screen_renderer, darken_mode);
		SDL_SetRenderDrawColor(
				screen_renderer,
				static_cast<Uint8>(dark_rgb),
				static_cast<Uint8>(dark_rgb),
				static_cast<Uint8>(dark_rgb), 255);
		if (!dark_rects.empty()) {
			SDL_RenderFillRects(screen_renderer, dark_rects.data(), static_cast<int>(dark_rects.size()));
		}

		// At compensation=100, the bright line gets +strength, so the ideal
		// two-line mean is ((1-s) + (1+s)) / 2 = 1.0. The user can deliberately
		// under/over-compensate from 0..200%.
		const float boost = s * (static_cast<float>(compensation) / 100.0f);
		if (boost > 0.0f && !bright_rects.empty()) {
			const int boost_rgb = std::clamp(
					static_cast<int>(std::lround(boost * 255.0f)), 0, 255);
			SDL_SetRenderDrawBlendMode(screen_renderer, brighten_mode);
			SDL_SetRenderDrawColor(
					screen_renderer,
					static_cast<Uint8>(boost_rgb),
					static_cast<Uint8>(boost_rgb),
					static_cast<Uint8>(boost_rgb), 255);
			SDL_RenderFillRects(screen_renderer, bright_rects.data(), static_cast<int>(bright_rects.size()));
		}
	};

	// A CRT beam has a soft luminance profile, not alternating flat bands.
	// Each physical output row receives a Gaussian-shaped contribution near
	// the centre of the bright half-cycle. The remaining (wider) portion of
	// the period is dark; the vertical phosphor mask remains unchanged.
	if (crt_horizontal_strength > 0) {
		const float strength = static_cast<float>(crt_horizontal_strength) / 100.0f;
		const float compensation = static_cast<float>(crt_horizontal_compensation) / 100.0f;
		const int band_width = crt_horizontal_width;
		const int period = 2 * band_width;
		const float sigma = static_cast<float>(crt_beam_sigma) / 100.0f;
		for (int y = 0; y < display_height; ++y) {
			const int phase = ((y - row_origin) % period + period) % period;
			float lobe = 0.0f;
			if (phase < band_width) {
				const float position = (static_cast<float>(phase) + 0.5f) / static_cast<float>(band_width);
				const float distance = (position - 0.5f) / sigma;
				lobe = std::exp(-0.5f * distance * distance);
			}
			const float dark_amount = strength * (1.0f - lobe);
			const int dark_rgb = std::clamp(
					static_cast<int>(std::lround((1.0f - dark_amount) * 255.0f)), 0, 255);
			const SDL_FRect row_rect{
					0.0f, static_cast<float>(y), static_cast<float>(display_width), 1.0f};
			SDL_SetRenderDrawBlendMode(screen_renderer, darken_mode);
			SDL_SetRenderDrawColor(
					screen_renderer, static_cast<Uint8>(dark_rgb),
					static_cast<Uint8>(dark_rgb), static_cast<Uint8>(dark_rgb), 255);
			SDL_RenderFillRect(screen_renderer, &row_rect);

			const float boost = strength * compensation * lobe;
			if (boost > 0.0f) {
				const int boost_rgb = std::clamp(
						static_cast<int>(std::lround(boost * 255.0f)), 0, 255);
				SDL_SetRenderDrawBlendMode(screen_renderer, brighten_mode);
				SDL_SetRenderDrawColor(
						screen_renderer, static_cast<Uint8>(boost_rgb),
						static_cast<Uint8>(boost_rgb), static_cast<Uint8>(boost_rgb), 255);
				SDL_RenderFillRect(screen_renderer, &row_rect);
			}
		}
	}
	render_axis(
			static_cast<float>(crt_vertical_strength) * 0.5f, crt_vertical_compensation,
			cols_dark, cols_bright);

	// Rounded pixel spots: normalize the one-dimensional Gaussian envelopes
	// before multiplying X and Y. Narrow beams used to discard most of the
	// light: normalization holds mean emitted energy approximately constant
	// as sigma changes. Gains above one use the proportional brighten blend;
	// clipping on saturated highlights is the only remaining energy loss.
	if (scale > 1) {
		const int pixel_size = scale;
		const int origin_x = static_cast<int>(std::lround(
				-content_rect.x * static_cast<float>(display_width) / content_rect.w));
		const float sigma = static_cast<float>(crt_beam_sigma) / 100.0f;
		constexpr float edge_depth = 0.70f;
		std::vector<float> spot_gain(static_cast<size_t>(pixel_size));
		float total = 0.0f;
		for (int phase = 0; phase < pixel_size; ++phase) {
			const float position = (static_cast<float>(phase) + 0.5f) / static_cast<float>(pixel_size);
			const float d = (position - 0.5f) / sigma;
			const float envelope = 1.0f - edge_depth * (1.0f - std::exp(-0.5f * d * d));
			spot_gain[static_cast<size_t>(phase)] = envelope;
			total += envelope;
		}
		const float mean = total / static_cast<float>(pixel_size);
		for (float& gain : spot_gain) {
			gain /= mean;
		}

		const auto apply_spot_axis = [&](bool horizontal) {
			const int count = horizontal ? display_width : display_height;
			const int origin = horizontal ? origin_x : row_origin;
			for (int coord = 0; coord < count; ++coord) {
				const int phase = ((coord - origin) % pixel_size + pixel_size) % pixel_size;
				const float gain = spot_gain[static_cast<size_t>(phase)];
				const bool darken = gain < 1.0f;
				// Multiplicative: darken uses dst * source; brighten uses
				// dst + source * dst. Quantize only the final blend factor.
				const float blend_value = darken ? gain : gain - 1.0f;
				const int rgb = std::clamp(
						static_cast<int>(std::lround(blend_value * 255.0f)), 0, 255);
				if (darken) {
					SDL_SetRenderDrawBlendMode(screen_renderer, darken_mode);
				} else {
					SDL_SetRenderDrawBlendMode(screen_renderer, brighten_mode);
				}
				SDL_SetRenderDrawColor(screen_renderer,
						static_cast<Uint8>(rgb), static_cast<Uint8>(rgb),
						static_cast<Uint8>(rgb), 255);
				const SDL_FRect rect = horizontal
						? SDL_FRect{static_cast<float>(coord), 0.0f, 1.0f, static_cast<float>(display_height)}
						: SDL_FRect{0.0f, static_cast<float>(coord), static_cast<float>(display_width), 1.0f};
				SDL_RenderFillRect(screen_renderer, &rect);
			}
		};
		apply_spot_axis(true);
		apply_spot_axis(false);
	}

	// Avoid leaking CRT renderer state into the next frame.
	SDL_SetRenderDrawBlendMode(screen_renderer, old_blend);
	SDL_SetRenderDrawColor(screen_renderer, old_r, old_g, old_b, old_a);
}

#endif
