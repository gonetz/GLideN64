#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS
#include "RSP.h"
#include "PluginAPI.h"
#include "Config.h"

void Config::resetToDefaults()
{
	version = CONFIG_VERSION_CURRENT;

	video.fullscreen = 0;
	video.fullscreenWidth = video.windowedWidth = 640;
	video.fullscreenHeight = video.windowedHeight = 480;
	video.fullscreenRefresh = 60;
	video.multisampling = 0;
	video.verticalSync = 0;

	texture.maxAnisotropy = 0;
	texture.bilinearMode = BILINEAR_STANDARD;
	texture.maxBytes = 500 * gc_uMegabyte;
	texture.screenShotFormat = 0;

	generalEmulation.enableFog = 1;
	generalEmulation.enableLOD = 1;
	generalEmulation.enableNoise = 1;
	generalEmulation.enableHWLighting = 0;
	generalEmulation.enableCustomSettings = 1;
	generalEmulation.hacks = 0;

	frameBufferEmulation.enable = 1;
	frameBufferEmulation.copyDepthToRDRAM = 1;
	frameBufferEmulation.copyFromRDRAM = 0;
	frameBufferEmulation.copyToRDRAM = 1;
	frameBufferEmulation.detectCFB = 0;
	frameBufferEmulation.N64DepthCompare = 0;
	frameBufferEmulation.aspect = 1;
	frameBufferEmulation.validityCheckMethod = vcFingerprint;

	textureFilter.txCacheSize = 100 * gc_uMegabyte;
	textureFilter.txDump = 0;
	textureFilter.txEnhancementMode = 0;
	textureFilter.txFilterIgnoreBG = 0;
	textureFilter.txFilterMode = 0;
	textureFilter.txHiresEnable = 0;
	textureFilter.txHiresFullAlphaChannel = 0;
	textureFilter.txHresAltCRC = 0;

	textureFilter.txCacheCompression = 1;
	textureFilter.txForce16bpp = 0;
	textureFilter.txSaveCache = 1;

	api().FindPluginPath(textureFilter.txPath);
	wcscat(textureFilter.txPath, L"/hires_texture");

#ifdef OS_WINDOWS
	font.name = "arial.ttf";
#else
	font.name = "FreeSans.ttf";
#endif
	font.size = 18;
	font.color[0] = 0xB5;
	font.color[1] = 0xE6;
	font.color[2] = 0x1D;
	font.color[3] = 0xFF;
	for (int i = 0; i < 4; ++i)
		font.colorf[i] = font.color[i] / 255.0f;

	bloomFilter.enable = 0;
	bloomFilter.thresholdLevel = 4;
	bloomFilter.blendMode = 0;
	bloomFilter.blurAmount = 10;
	bloomFilter.blurStrength = 20;
}
