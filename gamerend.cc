/*
 *  gamerend.cc - Rendering methods.
 *
 *  Copyright (C) 1998-1999  Jeffrey S. Freedman
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

#include "gamerend.h"

#include "Gump.h"
#include "Gump_manager.h"
#include "actors.h"
#include "cheat.h"
#include "chunks.h"
#include "drag.h"
#include "effects.h"
#include "font.h"
#include "gameclk.h"
#include "gamemap.h"
#include "gamewin.h"
#include "ignore_unused_variable_warning.h"
#include "objiter.h"
#include "perf.h"
#include "palette.h"

#ifdef USE_HQ3X_SCALER
#	include "scale_hq3x.h"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>

#ifdef USE_HQ3X_SCALER
namespace {
class Rotate_hq3x_rgb_manip {
	const Palette* palette;

public:
	explicit Rotate_hq3x_rgb_manip(const Palette* p) : palette(p) {}

	void split_source(unsigned char pix, unsigned int& r, unsigned int& g, unsigned int& b) const {
		// Palette channels are 6-bit in Exult. HQ3x expects an 8-bit-like RGB
		// range for its YUV edge tests and interpolation.
		r = static_cast<unsigned int>(palette->get_red(pix)) << 2;
		g = static_cast<unsigned int>(palette->get_green(pix)) << 2;
		b = static_cast<unsigned int>(palette->get_blue(pix)) << 2;
	}

	uint32 rgb(unsigned int r, unsigned int g, unsigned int b) const {
		return ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
	}
};
}    // namespace
#endif

/*
 *  Paint just the map with given top-left-corner tile.
 */

void Game_window::paint_map_at_tile(
		int x, int y, int w, int h,    // Clip to this area.
		int toptx, int topty,
		int skip_above    // Don't display above this lift.
) {
	const int savescrolltx = scrolltx;
	const int savescrollty = scrollty;
	const int saveskip     = skip_lift;
	scrolltx               = toptx;
	scrollty               = topty;
	skip_lift              = skip_above;
	map->read_map_data();    // Gather in all objs., etc.
	win->set_clip(x, y, w, h);
	render->paint_map(0, 0, get_width(), get_height());
	win->clear_clip();
	scrolltx  = savescrolltx;
	scrollty  = savescrollty;
	skip_lift = saveskip;
}

/*
 *  Figure offsets on screen.
 */

inline int Figure_screen_offset(
		int ch,       // Chunk #
		int scroll    // Top/left tile of screen.
) {
	// Watch for wrapping.
	int t = ch * c_tiles_per_chunk - scroll;
	if (t < -c_num_tiles / 2) {
		t += c_num_tiles;
	}
	t %= c_num_tiles;
	return t * c_tilesize;
}

/*
 *  Show the outline around a chunk.
 */

inline void Paint_chunk_outline(
		Game_window* gwin,
		int          pixel,    // Pixel value to use.
		int cx, int cy,        // Chunk coords.
		int tnum,              // Terrain #.
		int xoff, int yoff     // Where chunk was painted.
) {
	gwin->get_win()->fill8(pixel, c_chunksize, 1, xoff, yoff);
	gwin->get_win()->fill8(pixel, 1, c_chunksize, xoff, yoff);
	char text[40];    // Show chunk #.
	snprintf(text, sizeof(text), "(%d,%d)T%d", cx, cy, tnum);
	Shape_manager::get_instance()->paint_text(2, text, xoff + 2, yoff + 2);
}

/*
 *  Paint tile grid.
 */

static void Paint_grid(
		Game_window*   gwin,
		Xform_palette& xform    // For transparency.
) {
	Image_window8* win = gwin->get_win();
	// Paint grid at edit height.
	const int xtiles     = gwin->get_width() / c_tilesize;
	const int ytiles     = gwin->get_height() / c_tilesize;
	const int lift       = cheat.get_edit_lift();
	const int liftpixels = lift * (c_tilesize / 2) + 1;
	for (int y = 0; y < ytiles; y++) {
		win->fill_translucent8(0, xtiles * c_tilesize, 1, -liftpixels, y * c_tilesize - liftpixels, xform);
	}
	for (int x = 0; x < xtiles; x++) {
		win->fill_translucent8(0, 1, ytiles * c_tilesize, x * c_tilesize - liftpixels, -liftpixels, xform);
	}
}

/*
 *  Highlight selected chunks.
 */

static void Paint_selected_chunks(
		Game_window*   gwin,
		Xform_palette& xform,    // For transparency.
		int start_chunkx, int start_chunky, int stop_chunkx, int stop_chunky) {
	Game_map*      map = gwin->get_map();
	Image_window8* win = gwin->get_win();
	int            cx;
	int            cy;    // Chunk #'s.
	// Paint all the flat scenery.
	for (cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
		const int yoff = Figure_screen_offset(cy, gwin->get_scrollty()) - gwin->get_scrollty_lo();
		for (cx = start_chunkx; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
			Map_chunk* chunk = map->get_chunk(cx, cy);
			if (!chunk->is_selected()) {
				continue;
			}
			const int xoff = Figure_screen_offset(cx, gwin->get_scrolltx()) - gwin->get_scrolltx_lo();
			win->fill_translucent8(0, c_chunksize, c_chunksize, xoff, yoff, xform);
		}
	}
}

/*
 *  Just paint terrain.  This is for terrain_editing mode.
 */

void Game_render::paint_terrain_only(int start_chunkx, int start_chunky, int stop_chunkx, int stop_chunky) {
	Game_window*   gwin = Game_window::get_instance();
	Game_map*      map  = gwin->map;
	Shape_manager* sman = Shape_manager::get_instance();
	int            cx;
	int            cy;    // Chunk #'s.
	// Paint all the flat scenery.
	for (int pass = 1; pass <= 3; pass++) {
		for (cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
			const int yoff = Figure_screen_offset(cy, gwin->scrollty) - gwin->get_scrollty_lo();
			for (cx = start_chunkx; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
				const int xoff = Figure_screen_offset(cx, gwin->scrolltx) - gwin->get_scrolltx_lo();
				if (pass < 3) {
					Map_chunk* chunk = map->get_chunk(cx, cy);
					chunk->get_terrain()->render_all(cx, cy, pass);
				}
				if (cheat.in_map_editor() && pass == 3) {
					Paint_chunk_outline(gwin, sman->get_special_pixel(HIT_PIXEL), cx, cy, map->get_terrain_num(cx, cy), xoff, yoff);
				}
			}
		}
	}
	// Paint tile grid if desired.
	if (cheat.show_tile_grid()) {
		Paint_grid(gwin, sman->get_xform(16));
	}
}

/*
 *  Paint just the map and its objects (no gumps, effects).
 *  (The caller should set/clear clip area.)
 *
 *  Output: # light-sources found.
 */

int Game_render::paint_map(
		int x, int y, int w, int h    // Rectangle to cover.
) {
	Game_window*   gwin = Game_window::get_instance();
	Game_map*      map  = gwin->map;
	Shape_manager* sman = gwin->shape_man;
	render_seq++;    // Increment sequence #.
	gwin->painted = true;

	const int scrolltx      = gwin->scrolltx;
	const int scrollty      = gwin->scrollty;
	int       light_sources = 0;    // Count light sources found.
	// Get chunks to start with, starting
	//   1 tile left/above.
	int start_chunkx = (scrolltx + x / c_tilesize - 1) / c_tiles_per_chunk;
	// Wrap around.
	start_chunkx     = (start_chunkx + c_num_chunks) % c_num_chunks;
	int start_chunky = (scrollty + y / c_tilesize - 1) / c_tiles_per_chunk;
	start_chunky     = (start_chunky + c_num_chunks) % c_num_chunks;
	// End 8 tiles to right.
	// The chunk limits were increased by 1 to support the Smooth Scrolling.
	// The same increase had to be added into the Game_map::read_map_data
	//   which builds the chunk cache that the Edit Terrain mode relies on.
	int stop_chunkx = 2 + (scrolltx + (x + w + c_tilesize - 2) / c_tilesize + c_tiles_per_chunk / 2) / c_tiles_per_chunk;
	int stop_chunky = 2 + (scrollty + (y + h + c_tilesize - 2) / c_tilesize + c_tiles_per_chunk / 2) / c_tiles_per_chunk;
	// Wrap around the world:
	stop_chunkx = (stop_chunkx + c_num_chunks) % c_num_chunks;
	stop_chunky = (stop_chunky + c_num_chunks) % c_num_chunks;
	if (!gwin->skip_lift) {    // Special mode for editing?
		paint_terrain_only(start_chunkx, start_chunky, stop_chunkx, stop_chunky);
		return 10;    // Pretend there's lots of light!
	}
	int cx;
	int cy;    // Chunk #'s.
	// Paint all the flat scenery.
	for (cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
		const int yoff = Figure_screen_offset(cy, scrollty) - gwin->get_scrollty_lo();
		for (cx = start_chunkx; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
			const int xoff = Figure_screen_offset(cx, scrolltx) - gwin->get_scrolltx_lo();
			paint_chunk_flats(cx, cy, xoff, yoff);
		}
	}
	// Now the flat RLE terrain.
	for (cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
		const int yoff = Figure_screen_offset(cy, scrollty) - gwin->get_scrollty_lo();
		for (cx = start_chunkx; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
			const int xoff = Figure_screen_offset(cx, scrolltx) - gwin->get_scrolltx_lo();
			paint_chunk_flat_rles(cx, cy, xoff, yoff);
		}
	}
	// Draw the chunk grid in Map editor cheat mode.
	if (cheat.in_map_editor()) {
		for (cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
			const int yoff = Figure_screen_offset(cy, scrollty) - gwin->get_scrollty_lo();
			for (cx = start_chunkx; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
				const int xoff = Figure_screen_offset(cx, scrolltx) - gwin->get_scrolltx_lo();
				Paint_chunk_outline(gwin, sman->get_special_pixel(HIT_PIXEL), cx, cy, map->get_terrain_num(cx, cy), xoff, yoff);
			}
		}
	}
	// Draw the chunks' objects
	//   diagonally NE.
	const int tmp_stopy = DECR_CHUNK(start_chunky);
	for (cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
		for (int dx = start_chunkx, dy = cy; dx != stop_chunkx && dy != tmp_stopy; dx = INCR_CHUNK(dx), dy = DECR_CHUNK(dy)) {
			light_sources += paint_chunk_objects(dx, dy);
		}
	}
	for (cx = (start_chunkx + 1) % c_num_chunks; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
		for (int dx = cx, dy = (stop_chunky - 1 + c_num_chunks) % c_num_chunks; dx != stop_chunkx && dy != tmp_stopy;
			 dx = INCR_CHUNK(dx), dy = DECR_CHUNK(dy)) {
			light_sources += paint_chunk_objects(dx, dy);
		}
	}
	/// Dungeon Blackness (but disable in map editor mode)
	if (static_cast<int>(gwin->in_dungeon) >= gwin->skip_above_actor && !cheat.in_map_editor()) {
		paint_blackness(start_chunkx, start_chunky, stop_chunkx, stop_chunky, gwin->ice_dungeon ? 73 : 0);
	}

	// Outline selected objects.
	const Game_object_shared_vector& sel         = cheat.get_selected();
	const int                        render_skip = gwin->get_render_skip_lift();
	for (const auto& it : sel) {
		Game_object* obj = it.get();
		if (!obj->get_owner() && obj->get_lift() < render_skip) {
			obj->paint_outline(HIT_PIXEL);
		}
	}

	// Paint tile grid if desired.
	if (cheat.in_map_editor()) {
		if (cheat.show_tile_grid()) {
			Paint_grid(gwin, sman->get_xform(16));
		}
		if (cheat.get_edit_mode() == Cheat::select_chunks) {
			Paint_selected_chunks(gwin, sman->get_xform(13), start_chunkx, start_chunky, stop_chunkx, stop_chunky);
		}
	}
	return light_sources;
}

static int Get_light_strength(const Game_object* obj, const Game_object* av, int brightness) {
	// Note: originals do not seem to use center tile.
	const Tile_coord t1 = obj->get_center_tile();
	const Tile_coord t2 = av->get_center_tile();
	// Note: originals do not care about distance in Z. Maybe we should?
	const int dx = std::abs(Tile_coord::delta(t1.tx, t2.tx));
	const int dy = std::abs(Tile_coord::delta(t1.ty, t2.ty));
	// This seems to match the originals as far as distance effects go.
	const int dist_decay_factor = std::max(0, 75 - 2 * dx - 3 * dy);
	// Finally, return how bright this light is.
	return dist_decay_factor * brightness;
}

int Game_render::get_light_strength(const Game_object* obj, const Game_object* av) const {
	const Shape_info& info = obj->get_info();
	return Get_light_strength(obj, av, info.get_object_light(obj->get_framenum()));
}

void Game_render::increment_bbox_index() {
	int  bbox_indices[] = {15, 0, 22, 38, 5, 64, 80, 94, -1};
	auto start          = bbox_indices;
	auto end            = bbox_indices + std::size(bbox_indices);

	size_t found = std::find(start, end, bbox_palindex) - start;

	if (found < std::size(bbox_indices)) {
		bbox_palindex = bbox_indices[(found + 1) % std::size(bbox_indices)];
	}
	Game_window::get_instance()->set_all_dirty();
}

/*
 *  Paint a rectangle in the window by pulling in vga chunks.
 */

void Game_window::paint(
		int x, int y, int w, int h    // Rectangle to cover.
) {
	if (rotate_world) {
		paint_rotated(x, y, w, h);
		return;
	}
	auto perftimer = PerformanceTimer::GetScopedPerfTimer(__func__);

	if (!win->ready()) {
		return;
	}
	// This will adjust and clip the rectangle as appropriate, it may end up
	// bigger or smaller
	win->BeginPaintIntoGuardBand(&x, &y, &w, &h);
	int gx = x;
	int gy = y;
	int gw = w;
	int gh = h;
	if (gx < 0) {
		gw += x;
		gx = 0;
	}
	if ((gx + gw) > get_width()) {
		gw = get_width() - gx;
	}
	if (gy < 0) {
		gh += gy;
		gy = 0;
	}
	if ((gy + gh) > get_height()) {
		gh = get_height() - gy;
	}
	win->set_clip(gx, gy, gw, gh);    // Clip to this area.

	int light_sources = 0;

	if (main_actor) {
		light_sources = render->paint_map(gx, gy, gw, gh);
	} else {
		win->fill8(0);
	}

	effects->paint();    // Draw sprites.

	win->set_clip(x, y, w, h);    // Clip to this area.
	// Fill black into unpainted regions
	if (y < 0) {
		win->fill8(pal->get_border_index(), w, -y, x, y);    // Region above window
	}
	if (x < 0) {
		win->fill8(pal->get_border_index(), -x, get_height(), x,
				   0);    // Region left of window
	}
	if ((x + w) > get_width()) {
		win->fill8(pal->get_border_index(), (x + w) - get_width(), get_height(), get_width(), 0);    // Region right of window
	}
	if ((y + h) > get_height()) {
		win->fill8(pal->get_border_index(), w, (y + h) - get_height(), x,
				   get_height());    // below window
	}

	gump_man->paint(false);
	if (dragging) {
		dragging->paint();    // Paint what user is dragging.
	}
	effects->paint_text();
	gump_man->paint(true);

	// Complete repaint?
	if (!gx && !gy && gw == get_width() && gh == get_height() && main_actor) {
		update_lighting(light_sources);
	}

	win->EndPaintIntoGuardBand();
	win->clear_clip();
}

/*
 *  Paint the world into an expanded logical buffer, rotate it into the
 *  normal game buffer, then paint UI layers on top without rotating them.
 */
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

		static bool printed_quad_stats = false;
		if (!printed_quad_stats) {
			std::cout << "Forward quadruplets ALGO=WEIGHTED-QUADRUPLETS-V1"
					 << ", nodes=" << node_contributions
					 << ", quadruplets=" << quadruplets
					 << ", segment_cells=" << segment_cells
					 << ", cohesion=[high:" << high_cohesion
					 << " medium:" << medium_cohesion
					 << " low:" << low_cohesion << "]"
					 << ", avg_confidence="
					 << (quadruplets > 0 ? confidence_sum / quadruplets : 0.0)
					 << ", holes=" << holes << std::endl;
			printed_quad_stats = true;
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

		static bool printed_pair_stats = false;
		if (!printed_pair_stats) {
			std::cout << "Forward pairs ALGO=WEIGHTED-PAIRS-V1"
					 << ", nodes=" << node_contributions
					 << ", pairs=" << pairs
					 << ", segment_cells=" << segment_cells
					 << ", cohesion=[high:" << high_cohesion
					 << " medium:" << medium_cohesion
					 << " low:" << low_cohesion << "]"
					 << ", avg_confidence="
					 << (pairs > 0 ? confidence_sum / pairs : 0.0)
					 << ", holes=" << holes << std::endl;
			printed_pair_stats = true;
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

		static bool printed_triplet_stats = false;
		if (!printed_triplet_stats) {
			std::cout << "Forward triplets ALGO=WEIGHTED-TRIPLETS-V1"
					 << ", nodes=" << node_contributions
					 << ", triplets=" << triplets
					 << ", segment_cells=" << segment_cells
					 << ", cohesion=[high:" << high_cohesion
					 << " medium:" << medium_cohesion
					 << " low:" << low_cohesion << "]"
					 << ", avg_confidence="
					 << (triplets > 0 ? confidence_sum / triplets : 0.0)
					 << ", holes=" << holes << std::endl;
			printed_triplet_stats = true;
		}

	} else {
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

void Game_window::update_lighting(int light_sources) {
	if (!main_actor) {
		return;
	}
	Actor* party[9];
	const int cnt = get_party(party, 1);
	int carried_light = 0;
	for (int i = 0; i < cnt; ++i) {
		carried_light += Get_light_strength(party[i], main_actor, party[i]->get_light_source());
	}
	if (special_light && clock->get_total_minutes() > special_light) {
		special_light = 0;
		clock->set_palette();
	}
	clock->set_light_source(carried_light + light_sources, in_dungeon);
}

/*
 *  Paint whole window.
 */
void Game_window::paint() {
	trace_target_camera("paint-raw");
	if (main_actor != nullptr) {
		map->read_map_data();    // Gather in all objs., etc.
	}
	set_all_dirty();
	paint_dirty();
}

void Game_window::lerp_reset() {
	scrolltx_lp = scrolltx_l;
	scrollty_lp = scrollty_l;
	scrolltx_l  = scrolltx;
	scrollty_l  = scrollty;

	if (camera_actor) {
		const Tile_coord t = camera_actor->get_tile();
		if (!lerp_actor_valid) {
			avtx_l = avtx_lp = t.tx;
			avty_l = avty_lp = t.ty;
			lerp_actor_valid = true;
		} else {
			avtx_lp = avtx_l;
			avty_lp = avty_l;
			avtx_l  = t.tx;
			avty_l  = t.ty;
		}
	}
}

void Game_window::paint_lerped(int factor) {
	if (factor < 0) {
		factor = 0;
	}
	if (factor > 0x10000) {
		factor = 0x10000;
	}

	const int saved_scrolltx = scrolltx;
	const int saved_scrollty = scrollty;

	// Actor motion and camera motion intentionally use different curves.
	// The actor follows a linear interpolation between discrete logical tile
	// updates; the camera follows a slower cubic ease-in and catches up at the
	// end of the segment. This breaks the rigid "avatar glued to screen center"
	// constraint while keeping both endpoints exact.
	const int actor_factor = factor;
	const int64_t f = static_cast<int64_t>(factor);
	const int camera_factor = static_cast<int>((f * f * f) / (static_cast<int64_t>(0x10000) * 0x10000));

	scrolltx = scrolltx_l;
	scrollty = scrollty_l;

	int dx = (scrolltx_lp - scrolltx);
	int dy = (scrollty_lp - scrollty);

	// wrap around fixing...
	while (dx < -c_num_tiles / 2) {
		dx += c_num_tiles;
	}
	while (dx > c_num_tiles / 2) {
		dx -= c_num_tiles;
	}
	while (dy < -c_num_tiles / 2) {
		dy += c_num_tiles;
	}
	while (dy > c_num_tiles / 2) {
		dy -= c_num_tiles;
	}

	if (dx > -4 && dx < 4 && dy > -4 && dy < 4) {
		dx *= c_tilesize;
		dy *= c_tilesize;
		scrolltx *= c_tilesize;
		scrollty *= c_tilesize;

		scrolltx = scrolltx + (dx * (0x10000 - camera_factor)) / 0x10000;
		scrollty = scrollty + (dy * (0x10000 - camera_factor)) / 0x10000;

		dx = scrolltx % c_tilesize;
		dy = scrollty % c_tilesize;

		scrolltx = ((scrolltx / c_tilesize) + c_num_tiles) % c_num_tiles;
		scrollty = ((scrollty / c_tilesize) + c_num_tiles) % c_num_tiles;
	} else {
		dx = 0;
		dy = 0;
	}

	// Interpolate the camera actor independently from the visual camera.
	avposx_ld = 0;
	avposy_ld = 0;
	if (lerp_actor_valid) {
		int adx = avtx_lp - avtx_l;
		int ady = avty_lp - avty_l;
		while (adx < -c_num_tiles / 2) adx += c_num_tiles;
		while (adx > c_num_tiles / 2) adx -= c_num_tiles;
		while (ady < -c_num_tiles / 2) ady += c_num_tiles;
		while (ady > c_num_tiles / 2) ady -= c_num_tiles;
		if (adx > -4 && adx < 4 && ady > -4 && ady < 4) {
			avposx_ld = (adx * c_tilesize * (0x10000 - actor_factor)) / 0x10000;
			avposy_ld = (ady * c_tilesize * (0x10000 - actor_factor)) / 0x10000;
		}
	}

	// Set pixel offset needed for camera interpolation.
	scrolltx_lo = dx;
	scrollty_lo = dy;

	paint();

	scrolltx    = saved_scrolltx;
	scrollty    = saved_scrollty;
	scrolltx_lo = scrollty_lo = 0;
	avposx_ld = avposy_ld = 0;
}

void Game_window::begin_target_camera_trace() {
	camera_target_trace_frames = 90;
	trace_target_camera("target-begin");
}

void Game_window::trace_target_camera(const char* event) {
	if (camera_target_trace_frames <= 0) {
		return;
	}
	std::fprintf(stderr,
			"[CAM-TARGET] %s logical=(%d,%d)+(%d,%d) visual=(%.2f,%.2f) stage1=(%.2f,%.2f) stage2=(%.2f,%.2f) valid=%d modern=%d frames=%d\n",
			event, scrolltx, scrollty, scrolltx_lo, scrollty_lo,
			smooth_cam_x, smooth_cam_y, smooth_cam_stage1_x, smooth_cam_stage1_y,
			smooth_cam_stage2_x, smooth_cam_stage2_y,
			static_cast<int>(smooth_cam_valid), static_cast<int>(modern_movement_enabled),
			camera_target_trace_frames);
	std::fflush(stderr);
}

void Game_window::paint_current_view() {
	trace_target_camera("paint-current");
	if (!modern_movement_enabled || !smooth_cam_valid) {
		paint();
		return;
	}

	const int world_pixels = c_num_tiles * c_tilesize;
	const auto wrap_pixel = [&](int p) {
		p %= world_pixels;
		if (p < 0) {
			p += world_pixels;
		}
		return p;
	};

	const int saved_scrolltx = scrolltx;
	const int saved_scrollty = scrollty;
	const int saved_scrolltx_lo = scrolltx_lo;
	const int saved_scrollty_lo = scrollty_lo;
	const int saved_avposx_ld = avposx_ld;
	const int saved_avposy_ld = avposy_ld;

	const int render_x = static_cast<int>(std::lround(smooth_cam_x));
	const int render_y = static_cast<int>(std::lround(smooth_cam_y));
	const int wrapped_x = wrap_pixel(render_x);
	const int wrapped_y = wrap_pixel(render_y);
	scrolltx = wrapped_x / c_tilesize;
	scrollty = wrapped_y / c_tilesize;
	scrolltx_lo = wrapped_x % c_tilesize;
	scrollty_lo = wrapped_y % c_tilesize;
	avposx_ld = 0;
	avposy_ld = 0;

	paint();

	scrolltx = saved_scrolltx;
	scrollty = saved_scrollty;
	scrolltx_lo = saved_scrolltx_lo;
	scrollty_lo = saved_scrollty_lo;
	avposx_ld = saved_avposx_ld;
	avposy_ld = saved_avposy_ld;
}

void Game_window::reset_velocity_camera() {
	trace_target_camera("camera-reset");
	smooth_cam_stage1_x = 0.0;
	smooth_cam_stage1_y = 0.0;
	smooth_cam_stage2_x = 0.0;
	smooth_cam_stage2_y = 0.0;
	smooth_cam_x = 0.0;
	smooth_cam_y = 0.0;
	smooth_cam_last_ticks = 0;
	smooth_cam_valid = false;
}

bool Game_window::paint_velocity_camera(uint32 ticks) {
	if (camera_target_trace_frames > 0) {
		trace_target_camera("velocity-frame");
		--camera_target_trace_frames;
	}
	if (!camera_actor) {
		reset_velocity_camera();
		return false;
	}

	const int world_pixels = c_num_tiles * c_tilesize;
	const double half_world = static_cast<double>(world_pixels) * 0.5;

	// The logical target still moves in discrete tile-sized steps. Feed it
	// through three identical first-order low-pass filters in cascade:
	//
	//   target -> stage 1 -> stage 2 -> rendered camera
	//
	// Each stage is a convex combination of its input and previous value, so a
	// stationary step target is approached monotonically and can never be
	// overshot. In the continuous-time equivalent H(s)=1/(1+tau*s)^3, camera
	// position, velocity and acceleration are continuous even when the input
	// target itself jumps.
	const Tile_coord actor_pos = camera_actor->get_tile();
	const int tw = get_width() / c_tilesize;
	const int th = get_height() / c_tilesize;
	double target_x = static_cast<double>((actor_pos.tx - tw / 2) * c_tilesize);
	double target_y = static_cast<double>((actor_pos.ty - th / 2) * c_tilesize);

	if (!smooth_cam_valid) {
		// Start exactly at the currently rendered logical camera. Initializing all
		// three stages to the same point avoids any startup transient.
		const double initial_x = static_cast<double>(scrolltx * c_tilesize + scrolltx_lo);
		const double initial_y = static_cast<double>(scrollty * c_tilesize + scrollty_lo);
		smooth_cam_stage1_x = initial_x;
		smooth_cam_stage1_y = initial_y;
		smooth_cam_stage2_x = initial_x;
		smooth_cam_stage2_y = initial_y;
		smooth_cam_x = initial_x;
		smooth_cam_y = initial_y;
		smooth_cam_last_ticks = ticks;
		smooth_cam_valid = true;
		return false;
	}

	// Choose the wrapped target image nearest to the current unwrapped camera.
	// Use the rendered stage as the reference so all three stages remain on one
	// continuous unwrapped world image while crossing the map seam.
	const auto nearest_wrapped = [&](double target, double current) {
		double delta = target - current;
		while (delta > half_world) {
			delta -= world_pixels;
		}
		while (delta < -half_world) {
			delta += world_pixels;
		}
		return current + delta;
	};
	target_x = nearest_wrapped(target_x, smooth_cam_x);
	target_y = nearest_wrapped(target_y, smooth_cam_y);

	uint32 elapsed_ms = ticks - smooth_cam_last_ticks;
	smooth_cam_last_ticks = ticks;
	if (elapsed_ms > 50) {
		elapsed_ms = 50;
	}
	const double dt = static_cast<double>(elapsed_ms) / 1000.0;
	if (dt <= 0.0) {
		return false;
	}

	// Map/teleport discontinuities should not spend seconds flowing across the
	// world. Normal walking produces only tile-sized target steps, nowhere near
	// this threshold, so snap all stages together only for genuine jumps.
	const double error_x = target_x - smooth_cam_x;
	const double error_y = target_y - smooth_cam_y;
	if (std::abs(error_x) > 128.0 || std::abs(error_y) > 128.0) {
		smooth_cam_stage1_x = target_x;
		smooth_cam_stage1_y = target_y;
		smooth_cam_stage2_x = target_x;
		smooth_cam_stage2_y = target_y;
		smooth_cam_x = target_x;
		smooth_cam_y = target_y;
	}

	const int old_render_x = static_cast<int>(std::lround(smooth_cam_x));
	const int old_render_y = static_cast<int>(std::lround(smooth_cam_y));

	// Time constant of each of the three cascaded stages. The effective group
	// delay while following steady motion is roughly 3*tau (165 ms here).
	// alpha is the exact frame-rate-independent update of one first-order stage.
	const double tau = static_cast<double>(modern_movement_tau_ms) / 1000.0;
	const double alpha = 1.0 - std::exp(-dt / tau);

	// IMPORTANT: update all stages from the *previous* frame's stage values.
	// Doing the assignments sequentially with freshly updated inputs would
	// partially collapse the cascade and lose the intended third-order
	// smoothness.
	const double old_stage1_x = smooth_cam_stage1_x;
	const double old_stage1_y = smooth_cam_stage1_y;
	const double old_stage2_x = smooth_cam_stage2_x;
	const double old_stage2_y = smooth_cam_stage2_y;
	const double old_cam_x = smooth_cam_x;
	const double old_cam_y = smooth_cam_y;

	smooth_cam_stage1_x = old_stage1_x + alpha * (target_x - old_stage1_x);
	smooth_cam_stage1_y = old_stage1_y + alpha * (target_y - old_stage1_y);
	smooth_cam_stage2_x = old_stage2_x + alpha * (old_stage1_x - old_stage2_x);
	smooth_cam_stage2_y = old_stage2_y + alpha * (old_stage1_y - old_stage2_y);
	smooth_cam_x = old_cam_x + alpha * (old_stage2_x - old_cam_x);
	smooth_cam_y = old_cam_y + alpha * (old_stage2_y - old_cam_y);

	const int render_x = static_cast<int>(std::lround(smooth_cam_x));
	const int render_y = static_cast<int>(std::lround(smooth_cam_y));
	const bool camera_changed = render_x != old_render_x || render_y != old_render_y;
	if (!camera_changed && !is_dirty()) {
		return false;
	}

	const int saved_scrolltx = scrolltx;
	const int saved_scrollty = scrollty;

	const auto wrap_pixel = [&](int p) {
		p %= world_pixels;
		if (p < 0) {
			p += world_pixels;
		}
		return p;
	};
	const int wrapped_x = wrap_pixel(render_x);
	const int wrapped_y = wrap_pixel(render_y);
	scrolltx = wrapped_x / c_tilesize;
	scrollty = wrapped_y / c_tilesize;
	scrolltx_lo = wrapped_x % c_tilesize;
	scrollty_lo = wrapped_y % c_tilesize;

	// The Avatar is deliberately not compensated back to screen centre: the
	// camera is a true lagging follower, not an interpolation offset glued to
	// the actor.
	avposx_ld = 0;
	avposy_ld = 0;

	paint();

	scrolltx = saved_scrolltx;
	scrollty = saved_scrollty;
	scrolltx_lo = 0;
	scrollty_lo = 0;
	avposx_ld = 0;
	avposy_ld = 0;
	return true;
}

/*
 *  Paint the flat (non-rle) shapes in a chunk.
 */

void Game_render::paint_chunk_flats(
		int cx, int cy,       // Chunk coords (0 - 12*16).
		int xoff, int yoff    // Pixel offset of top-of-screen.
) {
	Game_window* gwin  = Game_window::get_instance();
	Map_chunk*   olist = gwin->map->get_chunk(cx, cy);
	// Paint flat tiles.
	Image_buffer8* cflats = olist->get_rendered_flats();
	if (cflats) {
		gwin->win->copy8(cflats->get_bits(), c_chunksize, c_chunksize, xoff, yoff);
	}
}

/*
 *  Paint the flat RLE (terrain) shapes in a chunk.
 */

void Game_render::paint_chunk_flat_rles(
		int cx, int cy,       // Chunk coords (0 - 12*16).
		int xoff, int yoff    // Pixel offset of top-of-screen.
) {
	ignore_unused_variable_warning(xoff, yoff);
	Game_window*         gwin  = Game_window::get_instance();
	Map_chunk*           olist = gwin->map->get_chunk(cx, cy);
	Flat_object_iterator next(olist);    // Do flat RLE objects.
	Game_object*         obj;
	while ((obj = next.get_next()) != nullptr) {
		obj->paint();
	}
}

/*
 *  Paint a chunk's objects, left-to-right, top-to-bottom.
 *
 *  Output: # light sources found.
 */

int Game_render::paint_chunk_objects(
		int cx, int cy    // Chunk coords (0 - 12*16).
) {
	Game_object*      obj;
	Game_window*      gwin          = Game_window::get_instance();
	Map_chunk*        olist         = gwin->map->get_chunk(cx, cy);
	int               light_sources = 0;    // Also check for light sources.
	Main_actor* const main_actor    = gwin->get_main_actor();
	if (main_actor != nullptr) {
		const auto& lights = gwin->is_in_dungeon() ? olist->get_dungeon_lights() : olist->get_non_dungeon_lights();
		for (const auto& light_obj : lights) {
			const Shape_info& info = light_obj->get_info();
			if (info.get_object_light(light_obj->get_framenum()) > 0) {
				// Count light sources.
				light_sources += get_light_strength(light_obj, main_actor);
			}
		}
	}
	skip = gwin->get_render_skip_lift();
	Nonflat_object_iterator next(olist);

	while ((obj = next.get_next()) != nullptr) {
		if (obj->render_seq != render_seq) {
			paint_object(obj);
		}
	}

	skip = 255;    // Back to a safe #.
	return light_sources;
}

/*
 *  Render an object after first rendering any that it depends on.
 */

void Game_render::paint_object(Game_object* obj) {
	const int lift = obj->get_lift();
	if (lift >= skip) {
		return;
	}
	obj->render_seq                          = render_seq;
	const Game_object::Game_object_set& deps = obj->get_dependencies();
	for (auto* dep : deps) {
		if (dep && dep->render_seq != render_seq) {
			paint_object(dep);
		}
	}
	int bbox_x, bbox_y;
	Game_window::get_instance()->get_shape_location(obj, bbox_x, bbox_y);

	// paint bbox back
	if (bbox_palindex != -1) {
		obj->get_info().paint_bbox(
				bbox_x, bbox_y, obj->get_framenum(), Game_window::get_instance()->get_win()->get_ib8(), bbox_palindex, 2);
	}
	obj->paint();    // Finally, paint this one.
	// paint bbox front
	if (bbox_palindex != -1) {
		obj->get_info().paint_bbox(
				bbox_x, bbox_y, obj->get_framenum(), Game_window::get_instance()->get_win()->get_ib8(), bbox_palindex, 1);
	}
}

/*
 *  Paint 'dirty' rectangle.
 */

void Game_window::paint_dirty() {
	effects->update_dirty_text();

	TileRect box = clip_to_win(dirty);
	if (box.w > 0 && box.h > 0) {
		paint(box);    // (Could create new dirty rects.)
	}
	clear_dirty();
}

/*
 *  Dungeon Blacking
 *
 *  This is really simple. If there is a dungeon roof over our head we
 *  black out every tile on screen that doens't have a roof at the height
 *  of the roof that is directly over our head. The tiles are blacked out
 *  at the height of the the roof.
 *
 *  I've done some simple optimizations. Generally all the blackness will
 *  cover entire chunks. So, instead of drawing each tile individually, I
 *  work out home many tiles in a row that need to be blacked out, and then
 *  black them all out at the same time.
 */

void Game_render::paint_blackness(int start_chunkx, int start_chunky, int stop_chunkx, int stop_chunky, int index) {
	Game_window* gwin = Game_window::get_instance();
	// Calculate the offset due to the lift (4x the lift).
	const int off = gwin->in_dungeon << 2;

	// For each chunk that might be renderable
	for (int cy = start_chunky; cy != stop_chunky; cy = INCR_CHUNK(cy)) {
		for (int cx = start_chunkx; cx != stop_chunkx; cx = INCR_CHUNK(cx)) {
			// Coord of the left edge
			const int xoff = Figure_screen_offset(cx, gwin->scrolltx) - off - gwin->get_scrolltx_lo();
			// Coord of the top edge
			int y = Figure_screen_offset(cy, gwin->scrollty) - off - gwin->get_scrollty_lo();

			// Need the chunk cache (needs to be setup!)
			Map_chunk* mc = gwin->map->get_chunk(cx, cy);
			if (!mc->has_dungeon()) {
				gwin->win->fill8(index, c_tilesize * c_tiles_per_chunk, c_tilesize * c_tiles_per_chunk, xoff, y);
				continue;
			}
			// For each line in the chunk
			for (int tiley = 0; tiley < c_tiles_per_chunk; tiley++) {
				// Start and width of the area to black out
				int x = xoff;
				int w = 0;

				// For each tile in the line
				for (int tilex = 0; tilex < c_tiles_per_chunk; tilex++) {
					// If the tile is blocked by 'roof'
					if (!mc->is_dungeon(tilex, tiley)) {
						// Add to the width of the area
						w += c_tilesize;
					}
					// If not blocked and have area,
					else if (w) {
						// Draw blackness
						gwin->win->fill8(index, w, c_tilesize, x, y);

						// Set the start of the area to the next tile
						x += w + c_tilesize;

						// Clear the width
						w = 0;
					}
					// Not blocked, and no area
					else {
						// Increment the start of the area to the next tile
						x += c_tilesize;
					}
				}

				// If we have an area, paint it.
				if (w) {
					gwin->win->fill8(index, w, c_tilesize, x, y);
				}

				// Increment the y coord for the next line
				y += c_tilesize;
			}
		}
	}
}
