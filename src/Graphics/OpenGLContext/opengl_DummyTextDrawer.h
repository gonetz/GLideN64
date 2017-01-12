#ifndef OPENGL_DUMMY_TEXTDRAWER_H
#define OPENGL_DUMMY_TEXTDRAWER_H

#include "opengl_TextDrawer.h"

namespace opengl {

	class DummyTextDrawer : public TextDrawer
	{
	public:
		DummyTextDrawer() {}
		~DummyTextDrawer() {}

		void drawText(const char *_pText, float x, float y) const override {}
		void getTextSize(const char *_pText, float & _w, float & _h) const override {}
	};

}

#endif // OPENGL_DUMMY_TEXTDRAWER_H
