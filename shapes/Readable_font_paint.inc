// Readable conversation glyph paint passes; included by font.cc.
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

