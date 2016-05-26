#include "GLideN64_mupenplus.h"
#include <stdio.h>

#include "../GLideN64.h"
#include "../OpenGL.h"
#include "../gDP.h"
#include "../Config.h"
#include "../Revision.h"
#include "../Log.h"

#ifdef VC
#include <bcm_host.h>
#endif

#if !defined(OS_WINDOWS) || defined(GLES2) || defined(GLES3) || defined(GLES3_1)

void initGLFunctions()
{
}
#endif

class OGLVideoMupenPlus : public OGLVideo
{
public:
	OGLVideoMupenPlus() {}

private:
	void _setAttributes();
	void _getDisplaySize();

	virtual bool _start();
	virtual void _stop();
	virtual void _swapBuffers();
	virtual void _saveScreenshot();
	virtual bool _resizeWindow();
	virtual void _changeWindow();
};

OGLVideo & OGLVideo::get()
{
	static OGLVideoMupenPlus video;
	return video;
}

void OGLVideoMupenPlus::_setAttributes()
{

#ifdef GLES2
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MAJOR_VERSION, 2);
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MINOR_VERSION, 0);
	LOG(LOG_VERBOSE, "[gles2GlideN64]: _setAttributes\n");
#elif defined(GLES3)
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MAJOR_VERSION, 3);
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(GLES3_1)
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MAJOR_VERSION, 3);
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MINOR_VERSION, 1);
#elif defined(OS_MAC_OS_X)
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MAJOR_VERSION, 3);
	CoreVideo_GL_SetAttribute(M64P_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// Do nothing
#endif

#if defined(GLES3) || defined (GLES3_1)
	CoreVideo_GL_SetAttribute(M64P_GL_RED_SIZE, 8);
	CoreVideo_GL_SetAttribute(M64P_GL_GREEN_SIZE, 8);
	CoreVideo_GL_SetAttribute(M64P_GL_BLUE_SIZE, 8);
	CoreVideo_GL_SetAttribute(M64P_GL_ALPHA_SIZE, 8);
#endif

	CoreVideo_GL_SetAttribute(M64P_GL_DOUBLEBUFFER, 1);
	CoreVideo_GL_SetAttribute(M64P_GL_SWAP_CONTROL, config.video.verticalSync);
	CoreVideo_GL_SetAttribute(M64P_GL_BUFFER_SIZE, 32);
	CoreVideo_GL_SetAttribute(M64P_GL_DEPTH_SIZE, 16);
	if (config.video.multisampling > 0 && config.frameBufferEmulation.enable == 0) {
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
}

bool OGLVideoMupenPlus::_start()
{
	CoreVideo_Init();
	_setAttributes();

	m_bFullscreen = config.video.fullscreen > 0;
	m_screenWidth = config.video.windowedWidth;
	m_screenHeight = config.video.windowedHeight;
	_getDisplaySize();
	_setBufferSize();

	printf("(II) Setting video mode %dx%d...\n", m_screenWidth, m_screenHeight);
	const m64p_video_flags flags = M64VIDEOFLAG_SUPPORT_RESIZING;
	if (CoreVideo_SetVideoMode(m_screenWidth, m_screenHeight, 0, m_bFullscreen ? M64VIDEO_FULLSCREEN : M64VIDEO_WINDOWED, flags) != M64ERR_SUCCESS) {
		//printf("(EE) Error setting videomode %dx%d\n", m_screenWidth, m_screenHeight);
		LOG(LOG_ERROR, "[gles2GlideN64]: Error setting videomode %dx%d\n", m_screenWidth, m_screenHeight);
		CoreVideo_Quit();
		return false;
	}
	LOG(LOG_VERBOSE, "[gles2GlideN64]: Create setting videomode %dx%d\n", m_screenWidth, m_screenHeight);

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
	if (renderCallback != NULL) {
		glUseProgram(0);
		if (config.frameBufferEmulation.N64DepthCompare == 0) {
			glViewport(0, getHeightOffset(), getScreenWidth(), getScreenHeight());
			gSP.changed |= CHANGED_VIEWPORT;
		}
		gDP.changed |= CHANGED_COMBINE;
		(*renderCallback)((gDP.changed&CHANGED_CPU_FB_WRITE) == 0 ? 1 : 0);
	}
	CoreVideo_GL_SwapBuffers();
}

void OGLVideoMupenPlus::_saveScreenshot()
{
}

bool OGLVideoMupenPlus::_resizeWindow()
{
	_setAttributes();

	m_bFullscreen = false;
	m_width = m_screenWidth = m_resizeWidth;
	m_height = m_screenHeight = m_resizeHeight;
	if (CoreVideo_ResizeWindow(m_screenWidth, m_screenHeight) != M64ERR_SUCCESS) {
		printf("(EE) Error setting videomode %dx%d\n", m_screenWidth, m_screenHeight);
		m_width = m_screenWidth = config.video.windowedWidth;
		m_height = m_screenHeight = config.video.windowedHeight;
		CoreVideo_Quit();
		return false;
	}
	_setBufferSize();
	isGLError(); // reset GL error.
	return true;
}

void OGLVideoMupenPlus::_changeWindow()
{
	CoreVideo_ToggleFullScreen();
}

void OGLVideoMupenPlus::_getDisplaySize()
{
#ifdef VC
	if( m_bFullscreen ) {
		// Use VC get_display_size function to get the current screen resolution
		u32 fb_width;
		u32 fb_height;
		if (graphics_get_display_size(0 /* LCD */, &fb_width, &fb_height) < 0)
			printf("ERROR: Failed to get display size\n");
		else {
			m_screenWidth = fb_width;
			m_screenHeight = fb_height;
		}
	}
#endif
}
