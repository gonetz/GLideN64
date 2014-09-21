#include "../winlnxdefs.h"
#include "../GLideN64.h"
#include "../OpenGL.h"
#include "../Config.h"

void OGL_InitGLFunctions()
{
}

class OGLVideoPosix : public OGLVideo
{
public:
	OGLVideoPosix() : hScreen(NULL) {}

private:
	virtual bool _start();
	virtual void _stop();
	virtual void _swapBuffers();
	virtual void _saveScreenshot();
	virtual void _resizeWindow();
	virtual void _changeWindow();

#if defined(USE_SDL)
	SDL_Surface *hScreen;
#endif
};

OGLVideo & OGLVideo::get()
{
	static OGLVideoPosix video;
	return video;
}

bool OGLVideoPosix::_start()
{
#ifdef USE_SDL
	// init sdl & gl
	Uint32 videoFlags = 0;

    if (m_bFullscreen) {
        m_width = config.video.fullscreenWidth;
        m_height = config.video.fullscreenHeight;
	} else {
        m_width = config.video.windowedWidth;
        m_height = config.video.windowedHeight;
	}

	/* Initialize SDL */
	printf( "[glN64]: (II) Initializing SDL video subsystem...\n" );
	if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
	{
		printf( "[glN64]: (EE) Error initializing SDL video subsystem: %s\n", SDL_GetError() );
		return false;
	}

	/* Video Info */
	const SDL_VideoInfo *videoInfo;
	printf( "[glN64]: (II) Getting video info...\n" );
	if (!(videoInfo = SDL_GetVideoInfo()))
	{
		printf( "[glN64]: (EE) Video query failed: %s\n", SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return false;
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

    printf( "[glN64]: (II) Setting video mode %dx%d...\n", m_width, m_height );
    if (!(hScreen = SDL_SetVideoMode(m_width, m_height, 0, videoFlags)))
	{
        printf( "[glN64]: (EE) Error setting videomode %dx%d: %s\n", m_width, m_height, SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_VIDEO );
		return false;
	}

	SDL_WM_SetCaption( pluginName, pluginName );
#endif // USE_SDL

	return true;
}

void OGLVideoPosix::_stop()
{
#if defined(USE_SDL)
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
	hScreen = NULL;
#endif // _WINDOWS
}

void OGLVideoPosix::_swapBuffers()
{
#if defined(USE_SDL)
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
#endif // USE_SDL
}

void OGLVideoPosix::_saveScreenshot()
{
}

void OGLVideoPosix::_resizeWindow()
{
}

void OGLVideoPosix::_changeWindow()
{
#if defined(USE_SDL)
	SDL_WM_ToggleFullScreen( hScreen );
#endif
}
