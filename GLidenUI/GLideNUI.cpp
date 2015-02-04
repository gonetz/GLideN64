#include <thread>

#include "GLideNUI.h"
#include "ConfigDialog.h"
#include <QApplication>
#include <QSettings>

#include "../GBI.h"
#include "../Config.h"

#ifndef _DEBUG
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

static
void _loadSettings()
{
	QSettings settings("Emulation", "GLideN64");
	config.version = settings.value("version").toInt();

	settings.beginGroup("video");
	config.video.fullscreen = 0;
	config.video.fullscreenWidth = settings.value("fullscreenWidth", 640).toInt();
	config.video.fullscreenHeight = settings.value("fullscreenHeight", 480).toInt();
	config.video.windowedWidth = settings.value("windowedWidth", 640).toInt();
	config.video.windowedHeight = settings.value("windowedHeight", 480).toInt();
	config.video.fullscreenBits = settings.value("fullscreenBits", 32).toInt();
	config.video.fullscreenRefresh = settings.value("fullscreenRefresh", 60).toInt();
	config.video.multisampling = settings.value("multisampling", 0).toInt();
	config.video.verticalSync = settings.value("verticalSync", 0).toInt();
	settings.endGroup();

	settings.beginGroup("texture");
	config.texture.maxAnisotropy = settings.value("maxAnisotropy", 0).toInt();
	config.texture.forceBilinear = settings.value("forceBilinear", 0).toInt();
	config.texture.maxBytes = settings.value("maxBytes", 500 * gc_uMegabyte).toInt();
	settings.endGroup();

	settings.beginGroup("generalEmulation");
	config.generalEmulation.enableFog = settings.value("enableFog", 1).toInt();
	config.generalEmulation.enableNoise = settings.value("enableNoise", 1).toInt();
	config.generalEmulation.enableLOD = settings.value("enableLOD", 1).toInt();
	config.generalEmulation.enableHWLighting = settings.value("enableHWLighting", 0).toInt();
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	config.frameBufferEmulation.enable = settings.value("enable", 1).toInt();
	config.frameBufferEmulation.copyToRDRAM = settings.value("copyToRDRAM", 0).toInt();
	config.frameBufferEmulation.copyDepthToRDRAM = settings.value("copyDepthToRDRAM", 1).toInt();
	config.frameBufferEmulation.copyFromRDRAM = settings.value("copyFromRDRAM", 0).toInt();
	config.frameBufferEmulation.ignoreCFB = settings.value("ignoreCFB", 0).toInt();
	config.frameBufferEmulation.N64DepthCompare = settings.value("N64DepthCompare", 0).toInt();
	config.frameBufferEmulation.aspect = settings.value("aspect", 0).toInt();
	settings.endGroup();

	settings.beginGroup("textureFilter");
	config.textureFilter.txFilterMode = settings.value("txFilterMode", 0).toInt();
	config.textureFilter.txEnhancementMode = settings.value("txEnhancementMode", 0).toInt();
	config.textureFilter.txFilterForce16bpp = settings.value("txFilterForce16bpp", 0).toInt();
	config.textureFilter.txFilterIgnoreBG = settings.value("txFilterIgnoreBG", 0).toInt();
	config.textureFilter.txFilterCacheCompression = settings.value("txFilterCacheCompression", 1).toInt();
	config.textureFilter.txSaveCache = settings.value("txSaveCache", 1).toInt();
	config.textureFilter.txCacheSize = settings.value("txCacheSize", 100 * gc_uMegabyte).toInt();
	config.textureFilter.txHiresEnable = settings.value("txHiresEnable", 0).toInt();
	config.textureFilter.txHiresForce16bpp = settings.value("txHiresForce16bpp", 0).toInt();
	config.textureFilter.txHiresFullAlphaChannel = settings.value("txHiresFullAlphaChannel", 0).toInt();
	config.textureFilter.txHresAltCRC = settings.value("txHresAltCRC", 0).toInt();
	config.textureFilter.txHiresCacheCompression = settings.value("txHiresCacheCompression", 1).toInt();
	config.textureFilter.txDump = settings.value("txDump", 0).toInt();
	settings.endGroup();

	settings.beginGroup("font");
#ifdef OS_WINDOWS
	const char * defaultFontName = "arial.ttf";
	config.font.name = settings.value("name", defaultFontName).toString().toLocal8Bit().constData();
#else
	const char * defaultFontName = "FreeSans.ttf";
	config.font.name = settings.value("name", defaultFontName).toString().toStdString();
#endif
	config.font.size = settings.value("size", 18).toInt();
	QColor fontColor = settings.value("color", QColor(0xB5, 0xE6, 0x1D)).value<QColor>();
	config.font.color[0] = fontColor.red();
	config.font.color[1] = fontColor.green();
	config.font.color[2] = fontColor.blue();
	config.font.color[4] = fontColor.alpha();
	config.font.colorf[0] = _FIXED2FLOAT(config.font.color[0], 8);
	config.font.colorf[1] = _FIXED2FLOAT(config.font.color[1], 8);
	config.font.colorf[2] = _FIXED2FLOAT(config.font.color[2], 8);
	config.font.colorf[3] = config.font.color[3] == 0 ? 1.0f : _FIXED2FLOAT(config.font.color[3], 8);
	settings.endGroup();

	settings.beginGroup("bloomFilter");
	config.bloomFilter.mode = settings.value("enable", 0).toInt();
	config.bloomFilter.thresholdLevel = settings.value("thresholdLevel", 4).toInt();
	config.bloomFilter.blendMode = settings.value("blendMode", 0).toInt();
	config.bloomFilter.blurAmount = settings.value("blurAmount", 10).toInt();
	config.bloomFilter.blurStrength = settings.value("blurStrength", 20).toInt();
	settings.endGroup();
}

static
int openConfigDialog()
{
	_loadSettings();

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
	_loadSettings();
}
