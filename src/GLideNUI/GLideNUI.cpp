#include "About.h"

#include <thread>
#include <QApplication>
#include <QTranslator>

#include "GLideNUI.h"
#include "AboutDialog.h"
#include "ConfigDialog.h"
#include "Settings.h"
#include "../Config.h"

#ifdef QT_STATICPLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
#endif

//#define RUN_DIALOG_IN_THREAD

inline void initMyResource() { Q_INIT_RESOURCE(icon); }
inline void cleanMyResource() { Q_CLEANUP_RESOURCE(icon); }

static
int openConfigDialog(const wchar_t * _strFileName, const char * _romName, bool & _accepted)
{
    std::string IniFolder;
    uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
    IniFolder.resize(slength);
    slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
    IniFolder.resize(slength - 1); //Remove null end char

    cleanMyResource();
    initMyResource();
    loadSettings(IniFolder.c_str());
    if (config.generalEmulation.enableCustomSettings != 0 && _romName != nullptr && strlen(_romName) != 0)
        loadCustomRomSettings(IniFolder.c_str(), _romName);

    int argc = 0;
    char * argv = 0;
    QApplication a(argc, &argv);

    QTranslator translator;
    if (translator.load(getTranslationFile().c_str(), IniFolder.c_str()))
        a.installTranslator(&translator);

    ConfigDialog w(Q_NULLPTR, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);

    w.setIniPath(IniFolder.c_str());
    w.setRomName(_romName);
    w.setTitle();
    w.show();
    const int res = a.exec();
    _accepted = w.isAccepted();
    return res;
}

static
int openAboutDialog(const wchar_t * _strFileName)
{
    cleanMyResource();
    initMyResource();

    int argc = 0;
    char * argv = 0;
    QApplication a(argc, &argv);

    QTranslator translator;
    if (translator.load(getTranslationFile().c_str(), QString::fromWCharArray(_strFileName)))
        a.installTranslator(&translator);

    AboutDialog w(Q_NULLPTR, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    w.show();
    return a.exec();
}

bool runConfigThread(const wchar_t * _strFileName, const char * _romName) {
    bool accepted = false;
#ifdef RUN_DIALOG_IN_THREAD
    std::thread configThread(openConfigDialog, _strFileName, std::ref(accepted));
    configThread.join();
#else
    openConfigDialog(_strFileName, _romName, accepted);
#endif
    return accepted;

}

int runAboutThread(const wchar_t * _strFileName) {
#ifdef RUN_DIALOG_IN_THREAD
    std::thread aboutThread(openAboutDialog, _strFileName);
    aboutThread.join();
#else
    openAboutDialog(_strFileName);
#endif
    return 0;
}

EXPORT bool CALL RunConfig(const wchar_t * _strFileName, const char * _romName)
{
    return runConfigThread(_strFileName, _romName);
}

EXPORT int CALL RunAbout(const wchar_t * /*_strFileName*/)
{
    CAboutDlg Dlg;
    Dlg.DoModal();
    return 0;
}

EXPORT void CALL LoadConfig(const wchar_t * _strFileName)
{
    std::string IniFolder;
    uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
    IniFolder.resize(slength);
    slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
    IniFolder.resize(slength - 1); //Remove null end char

    loadSettings(IniFolder.c_str());
}

EXPORT void CALL LoadCustomRomSettings(const wchar_t * _strFileName, const char * _romName)
{
    std::string IniFolder;
    uint32_t slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, NULL, 0, NULL, NULL);
    IniFolder.resize(slength);
    slength = WideCharToMultiByte(CP_ACP, 0, _strFileName, -1, (LPSTR)IniFolder.c_str(), slength, NULL, NULL);
    IniFolder.resize(slength - 1); //Remove null end char

    loadCustomRomSettings(IniFolder.c_str(), _romName);
}
