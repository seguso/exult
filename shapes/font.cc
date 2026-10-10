/*
 *  Copyright (C) 2000-2022  The Exult Team
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

#include "font.h"
#include "gamewin.h"
#include "palette.h"

#include <array>
#include <algorithm>
#include <vector>

#include <cstring>

#include "U7file.h"
#include "databuf.h"
#include "exceptions.h"
#include "ibuf8.h"
#include "ignore_unused_variable_warning.h"
#include "vgafile.h"

using std::size_t;
using std::string;
using std::strncmp;
using std::toupper;

FontManager fontManager;

//	Want a more restrictive test for space.
inline bool Is_space(char c) {
	return c == ' ' || c == '\n' || c == '\t';
}

/*
 *  Pass space.
 */

static const char* Pass_whitespace(const char* text) {
	while (Is_space(*text)) {
		text++;
	}
	return text;
}

// Just spaces and tabs:
static const char* Pass_space(const char* text) {
	while (*text == ' ' || *text == '\t') {
		text++;
	}
	return text;
}

/*
 *  Pass a word.
 */

static const char* Pass_word(const char* text) {
	// '*' is Exult's in-band page-break marker. Stop the current word before
	// it even when it follows punctuation with no intervening space, so the
	// main text-box loop can consume it as control syntax instead of painting
	// the replacement TTF's visible asterisk glyph.
	while (*text && (*text != '^') && (*text != '*') && (!Is_space(*text) || (*text == '\f') || (*text == '\v'))) {
		text++;
	}
	return text;
}

/*
 *  Draw text within a rectangular area.
 *  Special characters handled are:
 *      \n  New line.
 *      space   Word break.
 *      tab Treated like a space for now.
 *      *   Page break.
 *      ^   Uppercase next letter.
 *
 *  Output: If out of room, -offset of end of text painted.
 *      Else height of text painted.
 */

int Font::paint_text_box(
		Image_buffer8* win,                // Buffer to paint in.
		const char* text, int x, int y,    // Top-left corner of box.
		int w, int h,                      // Dimensions.
		int            vert_lead,          // Extra spacing between lines.
		bool           pbreak,             // End at punctuation.
		bool           center,             // Center each line.
		Cursor_info*   cursor,             // We set x, y if not nullptr.
		unsigned char* trans) {
	const char* start    = text;    // Remember the start.
	auto        clipsave = win->SaveClip();
	auto        newclip  = clipsave.Rect().intersect(TileRect(x, y, w, h));
	win->set_clip(newclip.x, newclip.y, newclip.w, newclip.h);

	const int   endx           = x + w;    // Figure where to stop.
	int         curx           = x;
	int         cury           = y;
	const int   height         = get_text_height() + vert_lead + ver_lead;
	const int   space_width    = get_text_width(" ", 1);
	const int   max_lines      = h / height;    // # lines that can be shown.
	auto*       lines          = new string[max_lines + 1];
	int         cur_line       = 0;
	const char* last_punct_end = nullptr;    // ->last period, qmark, etc.
	// Last punct in 'lines':
	int last_punct_line   = -1;
	int last_punct_offset = -1;
	int coff              = -1;

	if (cursor) {
		coff      = cursor->offset;
		cursor->x = -1;
	}
	while (*text) {
		if (cursor && text - start == coff) {
			cursor->set_found(curx, cury, cur_line);
		}
		switch (*text) {    // Special cases.
		case '\n':          // Next line.
			curx = x;
			text++;
			cur_line++;
			cury += height;
			if (cur_line >= max_lines) {
				break;    // No more room.
			}
			continue;
		case '\r':    //??
			text++;
			continue;
		case ' ':    // Space.
		case '\t': {
			// Pass space.
			const char* wrd = Pass_space(text);
			if (wrd != text) {
				int w = get_text_width(text, static_cast<uint32>(wrd - text));
				if (w <= 0) {
					w = space_width;
				}
				const int nsp = w / space_width;
				lines[cur_line].append(nsp, ' ');
				if (cursor && coff > text - start && coff < wrd - start) {
					cursor->set_found(curx + static_cast<uint32>(coff - (text - start)) * space_width, cury, cur_line);
				}
				curx += nsp * space_width;
			}
			text = wrd;
			break;
		}
		}
		if (cur_line >= max_lines) {
			break;
		}

		if (*text == '*') {
			text++;
			if (cur_line) {
				break;
			}
		}
		const bool ucase_next = *text == '^';
		if (ucase_next) {    // Skip it.
			text++;
		}
		// Pass word & get its width.
		const char* ewrd = Pass_word(text);
		int         width;
		if (ucase_next) {
			const char c = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
			width        = get_text_width(&c, 1u) + get_text_width(text + 1, static_cast<uint32>(ewrd - text - 1));
		} else {
			width = get_text_width(text, static_cast<uint32>(ewrd - text));
		}
		if (curx + width - hor_lead > endx) {
			// Word-wrap.
			if (ucase_next) {
				text--;    // Put the '^' back.
			}
			curx = x;
			cur_line++;
			cury += height;
			if (cur_line >= max_lines) {
				break;    // No more room.
			}
		}
		if (cursor && coff >= text - start && coff < ewrd - start) {
			cursor->set_found(curx + get_text_width(text, static_cast<uint32>(coff - (text - start))), cury, cur_line);
		}
		// Store word.
		if (ucase_next) {
			lines[cur_line].push_back(static_cast<char>(toupper(static_cast<unsigned char>(*text))));
			++text;
		}
		lines[cur_line].append(text, ewrd - text);
		curx += width;
		text = ewrd;    // Continue past the word.
		// Keep loc. of punct. endings.
		if (text[-1] == '.' || text[-1] == '?' || text[-1] == '!' || text[-1] == ',' || text[-1] == '"') {
			last_punct_end    = text;
			last_punct_line   = cur_line;
			last_punct_offset = static_cast<int>(lines[cur_line].length());
		}
	}
	if (*text &&    // Out of room?
					// Break off at end of punct.
		pbreak && last_punct_end) {
		text = Pass_whitespace(last_punct_end);
	} else {
		last_punct_line = -1;
		if (cursor && text - start == coff &&    // Cursor at very end?
			cur_line < max_lines) {
			cursor->set_found(curx, cury, cur_line);
		}
	}
	if (cursor) {
		cursor->nlines = cur_line + (cur_line < max_lines);
	}
	cury = y;    // Render text.
	for (int i = 0; i <= cur_line; i++) {
		const char* str = lines[i].c_str();
		int         len = static_cast<int>(lines[i].length());
		if (i == last_punct_line) {
			len = last_punct_offset;
		}
		if (center) {
			center_text(win, x + w / 2, cury, str, trans);
		} else {
			paint_text(win, str, len, x, cury, trans);
		}
		cury += height;
		if (i == last_punct_line) {
			break;
		}
	}
	delete[] lines;
	if (*text) {                                   // Out of room?
		return -static_cast<int>(text - start);    // Return -offset of end.
	} else {                                       // Else return height.
		return cury - y;
	}
}

/*
 *  Draw text at a given location (which is the upper-left corner of the
 *  place to draw.
 *
 *  Output: Width in pixels of what was drawn.
 */

int Font::paint_text(
		Image_buffer8* win, const char* text, int xoff, int yoff, unsigned char* trans) {
	return paint_text(win, text, static_cast<int>(std::strlen(text)), xoff, yoff, trans);
}

/*
 * Draw each outlined glyph first, then repaint all yellow fills over every
 * outline. This avoids a later character's black halo erasing a preceding
 * character's yellow pixels when tracking is tight. The fill-only frames
 * share the same geometry as the outlined ones, so both passes use the same
 * advances and the existing wrapping/width calculations remain valid.
 */
int Font::paint_text(
		Image_buffer8* win, const char* text, int textlen,
		int xoff, int yoff, unsigned char* trans) {
	ignore_unused_variable_warning(win);
	if (!font_shapes) return 0;
	const int baseline_y = yoff + get_text_baseline();
	// Half-pixel tracking must actually blend neighboring pixel placements.
	// Alternating 0/1 pixel advances produces irregular pairs of letters.
	// Preserve the existing two-pass outline/fill order, and blend only
	// glyphs whose fractional origin is one half pixel.
	if (extra_tracking_halves == 1 && fill_only_shapes && textlen > 0 && win) {
		auto* game = Game_window::get_instance();
		const Palette* pal = game ? game->get_pal() : nullptr;
		if (pal) {
			std::array<unsigned char, 65536> blend_cache;
			blend_cache.fill(255);
			auto half_color = [&](unsigned char a, unsigned char b) -> unsigned char {
				if (a == b) return a;
				const unsigned int key = (static_cast<unsigned int>(a) << 8) | b;
				if (blend_cache[key] != 255) return blend_cache[key];
				return blend_cache[key] = static_cast<unsigned char>(pal->find_color(
					(pal->get_red(a) + pal->get_red(b) + 1) / 2,
					(pal->get_green(a) + pal->get_green(b) + 1) / 2,
					(pal->get_blue(a) + pal->get_blue(b) + 1) / 2));
			};
			for (int pass = 0; pass < 2; ++pass) {
				int x_twice = xoff * 2;
				for (int i = 0; i < textlen; ++i) {
					const unsigned char chr = static_cast<unsigned char>(text[i]);
					auto* outlined = font_shapes->get_frame(chr);
					if (!outlined || !outlined->is_rle()) continue;
					auto* glyph = pass == 0 ? outlined : fill_only_shapes->get_frame(chr);
					const int x = x_twice / 2;
					if (glyph && glyph->is_rle()) {
						auto paint = [&](int xpos) {
							if (trans) glyph->paint_rle_remapped(xpos, baseline_y, trans);
							else glyph->paint_rle(xpos, baseline_y);
						};
						if ((x_twice & 1) == 0) {
							paint(x);
						} else {
							// Glyph frame offsets can extend to either side of
							// its origin. Clamp reads to the framebuffer, and
							// leave the ordinary shape painter to handle clip.
							const int margin = 8;
							const int left = std::max(0, x - glyph->get_width() - margin);
							const int top = std::max(0, baseline_y - glyph->get_height() - margin);
							const int right = std::min<int>(static_cast<int>(win->get_width()), x + glyph->get_width() + margin + 2);
							const int bottom = std::min<int>(static_cast<int>(win->get_height()), baseline_y + glyph->get_height() + margin);
							if (left < right && top < bottom) {
								const int w = right - left, h = bottom - top;
								std::vector<unsigned char> before(static_cast<size_t>(w) * h);
								std::vector<unsigned char> left_pixels(before.size());
								auto save = [&](std::vector<unsigned char>& dst) {
									for (int yy = 0; yy < h; ++yy)
										for (int xx = 0; xx < w; ++xx)
											dst[static_cast<size_t>(yy) * w + xx] = win->get_pixel8(left + xx, top + yy);
								};
								auto restore = [&]() {
									for (int yy = 0; yy < h; ++yy)
										for (int xx = 0; xx < w; ++xx)
											win->put_pixel8(before[static_cast<size_t>(yy) * w + xx], left + xx, top + yy);
								};
								save(before);
								paint(x);
								save(left_pixels);
								restore();
								paint(x + 1);
								for (int yy = 0; yy < h; ++yy) {
									for (int xx = 0; xx < w; ++xx) {
										const size_t k = static_cast<size_t>(yy) * w + xx;
										win->put_pixel8(half_color(left_pixels[k], win->get_pixel8(left + xx, top + yy)),
												left + xx, top + yy);
									}
								}
							} else paint(x);
						}
					}
					x_twice += 2 * (outlined->get_width() + hor_lead) + 1;
				}
			}
			return get_text_width(text, textlen);
		}
	}

	int total_advance = 0;
	for (int pass = 0; pass < (fill_only_shapes ? 2 : 1); ++pass) {
		int x = xoff;
		int glyph_count = 0;
		for (int i = 0; i < textlen; ++i) {
			const unsigned char chr = static_cast<unsigned char>(text[i]);
			Shape_frame* outlined = font_shapes->get_frame(chr);
			if (!outlined || !outlined->is_rle()) continue;
			Shape_frame* glyph = pass == 0 ? outlined : fill_only_shapes->get_frame(chr);
			if (glyph && glyph->is_rle()) {
				if (trans) glyph->paint_rle_remapped(x, baseline_y, trans);
				else glyph->paint_rle(x, baseline_y);
			}
			x += outlined->get_width() + hor_lead;
			x += ((++glyph_count * extra_tracking_halves) / 2)
					- (((glyph_count - 1) * extra_tracking_halves) / 2);
		}
		total_advance = x - xoff;
	}
	return total_advance;
}


/*
 *
 *  FIXED WIDTH RENDERING
 *
 */

/*
 *  Draw text within a rectangular area.
 *  Special characters handled are:
 *      \n  New line.
 *      space   Word break.
 *      tab Treated like a space for now.
 *      *   Page break.
 *      ^   Uppercase next letter.
 *
 *  Output: If out of room, -offset of end of text painted.
 *      Else height of text painted.
 */

int Font::paint_text_box_fixedwidth(
		Image_buffer8* win,                // Buffer to paint in.
		const char* text, int x, int y,    // Top-left corner of box.
		int w, int h,                      // Dimensions.
		int            char_width,         // Width of each character
		int            vert_lead,          // Extra spacing between lines.
		int            pbreak,             // End at punctuation.
		unsigned char* trans) {
	const char* start = text;    // Remember the start.
	win->set_clip(x, y, w, h);
	const int   endx           = x + w;    // Figure where to stop.
	int         curx           = x;
	int         cury           = y;
	const int   height         = get_text_height() + vert_lead + ver_lead;
	const int   max_lines      = h / height;    // # lines that can be shown.
	auto*       lines          = new string[max_lines + 1];
	int         cur_line       = 0;
	const char* last_punct_end = nullptr;    // ->last period, qmark, etc.
	// Last punct in 'lines':
	int last_punct_line   = -1;
	int last_punct_offset = -1;

	while (*text) {
		switch (*text) {    // Special cases.
		case '\n':          // Next line.
			curx = x;
			text++;
			cur_line++;
			if (cur_line >= max_lines) {
				break;    // No more room.
			}
			continue;
		case ' ':    // Space.
		case '\t': {
			// Pass space.
			const char* wrd = Pass_space(text);
			if (wrd != text) {
				const int w   = static_cast<int>(wrd - text) * char_width;
				const int nsp = w / char_width;
				lines[cur_line].append(nsp, ' ');
				curx += nsp * char_width;
			}
			text = wrd;
			break;
		}
		}

		if (cur_line >= max_lines) {
			break;
		}

		if (*text == '*') {
			text++;
			if (cur_line) {
				break;
			}
		}
		const bool ucase_next = *text == '^';
		if (ucase_next) {    // Skip it.
			text++;
		}
		// Pass word & get its width.
		const char* ewrd  = Pass_word(text);
		const int   width = static_cast<int>(ewrd - text) * char_width;
		if (curx + width - hor_lead > endx) {
			// Word-wrap.
			if (ucase_next) {
				text--;    // Put the '^' back.
			}
			curx = x;
			cur_line++;
			if (cur_line >= max_lines) {
				break;    // No more room.
			}
		}

		// Store word.
		if (ucase_next) {
			lines[cur_line].push_back(static_cast<char>(toupper(static_cast<unsigned char>(*text))));
			++text;
		}
		lines[cur_line].append(text, ewrd - text);
		curx += width;
		text = ewrd;    // Continue past the word.
		// Keep loc. of punct. endings.
		if (text[-1] == '.' || text[-1] == '?' || text[-1] == '!' || text[-1] == ',' || text[-1] == '"') {
			last_punct_end    = text;
			last_punct_line   = cur_line;
			last_punct_offset = static_cast<int>(lines[cur_line].length());
		}
	}
	if (*text &&    // Out of room?
					// Break off at end of punct.
		pbreak && last_punct_end) {
		text = Pass_whitespace(last_punct_end);
	} else {
		last_punct_line = -1;
	}
	// Render text.
	for (int i = 0; i <= cur_line; i++) {
		const char* str = lines[i].data();
		int         len = static_cast<int>(lines[i].length());
		if (i == last_punct_line) {
			len = last_punct_offset;
		}
		paint_text_fixedwidth(win, str, len, x, cury, char_width, trans);
		cury += height;
		if (i == last_punct_line) {
			break;
		}
	}
	win->clear_clip();
	delete[] lines;
	if (*text) {                                   // Out of room?
		return -static_cast<int>(text - start);    // Return -offset of end.
	} else {                                       // Else return height.
		return cury - y;
	}
}

/*
 *  Draw text at a given location (which is the upper-left corner of the
 *  place to draw. Text will be drawn with the fixed width specified.
 *
 *  Output: Width in pixels of what was drawn.
 */

int Font::paint_text_fixedwidth(
		Image_buffer8* win,      // Buffer to paint in.
		const char*    text,     // What to draw, 0-delimited.
		int xoff, int yoff,      // Upper-left corner of where to start.
		int            width,    // Width of each character
		unsigned char* trans) {
	ignore_unused_variable_warning(win);
	int x = xoff;
	int w;
	int chr;
	yoff += get_text_baseline();
	while ((chr = *text++) != 0) {
		Shape_frame* shape = font_shapes->get_frame(static_cast<unsigned char>(chr));
		if (!shape || !shape->is_rle()) {
			auto oldflags = std::cerr.flags();

			std::cerr << " unable to find rle frame for character '" << char(chr) << "' 0x" << std::hex << chr << " in font"
					  << std::endl;
			std::cerr.flags(oldflags);

			continue;
		}
		x += w = (width - shape->get_width()) / 2;
		if (trans) {
			shape->paint_rle_remapped(x, yoff, trans);
		} else {
			shape->paint_rle(x, yoff);
		}
		x += width - w;
	}
	return x - xoff;
}

/*
 *  Draw text at a given location (which is the upper-left corner of the
 *  place to draw. Text will be drawn with the fixed width specified.
 *
 *  Output: Width in pixels of what was drawn.
 */

int Font::paint_text_fixedwidth(
		Image_buffer8* win,        // Buffer to paint in.
		const char*    text,       // What to draw.
		int            textlen,    // Length of text.
		int xoff, int yoff,        // Upper-left corner of where to start.
		int            width,      // Width of each character
		unsigned char* trans) {
	ignore_unused_variable_warning(win);
	int w;
	int x = xoff;
	yoff += get_text_baseline();
	while (textlen--) {
		int          chr;
		Shape_frame* shape = font_shapes->get_frame(static_cast<unsigned char>(chr = *text++));
		if (!shape || !shape->is_rle()) {
			auto oldflags = std::cerr.flags();
			std::cerr << " unable to find rle frame for character '" << char(chr) << "' 0x" << std::hex << chr << " in font"
					  << std::endl;

			std::cerr.flags(oldflags);
			continue;
		}
		x += w = (width - shape->get_width()) / 2;
		if (trans) {
			shape->paint_rle_remapped(x, yoff, trans);
		} else {
			shape->paint_rle(x, yoff);
		}
		x += width - w;
	}
	return x - xoff;
}

/*
 *  Get the width in pixels of a 0-delimited string.
 */

int Font::get_text_width(const char* text) {
	int width = 0;
	int glyph_count = 0;
	if (font_shapes) {
		short chr;
		while ((chr = *text++) != 0) {
			Shape_frame* shape = font_shapes->get_frame(static_cast<unsigned char>(chr));
			if (shape && shape->is_rle()) {
				width += shape->get_width() + hor_lead;
				width += ((++glyph_count * extra_tracking_halves) / 2) - (((glyph_count - 1) * extra_tracking_halves) / 2);
			}
		}
	}
	return width;
}

/*
 *  Get the width in pixels of a string given by length.
 */

int Font::get_text_width(
		const char* text,
		int         textlen    // Length of text.
) {
	int width = 0;
	int glyph_count = 0;
	if (font_shapes) {
		while (textlen--) {
			Shape_frame* shape = font_shapes->get_frame(static_cast<unsigned char>(*text++));
			if (shape && shape->is_rle()) {
				width += shape->get_width() + hor_lead;
				width += ((++glyph_count * extra_tracking_halves) / 2) - (((glyph_count - 1) * extra_tracking_halves) / 2);
			}
		}
	}
	return width;
}

void Font::get_text_box_dims(const char* text, int& width, int& height, int vert_lead) {
	width         = 0;
	height        = 0;
	int cur_width = 0;
	int line_glyphs = 0;

	int num_lines = 1;
	if (font_shapes) {
		short chr;
		while ((chr = *text++) != 0) {
			if (chr == '\n') {
				num_lines++;
				width     = std::max(width, cur_width);
				cur_width = 0;
				line_glyphs = 0;
			}
			Shape_frame* shape = font_shapes->get_frame(static_cast<unsigned char>(chr));
			if (shape && shape->is_rle()) {
				cur_width += shape->get_width() + hor_lead;
				cur_width += ((++line_glyphs * extra_tracking_halves) / 2) - (((line_glyphs - 1) * extra_tracking_halves) / 2);
			}
		}
		width  = std::max(width, cur_width);
		height = num_lines * (get_text_height() + vert_lead + ver_lead);
	}
}

/*
 *  Get font line-height.
 */

int Font::get_text_height() {
	// Note, I wont assume the fonts exist
	// Shape_frame *A = font_shapes->get_frame('A');
	// Shape_frame *y = font_shapes->get_frame('y');
	return highest + lowest + 1;
}

/*
 *  Get font baseline as the distance from the top.
 */

int Font::get_text_baseline() {
	// Shape_frame *A = font_shapes->get_frame('A');
	return highest;
}

/*
 *  Find cursor location within text, given (x,y) screen coords.
 *  Note:  This has to match paint_text_box().
 *
 *  Output: Offset in text if found.
 *      If not found, -offset of end of text there was room to paint.
 */

int Font::find_cursor(
		const char* text, int x, int y,    // Top-left corner of box.
		int w, int h,                      // Dimensions.
		int cx, int cy,                    // Mouse loc. to find.
		int vert_lead                      // Extra spacing between lines.
) {
	const char* start       = text;     // Remember the start.
	const int   endx        = x + w;    // Figure where to stop.
	int         curx        = x;
	int         cury        = y;
	const int   height      = get_text_height() + vert_lead + ver_lead;
	const int   space_width = get_text_width(" ", 1);
	const int   max_lines   = h / height;    // # lines that can be shown.
	int         cur_line    = 0;
	int         chr;

	while ((chr = *text) != 0) {
		switch (chr) {    // Special cases.
		case '\n':        // Next line.
			if (cy >= cury && cy < cury + height && cx >= curx && cx < x + w) {
				return static_cast<int>(text - start);
			}
			++text;
			curx = x;
			cur_line++;
			cury += height;
			if (cur_line >= max_lines) {
				break;    // No more room.
			}
			continue;
		case '\r':    //??
			++text;
			continue;
		case ' ':    // Space.
		case '\t':
			if (cy >= cury && cy < cury + height && cx >= curx && cx < curx + space_width) {
				return static_cast<int>(text - start);
			}
			++text;
			curx += space_width;
			continue;
		}
		if (cur_line >= max_lines) {
			break;
		}

		if (*text == '*') {
			text++;
			if (cur_line) {
				break;
			}
		}
		const bool ucase_next = *text == '^';
		if (ucase_next) {    // Skip it.
			text++;
		}
		// Pass word & get its width.
		const char* ewrd = Pass_word(text);
		int         width;
		if (ucase_next) {
			const char c = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
			width        = get_text_width(&c, 1u) + get_text_width(text + 1, static_cast<uint32>(ewrd - text - 1));
		} else {
			width = get_text_width(text, static_cast<uint32>(ewrd - text));
		}
		if (curx + width - hor_lead > endx) {
			// Word-wrap.
			// Past end of this line?
			if (cy >= cury && cy < cury + height && cx >= curx && cx < x + w) {
				return static_cast<int>(text - start - 1);
			}
			curx = x;
			cur_line++;
			cury += height;
			if (cur_line >= max_lines) {
				break;    // No more room.
			}
		}
		if (cy >= cury && cy < cury + height && cx >= curx && cx < curx + width) {
			const int woff = find_xcursor(text, static_cast<int>(ewrd - text), cx - curx);
			if (woff >= 0) {
				return static_cast<int>(text - start) + woff;
			}
		}
		curx += width;
		text = ewrd;    // Continue past the word.
	}
	if (cy >= cury && cy < cury + height &&    // End of last line?
		cx >= curx && cx < x + w) {
		return static_cast<int>(text - start);
	}
	return -static_cast<int>(text - start);    // Failed, so indicate where we are.
}

/*
 *  Find an x-coord. within a piece of text.
 *
 *  Output: Offset if found, else -1.
 */

int Font::find_xcursor(
		const char* text,
		int         textlen,    // Length of text.
		int         cx          // Loc. to find.
) {
	const char* start = text;
	int         curx  = 0;
	while (textlen--) {
		Shape_frame* shape = font_shapes->get_frame(static_cast<unsigned char>(*text++));
		if (shape && shape->is_rle()) {
			const int w = shape->get_width() + hor_lead;
			if (cx >= curx && cx < curx + w) {
				return static_cast<int>(text - 1 - start);
			}
			curx += w;
		}
	}
	return -1;
}

Font::Font() = default;

Font::Font(const File_spec& fname0, int index, int hlead, int vlead) {
	load(fname0, index, hlead, vlead);
}

Font::Font(const File_spec& fname0, const File_spec& fname1, int index, int hlead, int vlead) {
	load(fname0, fname1, index, hlead, vlead);
}

Font::Font(std::unique_ptr<Shape_file> shapes, int hlead, int vlead)
		: hor_lead(hlead), ver_lead(vlead), font_shapes(std::move(shapes)) {
	calc_highlow();
}

void Font::clean_up() {
	font_shapes.reset();
}

/**
 *  Loads a font from a multiobject.
 *  @param font_obj Where we are loading from.
 *  @param hleah    Horizontal lead of the font.
 *  @param vleah    Vertical lead of the font.
 */
int Font::load_internal(IDataSource& data, int hlead, int vlead) {
	if (!data.good()) {
		font_shapes.reset();
		hor_lead = 0;
		ver_lead = 0;
		return -1;
	} else {
		// Is it an IFF archive?
		char hdr[5] = {0};
		data.read(hdr, 4);
		data.seek(0);
		if (!strncmp(hdr, "font", 4)) {
			data.skip(8);    // Yes, skip first 8 bytes.
		}
		font_shapes = std::make_unique<Shape_file>(&data);
		hor_lead    = hlead;
		ver_lead    = vlead;
		calc_highlow();
	}
	return 0;
}

int Font::load(const File_spec& fname0, int index, int hlead, int vlead) {
	clean_up();
	IExultDataSource data(fname0, index);
	return load_internal(data, hlead, vlead);
}

int Font::load(const File_spec& fname0, const File_spec& fname1, int index, int hlead, int vlead) {
	clean_up();
	IExultDataSource data(fname0, fname1, index);
	return load_internal(data, hlead, vlead);
}

int Font::center_text(Image_buffer8* win, int x, int y, const char* s, unsigned char* trans) {
	return draw_text(win, x - get_text_width(s) / 2, y, s, trans);
}

void Font::calc_highlow() {
	bool unset = true;

	for (int i = 0; i < font_shapes->get_num_frames(); i++) {
		Shape_frame* f = font_shapes->get_frame(i);

		if (!f || !f->is_rle()) {
			continue;
		}

		if (unset) {
			unset   = false;
			highest = f->get_yabove();
			lowest  = f->get_ybelow();
			continue;
		}

		if (f->get_yabove() > highest) {
			highest = f->get_yabove();
		}
		if (f->get_ybelow() > lowest) {
			lowest = f->get_ybelow();
		}
	}
}

FontManager::~FontManager() {
	reset();
}

/**
 *  Loads a font from a File_spec.
 *  @param name Name to give to this font.
 *  @param fname0   First file spec.
 *  @param index    Number of font to load.
 *  @param hleah    Horizontal lead of the font.
 *  @param vleah    Vertical lead of the font.
 */
void FontManager::add_font(const char* name, const File_spec& fname0, int index, int hlead, int vlead) {
	remove_font(name);

	auto font = std::make_shared<Font>(fname0, index, hlead, vlead);

	fonts[name] = font;
}

/**
 *  Loads a font from a File_spec.
 *  @param name Name to give to this font.
 *  @param fname0   First file spec.
 *  @param fname1   Second file spec.
 *  @param index    Number of font to load.
 *  @param hleah    Horizontal lead of the font.
 *  @param vleah    Vertical lead of the font.
 */
void FontManager::add_font(const char* name, const File_spec& fname0, const File_spec& fname1, int index, int hlead, int vlead) {
	remove_font(name);

	auto font = std::make_shared<Font>(fname0, fname1, index, hlead, vlead);

	fonts[name] = font;
}

void FontManager::remove_font(const char* name) {
	fonts.erase(name);
}

std::shared_ptr<Font> FontManager::get_font(const char* name) {
	return fonts[name];
}

void FontManager::reset() {
	fonts.clear();
}
