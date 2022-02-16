#include "../N64.h"
#include "../Config.h"
#include "../RSP.h"
#include "../PluginAPI.h"
#include "../GLideNUI/GLideNUI.h"
#include <DisplayWindow.h>


Config config;

void Config_DoConfig(/*HWND hParent*/)
{
	if (ConfigOpen)
		return;

	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	api().FindPluginPath(strIniFolderPath);

#ifdef M64P_GLIDENUI
	wchar_t strConfigFolderPath[PLUGIN_PATH_SIZE];
	api().GetUserConfigPath(strConfigFolderPath);

	if (!IsPathWriteable(strIniFolderPath)) {
		CopyConfigFiles(strIniFolderPath, strConfigFolderPath);
		api().GetUserConfigPath(strIniFolderPath);
	}
#endif // M64P_GLIDENUI

	ConfigOpen = true;
	const u32 maxMsaa = dwnd().maxMSAALevel();
	const u32 maxAnisotropy = dwnd().maxAnisotropy();
	const bool bRestart = RunConfig(strIniFolderPath, api().isRomOpen() ? RSP.romname : nullptr, maxMsaa, maxAnisotropy);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(strIniFolderPath, RSP.romname);
	config.validate();
	if (bRestart)
		dwnd().restart();
	ConfigOpen = false;
}

void Config_LoadConfig()
{
	wchar_t strIniFolderPath[PLUGIN_PATH_SIZE];
	api().FindPluginPath(strIniFolderPath);

#ifdef M64P_GLIDENUI
	wchar_t strConfigFolderPath[PLUGIN_PATH_SIZE];
	api().GetUserConfigPath(strConfigFolderPath);

	if (!IsPathWriteable(strIniFolderPath)) {
		CopyConfigFiles(strIniFolderPath, strConfigFolderPath);
		api().GetUserConfigPath(strIniFolderPath);
	}
#endif // M64P_GLIDENUI

	LoadConfig(strIniFolderPath);
	if (config.generalEmulation.enableCustomSettings != 0)
		LoadCustomRomSettings(strIniFolderPath, RSP.romname);
	config.validate();
}
