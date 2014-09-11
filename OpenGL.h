#ifndef OPENGL_H
#define OPENGL_H

#ifdef _WINDOWS
#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
#include "windows/GLFunctions.h"
#else
#include "winlnxdefs.h"
#ifdef GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define GL_DRAW_FRAMEBUFFER GL_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER GL_FRAMEBUFFER
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif // GLES2
#ifdef USE_SDL
#include <SDL.h>
#endif // USE_SDL
#endif // _WINDOWS

#include "gSP.h"

struct GLVertex
{
	float x, y, z, w;
	float s0, t0, s1, t1;
};

struct GLInfo
{
#ifdef _WINDOWS
	HGLRC	hRC;
	HDC		hDC;
	HWND	hWnd;
	HWND	hFullscreenWnd;
#elif defined(USE_SDL)
	SDL_Surface *hScreen;
#endif // _WINDOWS

	BOOL fullscreen;
	unsigned int fullscreenWidth, fullscreenHeight, fullscreenBits, fullscreenRefresh;
	unsigned int width, height, heightOffset;

	float	scaleX, scaleY;

#define INDEXMAP_SIZE 64U
#define VERTBUFF_SIZE 256U
#define ELEMBUFF_SIZE 1024U

	struct {
		SPVertex vertices[VERTBUFF_SIZE];
		GLubyte elements[ELEMBUFF_SIZE];
		int num;


		u32 indexmap[INDEXMAP_SIZE];
		u32 indexmapinv[VERTBUFF_SIZE];
		u32 indexmap_prev;
		u32 indexmap_nomap;

	} triangles;


	GLVertex rect[4];

	BYTE	numTriangles;
	BYTE	numVertices;

	BYTE	combiner;
	enum {
		fbNone,
		fbFBO
	} framebufferMode;
	enum {
		rsNone = 0,
		rsTriangle = 1,
		rsRect = 2,
		rsTexRect = 3,
		rsLine = 4
	} renderState;
	bool bImageTexture;
	bool captureScreen;
};

extern GLInfo OGL;

void OGL_InitGLFunctions();
void OGL_InitData();
void OGL_DestroyData();
bool OGL_Start();
void OGL_Stop();

void OGL_AddTriangle(int v0, int v1, int v2);
void OGL_DrawTriangles();
void OGL_DrawTriangle(SPVertex *vertices, int v0, int v1, int v2);
void OGL_DrawLine(int v0, int v1, float width);
void OGL_DrawRect(int ulx, int uly, int lrx, int lry, float *color);
void OGL_DrawTexturedRect(float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip);
void OGL_UpdateScale();
void OGL_ClearDepthBuffer();
void OGL_ClearColorBuffer( float *color );
void OGL_ResizeWindow();
void OGL_SaveScreenshot();
void OGL_SwapBuffers();
void OGL_ReadScreen( void **dest, long *width, long *height );

bool checkFBO();

#endif
