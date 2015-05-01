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

static const char * strIniFileName = "GLideN64.ini";
static const char * strCustomSettingsFileName = "GLideN64.custom.ini";

static
void _loadSettings(QSettings & settings)
{
	config.translationFile = settings.value("translation", config.translationFile.c_str()).toString().toLocal8Bit().constData();

	settings.beginGroup("video");
	config.video.fullscreenWidth = settings.value("fullscreenWidth", config.video.fullscreenWidth).toInt();
	config.video.fullscreenHeight = settings.value("fullscreenHeight", config.video.fullscreenHeight).toInt();
	config.video.windowedWidth = settings.value("windowedWidth", config.video.windowedWidth).toInt();
	config.video.windowedHeight = settings.value("windowedHeight", config.video.windowedHeight).toInt();
	config.video.fullscreenRefresh = settings.value("fullscreenRefresh", config.video.fullscreenRefresh).toInt();
	config.video.multisampling = settings.value("multisampling", config.video.multisampling).toInt();
	settings.endGroup();

	settings.beginGroup("texture");
	config.texture.maxAnisotropy = settings.value("maxAnisotropy", config.texture.maxAnisotropy).toInt();
	config.texture.bilinearMode = settings.value("bilinearMode", config.texture.bilinearMode).toInt();
	config.texture.maxBytes = settings.value("maxBytes", config.texture.maxBytes).toInt();
	config.texture.screenShotFormat = settings.value("screenShotFormat", config.texture.screenShotFormat).toInt();
	settings.endGroup();

	settings.beginGroup("generalEmulation");
	config.generalEmulation.enableFog = settings.value("enableFog", config.generalEmulation.enableFog).toInt();
	config.generalEmulation.enableNoise = settings.value("enableNoise", config.generalEmulation.enableNoise).toInt();
	config.generalEmulation.enableLOD = settings.value("enableLOD", config.generalEmulation.enableLOD).toInt();
	config.generalEmulation.enableHWLighting = settings.value("enableHWLighting", config.generalEmulation.enableHWLighting).toInt();
	config.generalEmulation.enableCustomSettings = settings.value("enableCustomSettings", config.generalEmulation.enableCustomSettings).toInt();
	settings.endGroup();

	settings.beginGroup("frameBufferEmulation");
	config.frameBufferEmulation.enable = settings.value("enable", config.frameBufferEmulation.enable).toInt();
	config.frameBufferEmulation.copyToRDRAM = settings.value("copyToRDRAM", config.frameBufferEmulation.copyToRDRAM).toInt();
	config.frameBufferEmulation.copyDepthToRDRAM = settings.value("copyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM).toInt();
	config.frameBufferEmulation.copyFromRDRAM = settings.value("copyFromRDRAM", config.frameBufferEmulation.copyFromRDRAM).toInt();
	config.frameBufferEmulation.detectCFB = settings.value("detectCFB", config.frameBufferEmulation.detectCFB).toInt();
	config.frameBufferEmulation.N64DepthCompare = settings.value("N64DepthCompare", config.frameBufferEmulation.N64DepthCompare).toInt();
	config.frameBufferEmulation.aspect = settings.value("aspect", config.frameBufferEmulation.aspect).toInt();
	config.frameBufferEmulation.validityCheckMethod = settings.value("validityCheckMethod", config.frameBufferEmulation.validityCheckMethod).toInt();
	settings.endGroup();

	settings.beginGroup("textureFilter");
	config.textureFilter.txFilterMode = settings.value("txFilterMode", config.textureFilter.txFilterMode).toInt();
	config.textureFilter.txEnhancementMode = settings.value("txEnhancementMode", config.textureFilter.txEnhancementMode).toInt();
	config.textureFilter.txFilterIgnoreBG = settings.value("txFilterIgnoreBG", config.textureFilter.txFilterIgnoreBG).toInt();
	config.textureFilter.txCacheSize = settings.value("txCacheSize", config.textureFilter.txCacheSize).toInt();
	config.textureFilter.txHiresEnable = settings.value("txHiresEnable", config.textureFilter.txHiresEnable).toInt();
	config.textureFilter.txHiresFullAlphaChannel = settings.value("txHiresFullAlphaChannel", config.textureFilter.txHiresFullAlphaChannel).toInt();
	config.textureFilter.txHresAltCRC = settings.value("txHresAltCRC", config.textureFilter.txHresAltCRC).toInt();
	config.textureFilter.txDump = settings.value("txDump", config.textureFilter.txDump).toInt();
	config.textureFilter.txForce16bpp = settings.value("txForce16bpp", config.textureFilter.txForce16bpp).toInt();
	config.textureFilter.txCacheCompression = settings.value("txCacheCompression", config.textureFilter.txCacheCompression).toInt();
	config.textureFilter.txSaveCache = settings.value("txSaveCache", config.textureFilter.txSaveCache).toInt();
	settings.value("txPath", "").toString().toWCharArray(config.textureFilter.txPath);
	settings.endGroup();

	settings.beginGroup("font");
	config.font.name = settings.value("name", config.font.name.c_str()).toString().toLocal8Bit().constData();
	config.font.size = settings.value("size", config.font.size).toInt();
	QColor fontColor = settings.value("color", QColor(config.font.color[0], config.font.color[1], config.font.color[2])).value<QColor>();
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
	config.bloomFilter.enable = settings.value("enable", config.bloomFilter.enable).toInt();
	config.bloomFilter.thresholdLevel = settings.value("thresholdLevel", config.bloomFilter.thresholdLevel).toInt();
	config.bloomFilter.blendMode = settings.value("blendMode", config.bloomFilter.blendMode).toInt();
	config.bloomFilter.blurAmount = settings.value("blurAmount", config.bloomFilter.blurAmount).toInt();
	config.bloomFilter.blurStrength = settings.value("blurStrength", config.bloomFilter.blurStrength).toInt();
	settings.endGroup();
}

void loadSettings(const QString & _strIniFolder)
{
//	QSettings settings("Emulation", "GLideN64");
	const u32 hacks = config.generalEmulation.hacks;

	QSettings settings(_strIniFolder + "/" + strIniFileName, QSettings::IniFormat);
	config.version = settings.value("version").toInt();
	if (config.version != CONFIG_VERSION_CURRENT) {
		config.resetToDefaults();
		settings.clear();
		writeSettings(_strIniFolder);
		config.generalEmulation.hacks = hacks;
		return;
	}

	config.resetToDefaults();
	_loadSettings(settings);
	config.generalEmulation.hacks = hacks;
}

void writeSettings(const QString & _strIniFolder)
{
//	QSettings settings("Emulation", "GLideN64");
	QSettings settings(_strIniFolder + "/" + strIniFileName, QSettings::IniFormat);
	settings.setValue("version", config.version);

	settings.setValue("translation", config.translationFile.c_str());

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
	settings.setValue("validityCheckMethod", config.frameBufferEmulation.validityCheckMethod);
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

static
u32 Adler32(u32 crc, const void *buffer, u32 count)
{
	register u32 s1 = crc & 0xFFFF;
	register u32 s2 = (crc >> 16) & 0xFFFF;
	int k;
	const u8 *Buffer = (const u8*)buffer;

	if (Buffer == NULL)
		return 0;

	while (count > 0) {
		/* 5552 is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
		k = (count < 5552 ? count : 5552);
		count -= k;
		while (k--) {
			s1 += *Buffer++;
			s2 += s1;
		}
		/* 65521 is the largest prime smaller than 65536 */
		s1 %= 65521;
		s2 %= 65521;
	}

	return (s2 << 16) | s1;
}

void loadCustomRomSettings(const QString & _strIniFolder, const char * _strRomName)
{
	QSettings settings(_strIniFolder + "/" + strCustomSettingsFileName, QSettings::IniFormat);
	config.version = settings.value("version").toInt();
	if (config.version != CONFIG_VERSION_CURRENT)
		return;

	const QByteArray bytes(_strRomName);
	bool bASCII = true;
	for (int i = 0; i < bytes.length() && bASCII; ++i)
		bASCII = bytes.at(i) >= 0;

	const QString romName = bASCII ? QString::fromLatin1(_strRomName).toUpper() : QString::number(Adler32(0xFFFFFFFF, bytes.data(), bytes.length()), 16).toUpper();
	if (settings.childGroups().indexOf(romName) < 0)
		return;

	settings.beginGroup(romName);
	_loadSettings(settings);
	settings.endGroup();
}

QString getTranslationFile()
{
	return config.translationFile.c_str();
}
