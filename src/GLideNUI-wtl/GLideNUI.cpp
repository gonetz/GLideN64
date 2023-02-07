#include "About.h"

#include <thread>

#include "GLideNUI.h"
#include "Settings.h"
#include "ConfigDlg.h"
#include "UIConfig.h"

#ifdef QT_STATICPLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
#endif

//#define RUN_DIALOG_IN_THREAD

static
int openConfigDialog(const wchar_t * _strFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy, bool & _accepted)
{
	std::string IniFolder;
	uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
	IniFolder.resize(slength);
	slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
	IniFolder.resize(slength - 1); //Remove null end char

	loadSettings(config, IniFolder.c_str());
	if (config.generalEmulation.enableCustomSettings != 0 && _romName != nullptr && strlen(_romName) != 0)
		loadCustomRomSettings(config, IniFolder.c_str(), _romName);

	LoadCurrentStrings(IniFolder.c_str(), config.translationFile);

	CConfigDlg Dlg;
	Dlg.setIniPath(IniFolder.c_str());
	Dlg.setRomName(_romName);
	Dlg.setMSAALevel(_maxMSAALevel);
	Dlg.setMaxAnisotropy(_maxAnisotropy);
	auto err = Dlg.DoModal();
	auto errVal = GetLastError();
	_accepted = Dlg.Saved();
	return 0;
}

bool runConfigThread(const wchar_t * _strFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy)
{
	bool accepted = false;
#ifdef RUN_DIALOG_IN_THREAD
	std::thread configThread(openConfigDialog, _strFileName, _maxMSAALevel, std::ref(accepted));
	configThread.join();
#else
	openConfigDialog(_strFileName, _romName, _maxMSAALevel, _maxAnisotropy, accepted);
#endif
	return accepted;

}

bool RunConfig(const wchar_t * _strFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy)
{
	return runConfigThread(_strFileName, _romName, _maxMSAALevel, _maxAnisotropy);
}

int RunAbout(const wchar_t * _strFileName)
{
	std::string IniFolder;
	uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
	IniFolder.resize(slength);
	slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
	IniFolder.resize(slength - 1); //Remove null end char

	LoadCurrentStrings(IniFolder.c_str(), config.translationFile);

	CAboutDlg Dlg;
	Dlg.DoModal();
	return 0;
}

void LoadConfig(Config* cfg, const wchar_t * _strFileName)
{
	std::string IniFolder;
	uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
	IniFolder.resize(slength);
	slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
	IniFolder.resize(slength - 1); //Remove null end char

	loadSettings(*cfg, IniFolder.c_str());
}

void LoadCustomRomSettings(Config* cfg, const wchar_t * _strFileName, const char * _romName)
{
	std::string IniFolder;
	uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
	IniFolder.resize(slength);
	slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
	IniFolder.resize(slength - 1); //Remove null end char

	loadCustomRomSettings(*cfg, IniFolder.c_str(), _romName);
}
