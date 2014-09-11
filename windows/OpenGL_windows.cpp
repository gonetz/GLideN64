#include "GLideN64_Windows.h"
#include <stdio.h>
#include "../GLideN64.h"
#include "../Config.h"
#include "../OpenGL.h"

void OGL_ResizeWindow()
{
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
}

bool OGL_Start()
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

	if ((OGL.hDC = GetDC( hWnd )) == NULL) {
		MessageBox( hWnd, "Error while getting a device context!", pluginName, MB_ICONERROR | MB_OK );
		return false;
	}

	if ((pixelFormat = ChoosePixelFormat( OGL.hDC, &pfd )) == 0) {
		MessageBox( hWnd, "Unable to find a suitable pixel format!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return false;
	}

	if ((SetPixelFormat( OGL.hDC, pixelFormat, &pfd )) == FALSE) {
		MessageBox( hWnd, "Error while setting pixel format!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return false;
	}

	if ((OGL.hRC = wglCreateContext( OGL.hDC )) == NULL) {
		MessageBox( hWnd, "Error while creating OpenGL context!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return false;
	}

	if ((wglMakeCurrent( OGL.hDC, OGL.hRC )) == FALSE) {
		MessageBox( hWnd, "Error while making OpenGL context current!", pluginName, MB_ICONERROR | MB_OK );
		OGL_Stop();
		return false;
	}

	OGL_InitData();

	return true;
}

void OGL_Stop()
{
	OGL_DestroyData();
	wglMakeCurrent( NULL, NULL );

	if (OGL.hRC) {
		wglDeleteContext( OGL.hRC );
		OGL.hRC = NULL;
	}

	if (OGL.hDC) {
		ReleaseDC( hWnd, OGL.hDC );
		OGL.hDC = NULL;
	}
}

void OGL_SwapBuffers()
{
	if (OGL.hDC == NULL)
		SwapBuffers( wglGetCurrentDC() );
	else
		SwapBuffers( OGL.hDC );
}

void OGL_SaveScreenshot()
{
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
}
