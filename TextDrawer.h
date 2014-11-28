#ifndef TEXTDRAWER_H
#define TEXTDRAWER_H
#include "OpenGL.h"

class TextDrawer
{
	friend class OGLRender;
	TextDrawer();
	void init();
	void destroy();
	void renderText(const char *_pText, float x, float y) const;
	static TextDrawer & get();

	struct Atlas * m_pAtlas;
	GLuint m_program;
	GLint m_uTex;
	GLint m_uColor;
	GLuint m_vbo;
};

#endif // TEXTDRAWER_H
