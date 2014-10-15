#include "GLideN64_Windows.h"
#include <stdio.h>
#include "../GLideN64.h"
#include "../Config.h"
#include "../OpenGL.h"

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
		MessageBox( hWnd, "Error while getting a device context!", pluginName, MB_ICONERROR | MB_OK );
		return false;
	}

	if ((pixelFormat = ChoosePixelFormat(hDC, &pfd )) == 0) {
		MessageBox( hWnd, "Unable to find a suitable pixel format!", pluginName, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	if ((SetPixelFormat(hDC, pixelFormat, &pfd )) == FALSE) {
		MessageBox( hWnd, "Error while setting pixel format!", pluginName, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	if ((hRC = wglCreateContext(hDC)) == NULL) {
		MessageBox( hWnd, "Error while creating OpenGL context!", pluginName, MB_ICONERROR | MB_OK );
		_stop();
		return false;
	}

	if ((wglMakeCurrent(hDC, hRC)) == FALSE) {
		MessageBox( hWnd, "Error while making OpenGL context current!", pluginName, MB_ICONERROR | MB_OK );
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
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	HANDLE hBitmapFile;

	char *pixelData = (char*)malloc(m_screenWidth * m_screenHeight * 3);

	GLint oldMode;
	glGetIntegerv( GL_READ_BUFFER, &oldMode );
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer( GL_FRONT );
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixelData);
	glReadBuffer( oldMode );

	infoHeader.biSize = sizeof( BITMAPINFOHEADER );
	infoHeader.biWidth = m_screenWidth;
	infoHeader.biHeight = m_screenHeight;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 24;
	infoHeader.biCompression = BI_RGB;
	infoHeader.biSizeImage = m_screenWidth * m_screenHeight * 3;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	fileHeader.bfType = 19778;
	fileHeader.bfSize = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER ) + infoHeader.biSizeImage;
	fileHeader.bfReserved1 = fileHeader.bfReserved2 = 0;
	fileHeader.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER );

	char filename[256];

	CreateDirectory( m_strScreenDirectory, NULL );

	int i = 0;
	do {
		sprintf(filename, "%sscreen%02i.bmp", m_strScreenDirectory, i++);

		if (i > 99)
			return;

		hBitmapFile = CreateFile( filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
	} while (hBitmapFile == INVALID_HANDLE_VALUE);

	DWORD written;

	WriteFile( hBitmapFile, &fileHeader, sizeof( BITMAPFILEHEADER ), &written, NULL );
	WriteFile( hBitmapFile, &infoHeader, sizeof( BITMAPINFOHEADER ), &written, NULL );
	WriteFile( hBitmapFile, pixelData, infoHeader.biSizeImage, &written, NULL );

	CloseHandle( hBitmapFile );
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
		fullscreenMode.dmBitsPerPel = config.video.fullscreenBits;
		fullscreenMode.dmDisplayFrequency = config.video.fullscreenRefresh;
		fullscreenMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		if (ChangeDisplaySettings( &fullscreenMode, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL) {
			MessageBox( NULL, "Failed to change display mode", pluginName, MB_ICONERROR | MB_OK );
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
