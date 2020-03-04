#include "GLideN64_Windows.h"
#include "../GLideNUI/ConfigDlg.h"

HWND		hWnd;
HWND		hStatusBar;
HWND		hToolBar;
HINSTANCE	hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID /*lpvReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hinstDLL;
        ConfigInit(hinstDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ConfigCleanup();
    }
	return TRUE;
}
