// Runtime readable font generation; compiled within fontgen.cc.
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
