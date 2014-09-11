#ifdef _WINDOWS
# include <windows.h>
#else
# include "../winlnxdefs.h"
#endif // _WINDOWS

#include "../PluginAPI.h"

#include "../GLideN64.h"
#include "../OpenGL.h"
#include "../Config.h"
#include "../RSP.h"

void PluginAPI::CaptureScreen(char * _Directory)
{
	screenDirectory = _Directory;
	OGL.captureScreen = true;
}

void PluginAPI::DllConfig(HWND _hParent)
{
	Config_DoConfig(_hParent);
}

void PluginAPI::GetDllInfo(PLUGIN_INFO * PluginInfo)
{
	PluginInfo->Version = 0x103;
	PluginInfo->Type = PLUGIN_TYPE_GFX;
	strcpy( PluginInfo->Name, pluginName );
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
}

void PluginAPI::ReadScreen(void **_dest, long *_width, long *_height)
{
	OGL_ReadScreen(_dest, _width, _height);
}
