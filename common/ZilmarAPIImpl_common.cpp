#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "../winlnxdefs.h"
#endif // OS_WINDOWS

#include "../PluginAPI.h"

#include "../GLideN64.h"
#include "../Revision.h"
#include "../OpenGL.h"
#include "../Config.h"
#include "../RSP.h"

void PluginAPI::CaptureScreen(char * _Directory)
{
	video().setCaptureScreen(_Directory);
}

void PluginAPI::DllConfig(HWND _hParent)
{
	Config_DoConfig(_hParent);
}

void PluginAPI::GetDllInfo(PLUGIN_INFO * PluginInfo)
{
	PluginInfo->Version = 0x103;
	PluginInfo->Type = PLUGIN_TYPE_GFX;
	sprintf(PluginInfo->Name, "%s rev.%s", pluginName, PLUGIN_REVISION);
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
}

void PluginAPI::ReadScreen(void **_dest, long *_width, long *_height)
{
	video().readScreen(_dest, _width, _height);
}
