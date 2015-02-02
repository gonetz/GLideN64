#include "GLideNUI.h"
#include "ConfigDialog.h"
#include <QApplication>
#include <QSettings>

#include "../Config.h"

#ifndef _DEBUG
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

static
void _writeSettings()
{
	QSettings settings("Emulation", "GLideN64");
	settings.setValue("version", config.version);

	settings.beginGroup("video");
	settings.setValue("fullscreen", config.video.fullscreen);
	settings.setValue("fullscreenWidth", config.video.fullscreenWidth);
	settings.setValue("fullscreenHeight", config.video.fullscreenHeight);
	settings.setValue("windowedWidth", config.video.windowedWidth);
	settings.setValue("windowedHeight", config.video.windowedHeight);
	settings.setValue("fullscreenBits", config.video.fullscreenBits);
	settings.setValue("fullscreenRefresh", config.video.fullscreenRefresh);
	settings.setValue("multisampling", config.video.multisampling);
	settings.setValue("verticalSync", config.video.verticalSync);
	settings.setValue("aspect", config.video.aspect);
	settings.endGroup();

	settings.beginGroup("texture");
	settings.setValue("maxAnisotropy", config.texture.maxAnisotropy);
	settings.setValue("textureBitDepth", config.texture.textureBitDepth);
	settings.setValue("forceBilinear", config.texture.forceBilinear);
	settings.setValue("maxBytes", config.texture.maxBytes);
	settings.endGroup();

	settings.beginGroup("textureFilter");
	settings.setValue("txFilterMode", config.textureFilter.txFilterMode);
	settings.setValue("txEnhancementMode", config.textureFilter.txEnhancementMode);
	settings.setValue("txFilterForce16bpp", config.textureFilter.txFilterForce16bpp);
	settings.setValue("txFilterIgnoreBG", config.textureFilter.txFilterIgnoreBG);
	settings.setValue("txFilterCacheCompression", config.textureFilter.txFilterCacheCompression);
	settings.setValue("txSaveCache", config.textureFilter.txSaveCache);
	settings.setValue("txHiresEnable", config.textureFilter.txHiresEnable);
	settings.setValue("txHiresForce16bpp", config.textureFilter.txHiresForce16bpp);
	settings.setValue("txHiresFullAlphaChannel", config.textureFilter.txHiresFullAlphaChannel);
	settings.setValue("txHresAltCRC", config.textureFilter.txHresAltCRC);
	settings.setValue("txHiresCacheCompression", config.textureFilter.txHiresCacheCompression);
	settings.setValue("txDump", config.textureFilter.txDump);
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	settings.setValue("enable", config.frameBufferEmulation.enable);
	settings.setValue("copyToRDRAM", config.frameBufferEmulation.copyToRDRAM);
	settings.setValue("copyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM);
	settings.setValue("copyFromRDRAM", config.frameBufferEmulation.copyFromRDRAM);
	settings.setValue("ignoreCFB", config.frameBufferEmulation.ignoreCFB);
	settings.setValue("N64DepthCompare", config.frameBufferEmulation.N64DepthCompare);
	settings.endGroup();

	settings.beginGroup("bloomFilter");
	settings.setValue("enable", config.bloomFilter.mode);
	settings.setValue("thresholdLevel", config.bloomFilter.thresholdLevel);
	settings.setValue("blendMode", config.bloomFilter.blendMode);
	settings.setValue("blurAmount", config.bloomFilter.blurAmount);
	settings.setValue("blurStrength", config.bloomFilter.blurStrength);
	settings.endGroup();
}

static
void _loadSettings()
{
	QSettings settings("Emulation", "GLideN64");
	config.version = settings.value("version").toInt();

	settings.beginGroup("video");
	config.video.fullscreen = settings.value("fullscreen").toInt();
	config.video.fullscreenWidth = settings.value("fullscreenWidth").toInt();
	config.video.fullscreenHeight = settings.value("fullscreenHeight").toInt();
	config.video.windowedWidth = settings.value("windowedWidth").toInt();
	config.video.windowedHeight = settings.value("windowedHeight").toInt();
	config.video.fullscreenBits = settings.value("fullscreenBits").toInt();
	config.video.fullscreenRefresh = settings.value("fullscreenRefresh").toInt();
	config.video.multisampling = settings.value("multisampling").toInt();
	config.video.verticalSync = settings.value("verticalSync").toInt();
	config.video.aspect = settings.value("aspect").toInt();
	settings.endGroup();

	settings.beginGroup("texture");
	config.texture.maxAnisotropy = settings.value("maxAnisotropy").toInt();
	config.texture.textureBitDepth = settings.value("textureBitDepth").toInt();
	config.texture.forceBilinear = settings.value("forceBilinear").toInt();
	config.texture.maxBytes = settings.value("maxBytes").toInt();
	settings.endGroup();

	settings.beginGroup("textureFilter");
	config.textureFilter.txFilterMode = settings.value("txFilterMode").toInt();
	config.textureFilter.txEnhancementMode = settings.value("txEnhancementMode").toInt();
	config.textureFilter.txFilterForce16bpp = settings.value("txFilterForce16bpp").toInt();
	config.textureFilter.txFilterIgnoreBG = settings.value("txFilterIgnoreBG").toInt();
	config.textureFilter.txFilterCacheCompression = settings.value("txFilterCacheCompression").toInt();
	config.textureFilter.txSaveCache = settings.value("txSaveCache").toInt();
	config.textureFilter.txHiresEnable = settings.value("txHiresEnable").toInt();
	config.textureFilter.txHiresForce16bpp = settings.value("txHiresForce16bpp").toInt();
	config.textureFilter.txHiresFullAlphaChannel = settings.value("txHiresFullAlphaChannel").toInt();
	config.textureFilter.txHresAltCRC = settings.value("txHresAltCRC").toInt();
	config.textureFilter.txHiresCacheCompression = settings.value("txHiresCacheCompression").toInt();
	config.textureFilter.txDump = settings.value("txDump").toInt();
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	config.frameBufferEmulation.enable = settings.value("enable").toInt();
	config.frameBufferEmulation.copyToRDRAM = settings.value("copyToRDRAM").toInt();
	config.frameBufferEmulation.copyDepthToRDRAM = settings.value("copyDepthToRDRAM").toInt();
	config.frameBufferEmulation.copyFromRDRAM = settings.value("copyFromRDRAM").toInt();
	config.frameBufferEmulation.ignoreCFB = settings.value("ignoreCFB").toInt();
	config.frameBufferEmulation.N64DepthCompare = settings.value("N64DepthCompare").toInt();
	settings.endGroup();

	settings.beginGroup("bloomFilter");
	config.bloomFilter.mode = settings.value("enable").toInt();
	config.bloomFilter.thresholdLevel = settings.value("thresholdLevel").toInt();
	config.bloomFilter.blendMode = settings.value("blendMode").toInt();
	config.bloomFilter.blurAmount = settings.value("blurAmount").toInt();
	config.bloomFilter.blurStrength = settings.value("blurStrength").toInt();
	settings.endGroup();
}

//#define USE_WIN_THREADS

#ifdef USE_WIN_THREADS

int main(int argc, char *argv[])
{
	argc = 0;
	argv = 0;
	QApplication a(argc, argv);
	ConfigDialog w;
	w.show();

	return a.exec();
}


int runThread(HINSTANCE hInstance) {
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)hInstance, 0, NULL);
	DWORD res = WaitForSingleObject(hThread, INFINITE);
	return res;
}

#else
#include <thread>
int main()
{
	_loadSettings();
	int argc = 0;
	char * argv = 0;
	QApplication a(argc, &argv);
	ConfigDialog w;
	w.show();

	int res = a.exec();
	_writeSettings();
	return res;
}

int runThread(HINSTANCE /*hInstance*/) {
	std::thread first(main);
	first.join();
	return 0;
}

#endif

EXPORT int CALL RunConfig(HINSTANCE hInstance /*HWND hParent*/)
{
	return runThread(hInstance);
}


