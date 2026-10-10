// Rotated world rendering implementation; included by gamerend.cc.
// Kept in the original translation unit to preserve internal renderer access.
void Game_window::paint_rotated(int x, int y, int w, int h) {
	ignore_unused_variable_warning(x);
	ignore_unused_variable_warning(y);
	ignore_unused_variable_warning(w);
	ignore_unused_variable_warning(h);
	if (!rotate_scene) {
		resize_rotate_scene();
	}

	// Quantize camera translation only in the rotated view.  The renderer
	// itself still scrolls by exact source pixels; this phase term makes the
	// resampling lattice world-anchored, so a stationary object's contour does
	// not change merely because smooth scrolling advanced by one pixel.
	world_view.set_camera_pixel_origin(
			static_cast<double>(scrolltx * c_tilesize + get_scrolltx_lo()),
			static_cast<double>(scrollty * c_tilesize + get_scrollty_lo()));

	const int display_width  = world_view.get_display_width();
	const int display_height = world_view.get_display_height();
	const int scene_size = world_view.get_scene_size();
	const int scene_x    = -world_view.get_scene_offset_x();
	const int scene_y    = -world_view.get_scene_offset_y();
	rotate_scene->clear_clip();
	rotate_scene->fill8(pal->get_border_index());
	Image_buffer8* previous = push_render_target(rotate_scene.get());
	rotate_scene->set_clip(scene_x, scene_y, scene_size, scene_size);
	int light_sources = 0;
	if (main_actor) {
		light_sources = render->paint_map(scene_x, scene_y, scene_size, scene_size);
	}
	effects->paint();
	// A world object being dragged should go through the same rotated-world
	// raster pipeline as when it is at rest. Painting it here preserves the
	// exact Scale2x + rotate quality instead of promoting it to an unrotated UI
	// overlay.
	if (dragging && dragging->is_world_object_drag() && !dragging->is_over_gump()) {
		dragging->paint_world_object();
	}
	rotate_scene->clear_clip();
	pop_render_target(previous);

	// The world is repainted in full for this first implementation.
	int gx = 0;
	int gy = 0;
	int gw = display_width;
	int gh = display_height;
	win->BeginPaintIntoGuardBand(&gx, &gy, &gw, &gh);
	win->set_clip(gx, gy, gw, gh);
	win->fill8(pal->get_border_index());

	// Reconstruct the scene with a pixel-art-aware scaler before rotation.
	// Mode 0 deliberately keeps the clean2 Scale2x + 4-sample path bit-for-bit
	// equivalent. Modes 1/2 use the standard Scale3x neighbourhood rules and
	// differ only in the final sampling density. Modes 3/4 run Exult's HQ3x
	// algorithm into an RGB buffer. Modes 5/6/7 are the retained weighted
	// forward triplet/pair/quadruplet renderers.
	const auto scene_pixel = [&](int x, int y) {
		x = std::clamp(x, scene_x, scene_x + scene_size - 1);
		y = std::clamp(y, scene_y, scene_y + scene_size - 1);
		return rotate_scene->get_pixel8(x, y);
	};

	if (rotate_sampling_mode == 0) {
		rotate_scene_2x->clear_clip();
		rotate_scene_2x->fill8(pal->get_border_index());
		for (int sy = 0; sy < scene_size; ++sy) {
			const int y = scene_y + sy;
			for (int sx = 0; sx < scene_size; ++sx) {
				const int x = scene_x + sx;
				const unsigned char e = scene_pixel(x, y);
				const unsigned char b = scene_pixel(x, y - 1);
				const unsigned char d = scene_pixel(x - 1, y);
				const unsigned char f = scene_pixel(x + 1, y);
				const unsigned char h = scene_pixel(x, y + 1);

				unsigned char e0 = e;
				unsigned char e1 = e;
				unsigned char e2 = e;
				unsigned char e3 = e;
				if (b != h && d != f) {
					if (d == b) e0 = d;
					if (b == f) e1 = f;
					if (d == h) e2 = d;
					if (h == f) e3 = f;
				}

				const int hx = sx * 2;
				const int hy = sy * 2;
				rotate_scene_2x->put_pixel8(e0, hx, hy);
				rotate_scene_2x->put_pixel8(e1, hx + 1, hy);
				rotate_scene_2x->put_pixel8(e2, hx, hy + 1);
				rotate_scene_2x->put_pixel8(e3, hx + 1, hy + 1);
			}
		}
	} else if (rotate_sampling_mode <= 2) {
		rotate_scene_3x->clear_clip();
		rotate_scene_3x->fill8(pal->get_border_index());
		for (int sy = 0; sy < scene_size; ++sy) {
			const int y = scene_y + sy;
			for (int sx = 0; sx < scene_size; ++sx) {
				const int x = scene_x + sx;
				const unsigned char a = scene_pixel(x - 1, y - 1);
				const unsigned char b = scene_pixel(x, y - 1);
				const unsigned char cc = scene_pixel(x + 1, y - 1);
				const unsigned char d = scene_pixel(x - 1, y);
				const unsigned char epx = scene_pixel(x, y);
				const unsigned char fpx = scene_pixel(x + 1, y);
				const unsigned char g = scene_pixel(x - 1, y + 1);
				const unsigned char h = scene_pixel(x, y + 1);
				const unsigned char i = scene_pixel(x + 1, y + 1);

				std::array<unsigned char, 9> out;
				out.fill(epx);
				if (b != h && d != fpx) {
					out[0] = d == b ? d : epx;
					out[1] = (d == b && epx != cc) || (b == fpx && epx != a) ? b : epx;
					out[2] = b == fpx ? fpx : epx;
					out[3] = (d == b && epx != g) || (d == h && epx != a) ? d : epx;
					out[4] = epx;
					out[5] = (b == fpx && epx != i) || (h == fpx && epx != cc) ? fpx : epx;
					out[6] = d == h ? d : epx;
					out[7] = (d == h && epx != i) || (h == fpx && epx != g) ? h : epx;
					out[8] = h == fpx ? fpx : epx;
				}

				const int hx = sx * 3;
				const int hy = sy * 3;
				for (int oy = 0; oy < 3; ++oy) {
					for (int ox = 0; ox < 3; ++ox) {
						rotate_scene_3x->put_pixel8(out[oy * 3 + ox], hx + ox, hy + oy);
					}
				}
			}
		}
	}
#ifdef USE_HQ3X_SCALER
	else if (rotate_sampling_mode == 3 || rotate_sampling_mode == 4) {
		// HQ3x produces RGB directly. Feed it a compact copy of the expanded
		// paletted scene so no palette quantization happens between HQ3x and
		// the rotated 9-sample integration.
		std::vector<unsigned char> source8(
				static_cast<size_t>(scene_size) * static_cast<size_t>(scene_size));
		for (int sy = 0; sy < scene_size; ++sy) {
			for (int sx = 0; sx < scene_size; ++sx) {
				source8[static_cast<size_t>(sy) * scene_size + sx]
						= scene_pixel(scene_x + sx, scene_y + sy);
			}
		}
		Rotate_hq3x_rgb_manip manip(pal);
		Scale_Hq3x<uint32, Rotate_hq3x_rgb_manip>(
				source8.data(), 0, 0, scene_size, scene_size,
				scene_size, scene_size, rotate_scene_hq3x.data(),
				scene_size * 3, manip);
	}
#endif

	static thread_local std::array<unsigned char, 64 * 64 * 64> rotate_blend_cache;
	rotate_blend_cache.fill(255);
	const auto quantize_rgb = [&](int r, int g, int b) {
		r = std::clamp(r, 0, 63);
		g = std::clamp(g, 0, 63);
		b = std::clamp(b, 0, 63);
		const unsigned int key = static_cast<unsigned int>(r)
				| (static_cast<unsigned int>(g) << 6)
				| (static_cast<unsigned int>(b) << 12);
		unsigned char& cached = rotate_blend_cache[key];
		if (cached == 255) {
			cached = static_cast<unsigned char>(pal->find_color(r, g, b));
		}
		return cached;
	};
	const auto sample_scaled = [&](const World_view_point& source, int factor, Image_buffer8* scaled) {
		const double fx = (source.x - static_cast<double>(scene_x)) * factor + 0.5;
		const double fy = (source.y - static_cast<double>(scene_y)) * factor + 0.5;
		const int sx = static_cast<int>(std::floor(fx));
		const int sy = static_cast<int>(std::floor(fy));
		const int hi_size = scene_size * factor;
		if (sx < 0 || sx >= hi_size || sy < 0 || sy >= hi_size) {
			return static_cast<unsigned char>(pal->get_border_index());
		}
		return scaled->get_pixel8(sx, sy);
	};
	const auto sample_hq3x = [&](const World_view_point& source) {
		const double fx = (source.x - static_cast<double>(scene_x)) * 3.0 + 0.5;
		const double fy = (source.y - static_cast<double>(scene_y)) * 3.0 + 0.5;
		const int sx = static_cast<int>(std::floor(fx));
		const int sy = static_cast<int>(std::floor(fy));
		const int hi_size = scene_size * 3;
		if (sx < 0 || sx >= hi_size || sy < 0 || sy >= hi_size) {
			const unsigned char border = static_cast<unsigned char>(pal->get_border_index());
			return (static_cast<uint32>(pal->get_red(border) << 2) << 16)
					| (static_cast<uint32>(pal->get_green(border) << 2) << 8)
					| static_cast<uint32>(pal->get_blue(border) << 2);
		}
		return rotate_scene_hq3x[static_cast<size_t>(sy) * hi_size + sx];
	};
	const auto blend4_rgb = [&](uint32 a, uint32 b, uint32 c0, uint32 d) {
		const int r = static_cast<int>(((a >> 16) & 0xffu) + ((b >> 16) & 0xffu)
				+ ((c0 >> 16) & 0xffu) + ((d >> 16) & 0xffu));
		const int g = static_cast<int>(((a >> 8) & 0xffu) + ((b >> 8) & 0xffu)
				+ ((c0 >> 8) & 0xffu) + ((d >> 8) & 0xffu));
		const int blue = static_cast<int>((a & 0xffu) + (b & 0xffu)
				+ (c0 & 0xffu) + (d & 0xffu));
		// Average in HQ3x RGB space, then convert once to the live 6-bit palette.
		return quantize_rgb(
				(r + 8) / 16,
				(g + 8) / 16,
				(blue + 8) / 16);
	};

	const auto blend9_rgb = [&](const std::array<uint32, 9>& samples) {
		int r = 0;
		int g = 0;
		int b = 0;
		for (const uint32 sample : samples) {
			r += static_cast<int>((sample >> 16) & 0xffu);
			g += static_cast<int>((sample >> 8) & 0xffu);
			b += static_cast<int>(sample & 0xffu);
		}
		// HQ3x works in an 8-bit-like RGB range. Convert the 9-sample average
		// back to Exult's 6-bit palette channels only once, at the very end.
		return quantize_rgb(
				(r + 18) / 36,
				(g + 18) / 36,
				(b + 18) / 36);
	};

	const auto blend4 = [&](unsigned char a, unsigned char b, unsigned char c0, unsigned char d) {
		if (a == b && a == c0 && a == d) {
			return a;
		}
		const int r = (pal->get_red(a) + pal->get_red(b) + pal->get_red(c0) + pal->get_red(d) + 2) / 4;
		const int g = (pal->get_green(a) + pal->get_green(b) + pal->get_green(c0) + pal->get_green(d) + 2) / 4;
		const int blue = (pal->get_blue(a) + pal->get_blue(b) + pal->get_blue(c0) + pal->get_blue(d) + 2) / 4;
		return quantize_rgb(r, g, blue);
	};
	const auto blend9 = [&](const std::array<unsigned char, 9>& samples) {
		bool all_same = true;
		for (size_t n = 1; n < samples.size(); ++n) {
			if (samples[n] != samples[0]) {
				all_same = false;
				break;
			}
		}
		if (all_same) {
			return samples[0];
		}
		int r = 0;
		int g = 0;
		int b = 0;
		for (const unsigned char sample : samples) {
			r += pal->get_red(sample);
			g += pal->get_green(sample);
			b += pal->get_blue(sample);
		}
		return quantize_rgb((r + 4) / 9, (g + 4) / 9, (b + 4) / 9);
	};

	const double source_dx = world_view.display_to_scene_x_step();
	const double source_dy = world_view.display_to_scene_y_step();

	if (rotate_sampling_mode == 7) {
		// Forward weighted-quadruplet experiment.
		//
		// Every four-pixel diagonal window participates and windows overlap by one
		// source pixel: A-B-C-D, B-C-D-E, ... . A-B-C-D defines a continuous
		// piecewise-linear colour function along a transformed segment of length
		// 3*sqrt(2). Confidence is based on the third finite colour difference,
		// A-3B+3C-D: constant colours, linear gradients, and smoothly curved
		// quadratic-like gradients can all remain strong; abrupt irregular changes
		// are down-weighted.
		const size_t dest_count = static_cast<size_t>(display_width) * display_height;
		struct Quad_accum {
			float r;
			float g;
			float b;
			float w;
		};
		static thread_local std::vector<Quad_accum> accum;
		accum.resize(dest_count);
		std::fill(accum.begin(), accum.end(), Quad_accum{0.0f, 0.0f, 0.0f, 0.0f});

		std::array<int, 256> pal_r;
		std::array<int, 256> pal_g;
		std::array<int, 256> pal_b;
		for (int pi = 0; pi < 256; ++pi) {
			pal_r[pi] = pal->get_red(static_cast<unsigned char>(pi));
			pal_g[pi] = pal->get_green(static_cast<unsigned char>(pi));
			pal_b[pi] = pal->get_blue(static_cast<unsigned char>(pi));
		}

		// Per channel A-3B+3C-D is in [-252,252], so squared RGB magnitude is
		// at most 3*252^2 = 190512.
		static const std::array<float, 190513> quad_confidence_lut = [] {
			std::array<float, 190513> lut{};
			for (size_t i = 0; i < lut.size(); ++i) {
				const double third_difference = std::sqrt(static_cast<double>(i));
				lut[i] = static_cast<float>(1.0 / (1.0 + third_difference / 6.0));
			}
			return lut;
		}();

		const auto at = [&](int x, int y) {
			return static_cast<size_t>(y) * display_width + x;
		};
		const auto add_rgb = [&](int dx, int dy, float r, float g, float b, float weight) {
			if (weight <= 0.0f || dx < 0 || dx >= display_width || dy < 0 || dy >= display_height) {
				return;
			}
			Quad_accum& dst = accum[at(dx, dy)];
			dst.r += r * weight;
			dst.g += g * weight;
			dst.b += b * weight;
			dst.w += weight;
		};

		const auto average_colour_over = [&](unsigned char a, unsigned char b,
				unsigned char cc, unsigned char d, double t0, double t1,
				float& r, float& g, float& blue) {
			t0 = std::clamp(t0, 0.0, 1.0);
			t1 = std::clamp(t1, 0.0, 1.0);
			if (t1 < t0) {
				std::swap(t0, t1);
			}
			const double total = t1 - t0;
			if (total <= 0.0) {
				r = static_cast<float>(pal_r[a]);
				g = static_cast<float>(pal_g[a]);
				blue = static_cast<float>(pal_b[a]);
				return;
			}

			double ar = 0.0;
			double ag = 0.0;
			double ab = 0.0;
			double cur = t0;
			while (cur < t1) {
				double next;
				int r0, g0, b0, r1, g1, b1;
				double local_mid;
				if (cur < (1.0 / 3.0)) {
					next = std::min(t1, 1.0 / 3.0);
					r0 = pal_r[a]; g0 = pal_g[a]; b0 = pal_b[a];
					r1 = pal_r[b]; g1 = pal_g[b]; b1 = pal_b[b];
					local_mid = ((cur + next) * 0.5) * 3.0;
				} else if (cur < (2.0 / 3.0)) {
					next = std::min(t1, 2.0 / 3.0);
					r0 = pal_r[b]; g0 = pal_g[b]; b0 = pal_b[b];
					r1 = pal_r[cc]; g1 = pal_g[cc]; b1 = pal_b[cc];
					local_mid = (((cur + next) * 0.5) - 1.0 / 3.0) * 3.0;
				} else {
					next = t1;
					r0 = pal_r[cc]; g0 = pal_g[cc]; b0 = pal_b[cc];
					r1 = pal_r[d]; g1 = pal_g[d]; b1 = pal_b[d];
					local_mid = (((cur + next) * 0.5) - 2.0 / 3.0) * 3.0;
				}
				const double span = next - cur;
				ar += (r0 + (r1 - r0) * local_mid) * span;
				ag += (g0 + (g1 - g0) * local_mid) * span;
				ab += (b0 + (b1 - b0) * local_mid) * span;
				cur = next;
			}
			const double inv = 1.0 / total;
			r = static_cast<float>(ar * inv);
			g = static_cast<float>(ag * inv);
			blue = static_cast<float>(ab * inv);
		};

		constexpr float node_weight = 0.10f;
		constexpr double quad_extent = 4.2426406871192851464;
		constexpr double inv_quad_extent = 1.0 / quad_extent;
		constexpr double half_sqrt_2_fast = 0.7071067811865475244;
		int node_contributions = 0;
		int quadruplets = 0;
		int high_cohesion = 0;
		int medium_cohesion = 0;
		int low_cohesion = 0;
		int segment_cells = 0;
		double confidence_sum = 0.0;

		for (int sy = 0; sy < scene_size; ++sy) {
			const int src_y = scene_y + sy;
			const World_view_point row_p0 = world_view.scene_to_display(
					{static_cast<double>(scene_x) + 0.5, static_cast<double>(src_y) + 0.5});
			for (int sx = 0; sx < scene_size; ++sx) {
				const int src_x = scene_x + sx;
				const double offset = static_cast<double>(sx) * half_sqrt_2_fast;
				const World_view_point p0{row_p0.x + offset, row_p0.y + offset};

				const unsigned char node_color = scene_pixel(src_x, src_y);
				const int node_dx = static_cast<int>(std::floor(p0.x));
				const int node_dy = static_cast<int>(std::floor(p0.y));
				if (node_dx >= 0 && node_dx < display_width && node_dy >= 0 && node_dy < display_height) {
					add_rgb(node_dx, node_dy,
							static_cast<float>(pal_r[node_color]),
							static_cast<float>(pal_g[node_color]),
							static_cast<float>(pal_b[node_color]),
							node_weight);
					++node_contributions;
				}

				for (const auto delta : {std::pair<int, int>{1, 1}, std::pair<int, int>{1, -1}}) {
					const int x1 = src_x + delta.first;
					const int y1 = src_y + delta.second;
					const int x2 = src_x + delta.first * 2;
					const int y2 = src_y + delta.second * 2;
					const int x3 = src_x + delta.first * 3;
					const int y3 = src_y + delta.second * 3;
					if (x3 < scene_x || x3 >= scene_x + scene_size
							|| y3 < scene_y || y3 >= scene_y + scene_size) {
						continue;
					}

					const unsigned char a = node_color;
					const unsigned char b = scene_pixel(x1, y1);
					const unsigned char cc = scene_pixel(x2, y2);
					const unsigned char d = scene_pixel(x3, y3);
					const int d3r = pal_r[a] - 3 * pal_r[b] + 3 * pal_r[cc] - pal_r[d];
					const int d3g = pal_g[a] - 3 * pal_g[b] + 3 * pal_g[cc] - pal_g[d];
					const int d3b = pal_b[a] - 3 * pal_b[b] + 3 * pal_b[cc] - pal_b[d];
					const int third2 = d3r * d3r + d3g * d3g + d3b * d3b;
					const float confidence = quad_confidence_lut[static_cast<size_t>(third2)];
					++quadruplets;
					confidence_sum += confidence;
					if (confidence >= 0.80f) {
						++high_cohesion;
					} else if (confidence >= 0.40f) {
						++medium_cohesion;
					} else {
						++low_cohesion;
					}

					if (delta.second > 0) {
						const double lo = p0.y;
						const double hi = lo + quad_extent;
						const int dx = static_cast<int>(std::floor(p0.x));
						const int first = static_cast<int>(std::floor(lo));
						const int last = static_cast<int>(std::floor(std::nextafter(hi, lo)));
						for (int dy = first; dy <= last; ++dy) {
							const double cell_lo = std::max(lo, static_cast<double>(dy));
							const double cell_hi = std::min(hi, static_cast<double>(dy + 1));
							const double coverage = cell_hi - cell_lo;
							if (coverage <= 0.0) {
								continue;
							}
							const double t0 = (cell_lo - lo) * inv_quad_extent;
							const double t1 = (cell_hi - lo) * inv_quad_extent;
							float r, g, blue;
							average_colour_over(a, b, cc, d, t0, t1, r, g, blue);
							add_rgb(dx, dy, r, g, blue, confidence * static_cast<float>(coverage));
							++segment_cells;
						}
					} else {
						const double lo = p0.x;
						const double hi = lo + quad_extent;
						const int dy = static_cast<int>(std::floor(p0.y));
						const int first = static_cast<int>(std::floor(lo));
						const int last = static_cast<int>(std::floor(std::nextafter(hi, lo)));
						for (int dx = first; dx <= last; ++dx) {
							const double cell_lo = std::max(lo, static_cast<double>(dx));
							const double cell_hi = std::min(hi, static_cast<double>(dx + 1));
							const double coverage = cell_hi - cell_lo;
							if (coverage <= 0.0) {
								continue;
							}
							const double t0 = (cell_lo - lo) * inv_quad_extent;
							const double t1 = (cell_hi - lo) * inv_quad_extent;
							float r, g, blue;
							average_colour_over(a, b, cc, d, t0, t1, r, g, blue);
							add_rgb(dx, dy, r, g, blue, confidence * static_cast<float>(coverage));
							++segment_cells;
						}
					}
				}
			}
		}

		const unsigned char hole_color = static_cast<unsigned char>(pal->get_border_index());
		int holes = 0;
		for (int dy = 0; dy < display_height; ++dy) {
			for (int dx = 0; dx < display_width; ++dx) {
				const Quad_accum& src = accum[at(dx, dy)];
				if (src.w <= 0.0f) {
					++holes;
					win->put_pixel8(hole_color, dx, dy);
				} else {
					const float inv = 1.0f / src.w;
					win->put_pixel8(
							quantize_rgb(
									static_cast<int>(std::lround(src.r * inv)),
									static_cast<int>(std::lround(src.g * inv)),
									static_cast<int>(std::lround(src.b * inv))),
							dx, dy);
				}
			}
		}

	} else if (rotate_sampling_mode == 6) {
		// Forward weighted-pair experiment.
		//
		// Every adjacent diagonal pair participates, including pairs inside flat
		// colour fields. Pairs overlap by one source pixel along each diagonal run:
		// A-B, B-C, C-D, ... . The pair defines a continuous linear colour function
		// A->B over its transformed segment (length sqrt(2)); contribution weight
		// depends on RGB similarity of the pair. All destination contributions are
		// accumulated and normalized, with a small baseline node contribution so
		// isolated source details remain represented.
		const size_t dest_count = static_cast<size_t>(display_width) * display_height;
		struct Pair_accum {
			float r;
			float g;
			float b;
			float w;
		};
		static thread_local std::vector<Pair_accum> accum;
		accum.resize(dest_count);
		std::fill(accum.begin(), accum.end(), Pair_accum{0.0f, 0.0f, 0.0f, 0.0f});

		std::array<int, 256> pal_r;
		std::array<int, 256> pal_g;
		std::array<int, 256> pal_b;
		for (int pi = 0; pi < 256; ++pi) {
			pal_r[pi] = pal->get_red(static_cast<unsigned char>(pi));
			pal_g[pi] = pal->get_green(static_cast<unsigned char>(pi));
			pal_b[pi] = pal->get_blue(static_cast<unsigned char>(pi));
		}

		// RGB pair distance is in [0, sqrt(3*63^2)]. Precompute the exact
		// confidence function once; the hot loop indexes by squared distance.
		static const std::array<float, 11908> pair_confidence_lut = [] {
			std::array<float, 11908> lut{};
			for (size_t i = 0; i < lut.size(); ++i) {
				const double distance = std::sqrt(static_cast<double>(i));
				lut[i] = static_cast<float>(1.0 / (1.0 + distance / 6.0));
			}
			return lut;
		}();

		const auto at = [&](int x, int y) {
			return static_cast<size_t>(y) * display_width + x;
		};
		const auto add_rgb = [&](int dx, int dy, float r, float g, float b, float weight) {
			if (weight <= 0.0f || dx < 0 || dx >= display_width || dy < 0 || dy >= display_height) {
				return;
			}
			Pair_accum& dst = accum[at(dx, dy)];
			dst.r += r * weight;
			dst.g += g * weight;
			dst.b += b * weight;
			dst.w += weight;
		};

		constexpr float node_weight = 0.10f;
		constexpr double pair_extent = 1.4142135623730950488;
		constexpr double inv_pair_extent = 1.0 / pair_extent;
		constexpr double half_sqrt_2_fast = 0.7071067811865475244;
		int node_contributions = 0;
		int pairs = 0;
		int high_cohesion = 0;
		int medium_cohesion = 0;
		int low_cohesion = 0;
		int segment_cells = 0;
		double confidence_sum = 0.0;

		for (int sy = 0; sy < scene_size; ++sy) {
			const int src_y = scene_y + sy;
			const World_view_point row_p0 = world_view.scene_to_display(
					{static_cast<double>(scene_x) + 0.5, static_cast<double>(src_y) + 0.5});
			for (int sx = 0; sx < scene_size; ++sx) {
				const int src_x = scene_x + sx;
				const double offset = static_cast<double>(sx) * half_sqrt_2_fast;
				const World_view_point p0{row_p0.x + offset, row_p0.y + offset};

				const unsigned char node_color = scene_pixel(src_x, src_y);
				const int node_dx = static_cast<int>(std::floor(p0.x));
				const int node_dy = static_cast<int>(std::floor(p0.y));
				if (node_dx >= 0 && node_dx < display_width && node_dy >= 0 && node_dy < display_height) {
					add_rgb(node_dx, node_dy,
							static_cast<float>(pal_r[node_color]),
							static_cast<float>(pal_g[node_color]),
							static_cast<float>(pal_b[node_color]),
							node_weight);
					++node_contributions;
				}

				for (const auto delta : {std::pair<int, int>{1, 1}, std::pair<int, int>{1, -1}}) {
					const int x1 = src_x + delta.first;
					const int y1 = src_y + delta.second;
					if (x1 < scene_x || x1 >= scene_x + scene_size
							|| y1 < scene_y || y1 >= scene_y + scene_size) {
						continue;
					}

					const unsigned char a = node_color;
					const unsigned char b = scene_pixel(x1, y1);
					const int dr = pal_r[a] - pal_r[b];
					const int dg = pal_g[a] - pal_g[b];
					const int db = pal_b[a] - pal_b[b];
					const int distance2 = dr * dr + dg * dg + db * db;
					const float confidence = pair_confidence_lut[static_cast<size_t>(distance2)];
					++pairs;
					confidence_sum += confidence;
					if (confidence >= 0.80f) {
						++high_cohesion;
					} else if (confidence >= 0.40f) {
						++medium_cohesion;
					} else {
						++low_cohesion;
					}

					// The rasterized cell midpoint is guaranteed to lie on the
					// A->B segment, so no per-sample clamp/helper is needed.
					const float ar = static_cast<float>(pal_r[a]);
					const float ag = static_cast<float>(pal_g[a]);
					const float ab = static_cast<float>(pal_b[a]);
					const float dcr = static_cast<float>(pal_r[b] - pal_r[a]);
					const float dcg = static_cast<float>(pal_g[b] - pal_g[a]);
					const float dcb = static_cast<float>(pal_b[b] - pal_b[a]);

					if (delta.second > 0) {
						// Source '\\' pair -> vertical destination segment.
						const double lo = p0.y;
						const double hi = lo + pair_extent;
						const int dx = static_cast<int>(std::floor(p0.x));
						const int first = static_cast<int>(std::floor(lo));
						const int last = static_cast<int>(std::floor(std::nextafter(hi, lo)));
						for (int dy = first; dy <= last; ++dy) {
							const double cell_lo = std::max(lo, static_cast<double>(dy));
							const double cell_hi = std::min(hi, static_cast<double>(dy + 1));
							const double coverage = cell_hi - cell_lo;
							if (coverage <= 0.0) {
								continue;
							}
							const float tmid = static_cast<float>(((cell_lo + cell_hi) * 0.5 - lo) * inv_pair_extent);
							add_rgb(
									dx, dy,
									ar + dcr * tmid,
									ag + dcg * tmid,
									ab + dcb * tmid,
									confidence * static_cast<float>(coverage));
							++segment_cells;
						}
					} else {
						// Source '/' pair -> horizontal destination segment.
						const double lo = p0.x;
						const double hi = lo + pair_extent;
						const int dy = static_cast<int>(std::floor(p0.y));
						const int first = static_cast<int>(std::floor(lo));
						const int last = static_cast<int>(std::floor(std::nextafter(hi, lo)));
						for (int dx = first; dx <= last; ++dx) {
							const double cell_lo = std::max(lo, static_cast<double>(dx));
							const double cell_hi = std::min(hi, static_cast<double>(dx + 1));
							const double coverage = cell_hi - cell_lo;
							if (coverage <= 0.0) {
								continue;
							}
							const float tmid = static_cast<float>(((cell_lo + cell_hi) * 0.5 - lo) * inv_pair_extent);
							add_rgb(
									dx, dy,
									ar + dcr * tmid,
									ag + dcg * tmid,
									ab + dcb * tmid,
									confidence * static_cast<float>(coverage));
							++segment_cells;
						}
					}
				}
			}
		}

		const unsigned char hole_color = static_cast<unsigned char>(pal->get_border_index());
		int holes = 0;
		for (int dy = 0; dy < display_height; ++dy) {
			for (int dx = 0; dx < display_width; ++dx) {
				const Pair_accum& src = accum[at(dx, dy)];
				if (src.w <= 0.0f) {
					++holes;
					win->put_pixel8(hole_color, dx, dy);
				} else {
					const float inv = 1.0f / src.w;
					win->put_pixel8(
							quantize_rgb(
									static_cast<int>(std::lround(src.r * inv)),
									static_cast<int>(std::lround(src.g * inv)),
									static_cast<int>(std::lround(src.b * inv))),
							dx, dy);
				}
			}
		}

	} else if (rotate_sampling_mode == 5) {
		// Forward weighted-triplet experiment.
		//
		// Every three-pixel diagonal run participates, including runs inside flat
		// colour fields. The three source colours define a continuous piecewise-
		// linear colour function A->B->C along the transformed segment. Its weight
		// depends only on how regular that colour progression is: constant colours
		// and smooth gradients are strong, erratic colour changes are weak.
		//
		// Contributions are accumulated and normalized; there is no special-case
		// distinction between "real lines" and uniform areas. Original source nodes
		// contribute a small baseline weight so isolated details are not discarded.
		const size_t dest_count = static_cast<size_t>(display_width) * display_height;
		struct Triplet_accum {
			float r;
			float g;
			float b;
			float w;
		};
		static thread_local std::vector<Triplet_accum> accum;
		accum.resize(dest_count);
		std::fill(accum.begin(), accum.end(), Triplet_accum{0.0f, 0.0f, 0.0f, 0.0f});

		std::array<int, 256> pal_r;
		std::array<int, 256> pal_g;
		std::array<int, 256> pal_b;
		for (int pi = 0; pi < 256; ++pi) {
			pal_r[pi] = pal->get_red(static_cast<unsigned char>(pi));
			pal_g[pi] = pal->get_green(static_cast<unsigned char>(pi));
			pal_b[pi] = pal->get_blue(static_cast<unsigned char>(pi));
		}

		// A-2B+C is in [-126,126] for 6-bit palette channels. The confidence
		// function depends only on its squared RGB magnitude, so cache all possible
		// values once instead of doing two sqrt() operations per source pixel.
		static const std::array<float, 47629> confidence_lut = [] {
			std::array<float, 47629> lut{};
			for (size_t i = 0; i < lut.size(); ++i) {
				const double curvature = std::sqrt(static_cast<double>(i));
				lut[i] = static_cast<float>(1.0 / (1.0 + curvature / 6.0));
			}
			return lut;
		}();

		const auto at = [&](int x, int y) {
			return static_cast<size_t>(y) * display_width + x;
		};
		const auto add_rgb = [&](int dx, int dy, float r, float g, float b, float weight) {
			if (weight <= 0.0f || dx < 0 || dx >= display_width || dy < 0 || dy >= display_height) {
				return;
			}
			const size_t pos = at(dx, dy);
			Triplet_accum& dst = accum[pos];
			dst.r += r * weight;
			dst.g += g * weight;
			dst.b += b * weight;
			dst.w += weight;
		};
		const auto colour_at = [&](unsigned char a, unsigned char b, unsigned char cc, double t,
				float& r, float& g, float& blue) {
			t = std::clamp(t, 0.0, 1.0);
			const auto channel = [&](int av, int bv, int cv) {
				if (t <= 0.5) {
					const double u = t * 2.0;
					return static_cast<float>(av + (bv - av) * u);
				}
				const double u = (t - 0.5) * 2.0;
				return static_cast<float>(bv + (cv - bv) * u);
			};
			r = channel(pal_r[a], pal_r[b], pal_r[cc]);
			g = channel(pal_g[a], pal_g[b], pal_g[cc]);
			blue = channel(pal_b[a], pal_b[b], pal_b[cc]);
		};
		const auto average_colour_over = [&](unsigned char a, unsigned char b, unsigned char cc,
				double t0, double t1, float& r, float& g, float& blue) {
			// Midpoint is exact on either linear half. If the destination cell spans
			// B (t=.5), split the integral so all three colours retain their proper
			// influence rather than pre-averaging A/B/C.
			const auto sample_mid = [&](double lo, double hi, float& rr, float& gg, float& bb) {
				colour_at(a, b, cc, (lo + hi) * 0.5, rr, gg, bb);
			};
			if (t0 < 0.5 && t1 > 0.5) {
				float r0, g0, b0, r1, g1, b1;
				sample_mid(t0, 0.5, r0, g0, b0);
				sample_mid(0.5, t1, r1, g1, b1);
				const double w0 = 0.5 - t0;
				const double w1 = t1 - 0.5;
				const double inv = 1.0 / (w0 + w1);
				r = static_cast<float>((r0 * w0 + r1 * w1) * inv);
				g = static_cast<float>((g0 * w0 + g1 * w1) * inv);
				blue = static_cast<float>((b0 * w0 + b1 * w1) * inv);
			} else {
				sample_mid(t0, t1, r, g, blue);
			}
		};

		// Low-weight node evidence is emitted from the same source scan as the
		// triplets below, avoiding a second full traversal of the expanded scene.
		constexpr float node_weight = 0.10f;
		int node_contributions = 0;

		int triplets = 0;
		int high_cohesion = 0;
		int medium_cohesion = 0;
		int low_cohesion = 0;
		int segment_cells = 0;
		double confidence_sum = 0.0;

		constexpr double triplet_extent = 2.8284271247461900976;
		constexpr double inv_triplet_extent = 1.0 / triplet_extent;
		constexpr double half_sqrt_2_fast = 0.7071067811865475244;
		for (int sy = 0; sy < scene_size; ++sy) {
			const int src_y = scene_y + sy;
			const World_view_point row_p0 = world_view.scene_to_display(
					{static_cast<double>(scene_x) + 0.5, static_cast<double>(src_y) + 0.5});
			for (int sx = 0; sx < scene_size; ++sx) {
				const int src_x = scene_x + sx;
				const double offset = static_cast<double>(sx) * half_sqrt_2_fast;
				const World_view_point p0{row_p0.x + offset, row_p0.y + offset};

				const unsigned char node_color = scene_pixel(src_x, src_y);
				const int node_dx = static_cast<int>(std::floor(p0.x));
				const int node_dy = static_cast<int>(std::floor(p0.y));
				if (node_dx >= 0 && node_dx < display_width && node_dy >= 0 && node_dy < display_height) {
					add_rgb(node_dx, node_dy,
							static_cast<float>(pal_r[node_color]),
							static_cast<float>(pal_g[node_color]),
							static_cast<float>(pal_b[node_color]),
							node_weight);
					++node_contributions;
				}

				for (const auto delta : {std::pair<int, int>{1, 1}, std::pair<int, int>{1, -1}}) {
					const int x1 = src_x + delta.first;
					const int y1 = src_y + delta.second;
					const int x2 = src_x + delta.first * 2;
					const int y2 = src_y + delta.second * 2;
					if (x2 < scene_x || x2 >= scene_x + scene_size
							|| y2 < scene_y || y2 >= scene_y + scene_size) {
						continue;
					}

					const unsigned char a = scene_pixel(src_x, src_y);
					const unsigned char b = scene_pixel(x1, y1);
					const unsigned char cc = scene_pixel(x2, y2);

					// Second colour derivative: A-2B+C. Zero means either a perfectly
					// flat run or a perfectly regular gradient, both maximally coherent.
					const int d2r = pal_r[a] - 2 * pal_r[b] + pal_r[cc];
					const int d2g = pal_g[a] - 2 * pal_g[b] + pal_g[cc];
					const int d2b = pal_b[a] - 2 * pal_b[b] + pal_b[cc];
					const int curvature2 = d2r * d2r + d2g * d2g + d2b * d2b;
					const float confidence = confidence_lut[static_cast<size_t>(curvature2)];
					++triplets;
					confidence_sum += confidence;
					if (confidence >= 0.80f) {
						++high_cohesion;
					} else if (confidence >= 0.40f) {
						++medium_cohesion;
					} else {
						++low_cohesion;
					}

					if (delta.second > 0) {
						// Source '\\' run: vertical destination segment.
						const double lo = p0.y;
						const double hi = lo + triplet_extent;
						const int dx = static_cast<int>(std::floor(p0.x));
						const int first = static_cast<int>(std::floor(lo));
						const int last = static_cast<int>(std::floor(std::nextafter(hi, lo)));
						for (int dy = first; dy <= last; ++dy) {
							const double cell_lo = std::max(lo, static_cast<double>(dy));
							const double cell_hi = std::min(hi, static_cast<double>(dy + 1));
							const double coverage = cell_hi - cell_lo;
							if (coverage <= 0.0) {
								continue;
							}
							const double t0 = (cell_lo - lo) * inv_triplet_extent;
							const double t1 = (cell_hi - lo) * inv_triplet_extent;
							float r, g, blue;
							average_colour_over(a, b, cc, t0, t1, r, g, blue);
							add_rgb(dx, dy, r, g, blue, confidence * static_cast<float>(coverage));
							++segment_cells;
						}
					} else {
						// Source '/' run: horizontal destination segment.
						const double lo = p0.x;
						const double hi = lo + triplet_extent;
						const int dy = static_cast<int>(std::floor(p0.y));
						const int first = static_cast<int>(std::floor(lo));
						const int last = static_cast<int>(std::floor(std::nextafter(hi, lo)));
						for (int dx = first; dx <= last; ++dx) {
							const double cell_lo = std::max(lo, static_cast<double>(dx));
							const double cell_hi = std::min(hi, static_cast<double>(dx + 1));
							const double coverage = cell_hi - cell_lo;
							if (coverage <= 0.0) {
								continue;
							}
							const double t0 = (cell_lo - lo) * inv_triplet_extent;
							const double t1 = (cell_hi - lo) * inv_triplet_extent;
							float r, g, blue;
							average_colour_over(a, b, cc, t0, t1, r, g, blue);
							add_rgb(dx, dy, r, g, blue, confidence * static_cast<float>(coverage));
							++segment_cells;
						}
					}
				}
			}
		}

		const unsigned char hole_color = static_cast<unsigned char>(pal->get_border_index());
		int holes = 0;
		for (int dy = 0; dy < display_height; ++dy) {
			for (int dx = 0; dx < display_width; ++dx) {
				const size_t pos = at(dx, dy);
				const Triplet_accum& src = accum[pos];
				if (src.w <= 0.0f) {
					++holes;
					win->put_pixel8(hole_color, dx, dy);
				} else {
					const float inv = 1.0f / src.w;
					win->put_pixel8(
							quantize_rgb(
									static_cast<int>(std::lround(src.r * inv)),
									static_cast<int>(std::lround(src.g * inv)),
									static_cast<int>(std::lround(src.b * inv))),
							dx, dy);
				}
			}
		}


	} else if (rotate_sampling_mode == 0 || rotate_sampling_mode == 1 || rotate_sampling_mode == 4) {
		// Four-subpixel integration is independent of the nine-sample path.
		// In particular, modes 0/1 do not allocate an HQ3x RGB buffer.
		constexpr std::array<double, 2> offsets = {0.25, 0.75};
		for (int dy = 0; dy < display_height; ++dy) {
			for (int dx = 0; dx < display_width; ++dx) {
				std::array<World_view_point, 4> sources;
				for (int oy = 0; oy < 2; ++oy) {
					for (int ox = 0; ox < 2; ++ox) {
						sources[oy * 2 + ox] = world_view.display_to_scene(
								{static_cast<double>(dx) + offsets[ox], static_cast<double>(dy) + offsets[oy]});
					}
				}
#ifdef USE_HQ3X_SCALER
				if (rotate_sampling_mode == 4) {
					win->put_pixel8(blend4_rgb(
							sample_hq3x(sources[0]), sample_hq3x(sources[1]),
							sample_hq3x(sources[2]), sample_hq3x(sources[3])), dx, dy);
					continue;
				}
#endif
				const int factor = rotate_sampling_mode == 0 ? 2 : 3;
				Image_buffer8* scaled = rotate_sampling_mode == 0 ? rotate_scene_2x.get() : rotate_scene_3x.get();
				win->put_pixel8(blend4(
						sample_scaled(sources[0], factor, scaled),
						sample_scaled(sources[1], factor, scaled),
						sample_scaled(sources[2], factor, scaled),
						sample_scaled(sources[3], factor, scaled)), dx, dy);
			}
		}
	} else {
		// Modes 2 and 3: nine-subpixel integration.
		constexpr std::array<double, 3> offsets = {1.0 / 6.0, 0.5, 5.0 / 6.0};
		for (int dy = 0; dy < display_height; ++dy) {
			std::array<World_view_point, 9> sources;
			for (int oy = 0; oy < 3; ++oy) {
				for (int ox = 0; ox < 3; ++ox) {
					sources[oy * 3 + ox] = world_view.display_to_scene(
							{offsets[ox], static_cast<double>(dy) + offsets[oy]});
				}
			}
			for (int dx = 0; dx < display_width; ++dx) {
				if (rotate_sampling_mode == 2) {
					std::array<unsigned char, 9> samples;
					for (size_t n = 0; n < samples.size(); ++n) {
						samples[n] = sample_scaled(sources[n], 3, rotate_scene_3x.get());
						sources[n].x += source_dx;
						sources[n].y += source_dy;
					}
					win->put_pixel8(blend9(samples), dx, dy);
				} else {
#ifdef USE_HQ3X_SCALER
					std::array<uint32, 9> samples;
					for (size_t n = 0; n < samples.size(); ++n) {
						samples[n] = sample_hq3x(sources[n]);
						sources[n].x += source_dx;
						sources[n].y += source_dy;
					}
					win->put_pixel8(blend9_rgb(samples), dx, dy);
#else
					// Builds without HQ3x clamp mode selection to 0..2, so this is
					// unreachable. Keep source stepping complete for safety.
					for (auto& source : sources) {
						source.x += source_dx;
						source.y += source_dy;
					}
#endif
				}
			}
		}
	}

	win->set_clip(0, 0, display_width, display_height);
	gump_man->paint(false);
	if (dragging) {
		if (dragging->is_world_object_drag()) {
			if (dragging->is_over_gump()) {
				dragging->paint_gump_hover_overlay();
			}
		} else {
			dragging->paint();
		}
	}
	effects->paint_text();
	gump_man->paint(true);
	win->EndPaintIntoGuardBand();
	win->clear_clip();

	if (main_actor) {
		update_lighting(light_sources);
	}
}
