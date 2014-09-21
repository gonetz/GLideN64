#include "GLideN64_MupenPlus.h"
#include <stdio.h>

#include "../OpenGL.h"
#include "../Config.h"

#ifndef _WINDOWS

void initGLFunctions()
{
}
#endif

class OGLVideoMupenPlus : public OGLVideo
{
public:
	OGLVideoMupenPlus() {}

private:
	virtual bool _start();
	virtual void _stop();
	virtual void _swapBuffers();
	virtual void _saveScreenshot();
	virtual void _resizeWindow();
	virtual void _changeWindow();
};

OGLVideo & OGLVideo::get()
{
	static OGLVideoMupenPlus video;
	return video;
}

bool OGLVideoMupenPlus::_start()
{
	if (m_bFullscreen){
		m_width = config.video.fullscreenWidth;
		m_height = config.video.fullscreenHeight;
	} else {
		m_width = config.video.windowedWidth;
		m_height = config.video.windowedHeight;
	}

	CoreVideo_Init();
	CoreVideo_GL_SetAttribute(M64P_GL_DOUBLEBUFFER, 1);
	CoreVideo_GL_SetAttribute(M64P_GL_SWAP_CONTROL, 1);
	CoreVideo_GL_SetAttribute(M64P_GL_BUFFER_SIZE, 16);
	CoreVideo_GL_SetAttribute(M64P_GL_DEPTH_SIZE, 16);

	printf("(II) Setting video mode %dx%d...\n", m_width, m_height);
	if (CoreVideo_SetVideoMode(m_width, m_height, 0, m_bFullscreen ? M64VIDEO_FULLSCREEN : M64VIDEO_WINDOWED, (m64p_video_flags)0) != M64ERR_SUCCESS) {
		printf("(EE) Error setting videomode %dx%d\n", m_width, m_height);
		CoreVideo_Quit();
		return false;
	}

	char caption[128];
# ifdef _DEBUG
	sprintf(caption, "GLideN64 debug");
# else // _DEBUG
	sprintf(caption, "GLideN64");
# endif // _DEBUG
	CoreVideo_SetCaption(caption);

	return true;
}

void OGLVideoMupenPlus::_stop()
{
	CoreVideo_Quit();
}

void OGLVideoMupenPlus::_swapBuffers()
{
	CoreVideo_GL_SwapBuffers();
}

void OGLVideoMupenPlus::_saveScreenshot()
{
}

void OGLVideoMupenPlus::_resizeWindow()
{
	u32 newWidth, newHeight;
	if (m_bFullscreen) {
		newWidth = config.video.fullscreenWidth;
		newHeight = config.video.fullscreenHeight;
	} else {
		newWidth = config.video.windowedWidth;
		newHeight = config.video.windowedHeight;
	}
	if (m_width != newWidth || m_height != newHeight) {
		m_width = newWidth;
		m_height = newHeight;
		CoreVideo_ResizeWindow(m_width, m_height);
	}
}

void OGLVideoMupenPlus::_changeWindow()
{
	CoreVideo_ToggleFullScreen();
}
