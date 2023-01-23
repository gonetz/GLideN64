#include "GLideN64_Windows.h"

HWND		hWnd;
HWND		hStatusBar;
HWND		hToolBar;
HINSTANCE	hInstance;

#ifdef WTL_UI
void ConfigInit(void* hinst);
void ConfigCleanup(void);
#endif

extern "C" void loader_initialize(void);
extern "C" void loader_release(void);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
#ifdef WTL_UI
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hinstDLL;
        ConfigInit(hinstDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        ConfigCleanup();
    }
#else
    hInstance = hinstDLL;
#endif


    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        loader_initialize();
        break;
    case DLL_PROCESS_DETACH:
        if (NULL == lpvReserved) {
            loader_release();
        }
        break;
    default:
        // Do nothing
        break;
    }

	return TRUE;
}
