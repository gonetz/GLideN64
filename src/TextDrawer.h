#ifndef TEXTDRAWER_H
#define TEXTDRAWER_H
#include "OpenGL.h"

class TextDrawer
{
	void init();
	void destroy();
	void renderText(const char *_pText, float x, float y) const;
	void getTextSize(const char *_pText, float & _w, float & _h) const;
	static TextDrawer & get();

private:
	TextDrawer();
	TextDrawer(const TextDrawer &);
	friend class OGLRender;
	struct Atlas * m_pAtlas;
	GLuint m_program;
	GLint m_uTex;
	GLint m_uColor;
	GLuint m_vbo;
};

#endif // TEXTDRAWER_H
