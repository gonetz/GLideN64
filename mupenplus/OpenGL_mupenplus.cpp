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

bool OGL_Start()
{
	if (OGL.fullscreen){
		OGL.width = config.video.fullscreenWidth;
		OGL.height = config.video.fullscreenHeight;
	} else {
		OGL.width = config.video.windowedWidth;
		OGL.height = config.video.windowedHeight;
	}

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

	OGL_InitData();

	return true;
}

void OGL_Stop()
{
	OGL_DestroyData();

	CoreVideo_Quit();
}

void OGL_SwapBuffers()
{
	CoreVideo_GL_SwapBuffers();
}

void OGL_SaveScreenshot()
{
}
