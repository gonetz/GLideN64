#include "GLideN64_Windows.h"
#include "../PluginAPI.h"
#include "../GLideN64.h"
#include "../GLideNUI/GLideNUI.h"
#include "../OpenGL.h"
#include "../Config.h"
#include "../Revision.h"

void PluginAPI::DllAbout(/*HWND _hParent*/)
{
#ifndef NO_UI
	Config_LoadConfig();
	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	api().FindPluginPath(strIniFolderPath);
	RunAbout(strIniFolderPath);
#endif
}

void PluginAPI::CaptureScreen(char * _Directory)
{
	video().setCaptureScreen(_Directory);
}

void PluginAPI::DllConfig(HWND _hParent)
{
#ifndef NO_UI
	Config_DoConfig(/*_hParent*/);
#endif
}

void PluginAPI::GetDllInfo(PLUGIN_INFO * PluginInfo)
{
	PluginInfo->Version = 0x103;
	PluginInfo->Type = PLUGIN_TYPE_GFX;
	sprintf(PluginInfo->Name, "%s rev.%s", pluginName, PLUGIN_REVISION);
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
}
