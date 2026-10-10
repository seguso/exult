/*
Copyright (C) 2001-2024 The Exult Team

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

#ifndef GAMEDISPLAYOPTIONS_GUMP_H
#define GAMEDISPLAYOPTIONS_GUMP_H

#include "Modal_gump.h"

#include <array>
#include <memory>
#include <string>

class Gump_button;

class GameDisplayOptions_gump : public Modal_gump {
private:
	int                      facestats;
	int                      sc_enabled;
	int                      sc_outline;
	bool                     sb_hide_missing;
	std::vector<std::string> sc_outline_txt;
	int                      text_bg;
	int                      smooth_scrolling;
	int                      modern_smooth;
	int                      smooth_avatar_walk;
	int                      modern_mouse_target;
	int                      modern_tau;
	int                      rotate_world;
	int                      rotate_sampling_mode;
	int                      crt_enabled;
	int                      crt_horizontal_strength;
	int                      crt_vertical_strength;
	int                      crt_brightness_compensation;
	int                      crt_horizontal_width;
	int                      crt_vertical_width;
	int                      crt_beam_sigma;
	bool                     usecode_intro;
	bool                     extended_intro;
	bool                     menu_intro;
	int                      paperdolls;
	int                      language;
	int                      fonts;
	int                      conversation_font;
	int                      conversation_font_size;
	int                      conversation_font_default_size;

	enum button_ids {
		id_first = 0,
		id_ok    = id_first,
		id_help,
		id_cancel,
		id_first_setting,
		id_facestats = id_first_setting,
		id_sc_enabled,
		id_sc_outline,
		id_sb_hide_missing,
		id_text_bg,
		id_smooth_scrolling,
		id_modern_smooth,
		id_smooth_avatar_walk,
		id_modern_mouse_target,
		id_modern_keyboard,
		id_modern_tau,
		id_rotate_world,
		id_rotate_sampling_mode,
		id_crt_enabled,
		id_crt_horizontal_strength,
		id_crt_vertical_strength,
		id_crt_brightness_compensation,
		id_crt_horizontal_width,
		id_crt_vertical_width,
		id_crt_beam_sigma,
		id_menu_intro,
		id_usecode_intro,
		id_extended_intro,
		id_paperdolls,
		id_android_autolaunch,
		id_language,
		id_fonts,
		id_conversation_font,
		id_conversation_font_size,

		id_nav_movement,
		id_nav_crt,
		id_nav_fonts,
		id_nav_gameplay,
		id_back,
		id_count
	};

	std::array<std::unique_ptr<Gump_button>, id_count> buttons;
	public:
	enum class Page { display, movement, crt, rotation, fonts };
private:
	Page page = Page::display;
	int modern_keyboard = 0;
	void open_readable_fonts();

public:
	explicit GameDisplayOptions_gump(Page section = Page::display);

	// Paint it and its contents.
	void paint() override;
	void close() override;
	

	void build_buttons();
	void update_legacy_smooth_button();
	void update_conversation_font_size_button();
	void update_crt_strength_buttons();
	void update_crt_compensation_button();
	void update_crt_sigma_button();
	void choose_crt_sigma();
	void preview_crt();

	void load_settings();
	void save_settings();
	void cancel();
	void help();

	void toggle_facestats(int state) {
		facestats = state;
	}

	void toggle_sc_enabled(int state) {
		sc_enabled = state;
	}

	void toggle_sc_outline(int state) {
		sc_outline = state;
	}

	void toggle_language(int state) {
		language = state;
	}

	void toggle_fonts(int state) {
		fonts = state;
	}

	void toggle_conversation_font(int state) {
		conversation_font = state;
	}

	void choose_conversation_font_size();

	void toggle_sb_hide_missing(int state) {
		sb_hide_missing = state;
	}

	void toggle_text_bg(int state) {
		text_bg = state;
	}

	void toggle_smooth_scrolling(int state) {
		smooth_scrolling = state;
	}

	void toggle_modern_mouse_target(int state) { modern_mouse_target = state; }
	void toggle_modern_keyboard(int state) { modern_keyboard = state; }

	void toggle_smooth_avatar_walk(int state) { smooth_avatar_walk = state; }

	void toggle_modern_smooth(int state) {
		modern_smooth = state;
		update_legacy_smooth_button();
	}

	void toggle_modern_tau(int state) {
		modern_tau = state;
	}

	void toggle_rotate_world(int state) {
		rotate_world = state;
	}

	void toggle_rotate_sampling_mode(int state) {
		rotate_sampling_mode = state;
	}

	void toggle_crt_enabled(int state) {
		crt_enabled = state;
	}
	void choose_crt_horizontal_strength();
	void choose_crt_vertical_strength();
	void choose_crt_brightness_compensation();
	void toggle_crt_horizontal_width(int state) {
		crt_horizontal_width = state;
	}
	void toggle_crt_vertical_width(int state) {
		crt_vertical_width = state;
	}

	void toggle_menu_intro(int state) {
		menu_intro = state;
	}

	void toggle_usecode_intro(int state) {
		usecode_intro = state;
	}

	void toggle_extended_intro(int state) {
		extended_intro = state;
	}

	void toggle_paperdolls(int state) {
		paperdolls = state;
	}

private:
	int android_autolaunch;
	static bool (*Android_getAutoLaunch)();
	static void (*Android_setAutoLaunch)(bool);

public:
	void toggle_android_launcher(int state) {
		android_autolaunch = state;
	}

	static void SetAndroidAutoLaunchFPtrs(void (*setter)(bool), bool (*getter)());

	Gump_button* on_button(int mx, int my) override;
};

#endif
