#include "TextDrawer.h"

TextDrawer::TextDrawer() {}
void TextDrawer::init() {}
void TextDrawer::destroy() {}
void TextDrawer::renderText(const char * /*_pText*/, float /*x*/, float /*y*/) const {}
TextDrawer & TextDrawer::get() {
	static TextDrawer drawer;
	return drawer;
}

