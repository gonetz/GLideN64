#include <algorithm>
#include <string>
#include "GLideN64_Windows.h"
#include "../RSP.h"

#include "../GLideNUI-wtl/resource.h"
#include "../PluginAPI.h"

#include <fcntl.h>
#include <io.h>
#include <commctrl.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include <shlwapi.h>

#ifdef OS_WINDOWS
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif

BOOL CALLBACK FindToolBarProc( HWND _hWnd, LPARAM lParam )
{
	if (GetWindowLong( _hWnd, GWL_STYLE ) & RBS_VARHEIGHT) {
		hToolBar = _hWnd;
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

	if (FAILED(SHGetFolderPathW(NULL,
		CSIDL_APPDATA,
		NULL,
		0,
		_strPath)))
	{
		::GetModuleFileName((HINSTANCE)&__ImageBase, _strPath, PLUGIN_PATH_SIZE);
	}
	else
	{
		PathAppendW(_strPath, L"GLideN64");
		CreateDirectoryW(_strPath, nullptr); // can fail, ignore errors
		size_t length = wcslen(_strPath);

		PathAppendW(_strPath, L"GLideN64.custom.ini");
		int fd = _wopen(_strPath, _O_BINARY | _O_WRONLY | _O_CREAT | _O_EXCL, 0666);
		if (-1 != fd)
		{
			auto rc = FindResource(hInstance, MAKEINTRESOURCE(IDR_RCDATA_CUSTOM_DEFAULT), RT_RCDATA);
			auto res = LoadResource(hInstance, rc);
			void* data = (wchar_t*)LockResource(res);
			size_t size = SizeofResource(hInstance, rc);
			_write(fd, data, size);
			_close(fd);
		}

		wchar_t* namePos = wcsstr(_strPath + length, L"GLideN64");
		wcscpy(namePos, L"GLideN64.ini");
		fd = _wopen(_strPath, _O_BINARY | _O_WRONLY | _O_CREAT | _O_EXCL, 0666);
		if (-1 != fd)
		{
			auto rc = FindResource(hInstance, MAKEINTRESOURCE(IDR_RCDATA_DEFAULT), RT_RCDATA);
			auto res = LoadResource(hInstance, rc);
			void* data = (wchar_t*) LockResource(res);
			size_t size = SizeofResource(hInstance, rc);
			_write(fd, data, size);
			_close(fd);
		}
	}

	std::wstring pluginPath(_strPath);
	std::replace(pluginPath.begin(), pluginPath.end(), L'\\', L'/');
	std::wstring::size_type pos = pluginPath.find_last_of(L"/");
	wcscpy(_strPath, pluginPath.substr(0, pos).c_str());
}

void PluginAPI::GetUserDataPath(wchar_t * _strPath)
{
	FindPluginPath(_strPath);
}

void PluginAPI::GetUserCachePath(wchar_t * _strPath)
{
	FindPluginPath(_strPath);
}
