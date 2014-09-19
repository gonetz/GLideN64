#include "GLideN64_Windows.h"
#include <commctrl.h>
#include "../PluginAPI.h"
#include "../OpenGL.h"

BOOL CALLBACK FindToolBarProc( HWND hWnd, LPARAM lParam )
{
	if (GetWindowLong( hWnd, GWL_STYLE ) & RBS_VARHEIGHT) {
		hToolBar = hWnd;
		return FALSE;
	}
	return TRUE;
}

int PluginAPI::InitiateGFX(const GFX_INFO & _gfxInfo)
{
	_initiateGFX(_gfxInfo);

	hWnd = _gfxInfo.hWnd;
	hStatusBar = _gfxInfo.hStatusBar;
	hToolBar = NULL;

	EnumChildWindows( hWnd, FindToolBarProc, 0 );
	return TRUE;
}
