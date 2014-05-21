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
#endif // ANDROID
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

	DWORD	fullscreenWidth, fullscreenHeight, fullscreenBits, fullscreenRefresh;
	DWORD	width, height, windowedWidth, windowedHeight, heightOffset;

	BOOL	fullscreen, forceBilinear, fog;

	float	scaleX, scaleY;

#define INDEXMAP_SIZE 64
#define VERTBUFF_SIZE 256
#define ELEMBUFF_SIZE 1024

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

	int		maxTextureUnits; // TNT = 2, GeForce = 2-4, Rage 128 = 2, Radeon = 3-6

	BYTE	numTriangles;
	BYTE	numVertices;

	BYTE	combiner;
	enum {
		fbNone,
		fbFBO,
		fbFBOEXT
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

	// Settings. TODO: Move to Settings class
	BOOL bHWLighting;
	BOOL enable2xSaI;
	BOOL frameBufferTextures;
	u32	 textureBitDepth;
	float originAdjust;
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

void ogl_glGenFramebuffers (GLsizei n, GLuint *framebuffers);
void ogl_glBindFramebuffer (GLenum target, GLuint framebuffer);
void ogl_glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
void ogl_glFramebufferTexture (GLenum target, GLenum attachment, GLuint texture, GLint level);
void ogl_glGenRenderbuffers (GLsizei n, GLuint *renderbuffers);
void ogl_glBindRenderbuffer (GLenum target, GLuint renderbuffer);
void ogl_glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void ogl_glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers);
void ogl_glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void ogl_glDrawBuffers (GLsizei n, const GLenum *bufs, GLuint texture);
GLenum ogl_glCheckFramebufferStatus (GLenum target);
void ogl_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
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
extern PFNGLUNIFORM4FPROC glUniform4f;
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

extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
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

extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLBLITFRAMEBUFFEREXTPROC glBlitFramebufferEXT;
#endif // !_WINDOWS
#endif
