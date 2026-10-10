// Optional custom conversation font loader; included by shapeid.cc.
static std::shared_ptr<Font> make_custom_conversation_font(Fonts_vga_file* fonts, const Palette& pal) {
	if (!fonts) {
		return nullptr;
	}

	bool enabled = false;
	config->value("config/gameplay/conversation_font/enabled", enabled, false);
	if (!enabled) {
		return nullptr;
	}

	std::string file;
	std::string family;
	config->value("config/gameplay/conversation_font/file", file, "");
	config->value("config/gameplay/conversation_font/family", family, "");
	const bool automatic_family = family.empty() && file.empty();
	if (automatic_family) {
		family = "Tahoma";
	}

	// Gen_shadow adds one pixel on every side, so subtract two from the
	// original font's total height to keep the final outlined result close to
	// Exult's existing conversation text size.
	int pixels = 17;
	config->value("config/gameplay/conversation_font/pixels", pixels, pixels);
	pixels = std::clamp(pixels, 5, 32);

	// Each generated glyph reserves two pixels of outline on BOTH sides.
	// Its bounding-box width therefore contains four non-advance pixels.
	// Compensate those four pixels in tracking, independently of the extra
	// user-selected 0 / -0.5 / -1 px adjustment.
	int hlead = -4;
	int vlead = 0;
	config->value("config/gameplay/conversation_font/hlead", hlead, -4);
	config->value("config/gameplay/conversation_font/vlead", vlead, 0);

	// Match the classic conversation colour scheme: bright yellow glyphs and
	// transparent background, with a stronger two-pixel black outline for
	// readability over the world.
	const unsigned char fg     = static_cast<unsigned char>(pal.find_color(63, 63, 5));
	const unsigned char bg     = 255;
	const unsigned char shadow = static_cast<unsigned char>(pal.find_color(0, 0, 0));

	const std::string font_path = file.empty() ? std::string() : get_system_path(file);
	std::string resolved_font_path = font_path;
	auto generated = Gen_runtime_font_shape(
			font_path.c_str(), family.c_str(), 256, pixels, fg, bg, shadow, 2);
#if defined(__APPLE__)
	// Unlike Win32 GDI, FreeType does not resolve a font family name by
	// itself. When the user enables readable text without configuring a file
	// or family, pick a preinstalled macOS sans-serif font automatically.
	// Keep explicit user choices untouched, including their failure fallback.
	if (!generated && automatic_family) {
		const char* const mac_fonts[] = {
				"/System/Library/Fonts/Helvetica.ttc",
				"/System/Library/Fonts/Supplemental/Arial.ttf",
				"/Library/Fonts/Arial.ttf",
				"/System/Library/Fonts/Supplemental/Verdana.ttf"};
		for (const char* candidate : mac_fonts) {
			generated = Gen_runtime_font_shape(candidate, "", 256, pixels, fg, bg, shadow, 2);
			if (generated) {
				resolved_font_path = candidate;
				std::cout << "Using macOS system conversation font " << candidate << " at " << pixels << " px." << std::endl;
				break;
			}
		}
	}
#endif
	if (!generated) {
		std::cerr << "Unable to load custom conversation font";
		if (!file.empty()) {
			std::cerr << " file '" << file << "'";
		}
		if (!family.empty()) {
			std::cerr << " family '" << family << "'";
		}
		std::cerr << "; using the normal Exult font." << std::endl;
		return nullptr;
	}

	auto result = std::make_shared<Font>(std::move(generated), hlead, vlead);
	// Generate the same padded outlines with the shadow painted transparent.
	// This keeps identical glyph metrics, but lets Font draw all fills on top
	// after drawing the outlined glyphs of the complete string.
	auto fill_only = Gen_runtime_font_shape(
			resolved_font_path.c_str(), family.c_str(), 256, pixels, fg, bg, bg, 2);
	if (fill_only) {
		result->set_fill_only_shapes(std::move(fill_only));
	} else {
		std::cerr << "Unable to generate fill-only conversation glyphs; "
				  << "using single-pass rendering." << std::endl;
	}
	int tracking_halves = 0;
	config->value("config/gameplay/conversation_font/tighter_spacing", tracking_halves, 0);
	result->set_extra_tracking_halves(std::clamp(tracking_halves, 0, 2));
	std::cout << "Using custom conversation font";
	if (!family.empty()) {
		std::cout << " '" << family << "'";
	}
	if (!file.empty()) {
		std::cout << " from " << file;
	}
	std::cout << " at " << pixels << " px." << std::endl;
	return result;
}

