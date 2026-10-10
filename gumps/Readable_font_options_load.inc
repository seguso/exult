// Feature option loading; included inside GameDisplayOptions_gump::load_settings.
	bool conversation_font_enabled = false;
	config->value("config/gameplay/conversation_font/enabled", conversation_font_enabled, false);
	config->value("config/gameplay/conversation_font/file", conversation_font_file, "");
	config->value("config/gameplay/conversation_font/family", conversation_font_family, "");
	font_dialog_result = std::make_shared<ConversationFontDialogResult>();
	conversation_font = conversation_font_enabled ? 1 : 0;

	conversation_font_default_size = 17;
	conversation_font_size = conversation_font_default_size;
	config->value("config/gameplay/conversation_font/pixels", conversation_font_size, conversation_font_default_size);
	config->value("config/gameplay/conversation_font/tighter_spacing", conversation_font_tracking, 0);
	// Old half-pixel values become Normal; the former +1 value remains +1.
	conversation_font_tracking = conversation_font_tracking >= 2 ? 1 : 0;
	conversation_font_size = std::clamp(conversation_font_size, 5, 32);