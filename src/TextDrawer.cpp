/* Draw text on screen.
 * Requires freetype library.
 * Code is taken from "OpenGL source examples from the OpenGL Programming wikibook:
 * http://en.wikibooks.org/wiki/OpenGL_Programming"
 */

#define NOMINMAX

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "TextDrawer.h"
#include "RSP.h"
#include "Config.h"
#include "GLSLCombiner.h"
#include "ShaderUtils.h"
#include <FBOTextureFormats.h>
#include "Log.h"

struct point {
	GLfloat x;
	GLfloat y;
	GLfloat s;
	GLfloat t;
	point() : x(0), y(0), s(0), t(0) {}
	point(GLfloat _x, GLfloat _y, GLfloat _s, GLfloat _t) : x(_x), y(_y), s(_s), t(_t) {}
};

// Maximum texture width
#define MAXWIDTH 1024

#if defined(GLES3_1)
#define SHADER_VERSION "#version 310 es \n"
#elif defined(GLES3)
#define SHADER_VERSION "#version 300 es \n"
#elif defined(GLES2)
#define SHADER_VERSION "#version 100 \n"
#else
#define SHADER_VERSION "#version 330 core \n"
#endif

#ifdef GLES2
const GLenum monohromeformat = GL_LUMINANCE;
const GLenum monohromeInternalformat = GL_LUMINANCE;
#else
const GLenum monohromeformat = GL_RED;
const GLenum monohromeInternalformat = GL_R8;
#endif // GLES2

static
const char * strDrawTextVertexShader =
SHADER_VERSION
"#if (__VERSION__ > 120)						\n"
"# define IN in									\n"
"# define OUT out								\n"
"#else											\n"
"# define IN attribute							\n"
"# define OUT varying							\n"
"#endif // __VERSION							\n"
"IN highp vec4 aPosition;					\n"
"OUT mediump vec2 texpos;					\n"
"void main(void) {							\n"
"  gl_Position = vec4(aPosition.xy, 0, 1);	\n"
"  texpos = aPosition.zw;					\n"
"}											\n"
;

static
const char * strDrawTextFragmentShader =
SHADER_VERSION
"#if (__VERSION__ > 120)		\n"
"# define IN in					\n"
"# define OUT out				\n"
"# define texture2D texture		\n"
"#else							\n"
"# define IN varying			\n"
"# define OUT					\n"
"#endif // __VERSION __			\n"
"IN mediump vec2 texpos;							\n"
"uniform sampler2D uTex;							\n"
"uniform lowp vec4 uColor;							\n"
"OUT lowp vec4 fragColor;							\n"
"void main(void) {									\n"
"  fragColor = texture2D(uTex, texpos).r * uColor;	\n"
#ifdef GLES2
"  gl_FragColor = fragColor;						\n"
#endif
"}													\n"
;

/**
 * The atlas struct holds a texture that contains the visible US-ASCII characters
 * of a certain font rendered with a certain character height.
 * It also contains an array that contains all the information necessary to
 * generate the appropriate vertex and texture coordinates for each character.
 *
 * After the constructor is run, you don't need to use any FreeType functions anymore.
 */
struct Atlas {
	GLuint tex;		// texture object

	int w;			// width of texture in pixels
	int h;			// height of texture in pixels

	struct {
		float ax;	// advance.x
		float ay;	// advance.y

		float bw;	// bitmap.width;
		float bh;	// bitmap.height;

		float bl;	// bitmap_left;
		float bt;	// bitmap_top;

		float tx;	// x offset of glyph in texture coordinates
		float ty;	// y offset of glyph in texture coordinates
	} c[128];		// character information

	 Atlas(FT_Face face, int height) {
		FT_Set_Pixel_Sizes(face, 0, height);
		FT_GlyphSlot g = face->glyph;

		int roww = 0;
		int rowh = 0;
		 w = 0;
		 h = 0;

		 memset(c, 0, sizeof c);

		/* Find minimum size for a texture holding all visible ASCII characters */
		for (int i = 32; i < 128; i++) {
			if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
				fprintf(stderr, "Loading character %c failed!\n", i);
				continue;
			}
			if (roww + g->bitmap.width + 1 >= MAXWIDTH) {
				w = std::max(w, roww);
				h += rowh;
				roww = 0;
				rowh = 0;
			}
			roww += g->bitmap.width + 1;
			rowh = std::max(rowh, (int)g->bitmap.rows);
		}

		w = std::max(w, roww);
		h += rowh;

		/* Create a texture that will be used to hold all ASCII glyphs */
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexImage2D(GL_TEXTURE_2D, 0, monohromeInternalformat, w, h, 0, monohromeformat, GL_UNSIGNED_BYTE, 0);

		/* We require 1 byte alignment when uploading texture data */
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		/* Clamping to edges is important to prevent artifacts when scaling */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		/* Linear filtering usually looks best for text */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		/* Paste all glyph bitmaps into the texture, remembering the offset */
		int ox = 0;
		int oy = 0;

		rowh = 0;

		for (int i = 32; i < 128; i++) {
			if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
				fprintf(stderr, "Loading character %c failed!\n", i);
				continue;
			}

			if (ox + g->bitmap.width + 1 >= MAXWIDTH) {
				oy += rowh;
				rowh = 0;
				ox = 0;
			}

			glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows, monohromeformat, GL_UNSIGNED_BYTE, g->bitmap.buffer);
			c[i].ax = _FIXED2FLOAT(g->advance.x, 6);
			c[i].ay = _FIXED2FLOAT(g->advance.y, 6);

			c[i].bw = (float)g->bitmap.width;
			c[i].bh = (float)g->bitmap.rows;

			c[i].bl = (float)g->bitmap_left;
			c[i].bt = (float)g->bitmap_top;

			c[i].tx = ox / (float)w;
			c[i].ty = oy / (float)h;

			rowh = std::max(rowh, (int)g->bitmap.rows);
			ox += g->bitmap.width + 1;
		}

		fprintf(stderr, "Generated a %d x %d (%d kb) texture atlas\n", w, h, w * h / 1024);
	}

	~Atlas() {
		glDeleteTextures(1, &tex);
	}
};

TextDrawer::TextDrawer() :
	m_pAtlas(nullptr), m_program(0), m_uTex(0), m_uColor(0), m_vbo(0)
{}

TextDrawer & TextDrawer::get() {
	static TextDrawer drawer;
	return drawer;
}

static
bool getFontFileName(char * _strName)
{
#ifdef OS_WINDOWS
	char * pSysPath = getenv("WINDIR");
	if (pSysPath == nullptr)
		return false;
	sprintf(_strName, "%s/Fonts/%s", pSysPath, config.font.name.c_str());
#elif defined (ANDROID)
	sprintf(_strName, "/system/fonts/%s", config.font.name.c_str());
#elif defined (PANDORA)
	sprintf(_strName, "/usr/share/fonts/truetype/%s", config.font.name.c_str());
#else
    sprintf(_strName, "/usr/share/fonts/truetype/freefont/%s", config.font.name.c_str());
#endif
	return true;
}

FT_Library ft;
FT_Face face;

void TextDrawer::init()
{
	if (m_pAtlas != nullptr)
		return;

	char strBuffer[PLUGIN_PATH_SIZE];
	const char *fontfilename;
	if (getFontFileName(strBuffer))
		fontfilename = strBuffer;
	else
		return;

	/* Initialize the FreeType2 library */
	if (FT_Init_FreeType(&ft)) {
		fprintf(stderr, "Could not init freetype library\n");
		return;
	}

	/* Load a font */
	if (FT_New_Face(ft, fontfilename, 0, &face)) {
		fprintf(stderr, "Could not open font %s\n", fontfilename);
		return;
	}

	m_program = createShaderProgram(strDrawTextVertexShader, strDrawTextFragmentShader);
	if(m_program == 0)
		return;

	m_uTex = glGetUniformLocation(m_program, "uTex");
	m_uColor = glGetUniformLocation(m_program, "uColor");

	if(m_uTex == -1 || m_uColor == -1)
		return;

	// Create the vertex buffer object
	glGenBuffers(1, &m_vbo);

	/* Create texture atlas for selected font size */
	m_pAtlas = new Atlas(face, config.font.size);
}

void TextDrawer::destroy()
{
	if (m_pAtlas == nullptr)
		return;
	delete m_pAtlas;
	m_pAtlas = nullptr;
	glDeleteBuffers(1, &m_vbo);
	m_vbo = 0;
	glDeleteProgram(m_program);
	m_program = 0;
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 */
void TextDrawer::renderText(const char *_pText, float _x, float _y) const
{
	if (m_pAtlas == nullptr)
		return;
	OGLVideo & ogl = video();
	const float sx = 2.0f / ogl.getWidth();
	const float sy = 2.0f / ogl.getHeight();

	const u8 *p;

	glUseProgram(m_program);

	/* Enable blending, necessary for our alpha texture */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	/* Set color */
	glUniform4fv(m_uColor, 1, config.font.colorf);

	/* Use the texture containing the atlas */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_pAtlas->tex);
	glUniform1i(m_uTex, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef GLES2
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

	/* Set up the VBO for our vertex data */
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, 0, 0);

	std::vector<point> coords(6 * strlen(_pText));
	int c = 0;

	/* Loop through all characters */
	for (p = (const u8 *)_pText; *p; ++p) {
		/* Calculate the vertex and texture coordinates */
		float x2 = _x + m_pAtlas->c[*p].bl * sx;
		float y2 = -_y - m_pAtlas->c[*p].bt * sy;
		float w = m_pAtlas->c[*p].bw * sx;
		float h = m_pAtlas->c[*p].bh * sy;

		/* Advance the cursor to the start of the next character */
		_x += m_pAtlas->c[*p].ax * sx;
		_y += m_pAtlas->c[*p].ay * sy;

		/* Skip glyphs that have no pixels */
		if (!w || !h)
			continue;

		coords[c++] = point(x2, -y2, m_pAtlas->c[*p].tx, m_pAtlas->c[*p].ty);
		coords[c++] = point(x2 + w, -y2, m_pAtlas->c[*p].tx + m_pAtlas->c[*p].bw / m_pAtlas->w, m_pAtlas->c[*p].ty);
		coords[c++] = point(x2, -y2 - h, m_pAtlas->c[*p].tx, m_pAtlas->c[*p].ty + m_pAtlas->c[*p].bh / m_pAtlas->h);
		coords[c++] = point(x2 + w, -y2, m_pAtlas->c[*p].tx + m_pAtlas->c[*p].bw / m_pAtlas->w, m_pAtlas->c[*p].ty);
		coords[c++] = point(x2, -y2 - h, m_pAtlas->c[*p].tx, m_pAtlas->c[*p].ty + m_pAtlas->c[*p].bh / m_pAtlas->h);
		coords[c++] = point(x2 + w, -y2 - h, m_pAtlas->c[*p].tx + m_pAtlas->c[*p].bw / m_pAtlas->w, m_pAtlas->c[*p].ty + m_pAtlas->c[*p].bh / m_pAtlas->h);
	}

	/* Draw all the character on the screen in one go */
	glBufferData(GL_ARRAY_BUFFER, coords.size()*sizeof(point), coords.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, c);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TextDrawer::getTextSize(const char *_pText, float & _w, float & _h) const
{
	_w = _h = 0;
	if (m_pAtlas == nullptr)
		return;
	OGLVideo & ogl = video();
	const float sx = 2.0f / ogl.getWidth();
	const float sy = 2.0f / ogl.getHeight();
	float bw, bh;

	for (const u8 *p = (const u8 *)_pText; *p; ++p) {
		bw = m_pAtlas->c[*p].bw * sx;
		bh = m_pAtlas->c[*p].bh * sy;

		_w += m_pAtlas->c[*p].ax * sx;
		_h += m_pAtlas->c[*p].ay * sy;
	}
	_w += bw;
	_h += bh;
}
