// Readable conversation glyph paint passes; included by font.cc.
int Font::paint_text(
		Image_buffer8* win, const char* text, int xoff, int yoff, unsigned char* trans) {
	return paint_text(win, text, static_cast<int>(std::strlen(text)), xoff, yoff, trans);
}
