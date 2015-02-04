#include <thread>
#include <QApplication>

#include "GLideNUI.h"
#include "ConfigDialog.h"
#include "Settings.h"

#ifndef _DEBUG
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

static
int openConfigDialog()
{
	loadSettings();

	int argc = 0;
	char * argv = 0;
	QApplication a(argc, &argv);

	ConfigDialog w;
	w.show();
	return a.exec();
}

int runThread() {
	std::thread configThread(openConfigDialog);
	configThread.join();
	return 0;
}

EXPORT int CALL RunConfig()
{
	return runThread();
}

EXPORT void CALL LoadConfig()
{
	loadSettings();
}
