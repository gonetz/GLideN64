#include "GLideN64_Windows.h"
#include <stdio.h>
#include "../GLideN64.h"
#include "../Config.h"
#include "../OpenGL.h"
#include "../N64.h"
#include "../RSP.h"
#include "../FrameBuffer.h"
#include "../GLideNUI/GLideNUI.h"

class OGLVideoWindows : public OGLVideo
{
public:
	OGLVideoWindows() : hRC(NULL), hDC(NULL) {}

private:
	virtual bool _start();
	virtual void _stop();
	virtual void _swapBuffers();
	virtual void _saveScreenshot();
	virtual bool _resizeWindow();
	virtual void _changeWindow();

	HGLRC	hRC;
	HDC		hDC;
};

OGLVideo & OGLVideo::get()
{
	static OGLVideoWindows video;
	return video;
}

bool OGLVideoWindows::_start()
{
	int pixelFormat;

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

	if (hWnd == NULL)
		hWnd = GetActiveWindow();

	if ((hDC = GetDC( hWnd )) == NULL) {
		MessageBox( hWnd, L"Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK );
		return false;
	}

	if ((pixelFormat = ChoosePixelFormat(hDC, &pfd )) == 0) {
		MessageBox( hWnd, L"Unable to find a suitable pixel format!", pluginNameW, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	if ((SetPixelFormat(hDC, pixelFormat, &pfd )) == FALSE) {
		MessageBox( hWnd, L"Error while setting pixel format!", pluginNameW, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	if ((hRC = wglCreateContext(hDC)) == NULL) {
		MessageBox( hWnd, L"Error while creating OpenGL context!", pluginNameW, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	if ((wglMakeCurrent(hDC, hRC)) == FALSE) {
		MessageBox( hWnd, L"Error while making OpenGL context current!", pluginNameW, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	return _resizeWindow();
}

void OGLVideoWindows::_stop()
{
	wglMakeCurrent( NULL, NULL );

	if (hRC != NULL) {
		wglDeleteContext(hRC);
		hRC = NULL;
	}

	if (hDC != NULL) {
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
}

void OGLVideoWindows::_swapBuffers()
{
	if (hDC == NULL)
		SwapBuffers( wglGetCurrentDC() );
	else
		SwapBuffers( hDC );
}

void OGLVideoWindows::_saveScreenshot()
{
	unsigned char * pixelData = NULL;
	FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
	if (pBuffer == nullptr) {
		GLint oldMode;
		glGetIntegerv(GL_READ_BUFFER, &oldMode);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glReadBuffer(GL_FRONT);
		pixelData = (unsigned char*)malloc(m_screenWidth * m_screenHeight * 3);
		glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
		glReadBuffer(oldMode);
		SaveScreenshot(m_strScreenDirectory, RSP.romname, m_screenWidth, m_screenHeight, pixelData);
	}
	else {
		if (config.video.multisampling != 0) {
			pBuffer->resolveMultisampledTexture();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
		} else
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
		pixelData = (unsigned char*)malloc(pBuffer->m_pTexture->realWidth * pBuffer->m_pTexture->realHeight * 3);
		glReadPixels(0, 0, pBuffer->m_pTexture->realWidth, pBuffer->m_pTexture->realHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
		SaveScreenshot(m_strScreenDirectory, RSP.romname, pBuffer->m_pTexture->realWidth, pBuffer->m_pTexture->realHeight, pixelData);
	}
	free( pixelData );
}

void OGLVideoWindows::_changeWindow()
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

bool OGLVideoWindows::_resizeWindow()
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
