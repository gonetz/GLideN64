#include "TextDrawer.h"

TextDrawer::TextDrawer() {}
void TextDrawer::init() {}
void TextDrawer::destroy() {}
void TextDrawer::renderText(const char * /*_pText*/, float /*x*/, float /*y*/) const {}
void TextDrawer::getTextSize(const char *_pText, float & _w, float & _h) const {}
TextDrawer & TextDrawer::get() {
	static TextDrawer drawer;
	return drawer;
}

