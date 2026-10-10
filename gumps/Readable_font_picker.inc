// Readable conversation font selection. Included in the display options TU.
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
			update_conversation_font_source_buttons();
		}
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

