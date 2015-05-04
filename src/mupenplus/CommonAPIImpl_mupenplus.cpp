#include "../PluginAPI.h"
#include "../OpenGL.h"
#include "../RSP.h"

int PluginAPI::InitiateGFX(const GFX_INFO & _gfxInfo)
{
	_initiateGFX(_gfxInfo);

	return TRUE;
}

void PluginAPI::FindPluginPath(wchar_t * _strPath)
{
	if (_strPath == NULL)
		return;
#ifdef OS_WINDOWS
	GetModuleFileNameW(NULL, _strPath, PLUGIN_PATH_SIZE);
#elif OS_LINUX
	char path[512];
	int res = readlink("/proc/self/exe", path, 510);
	if (res != -1) {
		path[res] = 0;
		::mbstowcs(_strPath, path, PLUGIN_PATH_SIZE);
	}
#elif OS_MAC_OS_X
	char path[MAXPATHLEN];
	uint32_t pathLen = MAXPATHLEN * 2;
	if (_NSGetExecutablePath(path, pathLen) == 0) {
		::mbstowcs(_strPath, path, PLUGIN_PATH_SIZE);
	}
#else
	// TODO: Android implementation
	::mbstowcs(_strPath, "Plugin", PLUGIN_PATH_SIZE);
#endif
	std::wstring pluginPath(_strPath);
	std::wstring::size_type pos = pluginPath.find_last_of(L"\\/");
	wcscpy(_strPath, pluginPath.substr(0, pos).c_str());
}
