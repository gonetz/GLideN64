#include "WindowsWGL.h"
#include <Config.h>
#include <GLideN64.h>
#include <Graphics/OpenGLContext/GLFunctions.h>

HGLRC WindowsWGL::hRC = NULL;
HDC WindowsWGL::hDC = NULL;

bool WindowsWGL::start()
{
	int pixelFormat;

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST1\n");

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
	LOG(LOG_APIFUNC, "WindowsWGL::start TEST2\n");

	if (hWnd == NULL)
		hWnd = GetActiveWindow();

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST3\n");
	if ((hDC = GetDC(hWnd)) == NULL) {
		MessageBox(hWnd, L"Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK);
		return false;
	}

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST4\n");
	if ((pixelFormat = ChoosePixelFormat(hDC, &pfd)) == 0) {
		MessageBox(hWnd, L"Unable to find a suitable pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST5\n");
	if ((SetPixelFormat(hDC, pixelFormat, &pfd)) == FALSE) {
		MessageBox(hWnd, L"Error while setting pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST6\n");
	if ((hRC = wglCreateContext(hDC)) == NULL) {
		MessageBox(hWnd, L"Error while creating OpenGL context!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST7\n");
	if ((wglMakeCurrent(hDC, hRC)) == FALSE) {
		MessageBox(hWnd, L"Error while making OpenGL context current!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST7.1\n");
	initGLFunctions();
	LOG(LOG_APIFUNC, "WindowsWGL::start TEST7.2\n");

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST8\n");
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
		(PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

	LOG(LOG_APIFUNC, "WindowsWGL::start TEST9\n");
	if (wglGetExtensionsStringARB != NULL) {
		const char * wglextensions = wglGetExtensionsStringARB(hDC);
		LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.1\n");

		if (strstr(wglextensions, "WGL_ARB_create_context_profile") != nullptr) {
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.1.1\n");
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
				(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.2\n");

			GLint majorVersion = 0;
			ptrGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.3\n");
			GLint minorVersion = 0;
			ptrGetIntegerv(GL_MINOR_VERSION, &minorVersion);
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.4\n");

			const int attribList[] =
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
				WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				0        //End
			};

			HGLRC coreHrc = wglCreateContextAttribsARB(hDC, 0, attribList);
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.5\n");
			if (coreHrc != NULL) {
				wglDeleteContext(hRC);
				wglMakeCurrent(hDC, coreHrc);
				hRC = coreHrc;
			}
		}
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.1.1\n");

		if (strstr(wglextensions, "WGL_EXT_swap_control") != nullptr) {
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.6\n");
			PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.7\n");
			wglSwapIntervalEXT(config.video.verticalSync);
			LOG(LOG_APIFUNC, "WindowsWGL::start TEST9.8\n");
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

