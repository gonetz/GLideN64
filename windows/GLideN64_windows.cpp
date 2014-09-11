#include "GLideN64_Windows.h"
#include "../OpenGL.h"
#include "../Config.h"

HWND		hWnd;
HWND		hStatusBar;
HWND		hToolBar;
HINSTANCE	hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID /*lpvReserved*/)
{
	hInstance = hinstDLL;

	if (dwReason == DLL_PROCESS_ATTACH) {
		Config_LoadConfig();
		OGL.hRC = NULL;
		OGL.hDC = NULL;
	}
	return TRUE;
}
