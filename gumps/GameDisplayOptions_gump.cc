/*
 *  Copyright (C) 2001-2024  The Exult Team
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
#include "istring.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <mutex>
#include <iterator>

#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#	pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#	if !defined(__llvm__) && !defined(__clang__)
#		pragma GCC diagnostic ignored "-Wuseless-cast"
#	endif
#endif    // __GNUC__
#include <SDL3/SDL.h>
#if defined(SDL_PLATFORM_WINDOWS)
#	include <windows.h>
#	include <commdlg.h>
#	if defined(_MSC_VER)
#		pragma comment(lib, "Comdlg32.lib")
#	endif
#endif
#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif    // __GNUC__

#include "Configuration.h"
#include "Enabled_button.h"
#include "Face_stats.h"
#include "GameDisplayOptions_gump.h"
#include "Gump_ToggleButton.h"
#include "Gump_button.h"
#include "Gump_manager.h"
#include "ShortcutBar_gump.h"
#include "Text_button.h"
#include "exult.h"
#include "font.h"
#include "game.h"
#include "gamewin.h"
#include "palette.h"
#include "ibuf8.h"
#include <array>
#include <vector>
#include "items.h"

using std::string;

class Strings : public GumpStrings {
public:
	static auto Left() {
		return get_text_msg(0x5B0 - msg_file_start);
	}

	static auto Middle() {
		return get_text_msg(0x5B1 - msg_file_start);
	}

	static auto Right() {
		return get_text_msg(0x5B2 - msg_file_start);
	}

	static auto Vertical() {
		return get_text_msg(0x5B3 - msg_file_start);
	}

	static auto Transparent() {
		return get_text_msg(0x5B4 - msg_file_start);
	}

	static auto Black() {
		return get_text_msg(0x5B5 - msg_file_start);
	}

	static auto Green() {
		return get_text_msg(0x5B6 - msg_file_start);
	}

	static auto White() {
		return get_text_msg(0x5B7 - msg_file_start);
	}

	static auto Yellow() {
		return get_text_msg(0x5B8 - msg_file_start);
	}

	static auto Blue() {
		return get_text_msg(0x5B9 - msg_file_start);
	}

	static auto Red() {
		return get_text_msg(0x5BA - msg_file_start);
	}

	static auto Purple() {
		return get_text_msg(0x5BB - msg_file_start);
	}

	static auto SolidGray() {
		return get_text_msg(0x5BC - msg_file_start);
	}

	static auto DarkPurple() {
		return get_text_msg(0x5BD - msg_file_start);
	}

	static auto BrightYellow() {
		return get_text_msg(0x5BE - msg_file_start);
	}

	static auto LightBlue() {
		return get_text_msg(0x5BF - msg_file_start);
	}

	static auto LightGreen() {
		return get_text_msg(0x5C0 - msg_file_start);
	}

	static auto DarkRed() {
		return get_text_msg(0x5C1 - msg_file_start);
	}

	static auto Orange() {
		return get_text_msg(0x5C2 - msg_file_start);
	}

	static auto LightGray() {
		return get_text_msg(0x5C3 - msg_file_start);
	}

	static auto PaleBlue() {
		return get_text_msg(0x5C4 - msg_file_start);
	}

	static auto DarkGreen() {
		return get_text_msg(0x5C5 - msg_file_start);
	}

	static auto BrightWhite() {
		return get_text_msg(0x5C6 - msg_file_start);
	}

	static auto DarkGray() {
		return get_text_msg(0x5C7 - msg_file_start);
	}

	static auto StatusBars_() {
		return get_text_msg(0x5C8 - msg_file_start);
	}

	static auto UseShortcutBar_() {
		return get_text_msg(0x5C9 - msg_file_start);
	}

	static auto Useoutlinecolor_() {
		return get_text_msg(0x5CA - msg_file_start);
	}

	static auto Hidemissingitems_() {
		return get_text_msg(0x5CB - msg_file_start);
	}

	static auto TextBackground_() {
		return get_text_msg(0x5CC - msg_file_start);
	}

	static auto Smoothscrolling_() {
		return get_text_msg(0x5CD - msg_file_start);
	}

	static auto Skipintro_() {
		return get_text_msg(0x5CE - msg_file_start);
	}

	static auto Skipscriptedfirstscene_() {
		return get_text_msg(0x5CF - msg_file_start);
	}

	static auto UseextendedSIintro_() {
		return get_text_msg(0x5D0 - msg_file_start);
	}

	static auto Paperdolls_() {
		return get_text_msg(0x5D1 - msg_file_start);
	}

	static auto Androidautolaunch_() {
		return get_text_msg(0x5D2 - msg_file_start);
	}

	static auto Language_() {
		return get_text_msg(0x5D3 - msg_file_start);
	}

	static auto English() {
		return get_text_msg(0x5D4 - msg_file_start);
	}

	static auto French() {
		return get_text_msg(0x5D5 - msg_file_start);
	}

	static auto German() {
		return get_text_msg(0x5D6 - msg_file_start);
	}

	static auto Spanish() {
		return get_text_msg(0x5D7 - msg_file_start);
	}

	static auto Fonts_() {
		return get_text_msg(0x5D8 - msg_file_start);
	}

	static auto Original() {
		return get_text_msg(0x5D9 - msg_file_start);
	}

	static auto Serif() {
		return get_text_msg(0x5DA - msg_file_start);
	}

	// Keep the experimental labels self-contained for now. The generated
	// message resource is not rebuilt by every development build, which made
	// newly-added message IDs render as blank labels.
	static const char* Modernsmoothscrolling_() {
		return "Modern smooth scrolling:";
	}

	static const char* Smoothcameratau_() {
		return "Smooth camera tau:";
	}

	static const char* Rotateworld45deg_() {
		return "Rotate world 45 deg:";
	}

	static const char* Rotatequality_() {
		return "Rotate quality:";
	}

	static const char* Readableconversationfont_() {
		return "Readable conversation font:";
	}

	static const char* Conversationfontsize_() {
		return "Conversation font size:";
	}
};

using GameDisplayOptions_button = CallbackTextButton<GameDisplayOptions_gump>;
using GameDisplayTextToggle     = CallbackToggleTextButton<GameDisplayOptions_gump>;
using GameDisplayEnabledToggle  = CallbackEnabledButton<GameDisplayOptions_gump>;

static constexpr int modern_tau_values[] = {90, 120, 150, 180, 220, 300, 400, 500};
static constexpr int crt_width_values[] = {1, 2, 3, 4, 5, 6};

// Android stuff

// SDL3 native file dialogs are asynchronous. Keep their result separately
// from the modal gump, so closing a dialog while the picker is open is safe.
struct ConversationFontDialogResult {
	std::mutex mutex;
	std::string selected_path;
	bool pending = false;
};

static void SDLCALL conversation_font_picked(
		void* userdata, const char* const* files, int /*filter*/) {
	std::unique_ptr<std::shared_ptr<ConversationFontDialogResult>> state(
			static_cast<std::shared_ptr<ConversationFontDialogResult>*>(userdata));
	if (files && files[0] && files[0][0]) {
		{
			std::lock_guard<std::mutex> lock((*state)->mutex);
			(*state)->selected_path = files[0];
			(*state)->pending = true;
		}
		std::cout << "[FONT-PICKER] Accepted font file: " << files[0] << std::endl;
		// Wake the Exult event loop even when the cursor doesn't move.
		SDL_Event wake{};
		wake.type = SDL_EVENT_USER;
		SDL_PushEvent(&wake);
	} else {
		std::cerr << "[FONT-PICKER] " << (files ? "Canceled" : SDL_GetError()) << std::endl;
	}
}

bool (*GameDisplayOptions_gump::Android_getAutoLaunch)()     = nullptr;
void (*GameDisplayOptions_gump::Android_setAutoLaunch)(bool) = nullptr;

void GameDisplayOptions_gump::SetAndroidAutoLaunchFPtrs(void (*setter)(bool), bool (*getter)()) {
	Android_getAutoLaunch = getter;
	Android_setAutoLaunch = setter;
}

bool GameDisplayOptions_gump::is_dependent_option_inactive(button_ids id) const {
	if (page == Page::display) return modern_smooth && id == id_smooth_scrolling;
	if (page == Page::movement) return !modern_smooth && (id == id_smooth_avatar_walk || id == id_modern_tau);
	if (page == Page::rotation) return !rotate_world && id == id_rotate_sampling_mode;
	if (page == Page::crt) return !crt_enabled && id >= id_crt_horizontal_strength && id <= id_crt_beam_sigma;
	if (page == Page::fonts) return !conversation_font && id >= id_conversation_font_size && id <= id_conversation_font_reset;
	return false;
}

Gump_button* GameDisplayOptions_gump::on_button(int mx, int my) {
	for (size_t i = 0; i < buttons.size(); ++i) {
		if (is_dependent_option_inactive(static_cast<button_ids>(i))) continue;
		auto& btn = buttons[i];
		auto found = btn ? btn->on_button(mx, my) : nullptr;
		if (found) {
			return found;
		}
	}
	return Modal_gump::on_button(mx, my);
}

void GameDisplayOptions_gump::toggle_modern_smooth(int state) {
	modern_smooth = state;
	// Refresh dependent widgets without destroying the activated camera button.
	const std::vector<std::string> yesNo = {Strings::No(), Strings::Yes()};
	const int avatar_y = buttons[id_smooth_avatar_walk]->get_y();
	const int tau_y = buttons[id_modern_tau]->get_y();
	buttons[id_smooth_avatar_walk] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_smooth_avatar_walk,
			yesNo, smooth_avatar_walk,
			get_button_pos_for_label("Smooth avatar walk:"), avatar_y, 44);
	std::vector<std::string> tau_text;
	for (int ms : modern_tau_values) tau_text.emplace_back(std::to_string(ms) + " ms");
	buttons[id_modern_tau] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_modern_tau,
			std::move(tau_text), modern_tau,
			get_button_pos_for_label(Strings::Smoothcameratau_()), tau_y, 44);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
	gwin->set_all_dirty();
}

void GameDisplayOptions_gump::open_modern_scrolling() {
	GameDisplayOptions_gump child(Page::movement);
	gwin->get_gump_man()->do_modal_gump(&child, Mouse::hand);
	// The child owns the modern setting. Resync when it closes, including
	// cancellation, so the parent immediately fades/unfades legacy scrolling.
	modern_smooth = gwin->is_modern_movement_enabled() ? 1 : 0;
	gwin->set_all_dirty();
}

void GameDisplayOptions_gump::browse_conversation_font(bool installed) {
	static const SDL_DialogFileFilter filters[] = {
			{"Fonts (TTF, OTF, TTC)", "ttf;otf;ttc"},
			{"All files", "*"}};
	const char* start = nullptr;
	if (installed) {
#if defined(SDL_PLATFORM_WINDOWS)
		start = nullptr; // Windows uses the installed-family dialog instead.
#elif defined(SDL_PLATFORM_MACOS)
		start = "/System/Library/Fonts/";
#elif defined(SDL_PLATFORM_LINUX)
		start = "/usr/share/fonts/";
#endif
	} else if (!conversation_font_file.empty()) {
		// SDL's default_location may be a file, but the native Windows
		// picker can pre-fill an absolute filename while remaining in the
		// Documents folder (and refuse the Open action). Open its parent
		// directory instead; the user can select the file normally.
		static thread_local std::string last_directory;
		const size_t separator = conversation_font_file.find_last_of("/\\\\");
		last_directory = separator == std::string::npos ? std::string()
				: conversation_font_file.substr(0, separator + 1);
		if (!last_directory.empty()) start = last_directory.c_str();
	}
	// A system-font browse starts at the OS font directory; a custom browse
	// starts at the previously selected file, when present.
	auto* state = new std::shared_ptr<ConversationFontDialogResult>(font_dialog_result);
	SDL_ShowOpenFileDialog(conversation_font_picked, state,
			gwin->get_win()->get_screen_window(), filters, SDL_arraysize(filters), start, false);
}

void GameDisplayOptions_gump::choose_conversation_font_file() {
	browse_conversation_font(false);
}

void GameDisplayOptions_gump::choose_installed_conversation_font() {
#if defined(SDL_PLATFORM_WINDOWS)
	// The Fonts folder is a virtual Windows shell view, not a reliable
	// directory for SDL's ordinary file picker. ChooseFont enumerates
	// installed font families through the OS font subsystem instead.
	LOGFONTW selected{};
	selected.lfCharSet = DEFAULT_CHARSET;
	if (!conversation_font_family.empty()) {
		const int count = MultiByteToWideChar(CP_UTF8, 0, conversation_font_family.c_str(),
				-1, selected.lfFaceName, LF_FACESIZE);
		if (!count) selected.lfFaceName[0] = 0;
	}
	CHOOSEFONTW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.lpLogFont = &selected;
	dialog.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST;
	if (ChooseFontW(&dialog)) {
		char utf8[LF_FACESIZE * 4]{};
		if (WideCharToMultiByte(CP_UTF8, 0, selected.lfFaceName, -1,
				utf8, sizeof(utf8), nullptr, nullptr)) {
			conversation_font_family = utf8;
			conversation_font_file.clear(); // A newly chosen family overrides the previous file.
			std::cout << "[FONT-PICKER] Selected installed family: "
					<< conversation_font_family << std::endl;
			update_conversation_font_source_buttons();
		}
	} else {
		const DWORD error = CommDlgExtendedError();
		if (error) std::cerr << "[FONT-PICKER] ChooseFont error: " << error << std::endl;
	}
#else
	browse_conversation_font(true);
#endif
}

void GameDisplayOptions_gump::reset_conversation_font() {
	conversation_font_file.clear();
	conversation_font_family.clear();
	update_conversation_font_source_buttons();
}

void GameDisplayOptions_gump::update_conversation_font_source_buttons() {
	// Keep source buttons stable: their function is explicit, while the
	// currently chosen path is logged once at selection time.
	gwin->set_all_dirty();
}

void GameDisplayOptions_gump::open_readable_fonts() {
	GameDisplayOptions_gump child(Page::fonts);
	gwin->get_gump_man()->do_modal_gump(&child, Mouse::hand);
	gwin->set_all_dirty();
}

void GameDisplayOptions_gump::close() {
	save_settings();
	done = true;
}

void GameDisplayOptions_gump::cancel() {
	// A live preview must not survive cancelling the display options.
	gwin->preview_crt_filter_settings(
			gwin->is_crt_filter_enabled(),
			gwin->get_crt_horizontal_strength(),
			gwin->get_crt_vertical_strength(),
			gwin->get_crt_horizontal_compensation(),
			gwin->get_crt_horizontal_width(),
			gwin->get_crt_vertical_width(),
			gwin->get_crt_beam_sigma());
	done = true;
}

void GameDisplayOptions_gump::help() {
	SDL_OpenURL("https://exult.info/docs.html#game_display_gump");
}

void GameDisplayOptions_gump::build_buttons() {
	for (size_t i = id_first_setting; i < buttons.size(); ++i) buttons[i].reset();
	const std::vector<std::string> yesNo = {Strings::No(), Strings::Yes()};
	int y_index = page == Page::display ? 0 : -1;
	int small_size = 44, large_size = 85;
	if (page == Page::display) {
// Status Bar Positions
	std::vector<std::string> stats
			= {Strings::Disabled(), Strings::Left(), Strings::Middle(), Strings::Right(), Strings::Vertical()};
	buttons[id_facestats] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_facestats, std::move(stats), facestats,
			get_button_pos_for_label(Strings::StatusBars_()), yForRow(y_index), large_size);

	std::vector<std::string> sc_enabled_txt = {Strings::No(), Strings::Transparent(), Strings::Yes()};
	buttons[id_sc_enabled]                  = std::make_unique<GameDisplayTextToggle>(
            this, &GameDisplayOptions_gump::toggle_sc_enabled, std::move(sc_enabled_txt), sc_enabled,
            get_button_pos_for_label(Strings::UseShortcutBar_()), yForRow(++y_index), large_size);

	// keep in order of Pixel_colors
	// No needs to be last.
	sc_outline_txt         = std::vector<std::string>{Strings::Black(), Strings::Green(), Strings::White(),  Strings::Yellow(),
													  Strings::Blue(),  Strings::Red(),   Strings::Purple(), Strings::No()};
	buttons[id_sc_outline] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_sc_outline, sc_outline_txt, sc_outline,
			get_button_pos_for_label(Strings::Useoutlinecolor_()), yForRow(++y_index), small_size);

	buttons[id_sb_hide_missing] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_sb_hide_missing, yesNo, sb_hide_missing,
			get_button_pos_for_label(Strings::Hidemissingitems_()), yForRow(++y_index), small_size);

	std::vector<std::string> textbgcolor
			= {Strings::Disabled(),    Strings::SolidGray(), Strings::DarkPurple(), Strings::BrightYellow(), Strings::LightBlue(),
			   Strings::LightGreen(),  Strings::DarkRed(),   Strings::Purple(),     Strings::Orange(),       Strings::LightGray(),
			   Strings::Green(),       Strings::Yellow(),    Strings::PaleBlue(),   Strings::DarkGreen(),    Strings::Red(),
			   Strings::BrightWhite(), Strings::DarkGray(),  Strings::White()};
	buttons[id_text_bg] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_text_bg, std::move(textbgcolor), text_bg,
			get_button_pos_for_label(Strings::TextBackground_()), yForRow(++y_index), large_size);

	update_legacy_smooth_button();
	++y_index;  // Keep the original Smooth scrolling row in Game Display.
	buttons[id_nav_movement] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::open_modern_scrolling,
			"Set...", get_button_pos_for_label("Modern smooth scrolling:"), yForRow(++y_index), small_size);

	buttons[id_menu_intro] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_menu_intro, yesNo, menu_intro, get_button_pos_for_label(Strings::Skipintro_()),
			yForRow(++y_index), small_size);

	if (GAME_BG || gwin->is_in_exult_menu()) {
		buttons[id_usecode_intro] = std::make_unique<GameDisplayTextToggle>(
				this, &GameDisplayOptions_gump::toggle_usecode_intro, yesNo, usecode_intro,
				get_button_pos_for_label(Strings::Skipscriptedfirstscene_()), yForRow(++y_index), small_size);
	}
	if (GAME_SI || gwin->is_in_exult_menu()) {
		buttons[id_extended_intro] = std::make_unique<GameDisplayTextToggle>(
				this, &GameDisplayOptions_gump::toggle_extended_intro, yesNo, extended_intro,
				get_button_pos_for_label(Strings::UseextendedSIintro_()), yForRow(++y_index), small_size);
	}

	if (sman->can_use_paperdolls() && (GAME_BG || Game::get_game_type() == EXULT_DEVEL_GAME)) {
		buttons[id_paperdolls] = std::make_unique<GameDisplayTextToggle>(
				this, &GameDisplayOptions_gump::toggle_paperdolls, yesNo, paperdolls,
				get_button_pos_for_label(Strings::Paperdolls_()), yForRow(++y_index), small_size);
	}
	// Android
	if (Android_getAutoLaunch) {
		buttons[id_android_autolaunch] = std::make_unique<GameDisplayTextToggle>(
				this, &GameDisplayOptions_gump::toggle_android_launcher, yesNo, android_autolaunch,
				get_button_pos_for_label(Strings::Androidautolaunch_()), yForRow(++y_index), small_size);
	}

	auto languages_txt = std::vector<std::string>{
			Strings::Default(), Strings::English(), Strings::French(), Strings::German(), Strings::Spanish()};
	buttons[id_language] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_language, languages_txt, language,
			get_button_pos_for_label(Strings::Language_()), yForRow(++y_index), large_size);
auto fonts_txt    = std::vector<std::string>{Strings::Original(), Strings::Serif(), Strings::Disabled()};
	buttons[id_fonts] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_fonts, fonts_txt, fonts, get_button_pos_for_label(Strings::Fonts_()),
			yForRow(++y_index), large_size);
	buttons[id_nav_fonts] = std::make_unique<GameDisplayOptions_button>(
		this, &GameDisplayOptions_gump::open_readable_fonts, "Set...",
		get_button_pos_for_label("Readable fonts:"), yForRow(++y_index), small_size);
	}
	if (page == Page::movement) {

	buttons[id_modern_smooth] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_modern_smooth, yesNo, modern_smooth,
			get_button_pos_for_label(Strings::Modernsmoothscrolling_()), yForRow(++y_index), small_size);
	buttons[id_smooth_avatar_walk] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_smooth_avatar_walk,
			yesNo, smooth_avatar_walk,
			get_button_pos_for_label("Smooth avatar walk:"), yForRow(++y_index), small_size);

	std::vector<std::string> tau_text;
	for (int value : modern_tau_values) {
		tau_text.emplace_back(std::to_string(value) + " ms");
	}
	buttons[id_modern_tau] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_modern_tau,
			std::move(tau_text), modern_tau,
			get_button_pos_for_label(Strings::Smoothcameratau_()), yForRow(++y_index), small_size);

	}
	if (page == Page::crt) {
buttons[id_crt_enabled] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_crt_enabled, yesNo, crt_enabled,
			get_button_pos_for_label("CRT filter:"), yForRow(++y_index), small_size);

	buttons[id_crt_horizontal_strength] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_horizontal_strength,
			std::to_string(crt_horizontal_strength) + "%",
			get_button_pos_for_label("CRT horizontal scanlines:"), yForRow(++y_index), small_size);
	buttons[id_crt_vertical_strength] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_vertical_strength,
			(std::to_string(crt_vertical_strength / 2) + (crt_vertical_strength % 2 ? ".5%" : "%")),
			get_button_pos_for_label("CRT vertical mask:"), yForRow(++y_index), small_size);

	buttons[id_crt_brightness_compensation] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_brightness_compensation,
			std::to_string(crt_brightness_compensation) + "%",
			get_button_pos_for_label("CRT brightness comp:"), yForRow(++y_index), small_size);

	std::vector<std::string> crt_width_text;
	for (const int value : crt_width_values) {
		crt_width_text.emplace_back(std::to_string(value) + " px");
	}
	buttons[id_crt_horizontal_width] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_crt_horizontal_width, crt_width_text, crt_horizontal_width,
			get_button_pos_for_label("CRT H line width:"), yForRow(++y_index), small_size);
	buttons[id_crt_vertical_width] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_crt_vertical_width, std::move(crt_width_text), crt_vertical_width,
			get_button_pos_for_label("CRT V mask width:"), yForRow(++y_index), small_size);

	buttons[id_crt_beam_sigma] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_sigma,
			std::to_string(crt_beam_sigma / 100.0f).substr(0, 4),
			get_button_pos_for_label("CRT beam sigma:"), yForRow(++y_index), small_size);
	}
	if (page == Page::rotation) {
buttons[id_rotate_world] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_rotate_world, yesNo, rotate_world,
			get_button_pos_for_label(Strings::Rotateworld45deg_()), yForRow(++y_index), small_size);

	std::vector<std::string> rotate_quality_text = {
			"2x / 4 samples", "3x / 4 samples", "3x / 9 samples"};
#ifdef USE_HQ3X_SCALER
	rotate_quality_text.emplace_back("HQ3x / 9 samples");
	rotate_quality_text.emplace_back("HQ3x / 4 samples");
	rotate_quality_text.emplace_back("Forward weighted triplets");
	rotate_quality_text.emplace_back("Forward weighted pairs");
	rotate_quality_text.emplace_back("Forward weighted quadruplets");
#endif
	buttons[id_rotate_sampling_mode] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_rotate_sampling_mode,
			std::move(rotate_quality_text), rotate_sampling_mode,
			get_button_pos_for_label(Strings::Rotatequality_()), yForRow(++y_index), large_size);
	}
	if (page == Page::fonts) {
buttons[id_conversation_font] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_conversation_font, yesNo, conversation_font,
			get_button_pos_for_label(Strings::Readableconversationfont_()), yForRow(++y_index), small_size);

	buttons[id_conversation_font_size] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_conversation_font_size,
			std::to_string(conversation_font_size) + " px",
			get_button_pos_for_label(Strings::Conversationfontsize_()), yForRow(++y_index), small_size);
	buttons[id_conversation_font_tracking] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_conversation_font_tracking,
			std::vector<std::string>{"Normal", "+1 px"},
			conversation_font_tracking, get_button_pos_for_label("Character spacing:"),
			yForRow(++y_index), small_size);
	buttons[id_conversation_font_file] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_conversation_font_file,
			"Browse...", get_button_pos_for_label("TTF / OTF file:"), yForRow(++y_index), 65);
	buttons[id_conversation_system_font] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_installed_conversation_font,
			"Browse...", get_button_pos_for_label("Installed fonts:"), yForRow(++y_index), 65);
	buttons[id_conversation_font_reset] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::reset_conversation_font,
			"Reset", get_button_pos_for_label("Restore automatic font:"), yForRow(++y_index), 65);
	}
	constexpr int margin = 4;
	const int footer_row = page == Page::display ? 14 :
		page == Page::movement ? 5 : page == Page::crt ? 9 : page == Page::fonts ? 7 : 4;
	buttons[id_ok]->set_pos(margin, yForRow(footer_row));
	buttons[id_help]->set_pos(margin + 50, yForRow(footer_row));
	buttons[id_cancel]->set_pos(margin + 100, yForRow(footer_row));
	SetProceduralBackground(TileRect(0, yForRow(0) - margin, 100,
		yForRow(footer_row + 1) - yForRow(0) + 2 * margin), -1, true);
	ResizeWidthToFitWidgets(tcb::span(buttons.data() + id_first, id_count), margin);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
	HorizontalArrangeWidgets(tcb::span(buttons.data() + id_ok, 3));
}

void GameDisplayOptions_gump::preview_crt() {
	gwin->preview_crt_filter_settings(
			crt_enabled != 0, crt_horizontal_strength, crt_vertical_strength,
			crt_brightness_compensation, crt_horizontal_width, crt_vertical_width, crt_beam_sigma);
}

void GameDisplayOptions_gump::choose_crt_sigma() {
	bool escaped = false;
	const int previous = crt_beam_sigma;
	const int value = gwin->get_gump_man()->prompt_for_number(
			10, 100, 1, previous, this, &escaped,
			[this](int v) { crt_beam_sigma = v; preview_crt(); });
	crt_beam_sigma = escaped ? previous : value;
	preview_crt();
	update_crt_sigma_button();
}

void GameDisplayOptions_gump::update_crt_sigma_button() {
	constexpr int small_size = 44;
	const int button_y = buttons[id_crt_beam_sigma]->get_y();
	buttons[id_crt_beam_sigma] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_sigma,
			std::to_string(crt_beam_sigma / 100.0f).substr(0, 4),
			get_button_pos_for_label("CRT beam sigma:"), button_y, small_size);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

void GameDisplayOptions_gump::choose_crt_brightness_compensation() {
	bool escaped = false;
	const int original = crt_brightness_compensation;
	const int value = gwin->get_gump_man()->prompt_for_number(
			50, 150, 1, original, this, &escaped,
			[this](int v) {
				crt_brightness_compensation = v;
				preview_crt();
			});
	crt_brightness_compensation = escaped ? original : value;
	preview_crt();
	update_crt_compensation_button();
}

void GameDisplayOptions_gump::update_crt_compensation_button() {
	constexpr int small_size = 44;
	const int button_y = buttons[id_crt_brightness_compensation]->get_y();
	buttons[id_crt_brightness_compensation] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_crt_brightness_compensation,
			std::to_string(crt_brightness_compensation) + "%",
			get_button_pos_for_label("CRT brightness comp:"), button_y, small_size);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

void GameDisplayOptions_gump::choose_crt_horizontal_strength() {
	bool escaped = false;
	const int original = crt_horizontal_strength;
	const int value = gwin->get_gump_man()->prompt_for_number(
			0, 20, 1, crt_horizontal_strength, this, &escaped,
			[this](int v) { crt_horizontal_strength = v; preview_crt(); });
	if (!escaped) {
		crt_horizontal_strength = value;
		update_crt_strength_buttons();
		gwin->set_all_dirty();
	} else {
		crt_horizontal_strength = original;
		preview_crt();
	}
}

void GameDisplayOptions_gump::choose_crt_vertical_strength() {
	bool escaped = false;
	const int original = crt_vertical_strength;
	const int value = gwin->get_gump_man()->prompt_for_number(
			0, 40, 1, crt_vertical_strength, this, &escaped,
			[this](int v) { crt_vertical_strength = v; preview_crt(); });
	if (!escaped) {
		crt_vertical_strength = value;
		update_crt_strength_buttons();
		gwin->set_all_dirty();
	} else {
		crt_vertical_strength = original;
		preview_crt();
	}
}

void GameDisplayOptions_gump::update_crt_strength_buttons() {
	constexpr int small_size = 44;
	if (buttons[id_crt_horizontal_strength]) {
		const int button_y = buttons[id_crt_horizontal_strength]->get_y();
		buttons[id_crt_horizontal_strength] = std::make_unique<GameDisplayOptions_button>(
				this, &GameDisplayOptions_gump::choose_crt_horizontal_strength,
				std::to_string(crt_horizontal_strength) + "%",
				get_button_pos_for_label("CRT horizontal scanlines:"), button_y, small_size);
	}
	if (buttons[id_crt_vertical_strength]) {
		const int button_y = buttons[id_crt_vertical_strength]->get_y();
		buttons[id_crt_vertical_strength] = std::make_unique<GameDisplayOptions_button>(
				this, &GameDisplayOptions_gump::choose_crt_vertical_strength,
				(std::to_string(crt_vertical_strength / 2) + (crt_vertical_strength % 2 ? ".5%" : "%")),
				get_button_pos_for_label("CRT vertical mask:"), button_y, small_size);
	}
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

void GameDisplayOptions_gump::choose_conversation_font_size() {
	bool escaped = false;
	const int value = gwin->get_gump_man()->prompt_for_number(
			5, 32, 1, conversation_font_size, this, &escaped);
	if (!escaped) {
		conversation_font_size = value;
		update_conversation_font_size_button();
		gwin->set_all_dirty();
	}
}

void GameDisplayOptions_gump::update_conversation_font_size_button() {
	constexpr int small_size = 44;
	// The exact row depends on which game/platform-specific options are present.
	// Reuse the current button's y coordinate instead of assuming a fixed row.
	const int button_y = buttons[id_conversation_font_size]
			? buttons[id_conversation_font_size]->get_y()
			: yForRow(16);
	buttons[id_conversation_font_size] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::choose_conversation_font_size,
			std::to_string(conversation_font_size) + " px",
			get_button_pos_for_label(Strings::Conversationfontsize_()), button_y, small_size);
	RightAlignWidgets(tcb::span(buttons.data() + id_first_setting, id_count - id_first_setting));
}

void GameDisplayOptions_gump::update_legacy_smooth_button() {
	constexpr int legacy_row = 5;
	const int small_size = 44;
	// Preserve the actual original setting even while the modern algorithm
	// makes it inactive. Opacity and click suppression express that state.
	std::vector<std::string> smooth_text = {Strings::No(), "25%", "50%", "75%", "100%"};
	buttons[id_smooth_scrolling] = std::make_unique<GameDisplayTextToggle>(
			this, &GameDisplayOptions_gump::toggle_smooth_scrolling, std::move(smooth_text), smooth_scrolling,
			get_button_pos_for_label(Strings::Smoothscrolling_()), yForRow(legacy_row), small_size);
}

void GameDisplayOptions_gump::load_settings() {
	string value;
	config->value("config/gameplay/skip_intro", value, "no");
	usecode_intro = (value == "yes");
	config->value("config/gameplay/extended_intro", value, "no");
	extended_intro = (value == "yes");
	config->value("config/gameplay/skip_splash", value, "no");
	menu_intro      = (value == "yes");
	sc_enabled      = gwin->get_shortcutbar_type();
	sc_outline      = gwin->get_outline_color();
	sb_hide_missing = gwin->sb_hide_missing_items();
	if (gwin->is_in_exult_menu()) {
		config->value("config/gameplay/facestats", facestats, -1);
		facestats += 1;
	} else {
		facestats = Face_stats::get_state() + 1;
	}
	paperdolls = false;
	const string pdolls;
	paperdolls       = sman->are_paperdolls_enabled();
	text_bg          = gwin->get_text_bg() + 1;
	smooth_scrolling = gwin->is_lerping_enabled() / 25;
	modern_smooth    = gwin->is_modern_movement_enabled() ? 1 : 0;
	smooth_avatar_walk = gwin->is_smooth_avatar_walk_enabled() ? 1 : 0;
	modern_mouse_target = gwin->is_modern_mouse_target_enabled() ? 1 : 0;
	rotate_world         = gwin->is_rotate_world_enabled() ? 1 : 0;
	rotate_sampling_mode = gwin->get_rotate_sampling_mode();
	crt_enabled = gwin->is_crt_filter_enabled() ? 1 : 0;

	const auto nearest_index = [](int value, const int* values, size_t count) {
		size_t best = 0;
		for (size_t i = 1; i < count; ++i) {
			if (std::abs(values[i] - value) < std::abs(values[best] - value)) {
				best = i;
			}
		}
		return static_cast<int>(best);
	};
	crt_horizontal_strength = std::clamp(gwin->get_crt_horizontal_strength(), 0, 20);
	crt_vertical_strength = std::clamp(gwin->get_crt_vertical_strength(), 0, 40);
	crt_brightness_compensation = std::clamp(gwin->get_crt_horizontal_compensation(), 50, 150);
	crt_beam_sigma = std::clamp(gwin->get_crt_beam_sigma(), 10, 100);
	crt_horizontal_width = nearest_index(
			gwin->get_crt_horizontal_width(), crt_width_values, std::size(crt_width_values));
	crt_vertical_width = nearest_index(
			gwin->get_crt_vertical_width(), crt_width_values, std::size(crt_width_values));

	const int tau_ms = gwin->get_modern_movement_tau_ms();
	modern_tau = 0;
	for (size_t i = 1; i < std::size(modern_tau_values); ++i) {
		if (std::abs(modern_tau_values[i] - tau_ms) < std::abs(modern_tau_values[modern_tau] - tau_ms)) {
			modern_tau = static_cast<int>(i);
		}
	}

	android_autolaunch = Android_getAutoLaunch ? Android_getAutoLaunch() : 0;
	config->value("config/gameplay/language", value, "");
	Pentagram::tolower(value);
	if (value == "en") {
		language = 1;
	} else if (value == "fr") {
		language = 2;
	} else if (value == "de") {
		language = 3;
	} else if (value == "es") {
		language = 4;
	} else {
		language = 0;
	}

	config->value("config/gameplay/fonts", value, "original");
	Pentagram::tolower(value);
	if (value == "serif") {
		fonts = 1;
	} else if (value == "disabled") {
		fonts = 2;
	} else {
		fonts = 0;    // original
	}
	bool conversation_font_enabled = false;
	config->value("config/gameplay/conversation_font/enabled", conversation_font_enabled, false);
	config->value("config/gameplay/conversation_font/file", conversation_font_file, "");
	config->value("config/gameplay/conversation_font/family", conversation_font_family, "");
	font_dialog_result = std::make_shared<ConversationFontDialogResult>();
	conversation_font = conversation_font_enabled ? 1 : 0;
	modern_keyboard = gwin->is_modern_keyboard_enabled() ? 1 : 0;

	conversation_font_default_size = 17;
	conversation_font_size = conversation_font_default_size;
	config->value("config/gameplay/conversation_font/pixels", conversation_font_size, conversation_font_default_size);
	config->value("config/gameplay/conversation_font/tighter_spacing", conversation_font_tracking, 0);
	// Old half-pixel values become Normal; the former +1 value remains +1.
	conversation_font_tracking = conversation_font_tracking >= 2 ? 1 : 0;
	conversation_font_size = std::clamp(conversation_font_size, 5, 32);
}

GameDisplayOptions_gump::GameDisplayOptions_gump(Page section) : Modal_gump(nullptr, -1), page(section) {
	SetProceduralBackground(TileRect(0, 0, 100, yForRow(12)), -1);

	for (auto& btn : buttons) {
		btn.reset();
	}

	// Ok
	buttons[id_ok] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::close, Strings::OK(), 15, yForRow(11), 50);
	// Help
	buttons[id_help] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::help, Strings::HELP(), 50, yForRow(11), 50);
	// Cancel
	buttons[id_cancel] = std::make_unique<GameDisplayOptions_button>(
			this, &GameDisplayOptions_gump::cancel, Strings::CANCEL(), 75, yForRow(11), 50);

	load_settings();
	build_buttons();
}

void GameDisplayOptions_gump::save_settings() {
	if (page == Page::display) {

	if (gwin->is_in_exult_menu()) {
		config->set("config/gameplay/facestats", facestats - 1, false);
	} else {
		while (facestats != Face_stats::get_state() + 1) {
			Face_stats::AdvanceState();
		}
		Face_stats::save_config(config);
	}
	string str = "no";
	if (sc_enabled == 1) {
		str = "transparent";
	} else if (sc_enabled == 2) {
		str = "yes";
	}
	config->set("config/shortcutbar/use_shortcutbar", str, false);
	config->set("config/shortcutbar/use_outline_color", sc_outline_txt[sc_outline], false);
	config->set("config/shortcutbar/hide_missing_items", sb_hide_missing ? "yes" : "no", false);
	gwin->set_outline_color(static_cast<Pixel_colors>(sc_outline));
	gwin->set_sb_hide_missing_items(sb_hide_missing);
	gwin->set_shortcutbar(static_cast<uint8>(sc_enabled));
	if (g_shortcutBar) {
		g_shortcutBar->set_changed();
	}
	gwin->set_text_bg(text_bg - 1);
	config->set("config/gameplay/textbackground", text_bg - 1, false);
	if (smooth_scrolling < 0) {
		smooth_scrolling = 0;
	} else if (smooth_scrolling > 4) {
		smooth_scrolling = 4;
	}
	gwin->set_lerping_enabled(smooth_scrolling * 25);
	config->set("config/gameplay/smooth_scrolling", smooth_scrolling * 25, false);
	}
	if (page == Page::movement) {

	gwin->set_modern_movement_tau_ms(modern_tau_values[std::clamp(modern_tau, 0, static_cast<int>(std::size(modern_tau_values)) - 1)]);
	gwin->set_smooth_scrolling_enabled(modern_smooth != 0);
	gwin->set_smooth_avatar_walk_enabled(smooth_avatar_walk != 0);

	}
	if (page == Page::rotation) {

	gwin->set_rotate_sampling_mode(rotate_sampling_mode);
	gwin->set_rotate_world_enabled(rotate_world != 0);
	}
	if (page == Page::crt) {

	gwin->set_crt_filter_settings(
			crt_enabled != 0,
			std::clamp(crt_horizontal_strength, 0, 20),
			std::clamp(crt_vertical_strength, 0, 40),
			crt_brightness_compensation,
			crt_brightness_compensation,
			crt_width_values[std::clamp(crt_horizontal_width, 0, static_cast<int>(std::size(crt_width_values)) - 1)],
			crt_width_values[std::clamp(crt_vertical_width, 0, static_cast<int>(std::size(crt_width_values)) - 1)], crt_beam_sigma);
	}
	if (page == Page::display) {

	config->set("config/gameplay/skip_intro", usecode_intro ? "yes" : "no", false);
	config->set("config/gameplay/extended_intro", extended_intro ? "yes" : "no", false);
	gwin->set_extended_intro(extended_intro);
	config->set("config/gameplay/skip_splash", menu_intro ? "yes" : "no", false);
	if (sman->can_use_paperdolls() && (GAME_BG || Game::get_game_type() == EXULT_DEVEL_GAME)) {
		sman->set_paperdoll_status(paperdolls != 0);
		config->set("config/gameplay/bg_paperdolls", paperdolls ? "yes" : "no", false);
	}
	if (Android_setAutoLaunch) {
		Android_setAutoLaunch(android_autolaunch != 0);
	}

	const char* langcodes[] = {"", "en", "fr", "de", "es"};
	if (language >= 0 && size_t(language) < std::size(langcodes)) {
		config->set("config/gameplay/language", langcodes[language], false);

		// Setup text incase language changed
		Game::setup_text();
	}

	const char* fontcodes[] = {"original", "serif", "disabled"};
	if (fonts >= 0 && size_t(fonts) < std::size(fontcodes)) {
		config->set("config/gameplay/fonts", fontcodes[fonts], false);
	}
		Game::setup_fonts();
		Game::setup_text();
	}
	if (page == Page::fonts) {

	config->set("config/gameplay/conversation_font/enabled", conversation_font ? "yes" : "no", false);
	config->set("config/gameplay/conversation_font/pixels", conversation_font_size, false);
	config->set("config/gameplay/conversation_font/tighter_spacing", conversation_font_tracking * 2, false);
	config->set("config/gameplay/conversation_font/file", conversation_font_file, false);
	config->set("config/gameplay/conversation_font/family", conversation_font_family, false);
	// Reload fonts after both font-related settings have been stored.
	Game::setup_fonts();
	// Re-translate text messages with the correct UTF-8 map.
	Game::setup_text();

	}
	config->write_back();
}

void GameDisplayOptions_gump::paint() {
	if (page == Page::fonts && font_dialog_result) {
		std::string selection;
		{
			std::lock_guard<std::mutex> lock(font_dialog_result->mutex);
			if (font_dialog_result->pending) {
				selection = std::move(font_dialog_result->selected_path);
				font_dialog_result->pending = false;
			}
		}
		if (!selection.empty()) {
			conversation_font_file = selection;
			conversation_font_family.clear(); // Explicit file always wins.
			std::cout << "Readable conversation font selected: " << selection << std::endl;
			update_conversation_font_source_buttons();
		}
	}
	Modal_gump::paint();
	Image_window8* iwin = gwin->get_win();
	Image_buffer8* framebuffer = iwin->get_ib8();
	// Save the actual background under disabled dependent controls. Fade the
	// finished label AND button against it, without changing label lengths,
	// layout, or the stored Yes/No/tau values.
	struct FadedRow {
		int x, y, width, height;
		std::vector<unsigned char> backdrop;
	};
	std::vector<FadedRow> faded_rows;
	{
		for (size_t index = id_first_setting; index < buttons.size(); ++index) {
			const button_ids id = static_cast<button_ids>(index);
			if (!is_dependent_option_inactive(id)) continue;
			if (!buttons[id]) continue;
			const int row_y = y + buttons[id]->get_y();
			const int left = std::clamp<int>(x + label_margin, 0, static_cast<int>(framebuffer->get_width()));
			const int right = std::clamp<int>(x + get_rect().w - 4, 0, static_cast<int>(framebuffer->get_width()));
			const int top = std::clamp<int>(row_y, 0, static_cast<int>(framebuffer->get_height()));
			// Rows here are 12 px apart: a 16 px fade rectangle overlaps
			// the following row and applies the 50% blend twice there.
			const int row_height = yForRow(1) - yForRow(0);
			const int bottom = std::clamp<int>(row_y + row_height, 0, static_cast<int>(framebuffer->get_height()));
			if (right <= left || bottom <= top) continue;
			FadedRow row{left, top, right - left, bottom - top, {}};
			row.backdrop.reserve(static_cast<size_t>(row.width) * row.height);
			for (int yy = top; yy < bottom; ++yy)
				for (int xx = left; xx < right; ++xx)
					row.backdrop.push_back(framebuffer->get_pixel8(xx, yy));
			faded_rows.push_back(std::move(row));
		}
		// The selected font family/file summary is a separate text row.
		// Include it in the same one-time half-opacity compositing pass.
		if (page == Page::fonts && !conversation_font) {
			const int top = std::clamp<int>(y + yForRow(6), 0, static_cast<int>(framebuffer->get_height()));
			const int bottom = std::clamp<int>(top + yForRow(1) - yForRow(0), 0, static_cast<int>(framebuffer->get_height()));
			const int left = std::clamp<int>(x + label_margin, 0, static_cast<int>(framebuffer->get_width()));
			const int right = std::clamp<int>(x + get_rect().w - 4, 0, static_cast<int>(framebuffer->get_width()));
			if (left < right && top < bottom) {
				FadedRow row{left, top, right - left, bottom - top, {}};
				row.backdrop.reserve(static_cast<size_t>(row.width) * row.height);
				for (int yy = top; yy < bottom; ++yy)
					for (int xx = left; xx < right; ++xx)
						row.backdrop.push_back(framebuffer->get_pixel8(xx, yy));
				faded_rows.push_back(std::move(row));
			}
		}
	}
	for (auto& btn : buttons) {
		if (btn) btn->paint();
	}
	auto draw_label = [&](button_ids id, const char* label) {
		if (buttons[id]) {
			font->paint_text(iwin->get_ib8(), label, x + label_margin, y + buttons[id]->get_y() + 1);
		}
	};
	draw_label(id_facestats, Strings::StatusBars_());
	draw_label(id_sc_enabled, Strings::UseShortcutBar_());
	draw_label(id_sc_outline, Strings::Useoutlinecolor_());
	draw_label(id_sb_hide_missing, Strings::Hidemissingitems_());
	draw_label(id_text_bg, Strings::TextBackground_());
	draw_label(id_smooth_scrolling, Strings::Smoothscrolling_());
	draw_label(id_nav_movement, "Modern smooth scrolling:");
	draw_label(id_modern_smooth, Strings::Modernsmoothscrolling_());
	draw_label(id_smooth_avatar_walk, "Smooth avatar walk:");


	draw_label(id_modern_tau, Strings::Smoothcameratau_());
	draw_label(id_rotate_world, Strings::Rotateworld45deg_());
	draw_label(id_rotate_sampling_mode, Strings::Rotatequality_());
	draw_label(id_crt_enabled, "CRT filter:");
	draw_label(id_crt_horizontal_strength, "CRT horizontal scanlines:");
	draw_label(id_crt_vertical_strength, "CRT vertical mask:");
	draw_label(id_crt_brightness_compensation, "CRT brightness comp:");
	draw_label(id_crt_horizontal_width, "CRT H line width:");
	draw_label(id_crt_vertical_width, "CRT V mask width:");
	draw_label(id_crt_beam_sigma, "CRT beam sigma:");
	draw_label(id_menu_intro, Strings::Skipintro_());
	draw_label(id_usecode_intro, Strings::Skipscriptedfirstscene_());
	draw_label(id_extended_intro, Strings::UseextendedSIintro_());
	draw_label(id_paperdolls, Strings::Paperdolls_());
	draw_label(id_android_autolaunch, Strings::Androidautolaunch_());
	draw_label(id_language, Strings::Language_());
	draw_label(id_fonts, Strings::Fonts_());
	draw_label(id_nav_fonts, "Readable fonts:");
	draw_label(id_conversation_font, Strings::Readableconversationfont_());
	draw_label(id_conversation_font_size, Strings::Conversationfontsize_());
	draw_label(id_conversation_font_tracking, "Character spacing:");
	draw_label(id_conversation_font_file, "TTF / OTF file:");
	draw_label(id_conversation_system_font, "Installed fonts:");
	draw_label(id_conversation_font_reset, "Font selection:");
	if (page == Page::fonts) {
		std::string chosen = "Automatic system font";
		if (!conversation_font_file.empty()) {
			auto last = conversation_font_file.find_last_of("/\\\\");
			chosen = "Selected: " + conversation_font_file.substr(last == std::string::npos ? 0 : last + 1);
		} else if (!conversation_font_family.empty()) {
			chosen = "Family: " + conversation_font_family;
		}
		// Long absolute paths should not enlarge the dialog or overlap the footer.
		if (chosen.size() > 42) chosen = chosen.substr(0, 39) + "...";
		font->paint_text(iwin->get_ib8(), chosen.c_str(),
				x + label_margin, y + yForRow(6) + 1);
	}
	// Composite the disabled controls at half opacity onto the already
	// painted gump background. All other options retain normal rendering.
	if (!faded_rows.empty()) {
		const Palette* palette = gwin->get_pal();
		if (palette) {
			std::array<int, 65536> blend_cache;
			blend_cache.fill(-1);
			for (const auto& row : faded_rows) {
				for (int yy = 0; yy < row.height; ++yy) {
					for (int xx = 0; xx < row.width; ++xx) {
						const unsigned char bg = row.backdrop[static_cast<size_t>(yy) * row.width + xx];
						const unsigned char fg = framebuffer->get_pixel8(row.x + xx, row.y + yy);
						if (bg == fg) continue;
						const unsigned int key = (static_cast<unsigned int>(fg) << 8) | bg;
						int color = blend_cache[key];
						if (color < 0) {
							color = palette->find_color(
									(palette->get_red(fg) + palette->get_red(bg) + 1) / 2,
									(palette->get_green(fg) + palette->get_green(bg) + 1) / 2,
									(palette->get_blue(fg) + palette->get_blue(bg) + 1) / 2);
							blend_cache[key] = color;
						}
						framebuffer->put_pixel8(static_cast<unsigned char>(color), row.x + xx, row.y + yy);
					}
				}
			}
		}
	}

	gwin->set_painted();
}
