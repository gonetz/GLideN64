#include <stdio.h>
#include <Graphics/OpenGLContext/GLFunctions.h>
#include <GL/wglext.h>
#include <windows/GLideN64_Windows.h>
#include <GLideN64.h>
#include <Config.h>
#include <N64.h>
#include <RSP.h>
#include <FrameBuffer.h>
#include <GLideNUI/GLideNUI.h>
#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include <DisplayWindow.h>
#include <EGL/eglext.h>

extern "C"
{
	EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT(EGLenum platform, void* native_display, const EGLint* attrib_list);
}

extern void SaveScreenshot(const wchar_t* _folder, const char* _name, int _width, int _height, const unsigned char* _data);

class DisplayWindowWindows : public DisplayWindow
{
public:
	DisplayWindowWindows() : eglDisplay(NULL), eglSurface(NULL), hDC(NULL) {}

private:
	bool _start() override;
	void _stop() override;
	void _swapBuffers() override;
	void _saveScreenshot() override;
	void _saveBufferContent(graphics::ObjectHandle _fbo, CachedTexture *_pTexture) override;
	bool _resizeWindow() override;
	void _changeWindow() override;
	void _readScreen(void **_pDest, long *_pWidth, long *_pHeight) override;
	void _readScreen2(void * _dest, int * _width, int * _height, int _front) override {}
	graphics::ObjectHandle _getDefaultFramebuffer() override;

	HDC		hDC;

	EGLDisplay eglDisplay;
	EGLSurface eglSurface;
	EGLContext eglContext;
};

DisplayWindow & DisplayWindow::get()
{
	static DisplayWindowWindows video;
	return video;
}

#define EGL_PLATFORM_ANGLE_ANGLE          0x3202
#define EGL_PLATFORM_ANGLE_TYPE_ANGLE     0x3203

#define EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE 0x3207
#define EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE 0x3208
#define EGL_PLATFORM_ANGLE_D3D11ON12_ANGLE 0x3488
#define EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE 0x320D
#define EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE 0x3450

#define EGL_DIRECT_COMPOSITION_ANGLE 0x33A5

bool DisplayWindowWindows::_start()
{
	if (hWnd == NULL)
		hWnd = GetActiveWindow();

	hWndThread = GetWindowThreadProcessId(hWnd, nullptr);

	if ((hDC = GetDC(hWnd)) == NULL) {
		MessageBox(hWnd, L"Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK);
		return false;
	}

	std::vector<EGLint> dispOptions;
	dispOptions.reserve(6);

	dispOptions.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
	EGLint renderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
	switch (config.angle.renderer)
	{
	default:
	case Config::arDirectX11:
		renderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
		break;
	case Config::arVulkan:
		renderer = EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE;
		break;
	case Config::arOpenGL:
		renderer = EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE;
		break;
	}
	dispOptions.push_back(renderer);

#if 0
	if (renderer == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE && config.angle.dx11to12)
	{
		dispOptions.push_back(EGL_PLATFORM_ANGLE_D3D11ON12_ANGLE);
		dispOptions.push_back(EGL_TRUE);
	}
#endif

	dispOptions.push_back(EGL_NONE);
	dispOptions.push_back(EGL_NONE);

	eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, hDC, dispOptions.data());
	EGLint eglVersionMajor, eglVersionMinor;

	if (!eglInitialize(eglDisplay, &eglVersionMajor, &eglVersionMinor))
	{
		int err = eglGetError();
		MessageBox(hWnd, (L"eglInitialize failed: " + std::to_wstring(err)).c_str(), pluginNameW, MB_ICONERROR | MB_OK);
		_stop();
		return false;
	}

	if (!eglBindAPI(EGL_OPENGL_ES_API))
	{
		int err = eglGetError();
		MessageBox(hWnd, (L"eglBindAPI failed: " + std::to_wstring(err)).c_str(), pluginNameW, MB_ICONERROR | MB_OK);
		_stop();
		return false;
	}

	EGLint configAttributes[] =
	{
		EGL_BUFFER_SIZE, 0,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_LEVEL, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SAMPLE_BUFFERS, 0,
		EGL_SAMPLES, 0,
		EGL_STENCIL_SIZE, 0,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_TRANSPARENT_TYPE, EGL_NONE,
		EGL_NONE
	};

	EGLint numConfigs;
	EGLConfig windowConfig;
	if (!eglChooseConfig(eglDisplay, configAttributes, &windowConfig, 1, &numConfigs))
	{
		int err = eglGetError();
		MessageBox(hWnd, (L"eglChooseConfig failed: " + std::to_wstring(err)).c_str(), pluginNameW, MB_ICONERROR | MB_OK);
		_stop();
		return false;
	}

	EGLint surfaceAttributes[] = { EGL_NONE, EGL_NONE, EGL_NONE, EGL_NONE };
	bool directComposition = renderer == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE && config.angle.directComposition;
	if (directComposition)
	{
		surfaceAttributes[0] = EGL_DIRECT_COMPOSITION_ANGLE;
		surfaceAttributes[1] = EGL_TRUE;
	}

	eglSurface = eglCreateWindowSurface(eglDisplay, windowConfig, hWnd, surfaceAttributes);
	if (!eglSurface)
	{
		int err = eglGetError();
		MessageBox(hWnd, (L"eglCreateWindowSurface failed: " + std::to_wstring(err)).c_str(), pluginNameW, MB_ICONERROR | MB_OK);
		_stop();
		return false;
	}

	EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	eglContext = eglCreateContext(eglDisplay, windowConfig, NULL, contextAttributes);
	if (!eglContext)
	{
		int err = eglGetError();
		MessageBox(hWnd, (L"eglCreateContext failed: " + std::to_wstring(err)).c_str(), pluginNameW, MB_ICONERROR | MB_OK);
		_stop();
		return false;
	}

	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

	eglSwapInterval(eglDisplay, config.video.verticalSync);

	return _resizeWindow();
}

void DisplayWindowWindows::_stop()
{
	eglMakeCurrent(eglDisplay, NULL, NULL, NULL);

	if (eglContext != NULL) {
		eglDestroyContext(eglDisplay, eglContext);
		eglContext = NULL;
	}

	if (eglSurface != NULL) {
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = NULL;
	}

	if (eglDisplay != NULL) {
		eglTerminate(eglDisplay);
		eglDisplay = NULL;
	}

	if (hDC != NULL) {
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
}

void DisplayWindowWindows::_swapBuffers()
{
	eglSwapBuffers(eglDisplay, eglSurface);
}

void DisplayWindowWindows::_saveScreenshot()
{
	unsigned char * rgbaPixelData = NULL;
	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, graphics::ObjectHandle::defaultFramebuffer);
	glReadBuffer(GL_FRONT);
	rgbaPixelData = (unsigned char*)malloc(m_screenWidth * m_screenHeight * 4);
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixelData);
	unsigned char* pixelData = (unsigned char*)malloc(m_screenWidth * m_screenHeight * 3);
	for (int i = 0; i < m_screenWidth * m_screenHeight; i++)
	{
		for (int j = 0; j < 3; j++)
			pixelData[3 * i + j] = rgbaPixelData[4 * i + j];
	}
	if (graphics::BufferAttachmentParam(oldMode) == graphics::bufferAttachment::COLOR_ATTACHMENT0) {
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer != nullptr)
			gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, pBuffer->m_FBO);
	}
	glReadBuffer(oldMode);
	SaveScreenshot(m_strScreenDirectory, RSP.romname, m_screenWidth, m_screenHeight, pixelData);
	free( pixelData );
	free(rgbaPixelData);
}

void DisplayWindowWindows::_saveBufferContent(graphics::ObjectHandle _fbo, CachedTexture *_pTexture)
{
	unsigned char * pixelData = NULL;
	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, _fbo);
	pixelData = (unsigned char*)malloc(_pTexture->realWidth * _pTexture->realHeight * 3);
	glReadPixels(0, 0, _pTexture->realWidth, _pTexture->realHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
	if (graphics::BufferAttachmentParam(oldMode) == graphics::bufferAttachment::COLOR_ATTACHMENT0) {
		FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
		if (pCurrentBuffer != nullptr)
			gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, pCurrentBuffer->m_FBO);
	}
	glReadBuffer(oldMode);
	SaveScreenshot(m_strScreenDirectory, RSP.romname, _pTexture->realWidth, _pTexture->realHeight, pixelData);
	free(pixelData);
}

void DisplayWindowWindows::_changeWindow()
{
	static LONG		windowedStyle;
	static LONG		windowedExStyle;
	static RECT		windowedRect;
	static HMENU	windowedMenu;

	if (!m_bFullscreen) {
		DEVMODE fullscreenMode;
		memset( &fullscreenMode, 0, sizeof(DEVMODE) );
		fullscreenMode.dmSize = sizeof(DEVMODE);
		fullscreenMode.dmPelsWidth = config.video.fullscreenWidth;
		fullscreenMode.dmPelsHeight = config.video.fullscreenHeight;
		fullscreenMode.dmBitsPerPel = 32;
		fullscreenMode.dmDisplayFrequency = config.video.fullscreenRefresh;
		fullscreenMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		if (ChangeDisplaySettings( &fullscreenMode, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL) {
			MessageBox( NULL, L"Failed to change display mode", pluginNameW, MB_ICONERROR | MB_OK );
			return;
		}

		ShowCursor( FALSE );

		windowedMenu = GetMenu( hWnd );

		if (windowedMenu)
			SetMenu( hWnd, NULL );

		if (hStatusBar)
			ShowWindow( hStatusBar, SW_HIDE );

		windowedExStyle = GetWindowLong( hWnd, GWL_EXSTYLE );
		windowedStyle = GetWindowLong( hWnd, GWL_STYLE );

		SetWindowLong( hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST );
		SetWindowLong( hWnd, GWL_STYLE, WS_POPUP );

		GetWindowRect( hWnd, &windowedRect );

		m_bFullscreen = true;
		_resizeWindow();
	} else {
		ChangeDisplaySettings( NULL, 0 );

		ShowCursor( TRUE );

		if (windowedMenu)
			SetMenu( hWnd, windowedMenu );

		if (hStatusBar)
			ShowWindow( hStatusBar, SW_SHOW );

		SetWindowLong( hWnd, GWL_STYLE, windowedStyle );
		SetWindowLong( hWnd, GWL_EXSTYLE, windowedExStyle );
		SetWindowPos( hWnd, NULL, windowedRect.left, windowedRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

		m_bFullscreen = false;
		_resizeWindow();
	}
}

bool DisplayWindowWindows::_resizeWindow()
{
	RECT windowRect, statusRect, toolRect;

	if (m_bFullscreen) {
		m_screenWidth = config.video.fullscreenWidth;
		m_screenHeight = config.video.fullscreenHeight;
		m_heightOffset = 0;
		_setBufferSize();

		return (SetWindowPos(hWnd, NULL, 0, 0, m_screenWidth, m_screenHeight, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW) == TRUE);
	} else {
		m_screenWidth = m_width = config.video.windowedWidth;
		m_screenHeight = config.video.windowedHeight;
		_setBufferSize();

		GetClientRect( hWnd, &windowRect );
		GetWindowRect( hStatusBar, &statusRect );

		if (hToolBar)
			GetWindowRect( hToolBar, &toolRect );
		else
			toolRect.bottom = toolRect.top = 0;

		m_heightOffset = (statusRect.bottom - statusRect.top);
		windowRect.right = windowRect.left + config.video.windowedWidth - 1;
		windowRect.bottom = windowRect.top + config.video.windowedHeight - 1 + m_heightOffset;

		AdjustWindowRect( &windowRect, GetWindowLong( hWnd, GWL_STYLE ), GetMenu( hWnd ) != NULL );

		return (SetWindowPos( hWnd, NULL, 0, 0, windowRect.right - windowRect.left + 1,
			windowRect.bottom - windowRect.top + 1 + toolRect.bottom - toolRect.top + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE ) == TRUE);
	}
}

void DisplayWindowWindows::_readScreen(void **_pDest, long *_pWidth, long *_pHeight)
{
	*_pWidth = m_width;
	*_pHeight = m_height;

	*_pDest = malloc(m_height * m_width * 3);
	if (*_pDest == nullptr)
		return;

#ifndef GLESX
	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, graphics::ObjectHandle::defaultFramebuffer);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, m_heightOffset, m_width, m_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, *_pDest);
	if (graphics::BufferAttachmentParam(oldMode) == graphics::bufferAttachment::COLOR_ATTACHMENT0) {
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer != nullptr)
			gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, pBuffer->m_FBO);
	}
	glReadBuffer(oldMode);
#else
	glReadPixels(0, m_heightOffset, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, *_pDest);
#endif
}

graphics::ObjectHandle DisplayWindowWindows::_getDefaultFramebuffer()
{
	return graphics::ObjectHandle::null;
}
