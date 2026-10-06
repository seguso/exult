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

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>

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

	// Reconstruct the 8-bit scene at 2x with the Scale2x/EPX neighbourhood
	// rule before rotating it.  This is deliberately pixel-art-aware: diagonal
	// runs that are only corner-connected at 1x get extra coverage at 2x, while
	// interior colours (for example the lighter stripe inside a lamp post) stay
	// distinct instead of being swallowed by a generic dark-edge bias.
	rotate_scene_2x->clear_clip();
	rotate_scene_2x->fill8(pal->get_border_index());
	const auto scene_pixel = [&](int x, int y) {
		x = std::clamp(x, scene_x, scene_x + scene_size - 1);
		y = std::clamp(y, scene_y, scene_y + scene_size - 1);
		return rotate_scene->get_pixel8(x, y);
	};
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

	// Rotate from the reconstructed 2x scene and area-sample each destination
	// pixel at four quarter-pixel positions.  Unlike the previous dark-edge
	// resolver this treats light and dark detail symmetrically.
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
	const auto sample_scene_2x = [&](const World_view_point& source) {
		const double fx = (source.x - static_cast<double>(scene_x)) * 2.0 + 0.5;
		const double fy = (source.y - static_cast<double>(scene_y)) * 2.0 + 0.5;
		const int sx = static_cast<int>(std::floor(fx));
		const int sy = static_cast<int>(std::floor(fy));
		const int hi_size = scene_size * 2;
		if (sx < 0 || sx >= hi_size || sy < 0 || sy >= hi_size) {
			return static_cast<unsigned char>(pal->get_border_index());
		}
		return rotate_scene_2x->get_pixel8(sx, sy);
	};
	const auto blend4 = [&](unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
		if (a == b && a == c && a == d) {
			return a;
		}
		const int r = (pal->get_red(a) + pal->get_red(b) + pal->get_red(c) + pal->get_red(d) + 2) / 4;
		const int g = (pal->get_green(a) + pal->get_green(b) + pal->get_green(c) + pal->get_green(d) + 2) / 4;
		const int blue = (pal->get_blue(a) + pal->get_blue(b) + pal->get_blue(c) + pal->get_blue(d) + 2) / 4;
		return quantize_rgb(r, g, blue);
	};

	const double source_dx = world_view.display_to_scene_x_step();
	const double source_dy = world_view.display_to_scene_y_step();
	for (int dy = 0; dy < display_height; ++dy) {
		const double y0 = static_cast<double>(dy) + 0.25;
		const double y1 = static_cast<double>(dy) + 0.75;
		World_view_point source00 = world_view.display_to_scene({0.25, y0});
		World_view_point source10 = world_view.display_to_scene({0.75, y0});
		World_view_point source01 = world_view.display_to_scene({0.25, y1});
		World_view_point source11 = world_view.display_to_scene({0.75, y1});
		for (int dx = 0; dx < display_width; ++dx) {
			win->put_pixel8(
					blend4(
							sample_scene_2x(source00), sample_scene_2x(source10),
							sample_scene_2x(source01), sample_scene_2x(source11)),
					dx, dy);
			source00.x += source_dx;
			source00.y += source_dy;
			source10.x += source_dx;
			source10.y += source_dy;
			source01.x += source_dx;
			source01.y += source_dy;
			source11.x += source_dx;
			source11.y += source_dy;
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

void Game_window::paint_current_view() {
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
	if (obj->get_shapenum() == 440) {
		static int last_frame = -1;
		static int last_tx = -1;
		static int last_ty = -1;
		static int last_tz = -1;
		const Tile_coord t = obj->get_tile();
		const int frame = obj->get_framenum();
		if (frame != last_frame || t.tx != last_tx || t.ty != last_ty || t.tz != last_tz) {
			last_frame = frame;
			last_tx = t.tx;
			last_ty = t.ty;
			last_tz = t.tz;
			const Shape_info& info = obj->get_info();
			std::ofstream out("exult-illumination-440.log", std::ios::out | std::ios::app);
			if (out.good()) {
				out << "shape=440"
					<< " frame=" << frame
					<< " tile=(" << t.tx << "," << t.ty << "," << t.tz << ")"
					<< " animated=" << (info.is_animated() ? 1 : 0)
					<< " translucent=" << (info.has_translucency() ? 1 : 0)
					<< " light=" << info.get_object_light(frame)
					<< " frames=" << obj->get_num_frames()
					<< " owner=" << (obj->get_owner() ? 1 : 0)
					<< " deps=" << obj->get_dependencies().size();

				Game_object_vector lamps;
				Game_object::find_nearby(lamps, t, 526, 12, 0);
				out << " nearby526=" << lamps.size();
				for (auto* lamp : lamps) {
					if (!lamp) continue;
					const Tile_coord lt = lamp->get_tile();
					out << " [" << lamp->get_framenum()
						<< "@(" << lt.tx << "," << lt.ty << "," << lt.tz << ")]";
				}
				lamps.clear();
				Game_object::find_nearby(lamps, t, 889, 12, 0);
				out << " nearby889=" << lamps.size();
				for (auto* lamp : lamps) {
					if (!lamp) continue;
					const Tile_coord lt = lamp->get_tile();
					out << " [" << lamp->get_framenum()
						<< "@(" << lt.tx << "," << lt.ty << "," << lt.tz << ")]";
				}
				out << "\n";
			}
		}
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
