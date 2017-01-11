#ifndef OPENGL_DUMMY_TEXTDRAWER_H
#define OPENGL_DUMMY_TEXTDRAWER_H

#include <Graphics/TextDrawerImpl.h>

namespace opengl {

	class DummyTextDrawer : public graphics::TextDrawer
	{
	public:
		DummyTextDrawer() {}
		~DummyTextDrawer() {}

		void renderText(const char *_pText, float x, float y) const override {}
		void getTextSize(const char *_pText, float & _w, float & _h) const override {}
	};

}

#endif // OPENGL_DUMMY_TEXTDRAWER_H
