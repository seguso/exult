// Feature-specific settings persistence; included by GameDisplayOptions_gump.cc.
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
