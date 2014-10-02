#include "GLideN64_MupenPlus.h"
#include <stdio.h>

#include "../GLideN64.h"
#include "../OpenGL.h"
#include "../Config.h"
#include "../Revision.h"

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
	m_bFullscreen = config.video.fullscreen > 0;
	m_width = config.video.windowedWidth;
	m_height = config.video.windowedHeight;

	CoreVideo_Init();
	CoreVideo_GL_SetAttribute(M64P_GL_DOUBLEBUFFER, 1);
	CoreVideo_GL_SetAttribute(M64P_GL_SWAP_CONTROL, config.video.verticalSync);
	CoreVideo_GL_SetAttribute(M64P_GL_BUFFER_SIZE, 32);
	CoreVideo_GL_SetAttribute(M64P_GL_DEPTH_SIZE, 16);

	if (config.video.multisampling > 0) {
		CoreVideo_GL_SetAttribute(M64P_GL_MULTISAMPLEBUFFERS, 1);
		if (config.video.multisampling <= 2)
			CoreVideo_GL_SetAttribute(M64P_GL_MULTISAMPLESAMPLES, 2);
		else if (config.video.multisampling <= 4)
			CoreVideo_GL_SetAttribute(M64P_GL_MULTISAMPLESAMPLES, 4);
		else if (config.video.multisampling <= 8)
			CoreVideo_GL_SetAttribute(M64P_GL_MULTISAMPLESAMPLES, 8);
		else
			CoreVideo_GL_SetAttribute(M64P_GL_MULTISAMPLESAMPLES, 16);
	}

	printf("(II) Setting video mode %dx%d...\n", m_width, m_height);
	if (CoreVideo_SetVideoMode(m_width, m_height, 0, m_bFullscreen ? M64VIDEO_FULLSCREEN : M64VIDEO_WINDOWED, (m64p_video_flags)0) != M64ERR_SUCCESS) {
		printf("(EE) Error setting videomode %dx%d\n", m_width, m_height);
		CoreVideo_Quit();
		return false;
	}

	char caption[128];
# ifdef _DEBUG
	sprintf(caption, "%s debug. Revision %s", pluginName, PLUGIN_REVISION);
# else // _DEBUG
	sprintf(caption, "%s. Revision %s", pluginName, PLUGIN_REVISION);
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
	// if emulator defined a render callback function, call it before buffer swap
	if (renderCallback)
		(*renderCallback)((gSP.changed&CHANGED_CPU_FB_WRITE) == 0 ? 1 : 0);
	CoreVideo_GL_SwapBuffers();
}

void OGLVideoMupenPlus::_saveScreenshot()
{
}

void OGLVideoMupenPlus::_resizeWindow()
{
	u32 newWidth, newHeight;
	newWidth = config.video.windowedWidth;
	newHeight = config.video.windowedHeight;
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
