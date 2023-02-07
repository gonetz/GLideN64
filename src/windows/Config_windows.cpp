#include "GLideN64_Windows.h"
#include "../Config.h"
#include "../N64.h"
#include "../Config.h"
#include "../RSP.h"
#include "../PluginAPI.h"
#include "../GLideNUI/GLideNUI.h"
#include <DisplayWindow.h>


Config config;

void Config_DoConfig(/*HWND hParent*/)
{
	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	api().FindPluginPath(strIniFolderPath);

	ConfigOpen = true;
	const bool bRestart = RunConfig(strIniFolderPath, api().isRomOpen() ? RSP.romname : nullptr, 8, 16);
	if (bRestart)
	{
		PluginAPI::get().Restart();
	}
	ConfigOpen = false;
}

void Config_LoadConfig()
{
	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	api().FindPluginPath(strIniFolderPath);
	LoadConfig(&config, strIniFolderPath);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(&config, strIniFolderPath, RSP.romname);
	config.validate();
}
