#include "WindowsWGL.h"
#include <Config.h>
#include <GLideN64.h>
#include <Graphics/OpenGLContext/GLFunctions.h>

HGLRC WindowsWGL::hRC = NULL;
HDC WindowsWGL::hDC = NULL;

namespace
{
const wchar_t RenderWindowClass[] = L"GLideN64RenderWindow";
HWND hRenderWnd = nullptr;
int menuRepaints = 0;

LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Let the frame window keep mouse and keyboard input.
	if (msg == WM_NCHITTEST)
		return HTTRANSPARENT;
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RegisterRenderWindowClass()
{
	WNDCLASS wc = {};
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = RenderWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = RenderWindowClass;
	RegisterClass(&wc);
}

// The frame window owns the menu. OpenGL on that window's DC stops the menu
// painting until the pointer moves, and it covers the status bar except the
// pane GDI repaints. RomOpen runs off the UI thread, so the child has to be
// created back on the window thread or CreateWindow would own it to a thread
// that does not dispatch input.
UINT UiCommandMessage()
{
	static UINT message = RegisterWindowMessageW(L"Project64_GLideN64_UiCommand");
	return message;
}

void CreateRenderWindowOnThisThread()
{
	if (hWnd == nullptr || hRenderWnd != nullptr)
		return;

	RegisterRenderWindowClass();
	RECT client = {};
	GetClientRect(hWnd, &client);
	hRenderWnd = CreateWindowExW(WS_EX_NOACTIVATE, RenderWindowClass, L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, client.right > 0 ? client.right : 1, client.bottom > 0 ? client.bottom : 1,
		hWnd, nullptr, hInstance, nullptr);
}

void DestroyRenderWindowOnThisThread()
{
	if (hRenderWnd == nullptr)
		return;
	DestroyWindow(hRenderWnd);
	hRenderWnd = nullptr;
}

void RunOnWindowThread(void (*command)())
{
	if (command == nullptr || hWnd == nullptr)
		return;
	if (GetCurrentThreadId() == GetWindowThreadProcessId(hWnd, nullptr))
	{
		command();
		return;
	}
	SendMessage(hWnd, UiCommandMessage(), 0, (LPARAM)command);
}

bool CreateRenderWindow()
{
	if (hWnd == nullptr)
		return false;
	RunOnWindowThread(CreateRenderWindowOnThisThread);
	if (hRenderWnd == nullptr)
		CreateRenderWindowOnThisThread();
	return hRenderWnd != nullptr;
}
}

void WindowsWGL::resizeRenderWindow()
{
	if (hRenderWnd == nullptr || hWnd == nullptr)
		return;

	RECT client = {};
	GetClientRect(hWnd, &client);
	int width = client.right - client.left;
	int height = client.bottom - client.top;
	int statusHeight = 0;
	if (hStatusBar != nullptr && IsWindowVisible(hStatusBar))
	{
		RECT status = {};
		GetWindowRect(hStatusBar, &status);
		statusHeight = status.bottom - status.top;
		if (statusHeight <= 0 || statusHeight >= height || statusHeight > 80)
			statusHeight = 22;
		SetWindowPos(hStatusBar, HWND_TOP, 0, height - statusHeight, width, statusHeight, SWP_NOACTIVATE);
		InvalidateRect(hStatusBar, nullptr, TRUE);
		height -= statusHeight;
	}
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
	SetWindowPos(hRenderWnd, HWND_BOTTOM, 0, 0, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static void DrawFrameOnThisThread()
{
	if (hWnd == nullptr)
		return;
	DrawMenuBar(hWnd);
	RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);
}

void WindowsWGL::refreshFrame()
{
	if (hWnd == nullptr)
		return;
	RunOnWindowThread(DrawFrameOnThisThread);
}

bool WindowsWGL::hasRenderWindow()
{
	return hRenderWnd != nullptr;
}

bool WindowsWGL::start()
{
	int pixelFormat;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd
		1,                                // version number
		PFD_DRAW_TO_WINDOW |              // support window
		PFD_SUPPORT_OPENGL |              // support OpenGL
		PFD_DOUBLEBUFFER |                // double buffered
		PFD_SUPPORT_COMPOSITION,          // composition support, see https://www.opengl.org/pipeline/article/vol003_7/
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

	menuRepaints = 0;

	HWND dcWindow = hWnd;
	if (CreateRenderWindow())
		dcWindow = hRenderWnd;

	if ((hDC = GetDC(dcWindow)) == NULL) {
		MessageBoxW(hWnd, L"Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK);
		return false;
	}

	if ((pixelFormat = ChoosePixelFormat(hDC, &pfd)) == 0) {
		MessageBoxW(hWnd, L"Unable to find a suitable pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((SetPixelFormat(hDC, pixelFormat, &pfd)) == FALSE) {
		MessageBoxW(hWnd, L"Error while setting pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((hRC = wglCreateContext(hDC)) == NULL) {
		MessageBoxW(hWnd, L"Error while creating OpenGL context!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((wglMakeCurrent(hDC, hRC)) == FALSE) {
		MessageBoxW(hWnd, L"Error while making OpenGL context current!", pluginNameW, MB_ICONERROR | MB_OK);
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

			// use adaptive vsync when supported and
			// when vsync is enabled
			if (strstr(wglextensions, "WGL_EXT_swap_control_tear") != nullptr &&
				config.video.verticalSync > 0) {
				wglSwapIntervalEXT(-1);
			} else {
				wglSwapIntervalEXT(config.video.verticalSync);
			}
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
		ReleaseDC(hRenderWnd != nullptr ? hRenderWnd : hWnd, hDC);
		hDC = NULL;
	}

	if (hRenderWnd != nullptr)
		RunOnWindowThread(DestroyRenderWindowOnThisThread);
}

void WindowsWGL::swapBuffers()
{
	if (hDC == NULL)
		SwapBuffers(wglGetCurrentDC());
	else
		SwapBuffers(hDC);

	// The first presents still land before the menu has been composed.
	// A later non-client paint is what a mouse move or a screenshot does.
	if (hWnd != nullptr && menuRepaints < 5)
	{
		++menuRepaints;
		PostMessage(hWnd, WM_NCPAINT, 1, 0);
	}
}
