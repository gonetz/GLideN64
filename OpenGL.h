#ifndef OPENGL_H
#define OPENGL_H

#ifdef _WINDOWS
#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
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

#ifdef _WINDOWS
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM2IPROC glUniform2i;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;

extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLDEPTHRANGEFPROC glDepthRangef;
extern PFNGLCLEARDEPTHFPROC glClearDepthf;

extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
extern PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLMAPBUFFERPROC glMapBuffer;
extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;
extern PFNGLMEMORYBARRIERPROC glMemoryBarrier;

#endif // !_WINDOWS
#endif
