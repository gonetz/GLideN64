#include "GLideN64_Windows.h"

HWND		hWnd = {};
DWORD       hWndThread;
HWND		hStatusBar;
HWND		hToolBar;
HINSTANCE	hInstance;

#ifdef WTL_UI
void ConfigInit(void* hinst);
void ConfigCleanup(void);
#endif

namespace egl
{
    // extern void AllocateCurrentThread();
    extern void DeallocateCurrentThread();
}

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
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        // egl::AllocateCurrentThread();
        break;
    case DLL_THREAD_DETACH:
        egl::DeallocateCurrentThread();
        break;
    default:
        // Do nothing
        break;
    }

	return TRUE;
}
