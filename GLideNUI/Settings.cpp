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

void loadSettings(const QString & _strFileName)
{
//	QSettings settings("Emulation", "GLideN64");
	QSettings settings(_strFileName, QSettings::IniFormat);
	config.version = settings.value("version").toInt();
	if (config.version != CONFIG_VERSION_CURRENT) {
		config.resetToDefaults();
		settings.clear();
		writeSettings(_strFileName);
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
	settings.endGroup();

	settings.beginGroup("texture");
	config.texture.maxAnisotropy = settings.value("maxAnisotropy", 0).toInt();
	config.texture.bilinearMode = settings.value("bilinearMode", BILINEAR_STANDARD).toInt();
	config.texture.maxBytes = settings.value("maxBytes", 500 * gc_uMegabyte).toInt();
	config.texture.screenShotFormat = settings.value("screenShotFormat", 0).toInt();
	settings.endGroup();

	settings.beginGroup("generalEmulation");
	config.generalEmulation.enableFog = settings.value("enableFog", 1).toInt();
	config.generalEmulation.enableNoise = settings.value("enableNoise", 1).toInt();
	config.generalEmulation.enableLOD = settings.value("enableLOD", 1).toInt();
	config.generalEmulation.enableHWLighting = settings.value("enableHWLighting", 0).toInt();
	config.generalEmulation.enableCustomSettings = settings.value("enableCustomSettings", 0).toInt();
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	config.frameBufferEmulation.enable = settings.value("enable", 1).toInt();
	config.frameBufferEmulation.copyToRDRAM = settings.value("copyToRDRAM", 0).toInt();
	config.frameBufferEmulation.copyDepthToRDRAM = settings.value("copyDepthToRDRAM", 1).toInt();
	config.frameBufferEmulation.copyFromRDRAM = settings.value("copyFromRDRAM", 0).toInt();
	config.frameBufferEmulation.detectCFB = settings.value("detectCFB", 0).toInt();
	config.frameBufferEmulation.N64DepthCompare = settings.value("N64DepthCompare", 0).toInt();
	config.frameBufferEmulation.aspect = settings.value("aspect", 0).toInt();
	settings.endGroup();

	settings.beginGroup("textureFilter");
	config.textureFilter.txFilterMode = settings.value("txFilterMode", 0).toInt();
	config.textureFilter.txEnhancementMode = settings.value("txEnhancementMode", 0).toInt();
	config.textureFilter.txFilterIgnoreBG = settings.value("txFilterIgnoreBG", 0).toInt();
	config.textureFilter.txCacheSize = settings.value("txCacheSize", 100 * gc_uMegabyte).toInt();
	config.textureFilter.txHiresEnable = settings.value("txHiresEnable", 0).toInt();
	config.textureFilter.txHiresFullAlphaChannel = settings.value("txHiresFullAlphaChannel", 0).toInt();
	config.textureFilter.txHresAltCRC = settings.value("txHresAltCRC", 0).toInt();
	config.textureFilter.txDump = settings.value("txDump", 0).toInt();
	config.textureFilter.txForce16bpp = settings.value("txForce16bpp", 0).toInt();
	config.textureFilter.txCacheCompression = settings.value("txCacheCompression", 1).toInt();
	config.textureFilter.txSaveCache = settings.value("txSaveCache", 1).toInt();
	settings.value("txPath", "").toString().toWCharArray(config.textureFilter.txPath);
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
	config.bloomFilter.enable = settings.value("enable", 0).toInt();
	config.bloomFilter.thresholdLevel = settings.value("thresholdLevel", 4).toInt();
	config.bloomFilter.blendMode = settings.value("blendMode", 0).toInt();
	config.bloomFilter.blurAmount = settings.value("blurAmount", 10).toInt();
	config.bloomFilter.blurStrength = settings.value("blurStrength", 20).toInt();
	settings.endGroup();
}

void writeSettings(const QString & _strFileName)
{
//	QSettings settings("Emulation", "GLideN64");
	QSettings settings(_strFileName, QSettings::IniFormat);
	settings.setValue("version", config.version);

	settings.beginGroup("video");
	settings.setValue("fullscreenWidth", config.video.fullscreenWidth);
	settings.setValue("fullscreenHeight", config.video.fullscreenHeight);
	settings.setValue("windowedWidth", config.video.windowedWidth);
	settings.setValue("windowedHeight", config.video.windowedHeight);
	settings.setValue("fullscreenRefresh", config.video.fullscreenRefresh);
	settings.setValue("multisampling", config.video.multisampling);
	settings.endGroup();

	settings.beginGroup("texture");
	settings.setValue("maxAnisotropy", config.texture.maxAnisotropy);
	settings.setValue("bilinearMode", config.texture.bilinearMode);
	settings.setValue("maxBytes", config.texture.maxBytes);
	settings.setValue("screenShotFormat", config.texture.screenShotFormat);
	settings.endGroup();

	settings.beginGroup("generalEmulation");
	settings.setValue("enableFog", config.generalEmulation.enableFog);
	settings.setValue("enableNoise", config.generalEmulation.enableNoise);
	settings.setValue("enableLOD", config.generalEmulation.enableLOD);
	settings.setValue("enableHWLighting", config.generalEmulation.enableHWLighting);
	settings.setValue("enableCustomSettings", config.generalEmulation.enableCustomSettings);
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	settings.setValue("enable", config.frameBufferEmulation.enable);
	settings.setValue("copyToRDRAM", config.frameBufferEmulation.copyToRDRAM);
	settings.setValue("copyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM);
	settings.setValue("copyFromRDRAM", config.frameBufferEmulation.copyFromRDRAM);
	settings.setValue("detectCFB", config.frameBufferEmulation.detectCFB);
	settings.setValue("N64DepthCompare", config.frameBufferEmulation.N64DepthCompare);
	settings.setValue("aspect", config.frameBufferEmulation.aspect);
	settings.endGroup();

	settings.beginGroup("textureFilter");
	settings.setValue("txFilterMode", config.textureFilter.txFilterMode);
	settings.setValue("txEnhancementMode", config.textureFilter.txEnhancementMode);
	settings.setValue("txFilterIgnoreBG", config.textureFilter.txFilterIgnoreBG);
	settings.setValue("txCacheSize", config.textureFilter.txCacheSize);
	settings.setValue("txHiresEnable", config.textureFilter.txHiresEnable);
	settings.setValue("txHiresFullAlphaChannel", config.textureFilter.txHiresFullAlphaChannel);
	settings.setValue("txHresAltCRC", config.textureFilter.txHresAltCRC);
	settings.setValue("txDump", config.textureFilter.txDump);
	settings.setValue("txForce16bpp", config.textureFilter.txForce16bpp);
	settings.setValue("txCacheCompression", config.textureFilter.txCacheCompression);
	settings.setValue("txSaveCache", config.textureFilter.txSaveCache);
	settings.setValue("txPath", QString::fromWCharArray(config.textureFilter.txPath));
	settings.endGroup();

	settings.beginGroup("font");
	settings.setValue("name", config.font.name.c_str());
	settings.setValue("size", config.font.size);
	settings.setValue("color", QColor(config.font.color[0], config.font.color[1], config.font.color[2], config.font.color[3]));
	settings.endGroup();

	settings.beginGroup("bloomFilter");
	settings.setValue("enable", config.bloomFilter.enable);
	settings.setValue("thresholdLevel", config.bloomFilter.thresholdLevel);
	settings.setValue("blendMode", config.bloomFilter.blendMode);
	settings.setValue("blurAmount", config.bloomFilter.blurAmount);
	settings.setValue("blurStrength", config.bloomFilter.blurStrength);
	settings.endGroup();
}

void loadCustomRomSettings(const QString & _strFileName, const QString & _strRomName)
{
	QSettings settings(_strFileName, QSettings::IniFormat);
	config.version = settings.value("version").toInt();
	if (config.version != CONFIG_VERSION_CURRENT)
		return;

	if (settings.childGroups().indexOf(_strRomName) < 0)
		return;

	settings.beginGroup(_strRomName);

	settings.beginGroup("video");
	config.video.multisampling = settings.value("multisampling", config.video.multisampling).toInt();
	settings.endGroup();

	settings.beginGroup("texture");
	config.texture.maxAnisotropy = settings.value("maxAnisotropy", config.texture.maxAnisotropy).toInt();
	config.texture.bilinearMode = settings.value("bilinearMode", config.texture.bilinearMode).toInt();
	settings.endGroup();

	settings.beginGroup("generalEmulation");
	config.generalEmulation.enableFog = settings.value("enableFog", config.generalEmulation.enableFog).toInt();
	config.generalEmulation.enableNoise = settings.value("enableNoise", config.generalEmulation.enableNoise).toInt();
	config.generalEmulation.enableLOD = settings.value("enableLOD", config.generalEmulation.enableLOD).toInt();
	config.generalEmulation.enableHWLighting = settings.value("enableHWLighting", config.generalEmulation.enableHWLighting).toInt();
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	config.frameBufferEmulation.enable = settings.value("enable", config.frameBufferEmulation.enable).toInt();
	config.frameBufferEmulation.copyToRDRAM = settings.value("copyToRDRAM", config.frameBufferEmulation.copyToRDRAM).toInt();
	config.frameBufferEmulation.copyDepthToRDRAM = settings.value("copyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM).toInt();
	config.frameBufferEmulation.copyFromRDRAM = settings.value("copyFromRDRAM", config.frameBufferEmulation.copyFromRDRAM).toInt();
	config.frameBufferEmulation.detectCFB = settings.value("detectCFB", config.frameBufferEmulation.detectCFB).toInt();
	config.frameBufferEmulation.N64DepthCompare = settings.value("N64DepthCompare", config.frameBufferEmulation.N64DepthCompare).toInt();
	config.frameBufferEmulation.aspect = settings.value("aspect", config.frameBufferEmulation.aspect).toInt();
	settings.endGroup();

	settings.beginGroup("bloomFilter");
	config.bloomFilter.enable = settings.value("enable", config.bloomFilter.enable).toInt();
	config.bloomFilter.thresholdLevel = settings.value("thresholdLevel", config.bloomFilter.thresholdLevel).toInt();
	config.bloomFilter.blendMode = settings.value("blendMode", config.bloomFilter.blendMode).toInt();
	config.bloomFilter.blurAmount = settings.value("blurAmount", config.bloomFilter.blurAmount).toInt();
	config.bloomFilter.blurStrength = settings.value("blurStrength", config.bloomFilter.blurStrength).toInt();
	settings.endGroup();

	settings.endGroup();
}
