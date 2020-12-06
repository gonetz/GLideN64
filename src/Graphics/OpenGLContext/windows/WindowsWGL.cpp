#include "WindowsWGL.h"
#include <Config.h>
#include <GLideN64.h>
#include <Graphics/OpenGLContext/GLFunctions.h>

HGLRC WindowsWGL::hRC = NULL;
HDC WindowsWGL::hDC = NULL;
unsigned int WindowsWGL::m_sMaxMsaa = 0;

bool WindowsWGL::start()
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

	if ((hDC = GetDC(hWnd)) == NULL) {
		MessageBox(hWnd, L"Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK);
		return false;
	}

	if ((pixelFormat = ChoosePixelFormat(hDC, &pfd)) == 0) {
		MessageBox(hWnd, L"Unable to find a suitable pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((SetPixelFormat(hDC, pixelFormat, &pfd)) == FALSE) {
		MessageBox(hWnd, L"Error while setting pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((hRC = wglCreateContext(hDC)) == NULL) {
		MessageBox(hWnd, L"Error while creating OpenGL context!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((wglMakeCurrent(hDC, hRC)) == FALSE) {
		MessageBox(hWnd, L"Error while making OpenGL context current!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	initGLFunctions();

	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
		(PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtensionsStringARB != NULL) {
		const char * wglextensions = wglGetExtensionsStringARB(hDC);

		if (strstr(wglextensions, "WGL_ARB_create_context_profile") != nullptr) {
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
				(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

			GLint majorVersion = 0;
			ptrGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
			GLint minorVersion = 0;
			ptrGetIntegerv(GL_MINOR_VERSION, &minorVersion);

			const int attribList[] =
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
				WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,
#ifdef FORCE_UNBUFFERED_DRAWER
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
				0        //End
			};

			HGLRC coreHrc = wglCreateContextAttribsARB(hDC, 0, attribList);
			if (coreHrc != NULL) {
				wglDeleteContext(hRC);
				wglMakeCurrent(hDC, coreHrc);
				hRC = coreHrc;
			}
		}

		if (strstr(wglextensions, "WGL_EXT_swap_control") != nullptr) {
			PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
			wglSwapIntervalEXT(config.video.verticalSync);
		}
	}

	if (m_sMaxMsaa > 0)
		return true;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB =
		(PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	if (wglChoosePixelFormatARB != nullptr) {

		const int piAttribIList16[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_SAMPLE_BUFFERS_ARB, 1,
			WGL_SAMPLES_EXT, 16,
			0 // zero term
		};
		const float pfAttribFList[] = { 0.0f };

		int piFormats;
		unsigned int nNumFormats;
		int res = wglChoosePixelFormatARB(hDC, piAttribIList16, pfAttribFList, 1, &piFormats, &nNumFormats);
		if (res > 0 && nNumFormats > 0)
			m_sMaxMsaa = 16;
		else {
			const int piAttribIList8[] =
			{
				WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
				WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
				WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
				WGL_COLOR_BITS_ARB, 32,
				WGL_DEPTH_BITS_ARB, 24,
				WGL_STENCIL_BITS_ARB, 8,
				WGL_SAMPLE_BUFFERS_ARB, 1,
				WGL_SAMPLES_EXT, 8,
				0 // zero term
			};
			res = wglChoosePixelFormatARB(hDC, piAttribIList16, pfAttribFList, 1, &piFormats, &nNumFormats);
			if (res > 0 && nNumFormats > 0)
				m_sMaxMsaa = 8;
			else
				m_sMaxMsaa = 4;
		}
	}

	return true;
}

void WindowsWGL::stop()
{
	wglMakeCurrent(NULL, NULL);

	if (hRC != NULL) {
		wglDeleteContext(hRC);
		hRC = NULL;
	}

	if (hDC != NULL) {
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
}

void WindowsWGL::swapBuffers()
{
	if (hDC == NULL)
		SwapBuffers(wglGetCurrentDC());
	else
		SwapBuffers(hDC);
}

unsigned int WindowsWGL::maxMSAALevel()
{
	if (hRC != NULL || m_sMaxMsaa > 0)
		return m_sMaxMsaa;

	start();
	stop();
	return m_sMaxMsaa;
}
