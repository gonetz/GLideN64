#include "GLideN64_Windows.h"
#include "../Config.h"
#include "../RSP.h"
#include "../PluginAPI.h"
#include "../OpenGL.h"
#include "../GLideNUI/GLideNUI.h"

Config config;

static
void _getIniFileName(wchar_t * _buf)
{
	api().FindPluginPath(_buf);
	wcscat(_buf, L"/GLideN64.ini");
}

void Config_DoConfig(/*HWND hParent*/)
{
	wchar_t strIniFileName[PLUGIN_PATH_SIZE];
	_getIniFileName(strIniFileName);

	const bool bRestart = RunConfig(strIniFileName);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(strIniFileName, RSP.romname);
	if (bRestart)
		video().restart();
}

void Config_LoadConfig()
{
	wchar_t strIniFileName[PLUGIN_PATH_SIZE];
	_getIniFileName(strIniFileName);
	LoadConfig(strIniFileName);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(strIniFileName, RSP.romname);
}
