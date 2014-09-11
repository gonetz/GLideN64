#include "GLideN64_MupenPlus.h"
#include <stdio.h>

#include "../OpenGL.h"
#include "../Config.h"

#ifndef _WINDOWS
void OGL_InitGLFunctions()
{
}
#endif

void OGL_ResizeWindow()
{
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
	//// paulscode, added for switching between modes RGBA8888 and RGB565
	// (part of the color banding fix)
	int bitsPP;
	if( Android_JNI_UseRGBA8888() )
		bitsPP = 32;
	else
		bitsPP = 16;
	////

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
	if (OGL.fullscreen){
		OGL.width = config.video.fullscreenWidth;
		OGL.height = config.video.fullscreenHeight;
	} else {
		OGL.width = config.video.windowedWidth;
		OGL.height = config.video.windowedHeight;
	}

#ifndef GLES2
	CoreVideo_Init();
	CoreVideo_GL_SetAttribute(M64P_GL_DOUBLEBUFFER, 1);
	CoreVideo_GL_SetAttribute(M64P_GL_SWAP_CONTROL, 1);
	CoreVideo_GL_SetAttribute(M64P_GL_BUFFER_SIZE, 16);
	CoreVideo_GL_SetAttribute(M64P_GL_DEPTH_SIZE, 16);

	printf("(II) Setting video mode %dx%d...\n", OGL.width, OGL.height);
	if (CoreVideo_SetVideoMode(OGL.width, OGL.height, 0, OGL.fullscreen ? M64VIDEO_FULLSCREEN : M64VIDEO_WINDOWED, (m64p_video_flags) 0) != M64ERR_SUCCESS) {
		printf("(EE) Error setting videomode %dx%d\n", OGL.width, OGL.height);
		CoreVideo_Quit();
		return false;
	}

	char caption[500];
	# ifdef _DEBUG
	sprintf(caption, "GLideN64 debug");
	# else // _DEBUG
	sprintf(caption, "GLideN64");
	# endif // _DEBUG
	CoreVideo_SetCaption(caption);

#else // GLES2
	if (!OGL_SDL_Start())
		return false;
#endif // GLES2

	OGL_InitData();

	return true;
}

void OGL_Stop()
{
	OGL_DestroyData();

#ifndef GLES2
	CoreVideo_Quit();
#else
#if defined(USE_SDL)
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	OGL.hScreen = NULL;
#endif
#endif // GLES2
}

void OGL_SwapBuffers()
{
#ifndef GLES2
   CoreVideo_GL_SwapBuffers();
#else
	Android_JNI_SwapWindow(); // paulscode, fix for black-screen bug
#endif // GLES2
}

void OGL_SaveScreenshot()
{
}
