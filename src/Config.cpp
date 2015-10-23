#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS
#include "RSP.h"
#include "PluginAPI.h"
#include "Config.h"
#include "wst.h"

void Config::resetToDefaults()
{
	version = CONFIG_VERSION_CURRENT;

#if defined(PANDORA) || defined(VC)
	video.fullscreen = 1;
	video.fullscreenWidth = video.windowedWidth = 800;
#else
	video.fullscreen = 0;
	video.fullscreenWidth = video.windowedWidth = 640;
#endif
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
	generalEmulation.enableShadersStorage = 1;
	generalEmulation.hacks = 0;
#ifdef ANDROID
	generalEmulation.forcePolygonOffset = 0;
	generalEmulation.polygonOffsetFactor = 0.0f;
	generalEmulation.polygonOffsetUnits = 0.0f;
#endif

#ifdef VC
	frameBufferEmulation.enable = 0;
#else
	frameBufferEmulation.enable = 1;
#endif
	frameBufferEmulation.copyDepthToRDRAM = ctDisable;
	frameBufferEmulation.copyFromRDRAM = 0;
	frameBufferEmulation.copyAuxiliary = 0;
	frameBufferEmulation.copyToRDRAM = ctAsync;
	frameBufferEmulation.detectCFB = 0;
	frameBufferEmulation.N64DepthCompare = 0;
	frameBufferEmulation.aspect = 1;

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

	api().GetUserDataPath(textureFilter.txPath);
	gln_wcscat(textureFilter.txPath, wst("/hires_texture"));

#ifdef OS_WINDOWS
	font.name.assign("arial.ttf");
#elif defined (ANDROID)
	font.name.assign("DroidSans.ttf");
#elif defined (PANDORA)
	font.name.assign("LiberationMono-Regular.ttf");
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
