#ifndef TEXTDRAWERIMPL_H
#define TEXTDRAWERIMPL_H

namespace graphics {

	class TextDrawer
	{
	public:
		virtual ~TextDrawer() {}
		virtual void renderText(const char *_pText, float x, float y) const = 0;
		virtual void getTextSize(const char *_pText, float & _w, float & _h) const = 0;
	};

}

#endif // TEXTDRAWERIMPL_H
