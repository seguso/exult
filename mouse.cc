/*
 *  mouse.cc - Mouse pointers.
 *
 *  Copyright (C) 2000-2025  The Exult Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#	pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#	if !defined(__llvm__) && !defined(__clang__)
#		pragma GCC diagnostic ignored "-Wuseless-cast"
#	endif
#endif    // __GNUC__
#include <SDL3/SDL.h>
#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif    // __GNUC__

#include "Gump.h"
#include "Gump_manager.h"
#include "actors.h"
#include "barge.h"
#include "cheat.h"
#include "combat.h"
#include "combat_opts.h"
#include "fnames.h"
#include "gamewin.h"
#include "ibuf8.h"
#include "mouse.h"
#include "palette.h"
#include "schedule.h" /* To get Schedule::combat */
#include "ucsched.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#ifndef max
using std::max;
#endif

bool Mouse::use_touch_input = false;

static inline bool should_hide_frame(int frame) {
	// on touch input only we hide the cursor
	if (Mouse::use_touch_input) {
		return frame == 0 || (frame >= 8 && frame <= 47);
	} else {
		return false;
	}
}

short Mouse::short_arrows[8] = {8, 9, 10, 11, 12, 13, 14, 15};
short Mouse::med_arrows[8]   = {16, 17, 18, 19, 20, 21, 22, 23};
short Mouse::long_arrows[8]  = {24, 25, 26, 27, 28, 29, 30, 31};

short Mouse::short_combat_arrows[8] = {32, 33, 34, 35, 36, 37, 38, 39};
short Mouse::med_combat_arrows[8]   = {40, 41, 42, 43, 44, 45, 46, 47};

Mouse* Mouse::current_mouse = nullptr;
bool   Mouse::mouse_update  = false;

/*
 *  Create.
 */

void Mouse::MakeCurrent() {
	// we are already current so do nothing
	if (this == current_mouse) {
		return;
	}
	// remove this from the current list if it is in it
	for (Mouse** list = &current_mouse; *list; list = &(*list)->previous) {
		if (this == *list) {
			*list    = previous;
			previous = nullptr;
			break;
		}
	}

	previous      = current_mouse;
	current_mouse = this;
}

Mouse::Mouse(Game_window* gw    // Where to draw.
			 )
		: gwin(gw), iwin(gwin->get_win()), box(0, 0, 0, 0), dirty(0, 0, 0, 0), cur_framenum(0), cur(nullptr),
		  avatar_speed(100 * gwin->get_std_delay() / slow_speed_factor) {
	float fx, fy;
	SDL_GetMouseState(&fx, &fy);
	mousex = int(fx);
	mousey = int(fy);
	iwin->screen_to_game(mousex, mousey, gwin->get_fastmouse(), mousex, mousey);
	if (is_system_path_defined("<PATCH>") && U7exists(PATCH_POINTERS)) {
		pointers.load(PATCH_POINTERS);
	} else {
		pointers.load(POINTERS);
	}
	Init();
	set_shape(get_short_arrow(east));    // +++++For now.
	MakeCurrent();
}

Mouse::Mouse(
		Game_window* gw,    // Where to draw.
		IDataSource& shapes)
		: gwin(gw), iwin(gwin->get_win()), box(0, 0, 0, 0), dirty(0, 0, 0, 0), cur_framenum(0), cur(nullptr),
		  avatar_speed(100 * gwin->get_std_delay() / slow_speed_factor) {
	float fx, fy;
	SDL_GetMouseState(&fx, &fy);
	mousex = int(fx);
	mousey = int(fy);
	iwin->screen_to_game(mousex, mousey, gwin->get_fastmouse(), mousex, mousey);
	pointers.load(&shapes);
	Init();
	set_shape0(0);
	MakeCurrent();
}

Mouse::~Mouse() {
	// Free the cursor's layer, but only if the game window still exists. At
	// program exit gwin may already have been deleted (Play() deletes it), so
	// guard against touching a dangling pointer.
	if (mouse_layer >= 0 && Game_window::get_instance() == gwin) {
		gwin->destroy_layer(mouse_layer);
	}
	mouse_layer = -1;
	// remove this from the current list if it is in it
	for (Mouse** list = &current_mouse; *list; list = &(*list)->previous) {
		if (this == *list) {
			*list    = previous;
			previous = nullptr;
			break;
		}
	}
}

void Mouse::Init() {
	int maxleft  = 0;
	int maxright = 0;
	int maxabove = 0;
	int maxbelow = 0;
	for (auto& frame : pointers) {
		const int xleft  = frame->get_xleft();
		const int xright = frame->get_xright();
		const int yabove = frame->get_yabove();
		const int ybelow = frame->get_ybelow();
		if (xleft > maxleft) {
			maxleft = xleft;
		}
		if (xright > maxright) {
			maxright = xright;
		}
		if (yabove > maxabove) {
			maxabove = yabove;
		}
		if (ybelow > maxbelow) {
			maxbelow = ybelow;
		}
	}
	const int maxw = maxleft + maxright + 1;
	const int maxh = maxabove + maxbelow + 1;
	// Create backup buffer.
	backup = iwin->create_buffer(maxw, maxh);
	box.w  = maxw;
	box.h  = maxh;
	// Geometry for the cursor's overlay layer.
	layer_w          = maxw;
	layer_h          = maxh;
	hot_x            = maxleft;
	hot_y            = maxabove;
	last_layer_frame = -1;

	onscreen = false;    // initially offscreen
}

/*
 *  Overlay-layer helpers. The cursor is drawn onto its own layer, which is
 *  composited on top of every other layer (e.g. the conversation faces).
 */

// A very high z so the cursor is always composited last (on top).
static const int mouse_layer_z = 1 << 20;

bool Mouse::ensure_mouse_layer() {
	if (mouse_layer >= 0) {
		return true;
	}
	if (layer_w <= 0 || layer_h <= 0) {
		return false;
	}
	mouse_layer = gwin->create_layer("mouse", layer_w, layer_h, 255, 0, mouse_layer_z);
	if (mouse_layer < 0) {
		return false;
	}
	gwin->layer_set_ui_kind(mouse_layer, Image_window::UiLayerMousePointer);
	last_layer_frame = -1;    // Force a redraw.
	return true;
}

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
			if (b != h && d != f) {
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
			for (int i = 0; i < 4; ++i) {
				double sr, sg, sb, sa;
				rgba_for_index(hi_at(sample_x[i], sample_y[i]), sr, sg, sb, sa);
				const double wa = weights[i] * sa;
				alpha += wa;
				premul_r += sr * wa;
				premul_g += sg * wa;
				premul_b += sb * wa;
			}
			if (alpha <= 0.01) {
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

void Mouse::draw_cursor_to_layer(unsigned char* trans) {
	if (mouse_layer < 0 || !cur) {
		return;
	}
	Image_buffer8* lb = gwin->get_layer_ibuf(mouse_layer);
	if (!lb) {
		return;
	}
	if (smooth_arrow_active) {
		draw_rotated_arrow_to_layer(lb, trans);
	} else {
		// Restore the normal live-palette path for hands, targeting cursors,
		// editor pointers, etc.
		gwin->layer_set_index_argb(mouse_layer, nullptr);
		lb->fill8(255);    // Clear to transparent.
		if (trans) {
			cur->paint_rle_remapped(lb, hot_x, hot_y, trans);
		} else {
			cur->paint_rle(lb, hot_x, hot_y);
		}
	}
	gwin->layer_set_dirty(mouse_layer);
	last_layer_frame = cur_framenum;
	last_layer_trans = trans;
	last_layer_angle_bucket = smooth_arrow_active ? smooth_arrow_angle_bucket : -1;
}

void Mouse::get_pointer_scale(float& sx, float& sy) const {
	SDL_FRect fr;
	iwin->compute_ui_layer_dest(320, 200, fr, Image_window::UiLayerMousePointer);
	sx = fr.w / 320.0f;
	sy = fr.h / 200.0f;
}

void Mouse::position_mouse_layer() {
	if (mouse_layer < 0) {
		return;
	}
	// The cursor uses the same scale as the 320x200 overlay so it matches the
	// conversation.
	float sx;
	float sy;
	get_pointer_scale(sx, sy);
	// Map the cursor hotspot (game coords) to the display, then place the
	// layer so its local hotspot lands there.
	int cx;
	int cy;
	iwin->game_to_screen(mousex, mousey, false, cx, cy);
	const int dx = cx - static_cast<int>(hot_x * sx);
	const int dy = cy - static_cast<int>(hot_y * sy);
	gwin->layer_set_dest(mouse_layer, dx, dy, static_cast<int>(layer_w * sx), static_cast<int>(layer_h * sy));
}

/*
 *  Show the mouse.
 */

void Mouse::show(unsigned char* trans) {
	if (should_hide_frame(cur_framenum)) {
		hide();
		return;
	}
	if (!ensure_mouse_layer()) {
		return;    // No layer available: cursor not shown.
	}
	// (Re)draw the shape only when it (or its remap) changed.
	if (cur_framenum != last_layer_frame || trans != last_layer_trans) {
		draw_cursor_to_layer(trans);
	}
	position_mouse_layer();
	gwin->layer_set_visible(mouse_layer, true);
	onscreen = true;
}

/*
 *  Stop showing the cursor.
 */

void Mouse::hide() {
	if (mouse_layer >= 0) {
		gwin->layer_set_visible(mouse_layer, false);
	}
	if (onscreen) {
		onscreen = false;
		dirty    = box;    // Init. dirty to box.
	}
}

/*
 *  Move cursor
 */

int Mouse::fast_offset_x = 0;
int Mouse::fast_offset_y = 0;

// Apply the fastmouse offset to a position
// This is used by Image_window::screen_to_game

void Mouse::apply_fast_offset(int& gx, int& gy) {
	if (gwin->get_fastmouse()) {
		gx += fast_offset_x;
		gy += fast_offset_y;
	}
}

// Unapply the fastmouse offset to a position
// This is used by Image_window::game_to_screen

void Mouse::unapply_fast_offset(int& gx, int& gy) {
	if (gwin->get_fastmouse()) {
		gx -= fast_offset_x;
		gy -= fast_offset_y;
	}
}

void Mouse::move(int& x, int& y) {
	// COUT("Start mouse offset " << fast_offset_x << "," << fast_offset_y);
	// COUT("Start mouse coord " << x << "," << y);

	// Special handling for fast mouse to keep the pointer within the game
	// screen
	if (gwin->get_fastmouse()) {
		// Clip the mouse to be within the bounds of the actual game screen
		int wx = std::max(0, std::min(x, gwin->get_win()->get_end_x() - 1));
		int wy = std::max(0, std::min(y, gwin->get_win()->get_end_y() - 1));

		// Adjust offset if mouse position changes
		fast_offset_x += wx - x;
		fast_offset_y += wy - y;

		// Update the mouse position for the rest ofthe function
		x = wx;
		y = wy;
	}
	// Shift to new position.
	box.shift(x - mousex, y - mousey);
	dirty  = dirty.add(box);    // Enlarge dirty area.
	mousex = x;
	mousey = y;
	position_mouse_layer();    // Keep the cursor layer at the new spot.
}

/*
 *  Set to new shape.  Should be called after checking that frame #
 *  actually changed.
 */

void Mouse::set_shape0(int framenum) {
	cur_framenum = framenum;
	cur          = pointers.get_frame(framenum);
	while (!cur) {    // For newly-created games.
		cur = pointers.get_frame(--framenum);
	}
	// Set backup box to cover mouse.
	box.x = mousex - cur->get_xleft();
	box.y = mousey - cur->get_yabove();
	dirty = dirty.add(box);    // Update dirty area.
	if (should_hide_frame(cur_framenum)) {
		hide();
	}
}

/*
 *  Set to an arbitrary location.
 */

void Mouse::set_location(
		int x, int y    // Mouse position.
) {
	mousex = x;
	mousey = y;
	box.x  = mousex - cur->get_xleft();
	box.y  = mousey - cur->get_yabove();
}

/*
 *  Flash a desired shape for about 1/2 second.
 */

void Mouse::flash_shape(Mouse_shapes flash) {
	const Mouse_shapes saveshape = get_shape();
	hide();
	set_shape(flash);
	show();
	gwin->show(true);
	SDL_Delay(600);
	hide();
	gwin->paint();
	set_shape(saveshape);
	gwin->set_painted();
}

/*
 *  Set default cursor
 */

void Mouse::set_speed_cursor() {
	Game_window*  gwin     = Game_window::get_instance();
	Gump_manager* gump_man = gwin->get_gump_man();

	// Most pointer shapes are not directional steering arrows. The normal
	// movement branch below re-enables continuous rotation when appropriate.
	smooth_arrow_active = false;
	int cursor = dontchange;

	// Check if we are in dont_move mode, in this case display the hand cursor
	if (gwin->main_actor_dont_move()) {
		cursor = hand;
	}
	/* Can again, optionally move in gump mode. */
	else if (gump_man->gump_mode()) {
		if (gump_man->gumps_dont_pause_game()) {
			Gump* gump = gump_man->find_gump(mousex, mousey);

			if (gump && !gump->no_handcursor()) {
				cursor = hand;
			}
		} else {
			cursor = hand;
		}
	}

	else if (gwin->get_dragging_gump()) {
		cursor = hand;
	}

	else if (cheat.in_map_editor()) {
		switch (cheat.get_edit_mode()) {
		case Cheat::move:
			cursor = hand;
			break;
		case Cheat::paint:
			cursor = short_combat_arrows[4];
			break;    // Short S red arrow.
		case Cheat::paint_chunks:
			cursor = med_combat_arrows[0];
			break;    // Med. N red arrow.
		case Cheat::combo_pick:
		case Cheat::select_chunks:
		case Cheat::edit_pick:
			cursor = greenselect;
			break;    // Nice to have something else.
		}
	} else if (Combat::is_paused()) {
		cursor = short_combat_arrows[0];    // Short N red arrow.
	}
	if (cursor == dontchange) {
		int           ax;
		int           ay;    // Get Avatar/barge screen location.
		Barge_object* barge = gwin->get_moving_barge();
		if (barge) {
			// Use center of barge.
			gwin->get_shape_location(barge, ax, ay);
			ax -= barge->get_xtiles() * (c_tilesize / 2);
			ay -= barge->get_ytiles() * (c_tilesize / 2);
		} else {
			gwin->get_shape_location(gwin->get_main_actor(), ax, ay);
		}

		const int       dy  = ay - mousey;
		const int       dx  = mousex - ax;
		const Direction dir = Get_direction_NoWrap(dy, dx);

		// Keep the original 8-direction artwork (and therefore all three
		// short/medium/long arrow designs), but remember the exact angle. The
		// renderer rotates the selected frame only by the small residual from its
		// nearest 45-degree direction, preserving the original art while making
		// the pointer line up with mouse -> Avatar continuously.
		if (dx != 0 || dy != 0) {
			constexpr double pi = 3.14159265358979323846;
			const double exact_screen_angle = std::atan2(static_cast<double>(mousey - ay), static_cast<double>(mousex - ax));
			const double base_screen_angle = (-90.0 + 45.0 * static_cast<int>(dir)) * pi / 180.0;
			double residual = exact_screen_angle - base_screen_angle;
			while (residual > pi) {
				residual -= 2.0 * pi;
			}
			while (residual < -pi) {
				residual += 2.0 * pi;
			}
			smooth_arrow_residual_rad = residual;
			// 256 angular steps over a full turn (~1.4 degrees). Quantizing keeps
			// redraw/cache behavior stable while remaining visually continuous.
			const double exact_turn = exact_screen_angle / (2.0 * pi);
			int bucket = static_cast<int>(std::lround(exact_turn * 256.0));
			bucket %= 256;
			if (bucket < 0) {
				bucket += 256;
			}
			smooth_arrow_angle_bucket = bucket;
			smooth_arrow_active = true;
		}

		// Create a speed rectangle that's half of the game window dimensions
		// but with a minimum size of 200x200
		const TileRect game_rect = gwin->get_game_rect();
		const int rect_size = std::max(std::min(200, std::min(game_rect.w, game_rect.h)), std::min(game_rect.w, game_rect.h) / 2);
		const TileRect speed_rect(ax - rect_size / 2, ay - rect_size / 2, rect_size, rect_size);
		const bool     in_speed_rect
				= (mousex >= speed_rect.x && mousex < speed_rect.x + speed_rect.w && mousey >= speed_rect.y
				   && mousey < speed_rect.y + speed_rect.h);

		float speed_section = 1.0f;
		if (in_speed_rect) {
			// Calculate speed_section based on the dynamic rectangle size
			const int half_size = rect_size / 2;
			speed_section
					= max(max(-static_cast<float>(dx) / half_size, static_cast<float>(dx) / half_size),
						  max(static_cast<float>(dy) / half_size, -static_cast<float>(dy) / half_size));
		}

		const bool      nearby_hostile        = gwin->is_hostile_nearby() && !cheat.in_god_mode();
		bool            has_active_nohalt_scr = false;
		Usecode_script* scr                   = nullptr;
		Actor*          act                   = gwin->get_main_actor();
		while ((scr = Usecode_script::find_active(act, scr)) != nullptr) {
			// We should only be here if scripts are nohalt, but just
			// in case...
			if (scr->is_no_halt()) {
				has_active_nohalt_scr = true;
				break;
			}
		}

		const int base_speed = 200 * gwin->get_std_delay();
		if (!in_speed_rect) {
			// Beyond the 200x200 rectangle - use long arrow with fast movement
			// But respect combat/hostile conditions just like inside the
			// rectangle
			if (gwin->in_combat()) {
				cursor       = get_medium_combat_arrow(dir);    // No long combat arrows exist
				avatar_speed = base_speed / medium_combat_speed_factor;
			} else if (nearby_hostile || has_active_nohalt_scr) {
				cursor       = get_medium_arrow(dir);
				avatar_speed = base_speed / medium_speed_factor;
			} else {
				cursor       = get_long_arrow(dir);
				avatar_speed = base_speed / fast_speed_factor;
			}
		} else if (speed_section < 0.4f) {
			// Inside the rectangle with low speed_section - use short arrow
			if (gwin->in_combat()) {
				cursor = get_short_combat_arrow(dir);
			} else {
				cursor = get_short_arrow(dir);
			}
			avatar_speed = base_speed / slow_speed_factor;
		} else if (speed_section < 0.8f || gwin->in_combat() || nearby_hostile || has_active_nohalt_scr) {
			if (gwin->in_combat()) {
				cursor = get_medium_combat_arrow(dir);
			} else {
				cursor = get_medium_arrow(dir);
			}
			if (gwin->in_combat() || nearby_hostile) {
				avatar_speed = base_speed / medium_combat_speed_factor;
			} else {
				avatar_speed = base_speed / medium_speed_factor;
			}
		} else /* Fast - NB, we can't get here in combat mode; there is no
				* long combat arrow, nor is there a fast combat speed. */
		{
			cursor       = get_long_arrow(dir);
			avatar_speed = base_speed / fast_speed_factor;
		}
	}

	if (cursor != dontchange) {
		const bool angle_changed = smooth_arrow_active && smooth_arrow_angle_bucket != last_layer_angle_bucket;
		set_shape(cursor);
		if (angle_changed) {
			last_layer_frame = -1;
		}
	}
}
