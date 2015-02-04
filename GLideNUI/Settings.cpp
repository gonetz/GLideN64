#include <QSettings>
#include <QColor>

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include "../winlnxdefs.h"
#endif
#include "../GBI.h"
#include "../Config.h"

#include "Settings.h"

void loadSettings()
{
	QSettings settings("Emulation", "GLideN64");
	config.version = settings.value("version").toInt();
	if (config.version != CONFIG_VERSION_CURRENT) {
		config.resetToDefaults();
		settings.clear();
		writeSettings();
		return;
	}

	settings.beginGroup("video");
	config.video.fullscreen = 0;
	config.video.fullscreenWidth = settings.value("fullscreenWidth", 640).toInt();
	config.video.fullscreenHeight = settings.value("fullscreenHeight", 480).toInt();
	config.video.windowedWidth = settings.value("windowedWidth", 640).toInt();
	config.video.windowedHeight = settings.value("windowedHeight", 480).toInt();
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

void writeSettings()
{
	QSettings settings("Emulation", "GLideN64");
	settings.setValue("version", config.version);

	settings.beginGroup("video");
	settings.setValue("fullscreenWidth", config.video.fullscreenWidth);
	settings.setValue("fullscreenHeight", config.video.fullscreenHeight);
	settings.setValue("windowedWidth", config.video.windowedWidth);
	settings.setValue("windowedHeight", config.video.windowedHeight);
	settings.setValue("fullscreenRefresh", config.video.fullscreenRefresh);
	settings.setValue("multisampling", config.video.multisampling);
	settings.setValue("verticalSync", config.video.verticalSync);
	settings.endGroup();

	settings.beginGroup("texture");
	settings.setValue("maxAnisotropy", config.texture.maxAnisotropy);
	settings.setValue("forceBilinear", config.texture.forceBilinear);
	settings.setValue("maxBytes", config.texture.maxBytes);
	settings.endGroup();

	settings.beginGroup("generalEmulation");
	settings.setValue("enableFog", config.generalEmulation.enableFog);
	settings.setValue("enableNoise", config.generalEmulation.enableNoise);
	settings.setValue("enableLOD", config.generalEmulation.enableLOD);
	settings.setValue("enableHWLighting", config.generalEmulation.enableHWLighting);
	settings.setValue("hacks", config.generalEmulation.hacks);
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	settings.setValue("enable", config.frameBufferEmulation.enable);
	settings.setValue("copyToRDRAM", config.frameBufferEmulation.copyToRDRAM);
	settings.setValue("copyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM);
	settings.setValue("copyFromRDRAM", config.frameBufferEmulation.copyFromRDRAM);
	settings.setValue("ignoreCFB", config.frameBufferEmulation.ignoreCFB);
	settings.setValue("N64DepthCompare", config.frameBufferEmulation.N64DepthCompare);
	settings.setValue("aspect", config.frameBufferEmulation.aspect);
	settings.endGroup();

	settings.beginGroup("textureFilter");
	settings.setValue("txFilterMode", config.textureFilter.txFilterMode);
	settings.setValue("txEnhancementMode", config.textureFilter.txEnhancementMode);
	settings.setValue("txFilterForce16bpp", config.textureFilter.txFilterForce16bpp);
	settings.setValue("txFilterIgnoreBG", config.textureFilter.txFilterIgnoreBG);
	settings.setValue("txFilterCacheCompression", config.textureFilter.txFilterCacheCompression);
	settings.setValue("txSaveCache", config.textureFilter.txSaveCache);
	settings.setValue("txCacheSize", config.textureFilter.txCacheSize);
	settings.setValue("txHiresEnable", config.textureFilter.txHiresEnable);
	settings.setValue("txHiresForce16bpp", config.textureFilter.txHiresForce16bpp);
	settings.setValue("txHiresFullAlphaChannel", config.textureFilter.txHiresFullAlphaChannel);
	settings.setValue("txHresAltCRC", config.textureFilter.txHresAltCRC);
	settings.setValue("txHiresCacheCompression", config.textureFilter.txHiresCacheCompression);
	settings.setValue("txDump", config.textureFilter.txDump);
	settings.endGroup();

	settings.beginGroup("font");
	settings.setValue("name", config.font.name.c_str());
	settings.setValue("size", config.font.size);
	settings.setValue("color", QColor(config.font.color[0], config.font.color[1], config.font.color[2], config.font.color[3]));
	settings.endGroup();

	settings.beginGroup("bloomFilter");
	settings.setValue("enable", config.bloomFilter.mode);
	settings.setValue("thresholdLevel", config.bloomFilter.thresholdLevel);
	settings.setValue("blendMode", config.bloomFilter.blendMode);
	settings.setValue("blurAmount", config.bloomFilter.blurAmount);
	settings.setValue("blurStrength", config.bloomFilter.blurStrength);
	settings.endGroup();
}
