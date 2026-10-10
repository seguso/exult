/**
 ** Create RLE font shapes from a given font.
 **
 ** Written: 4/8/2002 - JSF
 **/

/*
Copyright (C) 2002-2022  The Exult Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "vgafile.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using std::make_unique;

#define USE_WIN32_FONTGEN

#if defined(HAVE_FREETYPE2) || (defined(_WIN32) && defined(USE_WIN32_FONTGEN))

/*
 *  Generate a shadow around a character.
 */

static void Gen_shadow(
		unsigned char* pixels, int w, int h,    // Dimensions.
		unsigned char fg,                       // Foreground color index.
		unsigned char shadow,                   // Shadow color index
		int           radius                    // Outline radius in pixels.
) {
	int r;
	int c;

	for (r = 0; r < h; r++) {
		for (c = 0; c < w; c++) {
			if (pixels[r * w + c] != fg) {
				continue;
			}
			int rr;
			int cc;    // Fill surrounding pixels;
			for (rr = r - radius; rr <= r + radius; rr++) {
				if (rr < 0 || rr >= h) {
					continue;
				}
				for (cc = c - radius; cc <= c + radius; cc++) {
					if (cc < 0 || cc >= w) {
						continue;
					}
					// Radius 1 keeps the historical full 3x3 outline. Radius 2
					// uses a rounded/diamond second ring instead of filling the
					// whole 5x5 square; visually this lands between the old
					// one-pixel and the very heavy two-pixel outline.
					if (radius > 1 && std::abs(rr - r) + std::abs(cc - c) > radius) {
						continue;
					}
					if (pixels[rr * w + cc] != fg) {
						pixels[rr * w + cc] = shadow;
					}
				}
			}
		}
	}
}

#endif

#if defined(_WIN32) && defined(USE_WIN32_FONTGEN)

#	undef NOGDI
#	ifdef _WIN32_WINNT
#		undef _WIN32_WINNT
#	endif
#	define _WIN32_WINNT 0x0500

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	include <windows.h>
#	if defined(_MSC_VER)
#		include <objbase.h>
#		include <gdiplus.h>
#		pragma comment(lib, "gdiplus.lib")
#	endif

static bool Gen_font_shape_win32(
		HDC dc, HFONT font,
		Shape*        shape,        // Shape to set frames.
		int           nframes,      // # frames to generate, starting at 0.
		int           pixels_ht,    // Desired height in pixels.
		unsigned char fg,           // Foreground color index.
		unsigned char bg,           // Background color index.
		int           shadow,       // Shadow color, or -1
		int           shadow_radius // Outline radius in pixels.
) {
	ignore_unused_variable_warning(font, pixels_ht);
	MAT2 matrix{};
	matrix.eM11.value = 1;
	matrix.eM22.value = 1;
	// matrix.eM22.fract = 0x10000 * 10 / 12;

	shape->resize(nframes);    // Make it big enough.
	for (int ii = 0; ii < nframes; ii++) {
		// Get each glyph.
		GLYPHMETRICS metrics;

		const unsigned int buffsize = GetGlyphOutline(dc, ii, GGO_BITMAP, &metrics, 0, nullptr, &matrix);
		int                offset   = 0;    // Starting row, col.

		const wchar_t chr = ii;
		if (buffsize == GDI_ERROR || buffsize == 0) {
			SIZE size;
			GetTextExtentPoint32W(dc, &chr, 1, &size);

			if (size.cy == 0 || size.cx == 0) {
				size.cx = size.cy = 1;
			} else if (shadow != -1) {    // Make room for shadow.

				size.cx += 2 * shadow_radius;
				size.cy += 2 * shadow_radius;
				offset = shadow_radius;
			}
			auto* pixels = new uint8[size.cy * size.cx];
			memset(pixels, bg, size.cy * size.cx);

			// Not sure about dims here+++++
			shape->set_frame(make_unique<Shape_frame>(pixels, size.cx, size.cy, offset, offset, true), chr);
			delete[] pixels;
		} else {
			auto* buffer = new uint32[buffsize];
			if (GetGlyphOutline(dc, chr, GGO_BITMAP, &metrics, buffsize, buffer, &matrix) == GDI_ERROR) {
				delete[] buffer;
				shape->set_frame(make_unique<Shape_frame>(&bg, 1, 1, 0, 0, true), chr);
				continue;
			}
			int sw = metrics.gmBlackBoxX;
			int sh = metrics.gmBlackBoxY;    // Shape width/height.

			if (sw < metrics.gmCellIncX) {
				sw = metrics.gmCellIncX;
			}
			if (sh < metrics.gmCellIncY) {
				sh = metrics.gmCellIncY;
			}

			if (shadow != -1) {    // Make room for shadow.
				sw += 2 * shadow_radius;
				sh += 2 * shadow_radius;
				offset = shadow_radius;
			}

			// Allocate our buffer.
			const int cnt    = sw * sh;    // Total #pixels.
			auto*     pixels = new unsigned char[cnt];
			memset(pixels, bg, cnt);    // Fill with background.

			unsigned char* dest = pixels + offset * sw + offset;
			const uint8*   src  = reinterpret_cast<uint8*>(buffer);
			for (unsigned int row = 0; row < metrics.gmBlackBoxY; row++) {
				for (unsigned int b = 0; b < metrics.gmBlackBoxX; b++) {
					if (src[b / 8] & (0x80 >> (b % 8))) {
						dest[b] = fg;
					}
				}
				dest += sw;    // Advance to next row.
				src += (metrics.gmBlackBoxX + 31) / 32 * 4;
			}
			delete[] buffer;

			if (shadow >= 0) {
				Gen_shadow(pixels, sw, sh, fg, static_cast<unsigned char>(shadow), shadow_radius);
			}
			// Not sure about dims here+++++
			shape->set_frame(
					make_unique<Shape_frame>(
							pixels, sw, sh, offset - metrics.gmptGlyphOrigin.x, offset + metrics.gmptGlyphOrigin.y, true),
					chr);
			delete[] pixels;
		}
	}
	return true;
}

int CALLBACK EnumFontFamProc(
		ENUMLOGFONT*   lpelf,       // logical-font data
		NEWTEXTMETRIC* lpntm,       // physical-font data
		DWORD          FontType,    // type of font
		LPARAM         lParam       // application-defined data
) {
	ignore_unused_variable_warning(lpntm, FontType);
	// MessageBox(nullptr,reinterpret_cast<const
	// char*>(lpelf->elfFullName),"lpelf->elfFullName",MB_OK);
	// MessageBox(nullptr,reinterpret_cast<const
	// char*>(lpelf->elfStyle),"lpelf->elfStyle",MB_OK);

	if (!lParam) {
		return 0;
	}
	if (!_strcmpi(reinterpret_cast<const char*>(lParam), reinterpret_cast<const char*>(lpelf->elfFullName))) {
		return 0;
	}
	if (!_strcmpi(reinterpret_cast<const char*>(lParam), reinterpret_cast<const char*>(lpelf->elfStyle))) {
		return 0;
	}
	return 1;
}

static bool Gen_font_shape_win32(
		Shape*      shape,    // Shape to set frames.
		const char* famname, const char* stylename,
		int           nframes,      // # frames to generate, starting at 0.
		int           pixels_ht,    // Desired height in pixels.
		unsigned char fg,           // Foreground color index.
		unsigned char bg,           // Background color index.
		int           shadow,       // Shadow color, or -1
		int           shadow_radius // Outline radius in pixels.
) {
	HDC dc = CreateCompatibleDC(nullptr);

	HFONT font = nullptr;

	LOGFONT logfont;

	logfont.lfHeight         = pixels_ht;
	logfont.lfWidth          = 0;
	logfont.lfEscapement     = 0;
	logfont.lfOrientation    = 0;
	logfont.lfWeight         = FW_DONTCARE;
	logfont.lfItalic         = FALSE;
	logfont.lfUnderline      = FALSE;
	logfont.lfStrikeOut      = FALSE;
	logfont.lfCharSet        = ANSI_CHARSET;
	logfont.lfOutPrecision   = OUT_RASTER_PRECIS;
	logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logfont.lfQuality        = DEFAULT_QUALITY;
	logfont.lfPitchAndFamily = DEFAULT_PITCH;

	if (font == nullptr && stylename) {
		snprintf(logfont.lfFaceName, LF_FACESIZE, "%s %s", famname, stylename);

		if (!EnumFontFamilies(dc, logfont.lfFaceName, reinterpret_cast<FONTENUMPROC>(&EnumFontFamProc), 0)) {
			font = CreateFontIndirect(&logfont);
		}
	}
	if (font == nullptr && stylename) {
		snprintf(logfont.lfFaceName, LF_FACESIZE, "%s%s", famname, stylename);
		if (!EnumFontFamilies(dc, logfont.lfFaceName, reinterpret_cast<FONTENUMPROC>(&EnumFontFamProc), 0)) {
			font = CreateFontIndirect(&logfont);
		}
	}
	if (font == nullptr) {
		snprintf(logfont.lfFaceName, LF_FACESIZE, "%s", famname);
		if (!EnumFontFamilies(
					dc, logfont.lfFaceName, reinterpret_cast<FONTENUMPROC>(&EnumFontFamProc),
					reinterpret_cast<LPARAM>(stylename))) {
			font = CreateFontIndirect(&logfont);
		}
	}

	if (font == nullptr) {
		DeleteDC(dc);
		return false;
	}

	HBITMAP bmp = CreateCompatibleBitmap(dc, 256, 256);

	SelectObject(dc, bmp);
	SelectObject(dc, font);

	const bool ret = Gen_font_shape_win32(dc, font, shape, nframes, pixels_ht, fg, bg, shadow, shadow_radius);
	DeleteObject(bmp);
	DeleteObject(font);
	DeleteDC(dc);
	return ret;
}

#endif

static void Install_runtime_font_specials(Shape_file* shape, unsigned char fg, unsigned char bg, unsigned char shadow) {
	if (!shape || shape->get_num_frames() <= 127) {
		return;
	}
	// Exult uses frame 127 as the round marker in Avatar conversation choices.
	// Standard TrueType fonts treat U+007F as a control character, so provide
	// an equivalent tiny outlined dot explicitly.
	unsigned char pixels[25] = {
		bg,     shadow, shadow, shadow, bg,
		shadow, fg,     fg,     fg,     shadow,
		shadow, fg,     fg,     fg,     shadow,
		shadow, fg,     fg,     fg,     shadow,
		bg,     shadow, shadow, shadow, bg,
	};
	shape->set_frame(std::make_unique<Shape_frame>(pixels, 5, 5, 0, 4, true), 127);
}

#if defined(HAVE_FREETYPE2)
#	include <ft2build.h>
#	ifdef __GNUC__
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wold-style-cast"
#		pragma GCC diagnostic ignored "-Wcast-qual"
#	endif    // __GNUC__
#	include FT_FREETYPE_H
#	ifdef __GNUC__
#		pragma GCC diagnostic pop
#	endif    // __GNUC__

/*
 *  Fill a shape with each frame containing the glyph for its ASCII
 *  code.  The shape has 128 frames.
 *
 *  Output: True if successful, false if error.
 */

bool Gen_font_shape(
		Shape*        shape,        // Shape to set frames.
		const char*   fontfile,     // Filename of font.
		int           nframes,      // # frames to generate, starting at 0.
		int           pixels_ht,    // Desired height in pixels.
		unsigned char fg,           // Foreground color index.
		unsigned char bg,           // Background color index.
		int           shadow,       // Shadow color, or -1
		int           shadow_radius // Outline radius in pixels.
) {
	FT_Library library;    // Initialize.
	int        error = FT_Init_FreeType(&library);
	if (error) {
		return false;
	}
	FT_Face face;    // Gets the font.
	error = FT_New_Face(library, fontfile, 0, &face);
	if (error) {
		FT_Done_FreeType(library);

		// Try to get windows to load it for us
#	if defined(_WIN32) && defined(USE_WIN32_FONTGEN)
		return Gen_font_shape_win32(shape, fontfile, nullptr, nframes, pixels_ht, fg, bg, shadow, shadow_radius);
#	else
		return false;
#	endif
	}

#	if defined(_WIN32) && defined(USE_WIN32_FONTGEN)
	static HANDLE(WINAPI * AddFontResourceExA)(LPCSTR, DWORD, PVOID);
	if (AddFontResourceExA == nullptr) {
		static_assert(sizeof(HANDLE) == sizeof(FARPROC), "sizeof(FARPROC) is not equal to sizeof(HANDLE)!");
		const FARPROC handle = GetProcAddress(LoadLibrary("GDI32"), "AddFontResourceExA");
		std::memcpy(&AddFontResourceExA, &handle, sizeof(HANDLE));
	}
	if (AddFontResourceExA != nullptr && AddFontResourceExA(fontfile, FR_PRIVATE, nullptr) != nullptr) {
		// if (face->family_name)
		// MessageBox(nullptr,face->family_name,"face->family_name",MB_OK); if
		// (face->style_name)
		// MessageBox(nullptr,face->style_name,"face->style_name",MB_OK);
		if (Gen_font_shape_win32(shape, face->family_name, face->style_name, nframes, pixels_ht, fg, bg, shadow, shadow_radius)) {
			FT_Done_FreeType(library);
			return true;
		}
	}
#	endif

	error = FT_Set_Pixel_Sizes(face, 0, pixels_ht);
	// Glyphs are rendered here:
	FT_GlyphSlot glyph = face->glyph;
	if (error) {
		return false;
	}
	shape->resize(nframes);    // Make it big enough.
	for (int chr = 0; chr < nframes; chr++) {
		// Get each glyph.
		error = FT_Load_Char(face, chr, FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
		if (error) {
			//+++++Do we need to store an empty frame?
			shape->set_frame(make_unique<Shape_frame>(&bg, 1, 1, 0, 0, true), chr);
			continue;
		}
		const int w      = glyph->bitmap.width;
		const int h      = glyph->bitmap.rows;
		int       sw     = w;
		int       sh     = h;                                          // Shape width/height.
		int       offset = 0;                                          // Starting row, col.
		if (!sw) {                                                     // 0 width (like for a space)?
			sw = static_cast<int>(glyph->metrics.horiAdvance) / 64;    // Guessin...
		}
		if (!sh) {
			sh = static_cast<int>(glyph->metrics.vertAdvance) / 64;
		}
		if (shadow != -1) {    // Make room for shadow.
			sw += 2 * shadow_radius;
			sh += 2 * shadow_radius;
			offset = shadow_radius;
		}
		// Allocate our buffer.
		const int cnt    = sw * sh;    // Total #pixels.
		auto*     pixels = new unsigned char[cnt];
		memset(pixels, bg, cnt);    // Fill with background.
		// I believe this is 1 bit/pixel:
		unsigned char* src  = glyph->bitmap.buffer;
		unsigned char* dest = pixels + offset * sw;
		for (int row = 0; row < h; row++) {
			for (int b = 0; b < w; b++) {
				if (src[b / 8] & (0x80 >> (b % 8))) {
					dest[offset + b] = fg;
				}
			}
			dest += sw;    // Advance to next row.
			src += glyph->bitmap.pitch;
		}
		if (shadow >= 0) {
			Gen_shadow(pixels, sw, sh, fg, static_cast<unsigned char>(shadow), shadow_radius);
		}
		// Not sure about dims here+++++
		shape->set_frame(
				make_unique<Shape_frame>(pixels, sw, sh, glyph->bitmap_left + offset, glyph->bitmap_top + offset, true), chr);
		delete[] pixels;
	}
	FT_Done_FreeType(library);
	return true;
}

#endif /* HAVE_FREETYPE2 */


std::unique_ptr<Shape_file> Gen_runtime_font_shape(
		const char* fontfile, const char* family, int nframes, int pixels_ht, unsigned char fg, unsigned char bg, int shadow,
		int shadow_radius) {
	auto shape = std::make_unique<Shape_file>();

#if defined(_WIN32) && defined(USE_WIN32_FONTGEN)
	bool private_font_loaded = false;
	if (fontfile && *fontfile) {
		private_font_loaded = AddFontResourceExA(fontfile, FR_PRIVATE, nullptr) > 0;
	}
	// Win32 GDI generates glyphs by family name, not by TTF filename.
	// For an explicitly selected TTF/OTF/TTC, discover the family stored in
	// that file instead of silently failing when the configured family is empty.
	std::string file_family;
#if defined(_MSC_VER)
	if (fontfile && *fontfile) {
		Gdiplus::GdiplusStartupInput startup{};
		ULONG_PTR token = 0;
		if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) == Gdiplus::Ok) {
			int length = MultiByteToWideChar(CP_UTF8, 0, fontfile, -1, nullptr, 0);
			if (length > 1) {
				std::wstring wide_path(static_cast<size_t>(length), L'\0');
				MultiByteToWideChar(CP_UTF8, 0, fontfile, -1, wide_path.data(), length);
				Gdiplus::PrivateFontCollection collection;
				if (collection.AddFontFile(wide_path.c_str()) == Gdiplus::Ok) {
					INT count = collection.GetFamilyCount();
					if (count > 0) {
						std::vector<Gdiplus::FontFamily> families(static_cast<size_t>(count));
						INT found = 0;
						if (collection.GetFamilies(count, families.data(), &found) == Gdiplus::Ok && found > 0) {
							WCHAR name[LF_FACESIZE]{};
							if (families[0].GetFamilyName(name) == Gdiplus::Ok) {
								int bytes = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
								if (bytes > 1) {
									std::string utf8(static_cast<size_t>(bytes), '\0');
									WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8.data(), bytes, nullptr, nullptr);
									utf8.resize(static_cast<size_t>(bytes - 1));
									file_family = std::move(utf8);
								}
							}
						}
					}
				}
			}
			Gdiplus::GdiplusShutdown(token);
		}
	}
#endif
	// A specific file wins over an installed-family choice. Only use the
	// configured family when no file is specified.
	const char* chosen_family = fontfile && *fontfile
			? (file_family.empty() ? nullptr : file_family.c_str()) : family;
	const bool ok = chosen_family && *chosen_family
			&& Gen_font_shape_win32(shape.get(), chosen_family, nullptr, nframes, pixels_ht, fg, bg, shadow, shadow_radius);
	if (private_font_loaded) {
		RemoveFontResourceExA(fontfile, FR_PRIVATE, nullptr);
	}
	if (ok) {
		Install_runtime_font_specials(shape.get(), fg, bg, static_cast<unsigned char>(shadow >= 0 ? shadow : fg));
		return shape;
	}
#endif

#if defined(HAVE_FREETYPE2)
	if (fontfile && *fontfile && Gen_font_shape(shape.get(), fontfile, nframes, pixels_ht, fg, bg, shadow, shadow_radius)) {
		Install_runtime_font_specials(shape.get(), fg, bg, static_cast<unsigned char>(shadow >= 0 ? shadow : fg));
		return shape;
	}
#endif
	return nullptr;
}
