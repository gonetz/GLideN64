#ifndef OPENGL_H
#define OPENGL_H

#ifndef __LINUX__
# include <windows.h>
# include <GL/gl.h>
# include "wglext.h"
# include "glext.h"
#else
# include "winlnxdefs.h"
# include <GL/gl.h>
# include <GL/glext.h>
# include "SDL.h"
#endif // __LINUX__

#include "glATI.h"
#include "gSP.h"

struct GLVertex
{
	float x, y, z, w;
	struct
	{
		float r, g, b, a;
	} color, secondaryColor;
	float s0, t0, s1, t1;
	float fog;
};

struct GLInfo
{
#ifndef __LINUX__
	HGLRC	hRC, hPbufferRC;
	HDC		hDC, hPbufferDC;
	HWND	hWnd;
	HPBUFFERARB	hPbuffer;
#else
	SDL_Surface *hScreen;
#endif // __LINUX__

	DWORD	fullscreenWidth, fullscreenHeight, fullscreenBits, fullscreenRefresh;
	DWORD	width, height, windowedWidth, windowedHeight, heightOffset;

	BOOL	fullscreen, forceBilinear, fog;

	float	scaleX, scaleY;

	BOOL	ATI_texture_env_combine3;	// Radeon
	BOOL	ATIX_texture_env_route;		// Radeon

	BOOL	ARB_multitexture;			// TNT, GeForce, Rage 128, Radeon
	BOOL	ARB_texture_env_combine;	// GeForce, Rage 128, Radeon
	BOOL	ARB_texture_env_crossbar;	// Radeon (GeForce supports it, but doesn't report it)

	BOOL	EXT_fog_coord;				// TNT, GeForce, Rage 128, Radeon
	BOOL	EXT_texture_env_combine;	// TNT, GeForce, Rage 128, Radeon
	BOOL	EXT_secondary_color;		// GeForce, Radeon

	BOOL	NV_texture_env_combine4;	// TNT, GeForce
	BOOL	NV_register_combiners;		// GeForce
	BOOL	GLSL;						// GeForce, Radeon
	BOOL	ARB_buffer_region;
	BOOL	ARB_pbuffer;
	BOOL	ARB_render_texture;
	BOOL	ARB_pixel_format;

	int		maxTextureUnits;			// TNT = 2, GeForce = 2-4, Rage 128 = 2, Radeon = 3-6
	int		maxGeneralCombiners;

	BOOL	enable2xSaI;
	BOOL	frameBufferTextures;
	int		textureBitDepth;
	float	originAdjust;

	GLVertex vertices[256];
	BYTE	triangles[80][3];
	BYTE	numTriangles;
	BYTE	numVertices;
#ifndef __LINUX__
	HWND	hFullscreenWnd;
#endif
	BOOL	usePolygonStipple;
	GLubyte	stipplePattern[32][8][128];
	BYTE	lastStipple;

	BYTE	combiner;
	enum {
		fbNone,
		fbFBO,
		fbFBOEXT
	} framebuffer_mode;
	bool captureScreen;
};

extern GLInfo OGL;

struct GLcolor
{
	float r, g, b, a;
};

#ifndef __LINUX__
extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
extern PFNGLUNIFORM4IARBPROC glUniform4iARB;
extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
extern PFNGLSECONDARYCOLOR3FPROC glSecondaryColor3f;

extern PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
extern PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
extern PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
extern PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
extern PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
extern PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
extern PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
extern PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
extern PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;

extern PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;

extern PFNGLSECONDARYCOLOR3BEXTPROC glSecondaryColor3bEXT;
extern PFNGLSECONDARYCOLOR3BVEXTPROC glSecondaryColor3bvEXT;
extern PFNGLSECONDARYCOLOR3DEXTPROC glSecondaryColor3dEXT;
extern PFNGLSECONDARYCOLOR3DVEXTPROC glSecondaryColor3dvEXT;
extern PFNGLSECONDARYCOLOR3FEXTPROC glSecondaryColor3fEXT;
extern PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;
extern PFNGLSECONDARYCOLOR3IEXTPROC glSecondaryColor3iEXT;
extern PFNGLSECONDARYCOLOR3IVEXTPROC glSecondaryColor3ivEXT;
extern PFNGLSECONDARYCOLOR3SEXTPROC	glSecondaryColor3sEXT;
extern PFNGLSECONDARYCOLOR3SVEXTPROC glSecondaryColor3svEXT;
extern PFNGLSECONDARYCOLOR3UBEXTPROC glSecondaryColor3ubEXT;
extern PFNGLSECONDARYCOLOR3UBVEXTPROC glSecondaryColor3ubvEXT;
extern PFNGLSECONDARYCOLOR3UIEXTPROC glSecondaryColor3uiEXT;
extern PFNGLSECONDARYCOLOR3UIVEXTPROC glSecondaryColor3uivEXT;
extern PFNGLSECONDARYCOLOR3USEXTPROC glSecondaryColor3usEXT;
extern PFNGLSECONDARYCOLOR3USVEXTPROC glSecondaryColor3usvEXT;
extern PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointerEXT;

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
#endif // !__LINUX__

bool OGL_Start();
void OGL_Stop();
void OGL_AddTriangle( SPVertex *vertices, int v0, int v1, int v2 );
void OGL_DrawTriangles();
void OGL_DrawLine( SPVertex *vertices, int v0, int v1, float width );
void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color );
void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip );
void OGL_UpdateScale();
void OGL_ClearDepthBuffer();
void OGL_ClearColorBuffer( float *color );
void OGL_ResizeWindow();
void OGL_SaveScreenshot();
#ifdef __LINUX__
void OGL_SwapBuffers();
#endif // __LINUX__
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

#endif
