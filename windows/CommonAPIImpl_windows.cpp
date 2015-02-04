#include "GLideN64_Windows.h"
#include <commctrl.h>
#include "../PluginAPI.h"
#include "../OpenGL.h"
#include "../RSP.h"

#ifdef OS_WINDOWS
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif

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

void PluginAPI::FindPluginPath(wchar_t * _strPath)
{
	if (_strPath == NULL)
		return;
#ifdef OS_WINDOWS
	TCHAR  strDLLPath[PLUGIN_PATH_SIZE];
	::GetModuleFileName((HINSTANCE)&__ImageBase, strDLLPath, PLUGIN_PATH_SIZE);
	::mbstowcs(_strPath, strDLLPath, PLUGIN_PATH_SIZE);
#else
	// TODO: for other OS
	return;
#endif
	std::wstring pluginPath(_strPath);
	std::wstring::size_type pos = pluginPath.find_last_of(L"\\/");
	wcscpy(_strPath, pluginPath.substr(0, pos).c_str());
}
