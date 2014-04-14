#include "OpenGL.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

//// paulscode, added for SDL linkage:
#if defined(GLES2) && defined (USE_SDL)
	#include <SDL.h>
	 // TODO: Remove this bandaid for SDL 2.0 compatibility (needed for SDL_SetVideoMode)
	#if SDL_VERSION_ATLEAST(2,0,0)
	#include "sdl2_compat.h" // Slightly hacked version of core/vidext_sdl2_compat.h
	#endif
#endif
////

#include "GLideN64.h"
#include "Types.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "VI.h"
#include "Config.h"
#include "Log.h"

GLInfo OGL;

#ifdef _WINDOWS
// GLSL functions
PFNGLCREATESHADERPROC glCreateShader;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETPROGRAMIVPROC glGetProgramiv;

PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;

// multitexture functions
PFNGLACTIVETEXTUREPROC glActiveTexture;

// EXT_fog_coord functions
PFNGLFOGCOORDFEXTPROC glFogCoordfEXT;
PFNGLFOGCOORDFVEXTPROC glFogCoordfvEXT;
PFNGLFOGCOORDDEXTPROC glFogCoorddEXT;
PFNGLFOGCOORDDVEXTPROC glFogCoorddvEXT;
PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;

PFNGLDRAWBUFFERSPROC glDrawBuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;
PFNGLMEMORYBARRIERPROC glMemoryBarrier;

#endif // _WINDOWS

BOOL isExtensionSupported( const char *extension )
{
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	extensions = glGetString(GL_EXTENSIONS);

	start = extensions;
	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return TRUE;

		start = terminator;
	}

	return FALSE;
}

void OGL_InitExtensions()
{
	const char *version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	u32 uVersion = atol(version);

#ifdef _WINDOWS
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
	glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");

	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
	glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)wglGetProcAddress("glVertexAttrib4f");
	glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib4fv");

	glActiveTexture	= (PFNGLACTIVETEXTUREPROC)wglGetProcAddress( "glActiveTexture" );

	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress( "glDrawBuffers" );
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress( "glBindFramebuffer" );
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress( "glDeleteFramebuffers" );
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress( "glGenFramebuffers" );
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress( "glFramebufferTexture2D" );
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress( "glGenRenderbuffers" );
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress( "glBindRenderbuffer" );
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress( "glRenderbufferStorage" );
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress( "glFramebufferRenderbuffer" );
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress( "glDeleteRenderbuffers" );
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress( "glCheckFramebufferStatus" );
	glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)wglGetProcAddress( "glBlitFramebuffer" );
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress( "glGenBuffers" );
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress( "glBindBuffer" );
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress( "glBufferData" );
	glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress( "glMapBuffer" );
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress( "glUnmapBuffer" );
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress( "glDeleteBuffers" );
	glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)wglGetProcAddress( "glBindImageTexture" );
	glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)wglGetProcAddress( "glMemoryBarrier" );

#endif // _WINDOWS

	if (glGenFramebuffers != NULL)
		OGL.framebufferMode = GLInfo::fbFBO;
	else
		OGL.framebufferMode = GLInfo::fbNone;

#ifndef GLES2
	OGL.bImageTexture = (uVersion >= 4) && (glBindImageTexture != NULL);
#else
	OGL.bImageTexture = false;
#endif
}

void OGL_InitStates()
{
    glEnable( GL_CULL_FACE );
    glEnableVertexAttribArray( SC_POSITION );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_ALWAYS );
    glDepthMask( GL_FALSE );
    glEnable( GL_SCISSOR_TEST );

	if (config.frameBufferEmulation.N64DepthCompare) {
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_POLYGON_OFFSET_FILL );
		glDepthFunc( GL_ALWAYS );
		glDepthMask( FALSE );
	} else
		glPolygonOffset( -3.0f, -3.0f );

	glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	srand( timeGetTime() );

	OGL_SwapBuffers();
}

void OGL_UpdateScale()
{
	OGL.scaleX = OGL.width / (float)VI.width;
	OGL.scaleY = OGL.height / (float)VI.height;
}

void OGL_ResizeWindow()
{
#ifdef _WINDOWS
	RECT	windowRect, statusRect, toolRect;

	if (OGL.fullscreen)
	{
		OGL.width = config.video.fullscreenWidth;
		OGL.height = config.video.fullscreenHeight;
		OGL.heightOffset = 0;

		SetWindowPos( hWnd, NULL, 0, 0,	OGL.width, OGL.height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
	}
	else
	{
		OGL.width = config.video.windowedWidth;
		OGL.height = config.video.windowedHeight;

		GetClientRect( hWnd, &windowRect );
		GetWindowRect( hStatusBar, &statusRect );

		if (hToolBar)
			GetWindowRect( hToolBar, &toolRect );
		else
			toolRect.bottom = toolRect.top = 0;

		OGL.heightOffset = (statusRect.bottom - statusRect.top);
		windowRect.right = windowRect.left + config.video.windowedWidth - 1;
		windowRect.bottom = windowRect.top + config.video.windowedHeight - 1 + OGL.heightOffset;

		AdjustWindowRect( &windowRect, GetWindowLong( hWnd, GWL_STYLE ), GetMenu( hWnd ) != NULL );

		SetWindowPos( hWnd, NULL, 0, 0,	windowRect.right - windowRect.left + 1,
						windowRect.bottom - windowRect.top + 1 + toolRect.bottom - toolRect.top + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE );
	}
#endif // _WINDOWS
}

////// paulscode, added for SDL linkage
#if defined(GLES2) && defined (USE_SDL)
//#if defined (USE_SDL)
bool OGL_SDL_Start()
{
	/* Initialize SDL */
	LOG(LOG_MINIMAL, "Initializing SDL video subsystem...\n" );
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
	{
		 LOG(LOG_ERROR, "Error initializing SDL video subsystem: %s\n", SDL_GetError() );
		return FALSE;
	}

	int current_w = config.video.windowedWidth;
	int current_h = config.video.windowedHeight;

	/* Set the video mode */
	LOG(LOG_MINIMAL, "Setting video mode %dx%d...\n", current_w, current_h );

	// TODO: I should actually check what the pixelformat is, rather than assuming 16 bpp (RGB_565) or 32 bpp (RGBA_8888):
	int bitsPP = 16;

	// TODO: Replace SDL_SetVideoMode with something that is SDL 2.0 compatible
	//       Better yet, eliminate all SDL calls by using the Mupen64Plus core api
	if (!(OGL.hScreen = SDL_SetVideoMode( current_w, current_h, bitsPP, SDL_HWSURFACE )))
	{
		LOG(LOG_ERROR, "Problem setting videomode %dx%d: %s\n", current_w, current_h, SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return FALSE;
	}

	/*
//// paulscode, fixes the screen-size problem
	int videoWidth = current_w;
	int videoHeight = current_h;
	int x = 0;
	int y = 0;

	//re-scale width and height on per-rom basis
	float width = (float)videoWidth * (float)config.window.refwidth / 800.f;
	float height = (float)videoHeight * (float)config.window.refheight / 480.f;

	//re-center video if it was re-scaled per-rom
	x -= (width - (float)videoWidth) / 2.f;
	y -= (height - (float)videoHeight) / 2.f;

	//set xpos and ypos
	config.window.xpos = x;
	config.window.ypos = y;
	config.framebuffer.xpos = x;
	config.framebuffer.ypos = y;

	//set width and height
	config.window.width = (int)width;
	config.window.height = (int)height;
	config.framebuffer.width = (int)width;
	config.framebuffer.height = (int)height;
////
*/
	return true;
}
#endif
//////


bool OGL_Start()
{
#ifdef _WINDOWS
	int		pixelFormat;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd
		1,                                // version number
		PFD_DRAW_TO_WINDOW |              // support window
		PFD_SUPPORT_OPENGL |              // support OpenGL
		PFD_DOUBLEBUFFER,                 // double buffered
		PFD_TYPE_RGBA,                    // RGBA type
		32,								  // color depth
		0, 0, 0, 0, 0, 0,                 // color bits ignored
		0,                                // no alpha buffer
		0,                                // shift bit ignored
		0,                                // no accumulation buffer
		0, 0, 0, 0,                       // accum bits ignored
		32,								  // z-buffer
		0,                                // no stencil buffer
		0,                                // no auxiliary buffer
		PFD_MAIN_PLANE,                   // main layer
		0,                                // reserved
		0, 0, 0                           // layer masks ignored
	};

	if ((OGL.hDC = GetDC( hWnd )) == NULL)
	{
		MessageBox( hWnd, "Error while getting a device context!", pluginName, MB_ICONERROR | MB_OK );
		return FALSE;
	}

	if ((pixelFormat = ChoosePixelFormat( OGL.hDC, &pfd )) == 0)
	{
		MessageBox( hWnd, "Unable to find a suitable pixel format!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}

	if ((SetPixelFormat( OGL.hDC, pixelFormat, &pfd )) == FALSE)
	{
		MessageBox( hWnd, "Error while setting pixel format!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}

	if ((OGL.hRC = wglCreateContext( OGL.hDC )) == NULL)
	{
		MessageBox( hWnd, "Error while creating OpenGL context!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}

	if ((wglMakeCurrent( OGL.hDC, OGL.hRC )) == FALSE)
	{
		MessageBox( hWnd, "Error while making OpenGL context current!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return FALSE;
	}
#endif // _WINDOWS
#ifdef USE_SDL
	// init sdl & gl
	Uint32 videoFlags = 0;

	if (OGL.fullscreen)
	{
		OGL.width = config.video.fullscreenWidth;
		OGL.height = config.video.fullscreenHeight;
	}
	else
	{
		OGL.width = config.video.windowedWidth;
		OGL.height = config.video.windowedHeight;
	}

#ifndef GLES2
	/* Initialize SDL */
	printf( "[glN64]: (II) Initializing SDL video subsystem...\n" );
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
	{
		printf( "[glN64]: (EE) Error initializing SDL video subsystem: %s\n", SDL_GetError() );
		return FALSE;
	}

	/* Video Info */
	const SDL_VideoInfo *videoInfo;
	printf( "[glN64]: (II) Getting video info...\n" );
	if (!(videoInfo = SDL_GetVideoInfo()))
	{
		printf( "[glN64]: (EE) Video query failed: %s\n", SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return FALSE;
	}

	/* Set the video mode */
	videoFlags |= SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;

	if (videoInfo->hw_available)
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;

	if (videoInfo->blit_hw)
		videoFlags |= SDL_HWACCEL;

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
/*	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );*/
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );	// 32 bit z-buffer

	printf( "[glN64]: (II) Setting video mode %dx%d...\n", OGL.width, OGL.height );
	if (!(OGL.hScreen = SDL_SetVideoMode( OGL.width, OGL.height, 0, videoFlags )))
	{
		printf( "[glN64]: (EE) Error setting videomode %dx%d: %s\n", OGL.width, OGL.height, SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return FALSE;
	}

	SDL_WM_SetCaption( pluginName, pluginName );
#else // GLES2
	if (!OGL_SDL_Start())
		return false;
#endif // GLES2
#endif // USE_SDL

	OGL_InitExtensions();
	OGL_InitStates();

	TextureCache_Init();
	DepthBuffer_Init();
	FrameBuffer_Init();
	Combiner_Init();
	OGL.renderState = GLInfo::rsNone;

	gSP.changed = gDP.changed = 0xFFFFFFFF;
	OGL_UpdateScale();
	OGL.captureScreen = false;

	memset(OGL.triangles.vertices, 0, VERTBUFF_SIZE * sizeof(SPVertex));
	memset(OGL.triangles.elements, 0, ELEMBUFF_SIZE * sizeof(GLubyte));
	OGL.triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	__indexmap_init();
#endif

	return TRUE;
}

void OGL_Stop()
{
	Combiner_Destroy();
	FrameBuffer_Destroy();
	DepthBuffer_Destroy();
	TextureCache_Destroy();
	OGL.renderState = GLInfo::rsNone;

#ifdef _WINDOWS
	wglMakeCurrent( NULL, NULL );

	if (OGL.hRC)
	{
		wglDeleteContext( OGL.hRC );
		OGL.hRC = NULL;
	}

	if (OGL.hDC)
	{
		ReleaseDC( hWnd, OGL.hDC );
		OGL.hDC = NULL;
	}
#elif defined(USE_SDL)
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	OGL.hScreen = NULL;
#endif // _WINDOWS
}

void OGL_UpdateCullFace()
{
	if (gSP.geometryMode & G_CULL_BOTH)
	{
		glEnable( GL_CULL_FACE );

		if (gSP.geometryMode & G_CULL_BACK)
			glCullFace( GL_BACK );
		else
			glCullFace( GL_FRONT );
	}
	else
		glDisable( GL_CULL_FACE );
}

void OGL_UpdateViewport()
{
	if (frameBuffer.drawBuffer == GL_BACK)
		glViewport( gSP.viewport.x * OGL.scaleX, (VI.height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY + OGL.heightOffset,
					gSP.viewport.width * OGL.scaleX, gSP.viewport.height * OGL.scaleY );
	else
		glViewport( gSP.viewport.x * OGL.scaleX, (frameBuffer.top->height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY,
					gSP.viewport.width * OGL.scaleX, gSP.viewport.height * OGL.scaleY );
}

void OGL_UpdateDepthUpdate()
{
	if (gDP.otherMode.depthUpdate)
		glDepthMask( TRUE );
	else
		glDepthMask( FALSE );
}

//copied from RICE VIDEO
void OGL_SetBlendMode()
{
#define BLEND_NOOP              0x0000
#define BLEND_NOOP5             0xcc48  // Fog * 0 + Mem * 1
#define BLEND_NOOP4             0xcc08  // Fog * 0 + In * 1
#define BLEND_FOG_ASHADE        0xc800
#define BLEND_FOG_3             0xc000  // Fog * AIn + In * 1-A
#define BLEND_FOG_MEM           0xc440  // Fog * AFog + Mem * 1-A
#define BLEND_FOG_APRIM         0xc400  // Fog * AFog + In * 1-A
#define BLEND_BLENDCOLOR        0x8c88
#define BLEND_BI_AFOG           0x8400  // Bl * AFog + In * 1-A
#define BLEND_BI_AIN            0x8040  // Bl * AIn + Mem * 1-A
#define BLEND_MEM               0x4c40  // Mem*0 + Mem*(1-0)?!
#define BLEND_FOG_MEM_3         0x44c0  // Mem * AFog + Fog * 1-A
#define BLEND_NOOP3             0x0c48  // In * 0 + Mem * 1
#define BLEND_PASS              0x0c08  // In * 0 + In * 1
#define BLEND_FOG_MEM_IN_MEM    0x0440  // In * AFog + Mem * 1-A
#define BLEND_FOG_MEM_FOG_MEM   0x04c0  // In * AFog + Fog * 1-A
#define BLEND_OPA               0x0044  //  In * AIn + Mem * AMem
#define BLEND_XLU               0x0040
#define BLEND_MEM_ALPHA_IN      0x4044  //  Mem * AIn + Mem * AMem

	u32 blender = gDP.otherMode.l >> 16;
	u32 blendmode_1 = blender&0xcccc;
	u32 blendmode_2 = blender&0x3333;

	glEnable(GL_BLEND);
	switch(gDP.otherMode.cycleType)
	{
		case G_CYC_FILL:
			glDisable(GL_BLEND);
			break;

		case G_CYC_COPY:
			glBlendFunc(GL_ONE, GL_ZERO);
			break;

		case G_CYC_2CYCLE:
			if (gDP.otherMode.forceBlender && gDP.otherMode.depthCompare)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			}

			switch(blendmode_1+blendmode_2)
			{
				case BLEND_PASS+(BLEND_PASS>>2):    // In * 0 + In * 1
				case BLEND_FOG_APRIM+(BLEND_PASS>>2):
				case BLEND_FOG_MEM_FOG_MEM + (BLEND_OPA>>2):
				case BLEND_FOG_APRIM + (BLEND_OPA>>2):
				case BLEND_FOG_ASHADE + (BLEND_OPA>>2):
				case BLEND_BI_AFOG + (BLEND_OPA>>2):
				case BLEND_FOG_ASHADE + (BLEND_NOOP>>2):
				case BLEND_NOOP + (BLEND_OPA>>2):
				case BLEND_NOOP4 + (BLEND_NOOP>>2):
				case BLEND_FOG_ASHADE+(BLEND_PASS>>2):
				case BLEND_FOG_3+(BLEND_PASS>>2):
					glDisable(GL_BLEND);
					break;

				case BLEND_PASS+(BLEND_OPA>>2):
					if (gDP.otherMode.cvgXAlpha && gDP.otherMode.alphaCvgSel)
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					else
						glDisable(GL_BLEND);
					break;

				case BLEND_PASS + (BLEND_XLU>>2):
				case BLEND_FOG_ASHADE + (BLEND_XLU>>2):
				case BLEND_FOG_APRIM + (BLEND_XLU>>2):
				case BLEND_FOG_MEM_FOG_MEM + (BLEND_PASS>>2):
				case BLEND_XLU + (BLEND_XLU>>2):
				case BLEND_BI_AFOG + (BLEND_XLU>>2):
				case BLEND_XLU + (BLEND_FOG_MEM_IN_MEM>>2):
				case BLEND_PASS + (BLEND_FOG_MEM_IN_MEM>>2):
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;

				case BLEND_FOG_ASHADE+0x0301:
					glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
					break;

				case 0x0c08+0x1111:
					glBlendFunc(GL_ZERO, GL_DST_ALPHA);
					break;

				default:
					if (blendmode_2 == (BLEND_PASS>>2))
						glDisable(GL_BLEND);
					else
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				}
				break;

	default:

		if (gDP.otherMode.forceBlender && gDP.otherMode.depthCompare && blendmode_1 != BLEND_FOG_ASHADE )
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		}

		switch (blendmode_1)
		{
			case BLEND_XLU:
			case BLEND_BI_AIN:
			case BLEND_FOG_MEM:
			case BLEND_FOG_MEM_IN_MEM:
			case BLEND_BLENDCOLOR:
			case 0x00c0:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case BLEND_MEM_ALPHA_IN:
				glBlendFunc(GL_ZERO, GL_DST_ALPHA);
				break;

			case BLEND_OPA:
				glDisable(GL_BLEND);
				break;

			case BLEND_PASS:
			case BLEND_NOOP:
			case BLEND_FOG_ASHADE:
			case BLEND_FOG_MEM_3:
			case BLEND_BI_AFOG:
				glDisable(GL_BLEND);
				break;

			case BLEND_FOG_APRIM:
				glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);
				break;

			case BLEND_NOOP3:
			case BLEND_NOOP5:
			case BLEND_MEM:
				glBlendFunc(GL_ZERO, GL_ONE);
				break;

			default:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

}

void OGL_UpdateStates()
{

	if (gDP.otherMode.cycleType == G_CYC_COPY)
		Combiner_SetCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
	else if (gDP.otherMode.cycleType == G_CYC_FILL)
		Combiner_SetCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1));
	else
		Combiner_SetCombine(gDP.combine.mux);

	if (!config.frameBufferEmulation.N64DepthCompare && (gDP.changed & CHANGED_RENDERMODE) != 0) {
		if (gDP.otherMode.depthCompare) {
			glEnable( GL_DEPTH_TEST );
			glDepthFunc( GL_LEQUAL );
		} else
			glDisable(GL_DEPTH_TEST);
	}

	if (gSP.changed & CHANGED_GEOMETRYMODE)
		OGL_UpdateCullFace();

	if (gSP.changed & CHANGED_LIGHT)
		combiner.current->compiled->UpdateLight();

	if (gDP.otherMode.depthMode == ZMODE_DEC)
		glEnable( GL_POLYGON_OFFSET_FILL );
	else
		glDisable( GL_POLYGON_OFFSET_FILL );

	if (gDP.changed & CHANGED_RENDERMODE || gDP.changed & CHANGED_CYCLETYPE)
	{
		if (gDP.otherMode.cycleType == G_CYC_1CYCLE || gDP.otherMode.cycleType == G_CYC_2CYCLE)
		{
			//glDepthFunc((gDP.otherMode.depthCompare) ? GL_GEQUAL : GL_ALWAYS);
			glDepthFunc((gDP.otherMode.depthCompare) ? GL_LESS : GL_ALWAYS);
			glDepthMask((gDP.otherMode.depthUpdate) ? GL_TRUE : GL_FALSE);

			if (gDP.otherMode.depthMode == ZMODE_DEC)
				glEnable(GL_POLYGON_OFFSET_FILL);
		   else
				glDisable(GL_POLYGON_OFFSET_FILL);
		}
		else
		{
			glDepthFunc(GL_ALWAYS);
			glDepthMask(GL_FALSE);
		}
	}

	if ((gDP.changed & CHANGED_ALPHACOMPARE) || (gDP.changed & CHANGED_RENDERMODE))
		Combiner_UpdateAlphaTestInfo();

//	if (gDP.changed & CHANGED_SCISSOR)
//		OGL_UpdateScissor();

	if (gSP.changed & CHANGED_VIEWPORT)
		OGL_UpdateViewport();

	if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM))
	{
		//For some reason updating the texture cache on the first frame of LOZ:OOT causes a NULL Pointer exception...
		if (combiner.current != NULL)
		{
			if (combiner.usesT0)
				TextureCache_Update(0);
			else
				TextureCache_ActivateDummy(0);

			//Note: enabling dummies makes some F-zero X textures flicker.... strange.

			if (combiner.usesT1)
				TextureCache_Update(1);
			else
				TextureCache_ActivateDummy(1);
			combiner.current->compiled->UpdateTextureInfo(true);
		}
	}

	if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
	{
#ifndef OLD_BLENDMODE
		OGL_SetBlendMode();
#else
		if ((gDP.otherMode.forceBlender) &&
			(gDP.otherMode.cycleType != G_CYC_COPY) &&
			(gDP.otherMode.cycleType != G_CYC_FILL) &&
			!(gDP.otherMode.alphaCvgSel))
		{
			glEnable( GL_BLEND );

			switch (gDP.otherMode.l >> 16)
			{
				case 0x0448: // Add
				case 0x055A:
					glBlendFunc( GL_ONE, GL_ONE );
					break;
				case 0x0C08: // 1080 Sky
				case 0x0F0A: // Used LOTS of places
					glBlendFunc( GL_ONE, GL_ZERO );
					break;

				case 0x0040: // Fzero
				case 0xC810: // Blends fog
				case 0xC811: // Blends fog
				case 0x0C18: // Standard interpolated blend
				case 0x0C19: // Used for antialiasing
				case 0x0050: // Standard interpolated blend
				case 0x0055: // Used for antialiasing
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;

				case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
				case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
				case 0xAF50: // LOT in Zelda: MM
				case 0x0F5A: // LOT in Zelda: MM
					//clr_in * 0 + clr_mem * 1
					glBlendFunc( GL_ZERO, GL_ONE );
					break;

				default:
					LOG(LOG_VERBOSE, "Unhandled blend mode=%x", gDP.otherMode.l >> 16);
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;
			}
		}
		else
		{
			glDisable( GL_BLEND );
		}

		if (gDP.otherMode.cycleType == G_CYC_FILL)
		{
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glEnable( GL_BLEND );
		}
#endif
	}

	gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
	gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGL_AddTriangle(int v0, int v1, int v2)
{
	OGL.triangles.elements[OGL.triangles.num++] = v0;
	OGL.triangles.elements[OGL.triangles.num++] = v1;
	OGL.triangles.elements[OGL.triangles.num++] = v2;
}

void OGL_SetColorArray()
{
	if (combiner.usesShadeColor)
		glEnableVertexAttribArray(SC_COLOR);
	else
		glDisableVertexAttribArray(SC_COLOR);
}

void OGL_SetTexCoordArrays()
{
	if (combiner.usesT0)
		glEnableVertexAttribArray(SC_TEXCOORD0);
	else
		glDisableVertexAttribArray(SC_TEXCOORD0);

	if (combiner.usesT1)
		glEnableVertexAttribArray(SC_TEXCOORD1);
	else
		glDisableVertexAttribArray(SC_TEXCOORD1);

	if (OGL.renderState == GLInfo::rsTriangle && (combiner.usesT0 || combiner.usesT1))
		glEnableVertexAttribArray(SC_STSCALED);
	else
		glDisableVertexAttribArray(SC_STSCALED);
}

void OGL_DrawTriangles()
{
	if (OGL.triangles.num == 0) return;

#ifndef GLES2
	if (OGL.bImageTexture)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif

	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	const bool updateArrays = OGL.renderState != GLInfo::rsTriangle;
	if (updateArrays || combiner.changed) {
		OGL.renderState = GLInfo::rsTriangle;
		OGL_SetColorArray();
		OGL_SetTexCoordArrays();
		glDisableVertexAttribArray(SC_TEXCOORD1);
	}

	if (updateArrays) {
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].r);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].s);
		glVertexAttribPointer(SC_STSCALED, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].st_scaled);
		if (config.enableHWLighting) {
			glEnableVertexAttribArray(SC_NUMLIGHTS);
			glVertexAttribPointer(SC_NUMLIGHTS, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].HWLight);
		}

		OGL_UpdateCullFace();
		OGL_UpdateViewport();
		glEnable(GL_SCISSOR_TEST);
		Combiner_UpdateRenderState();
	}

	combiner.current->compiled->UpdateColors(true);
	combiner.current->compiled->UpdateLight(true);
	glDrawElements(GL_TRIANGLES, OGL.triangles.num, GL_UNSIGNED_BYTE, OGL.triangles.elements);
	OGL.triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	__indexmap_clear();
#endif
}

void OGL_DrawLine(int v0, int v1, float width )
{
	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	if (OGL.renderState != GLInfo::rsLine || combiner.changed)	{
		OGL_SetColorArray();
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].r);

		OGL_UpdateCullFace();
		OGL_UpdateViewport();
		OGL.renderState = GLInfo::rsLine;
		Combiner_UpdateRenderState();
	}

	unsigned short elem[2];
	elem[0] = v0;
	elem[1] = v1;
	glLineWidth( width * OGL.scaleX );
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, elem);
}

void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color )
{
	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	const bool updateArrays = OGL.renderState != GLInfo::rsRect;
	if (updateArrays || combiner.changed) {
		OGL.renderState = GLInfo::rsRect;
		glDisableVertexAttribArray(SC_COLOR);
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glDisableVertexAttribArray(SC_STSCALED);
	}

	if (updateArrays) {
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttrib4f(SC_POSITION, 0, 0, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0);
		glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
		combiner.current->compiled->UpdateRenderState();
	}

	if (frameBuffer.drawBuffer != GL_FRAMEBUFFER)
		glViewport( 0, (frameBuffer.drawBuffer == GL_BACK ? OGL.heightOffset : 0), OGL.width, OGL.height );
	else
		glViewport( 0, 0, frameBuffer.top->width*frameBuffer.top->scaleX, frameBuffer.top->height*frameBuffer.top->scaleY );
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);

	const float scaleX = frameBuffer.drawBuffer == GL_FRAMEBUFFER ? 1.0f/frameBuffer.top->width :  VI.rwidth;
	const float scaleY = frameBuffer.drawBuffer == GL_FRAMEBUFFER ? 1.0f/frameBuffer.top->height :  VI.rheight;
	OGL.rect[0].x = (float) ulx * (2.0f * scaleX) - 1.0;
	OGL.rect[0].y = (float) uly * (-2.0f * scaleY) + 1.0;
	OGL.rect[1].x = (float) (lrx+1) * (2.0f * scaleX) - 1.0;
	OGL.rect[1].y = OGL.rect[0].y;
	OGL.rect[2].x = OGL.rect[0].x;
	OGL.rect[2].y = (float) (lry+1) * (-2.0f * scaleY) + 1.0;
	OGL.rect[3].x = OGL.rect[1].x;
	OGL.rect[3].y = OGL.rect[2].y;

	glVertexAttrib4fv(SC_COLOR, color);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_SCISSOR_TEST);
	OGL_UpdateViewport();
}

void GLS_SetShadowMapCombiner();
void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip )
{
	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	const bool updateArrays = OGL.renderState != GLInfo::rsTexRect;
	if (updateArrays || combiner.changed) {
		OGL.renderState = GLInfo::rsTexRect;
		glDisableVertexAttribArray(SC_COLOR);
		OGL_SetTexCoordArrays();
	}

	if (updateArrays) {
#ifdef RENDERSTATE_TEST
		StateChanges++;
#endif
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttrib4f(SC_POSITION, 0, 0, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0);
		glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s0);
		glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s1);
		combiner.current->compiled->UpdateRenderState();
	}

#ifndef GLES2
	//	if ((gDP.otherMode.l >> 16) == 0x3c18 && gDP.combine.muxs0 == 0x00ffffff && gDP.combine.muxs1 == 0xfffff238) //depth image based fog
	if (gSP.textureTile[0]->frameBuffer == NULL && gSP.textureTile[1]->frameBuffer == NULL && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress +  gDP.colorImage.width*gDP.colorImage.width*6/4))
		GLS_SetShadowMapCombiner();
#endif // GLES2

	if (frameBuffer.drawBuffer != GL_FRAMEBUFFER)
		glViewport( 0, (frameBuffer.drawBuffer == GL_BACK ? OGL.heightOffset : 0), OGL.width, OGL.height );
	else
		glViewport( 0, 0, frameBuffer.top->width*frameBuffer.top->scaleX, frameBuffer.top->height*frameBuffer.top->scaleY );
	glDisable( GL_CULL_FACE );

	const float scaleX = frameBuffer.drawBuffer == GL_FRAMEBUFFER ? 1.0f/frameBuffer.top->width :  VI.rwidth;
	const float scaleY = frameBuffer.drawBuffer == GL_FRAMEBUFFER ? 1.0f/frameBuffer.top->height :  VI.rheight;
	OGL.rect[0].x = (float) ulx * (2.0f * scaleX) - 1.0f;
	OGL.rect[0].y = (float) uly * (-2.0f * scaleY) + 1.0f;
	OGL.rect[1].x = (float) (lrx) * (2.0f * scaleX) - 1.0f;
	OGL.rect[1].y = OGL.rect[0].y;
	OGL.rect[2].x = OGL.rect[0].x;
	OGL.rect[2].y = (float) (lry) * (-2.0f * scaleY) + 1.0f;
	OGL.rect[3].x = OGL.rect[1].x;
	OGL.rect[3].y = OGL.rect[2].y;

	if (combiner.usesT0 && cache.current[0] && gSP.textureTile[0]) {
		OGL.rect[0].s0 = uls * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		OGL.rect[0].t0 = ult * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
		OGL.rect[3].s0 = (lrs + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		OGL.rect[3].t0 = (lrt + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

		if ((cache.current[0]->maskS) && !(cache.current[0]->mirrorS) && (fmod( OGL.rect[0].s0, cache.current[0]->width ) == 0.0f)) {
			OGL.rect[3].s0 -= OGL.rect[0].s0;
			OGL.rect[0].s0 = 0.0f;
		}

		if ((cache.current[0]->maskT)  && !(cache.current[0]->mirrorT) && (fmod( OGL.rect[0].t0, cache.current[0]->height ) == 0.0f)) {
			OGL.rect[3].t0 -= OGL.rect[0].t0;
			OGL.rect[0].t0 = 0.0f;
		}

		if (cache.current[0]->frameBufferTexture)
		{
			OGL.rect[0].s0 = cache.current[0]->offsetS + OGL.rect[0].s0;
			OGL.rect[0].t0 = cache.current[0]->offsetT - OGL.rect[0].t0;
			OGL.rect[3].s0 = cache.current[0]->offsetS + OGL.rect[3].s0;
			OGL.rect[3].t0 = cache.current[0]->offsetT - OGL.rect[3].t0;
		}

		glActiveTexture( GL_TEXTURE0 );

		if ((OGL.rect[0].s0 >= 0.0f) && (OGL.rect[3].s0 <= cache.current[0]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((OGL.rect[0].t0 >= 0.0f) && (OGL.rect[3].t0 <= cache.current[0]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		OGL.rect[0].s0 *= cache.current[0]->scaleS;
		OGL.rect[0].t0 *= cache.current[0]->scaleT;
		OGL.rect[3].s0 *= cache.current[0]->scaleS;
		OGL.rect[3].t0 *= cache.current[0]->scaleT;
	}

	if (combiner.usesT1 && cache.current[1] && gSP.textureTile[1])
	{
		OGL.rect[0].s1 = uls * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		OGL.rect[0].t1 = ult * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
		OGL.rect[3].s1 = (lrs + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		OGL.rect[3].t1 = (lrt + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

		if ((cache.current[1]->maskS) && (fmod( OGL.rect[0].s1, cache.current[1]->width ) == 0.0f) && !(cache.current[1]->mirrorS))
		{
			OGL.rect[3].s1 -= OGL.rect[0].s1;
			OGL.rect[0].s1 = 0.0f;
		}

		if ((cache.current[1]->maskT) && (fmod( OGL.rect[0].t1, cache.current[1]->height ) == 0.0f) && !(cache.current[1]->mirrorT))
		{
			OGL.rect[3].t1 -= OGL.rect[0].t1;
			OGL.rect[0].t1 = 0.0f;
		}

		if (cache.current[1]->frameBufferTexture)
		{
			OGL.rect[0].s1 = cache.current[1]->offsetS + OGL.rect[0].s1;
			OGL.rect[0].t1 = cache.current[1]->offsetT - OGL.rect[0].t1;
			OGL.rect[3].s1 = cache.current[1]->offsetS + OGL.rect[3].s1;
			OGL.rect[3].t1 = cache.current[1]->offsetT - OGL.rect[3].t1;
		}

		glActiveTexture( GL_TEXTURE1 );

		if ((OGL.rect[0].s1 == 0.0f) && (OGL.rect[3].s1 <= cache.current[1]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((OGL.rect[0].t1 == 0.0f) && (OGL.rect[3].t1 <= cache.current[1]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		OGL.rect[0].s1 *= cache.current[1]->scaleS;
		OGL.rect[0].t1 *= cache.current[1]->scaleT;
		OGL.rect[3].s1 *= cache.current[1]->scaleS;
		OGL.rect[3].t1 *= cache.current[1]->scaleT;
	}

	if ((gDP.otherMode.cycleType == G_CYC_COPY) && !config.texture.forceBilinear)
	{
		glActiveTexture( GL_TEXTURE0 );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	if (flip)
	{
		OGL.rect[1].s0 = OGL.rect[0].s0;
		OGL.rect[1].t0 = OGL.rect[3].t0;
		OGL.rect[1].s1 = OGL.rect[0].s1;
		OGL.rect[1].t1 = OGL.rect[3].t1;
		OGL.rect[2].s0 = OGL.rect[3].s0;
		OGL.rect[2].t0 = OGL.rect[0].t0;
		OGL.rect[2].s1 = OGL.rect[3].s1;
		OGL.rect[2].t1 = OGL.rect[0].t1;
	}
	else
	{
		OGL.rect[1].s0 = OGL.rect[3].s0;
		OGL.rect[1].t0 = OGL.rect[0].t0;
		OGL.rect[1].s1 = OGL.rect[3].s1;
		OGL.rect[1].t1 = OGL.rect[0].t1;
		OGL.rect[2].s0 = OGL.rect[0].s0;
		OGL.rect[2].t0 = OGL.rect[3].t0;
		OGL.rect[2].s1 = OGL.rect[0].s1;
		OGL.rect[2].t1 = OGL.rect[3].t1;
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	OGL_UpdateViewport();
}

void OGL_ClearDepthBuffer()
{
	if (config.frameBufferEmulation.enable && frameBuffer.top == NULL)
		return;

	DepthBuffer_ClearBuffer();

	glDisable( GL_SCISSOR_TEST );

	OGL_UpdateStates();
	glDepthMask( TRUE );
	glClear( GL_DEPTH_BUFFER_BIT );

	OGL_UpdateDepthUpdate();

	glEnable( GL_SCISSOR_TEST );
}

void OGL_ClearColorBuffer( float *color )
{
	glDisable( GL_SCISSOR_TEST );

	glClearColor( color[0], color[1], color[2], color[3] );
	glClear( GL_COLOR_BUFFER_BIT );

	glEnable( GL_SCISSOR_TEST );
}

void OGL_SaveScreenshot()
{
#ifdef _WINDOWS
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	HANDLE hBitmapFile;

	char *pixelData = (char*)malloc( OGL.width * OGL.height * 3 );

	GLint oldMode;
	glGetIntegerv( GL_READ_BUFFER, &oldMode );
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer( GL_FRONT );
	glReadPixels( 0, OGL.heightOffset, OGL.width, OGL.height, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixelData );
	glReadBuffer( oldMode );

	infoHeader.biSize = sizeof( BITMAPINFOHEADER );
	infoHeader.biWidth = OGL.width;
	infoHeader.biHeight = OGL.height;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 24;
	infoHeader.biCompression = BI_RGB;
	infoHeader.biSizeImage = OGL.width * OGL.height * 3;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	fileHeader.bfType = 19778;
	fileHeader.bfSize = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) + infoHeader.biSizeImage;
	fileHeader.bfReserved1 = fileHeader.bfReserved2 = 0;
	fileHeader.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER );

	char filename[256];

	CreateDirectory( screenDirectory, NULL );

	int i = 0;
	do
	{
		sprintf( filename, "%sscreen%02i.bmp", screenDirectory, i );
		i++;

		if (i > 99)
			return;

		hBitmapFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
	}
	while (hBitmapFile == INVALID_HANDLE_VALUE);

	DWORD written;

	WriteFile( hBitmapFile, &fileHeader, sizeof( BITMAPFILEHEADER ), &written, NULL );
	WriteFile( hBitmapFile, &infoHeader, sizeof( BITMAPINFOHEADER ), &written, NULL );
	WriteFile( hBitmapFile, pixelData, infoHeader.biSizeImage, &written, NULL );

	CloseHandle( hBitmapFile );
	free( pixelData );
#endif // _WINDOWS
}

void OGL_ReadScreen( void **dest, long *width, long *height )
{
	*width = OGL.width;
	*height = OGL.height;

	*dest = malloc( OGL.height * OGL.width * 3 );
	if (*dest == NULL)
		return;

#ifndef GLES2
	const GLenum format = GL_BGR_EXT;
	glReadBuffer( GL_FRONT );
#else
	const GLenum format = GL_RGB;
#endif
	glReadPixels( 0, OGL.heightOffset, OGL.width, OGL.height, format, GL_UNSIGNED_BYTE, *dest );
}

void OGL_SwapBuffers()
{
#ifdef _WINDOWS
	if (OGL.hDC == NULL)
		SwapBuffers( wglGetCurrentDC() );
	else
		SwapBuffers( OGL.hDC );
#elif defined(USE_SDL)
	static int frames[5] = { 0, 0, 0, 0, 0 };
	static int framesIndex = 0;
	static Uint32 lastTicks = 0;
	Uint32 ticks = SDL_GetTicks();

	frames[framesIndex]++;
	if (ticks >= (lastTicks + 1000))
	{
		char caption[500];
		float fps = 0.0;
		for (int i = 0; i < 5; i++)
			fps += frames[i];
		fps /= 5.0;
		snprintf( caption, 500, "%s - %.2f fps", pluginName, fps );
		SDL_WM_SetCaption( caption, pluginName );
		framesIndex = (framesIndex + 1) % 5;
		frames[framesIndex] = 0;
		lastTicks = ticks;
	}

	SDL_GL_SwapBuffers();
#endif // _WINDOWS
}

bool checkFBO() {
	GLenum e = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	switch (e) {
//		case GL_FRAMEBUFFER_UNDEFINED:
//			printf("FBO Undefined\n");
//			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
			printf("FBO Incomplete Attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
			printf("FBO Missing Attachment\n");
			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
//			printf("FBO Incomplete Draw Buffer\n");
//			break;
		case GL_FRAMEBUFFER_UNSUPPORTED :
			printf("FBO Unsupported\n");
			break;
		case GL_FRAMEBUFFER_COMPLETE:
			printf("FBO OK\n");
			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
//			printf("framebuffer FRAMEBUFFER_DIMENSIONS\n");
//			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
//			printf("framebuffer INCOMPLETE_FORMATS\n");
//			break;
		default:
			printf("FBO Problem?\n");
	}
	return e == GL_FRAMEBUFFER_COMPLETE;
}
