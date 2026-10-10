// Feature option loading; included inside GameDisplayOptions_gump::load_settings.
	const int tau_ms = gwin->get_modern_movement_tau_ms();
	modern_tau = 0;
	for (size_t i = 1; i < std::size(modern_tau_values); ++i) {
		if (std::abs(modern_tau_values[i] - tau_ms) < std::abs(modern_tau_values[modern_tau] - tau_ms)) {
			modern_tau = static_cast<int>(i);
		}
	}

