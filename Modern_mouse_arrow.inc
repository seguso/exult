// Modern smooth mouse arrow paint implementation; included by mouse.cc.
void Mouse::draw_rotated_arrow_to_layer(Image_buffer8* lb, unsigned char* trans) {
	if (!lb || !cur || layer_w <= 0 || layer_h <= 0) {
		return;
	}

	// Paint the selected original arrow frame first. This deliberately keeps
	// Exult's three different arrow lengths and the combat artwork; the software
	// rotation only corrects the <=22.5 degree residual from the nearest of the
	// existing eight directions.
	Image_buffer8 source(layer_w, layer_h);
	source.fill8(255);
	if (trans) {
		cur->paint_rle_remapped(&source, hot_x, hot_y, trans);
	} else {
		cur->paint_rle(&source, hot_x, hot_y);
	}

	// Scale2x/EPX reconstruction before rotation. Rotating the native tiny
	// sprite directly makes its one-pixel shaft and outline break up badly.
	const int hi_w = layer_w * 2;
	const int hi_h = layer_h * 2;
	std::vector<unsigned char> hi(static_cast<size_t>(hi_w) * hi_h, 255);
	const auto src_at = [&](int x, int y) -> unsigned char {
		if (x < 0 || y < 0 || x >= layer_w || y >= layer_h) {
			return 255;
		}
		return source.get_pixel8(x, y);
	};
	for (int y = 0; y < layer_h; ++y) {
		for (int x = 0; x < layer_w; ++x) {
			const unsigned char e = src_at(x, y);
			const unsigned char b = src_at(x, y - 1);
			const unsigned char d = src_at(x - 1, y);
			const unsigned char f = src_at(x + 1, y);
			const unsigned char h = src_at(x, y + 1);
			unsigned char e0 = e;
			unsigned char e1 = e;
			unsigned char e2 = e;
			unsigned char e3 = e;
			// Treat transparency as coverage, not as an ordinary Scale2x colour.
			// Allowing EPX to replace a transparent source pixel with one of its
			// opaque neighbours creates tiny coloured islands just outside the
			// cursor silhouette. After bilinear rotation those become the bright
			// white/orange/green speckles visible at only some residual angles.
			if (e != 255 && b != h && d != f) {
				e0 = d == b ? d : e;
				e1 = b == f ? f : e;
				e2 = d == h ? d : e;
				e3 = h == f ? f : e;
			}
			const int ox = x * 2;
			const int oy = y * 2;
			hi[static_cast<size_t>(oy) * hi_w + ox] = e0;
			hi[static_cast<size_t>(oy) * hi_w + ox + 1] = e1;
			hi[static_cast<size_t>(oy + 1) * hi_w + ox] = e2;
			hi[static_cast<size_t>(oy + 1) * hi_w + ox + 1] = e3;
		}
	}

	Palette* pal = gwin->get_pal();
	const int max_val = pal ? std::max(1, pal->get_max_val()) : 63;
	const int brightness = pal ? pal->get_brightness() : 100;
	const auto channel8 = [&](unsigned char v) -> double {
		const double scaled = static_cast<double>(v) * 255.0 / max_val * brightness / 100.0;
		return std::clamp(scaled, 0.0, 255.0);
	};
	const auto rgba_for_index = [&](unsigned char idx, double& r, double& g, double& b, double& a) {
		if (idx == 255) {
			r = g = b = a = 0.0;
			return;
		}
		a = 1.0;
		if (pal) {
			r = channel8(pal->get_red(idx));
			g = channel8(pal->get_green(idx));
			b = channel8(pal->get_blue(idx));
		} else {
			r = g = b = static_cast<double>(idx);
		}
	};

	// A per-layer ARGB palette gives us genuine fractional alpha at the rotated
	// outline even though the backing buffer is still 8-bit indexed.
	std::array<uint32, 256> argb{};
	std::vector<uint32> generated;
	generated.reserve(128);
	std::unordered_map<uint32, unsigned char> generated_index;
	const auto encode_rgba = [&](int r, int g, int b, int a) -> unsigned char {
		if (a <= 3) {
			return 255;
		}
		// Small quantization keeps the number of dynamic palette entries bounded
		// and prevents tiny floating-point changes from creating new slots.
		r = std::clamp((r + 3) & ~7, 0, 255);
		g = std::clamp((g + 3) & ~7, 0, 255);
		b = std::clamp((b + 3) & ~7, 0, 255);
		a = std::clamp((a + 7) & ~15, 0, 255);
		// index_argb uses a zero entry as the sentinel for "no ARGB override;
		// fall back to the live game palette".  Very faint black AA samples can
		// quantize to A=R=G=B=0, which would therefore turn into an unrelated
		// live-palette colour (often a bright white/orange speck).  Never allocate
		// such an entry: once alpha quantizes to zero, use the real transparent
		// index instead.
		if (a == 0) {
			return 255;
		}
		const uint32 packed = (static_cast<uint32>(a) << 24) | (static_cast<uint32>(r) << 16)
							  | (static_cast<uint32>(g) << 8) | static_cast<uint32>(b);
		const auto found = generated_index.find(packed);
		if (found != generated_index.end()) {
			return found->second;
		}
		if (generated.size() < 255) {
			const auto index = static_cast<unsigned char>(generated.size());
			generated.push_back(packed);
			generated_index.emplace(packed, index);
			argb[index] = packed;
			return index;
		}

		// Extremely unlikely for these tiny, few-colour sprites, but retain a
		// deterministic fallback if an exotic pointer frame exceeds 255 RGBA
		// combinations.
		int best = 0;
		long best_dist = LONG_MAX;
		for (size_t i = 0; i < generated.size(); ++i) {
			const uint32 p = generated[i];
			const int pa = static_cast<int>((p >> 24) & 0xff);
			const int pr = static_cast<int>((p >> 16) & 0xff);
			const int pg = static_cast<int>((p >> 8) & 0xff);
			const int pb = static_cast<int>(p & 0xff);
			const long da = a - pa;
			const long dr = r - pr;
			const long dg = g - pg;
			const long db = b - pb;
			const long dist = da * da * 2 + dr * dr + dg * dg + db * db;
			if (dist < best_dist) {
				best_dist = dist;
				best = static_cast<int>(i);
			}
		}
		return static_cast<unsigned char>(best);
	};

	lb->fill8(255);
	const double cs = std::cos(smooth_arrow_residual_rad);
	const double sn = std::sin(smooth_arrow_residual_rad);
	const auto hi_at = [&](int x, int y) -> unsigned char {
		if (x < 0 || y < 0 || x >= hi_w || y >= hi_h) {
			return 255;
		}
		return hi[static_cast<size_t>(y) * hi_w + x];
	};

	for (int y = 0; y < layer_h; ++y) {
		for (int x = 0; x < layer_w; ++x) {
			// Inverse-map destination pixel centre around the cursor hotspot.
			const double rx = (x + 0.5) - hot_x;
			const double ry = (y + 0.5) - hot_y;
			const double sx = cs * rx + sn * ry + hot_x;
			const double sy = -sn * rx + cs * ry + hot_y;

			// Bilinear sample from the 2x Scale2x image. Transparent samples
			// contribute alpha=0, so the resulting edge is genuinely antialiased.
			const double hx = sx * 2.0 - 0.5;
			const double hy = sy * 2.0 - 0.5;
			const int x0 = static_cast<int>(std::floor(hx));
			const int y0 = static_cast<int>(std::floor(hy));
			const double fx = hx - x0;
			const double fy = hy - y0;
			const double weights[4] = {
				(1.0 - fx) * (1.0 - fy), fx * (1.0 - fy), (1.0 - fx) * fy, fx * fy};
			const int sample_x[4] = {x0, x0 + 1, x0, x0 + 1};
			const int sample_y[4] = {y0, y0, y0 + 1, y0 + 1};
			double alpha = 0.0;
			double premul_r = 0.0;
			double premul_g = 0.0;
			double premul_b = 0.0;
			int opaque_samples = 0;
			bool has_dark_support = false;
			for (int i = 0; i < 4; ++i) {
				double sr, sg, sb, sa;
				rgba_for_index(hi_at(sample_x[i], sample_y[i]), sr, sg, sb, sa);
				if (sa > 0.0) {
					++opaque_samples;
					// The steering arrows have a dark outline.  A legitimate outer
					// antialiasing sample should normally include that outline among
					// its opaque support.  Bright-only support is interior colour
					// leaking directly into transparency.
					const double luminance = 0.2126 * sr + 0.7152 * sg + 0.0722 * sb;
					if (luminance < 96.0) {
						has_dark_support = true;
					}
				}
				const double wa = weights[i] * sa;
				alpha += wa;
				premul_r += sr * wa;
				premul_g += sg * wa;
				premul_b += sb * wa;
			}
			if (alpha <= 0.01) {
				continue;
			}
			// A single diagonally-touched opaque Scale2x texel can leak a bright
			// interior highlight into otherwise transparent space.  It shows up as
			// an isolated white/orange/green speck just outside the black outline at
			// certain residual angles.  Suppress only these low-coverage one-tap
			// samples; real antialiased edges normally have support from multiple
			// opaque texels (or high coverage from the nearest one).
			if (opaque_samples == 1 && alpha < 0.75) {
				continue;
			}
			// Also reject partial edge samples that contain only bright interior
			// colours and no dark outline support. These are the remaining white/
			// green fringe pixels that can survive the one-tap test at some angles.
			if (alpha < 0.98 && !has_dark_support) {
				continue;
			}
			const int rr = static_cast<int>(std::lround(premul_r / alpha));
			const int gg = static_cast<int>(std::lround(premul_g / alpha));
			const int bb = static_cast<int>(std::lround(premul_b / alpha));
			const int aa = static_cast<int>(std::lround(alpha * 255.0));
			lb->put_pixel8(encode_rgba(rr, gg, bb, aa), x, y);
		}
	}

	argb[255] = 0;
	gwin->layer_set_index_argb(mouse_layer, argb.data());
}
