#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS
#include "RSP.h"
#include "PluginAPI.h"
#include "Config.h"
#include "GBI.h"
#include "wst.h"
#include "osal_keys.h"

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
	video.borderless = 0u;
	video.fullscreenHeight = video.windowedHeight = 480;
	video.fullscreenRefresh = 60;
	video.fxaa = 0;
	video.multisampling = 0;
	video.maxMultiSampling = 0;
	video.verticalSync = 0;
	video.deviceName[0] = L'\0';

#if defined(OS_ANDROID)
	video.threadedVideo = 1;
#else
	video.threadedVideo = 0;
#endif

	texture.anisotropy = 0;
	texture.maxAnisotropy = 0;
	texture.bilinearMode = BILINEAR_STANDARD;
	texture.enableHalosRemoval = 0;

	generalEmulation.enableLOD = 1;
	generalEmulation.enableHiresNoiseDithering = 0;
	generalEmulation.enableDitheringPattern = 0;
	generalEmulation.enableDitheringQuantization = 1;
	generalEmulation.rdramImageDitheringMode = BufferDitheringMode::bdmBlueNoise;
	generalEmulation.enableHWLighting = 0;
	generalEmulation.enableCoverage = 0;
	generalEmulation.enableClipping = 1;
	generalEmulation.enableCustomSettings = 1;
	generalEmulation.enableShadersStorage = 1;
	generalEmulation.enableLegacyBlending = 0;
	generalEmulation.enableHybridFilter = 1;
	generalEmulation.enableInaccurateTextureCoordinates = 0;
	generalEmulation.hacks = 0;
#if defined(OS_ANDROID) || defined(OS_IOS)
	generalEmulation.enableFragmentDepthWrite = 0;
	generalEmulation.forcePolygonOffset = 0;
	generalEmulation.polygonOffsetFactor = 0.0f;
	generalEmulation.polygonOffsetUnits = 0.0f;
#else
	generalEmulation.enableFragmentDepthWrite = 1;
#endif

	graphics2D.correctTexrectCoords = tcDisable;
	graphics2D.enableNativeResTexrects = NativeResTexrectsMode::ntDisable;
	graphics2D.bgMode = BGMode::bgStripped;
	graphics2D.enableTexCoordBounds = 0;

	frameBufferEmulation.enable = 1;
	frameBufferEmulation.copyDepthToRDRAM = cdSoftwareRender;
	frameBufferEmulation.copyFromRDRAM = 0;
	frameBufferEmulation.copyAuxToRDRAM = 0;
#ifdef M64P_GLIDENUI
	frameBufferEmulation.copyToRDRAM = ctSync;
#else
	frameBufferEmulation.copyToRDRAM = ctDoubleBuffer;
#endif
	frameBufferEmulation.N64DepthCompare = dcDisable;
	frameBufferEmulation.forceDepthBufferClear = 0;
	frameBufferEmulation.aspect = a43;
	frameBufferEmulation.bufferSwapMode = bsOnVerticalInterrupt;
	frameBufferEmulation.nativeResFactor = 0;
	frameBufferEmulation.fbInfoReadColorChunk = 0;
	frameBufferEmulation.fbInfoReadDepthChunk = 1;
	frameBufferEmulation.copyDepthToMainDepthBuffer = 0;
#ifndef MUPENPLUSAPI
	frameBufferEmulation.fbInfoDisabled = 0;
#else
	frameBufferEmulation.fbInfoDisabled = 1;
#endif
	frameBufferEmulation.enableOverscan = 0;

	textureFilter.txFilterMode = 0;
	textureFilter.txEnhancementMode = 0;
	textureFilter.txDeposterize = 0;
	textureFilter.txFilterIgnoreBG = 0;
	textureFilter.txCacheSize = 100 * gc_uMegabyte;

	textureFilter.txHiresEnable = 0;
	textureFilter.txHiresFullAlphaChannel = 1;
	textureFilter.txHresAltCRC = 0;

	textureFilter.txForce16bpp = 0;
	textureFilter.txCacheCompression = 1;
	textureFilter.txSaveCache = 1;
	textureFilter.txDump = 0;
	textureFilter.txStrongCRC = 0;

	textureFilter.txEnhancedTextureFileStorage = 0;
	textureFilter.txHiresTextureFileStorage = 0;
	textureFilter.txNoTextureFileStorage = 0;

	textureFilter.txHiresVramLimit = 0u;

	api().GetUserDataPath(textureFilter.txPath);
	gln_wcscat(textureFilter.txPath, wst("/hires_texture"));
	api().GetUserCachePath(textureFilter.txCachePath);
	gln_wcscat(textureFilter.txCachePath, wst("/cache"));
	api().GetUserCachePath(textureFilter.txDumpPath);
	gln_wcscat(textureFilter.txDumpPath, wst("/texture_dump"));

#ifdef OS_WINDOWS
	font.name.assign("arial.ttf");
#elif defined (OS_ANDROID)
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

	gammaCorrection.force = 0;
	gammaCorrection.level = 2.0f;

	onScreenDisplay.vis = 0;
	onScreenDisplay.fps = 0;
	onScreenDisplay.percent = 0;
	onScreenDisplay.internalResolution = 0;
	onScreenDisplay.renderingResolution = 0;
	onScreenDisplay.statistics = 0;
	onScreenDisplay.pos = posBottomLeft;

	for (u32 idx = 0; idx < HotKey::hkTotal; ++idx) {
		hotkeys.enabledKeys[idx] = 0;
		hotkeys.keys[idx] = 0;
	}

	debug.dumpMode = 0;
}

bool isHWLightingAllowed()
{
	if (config.generalEmulation.enableHWLighting == 0)
		return false;
	return GBI.isHWLSupported();
}

void Config::validate()
{
	if (frameBufferEmulation.enable != 0 && frameBufferEmulation.N64DepthCompare != dcDisable)
		video.multisampling = 0;
	if (frameBufferEmulation.nativeResFactor == 1) {
		graphics2D.enableNativeResTexrects = 0;
		graphics2D.correctTexrectCoords = tcDisable;
	} else {
		if (graphics2D.enableNativeResTexrects != 0)
			graphics2D.correctTexrectCoords = tcDisable;
	}
}

const char* Config::hotkeyIniName(u32 _idx)
{
	switch (_idx)
	{
	case Config::HotKey::hkTexDump:
		return "hkTexDump";
	case Config::HotKey::hkHdTexReload:
		return "hkHdTexReload";
	case Config::HotKey::hkHdTexToggle:
		return "hkHdTexToggle";
	case Config::HotKey::hkTexCoordBounds:
		return "hkTexCoordBounds";
	case Config::HotKey::hkNativeResTexrects:
		return "hkNativeResTexrects";
	case Config::HotKey::hkVsync:
		return "hkVsync";
	case Config::HotKey::hkFBEmulation:
		return "hkFBEmulation";
	case Config::HotKey::hkN64DepthCompare:
		return "hkN64DepthCompare";
	case Config::HotKey::hkOsdVis:
		return "hkOsdVis";
	case Config::HotKey::hkOsdFps:
		return "hkOsdFps";
	case Config::HotKey::hkOsdPercent:
		return "hkOsdPercent";
	case Config::HotKey::hkOsdInternalResolution:
		return "hkOsdInternalResolution";
	case Config::HotKey::hkOsdRenderingResolution:
		return "hkOsdRenderingResolution";
	case Config::HotKey::hkForceGammaCorrection:
		return "hkForceGammaCorrection";
	case Config::HotKey::hkInaccurateTexCords:
		return "hkInaccurateTexCords";
	case Config::HotKey::hkStrongCRC:
		return "hkStrongCRC";
	}
	return nullptr;
}

const char* Config::enabledHotkeyIniName(u32 _idx)
{
	switch (_idx)
	{
	case Config::HotKey::hkTexDump:
		return "hkTexDumpEnabled";
	case Config::HotKey::hkHdTexReload:
		return "hkHdTexReloadEnabled";
	case Config::HotKey::hkHdTexToggle:
		return "hkHdTexToggleEnabled";
	case Config::HotKey::hkTexCoordBounds:
		return "hkTexCoordBoundsEnabled";
	case Config::HotKey::hkNativeResTexrects:
		return "hkNativeResTexrectsEnabled";
	case Config::HotKey::hkVsync:
		return "hkVsyncEnabled";
	case Config::HotKey::hkFBEmulation:
		return "hkFBEmulationEnabled";
	case Config::HotKey::hkN64DepthCompare:
		return "hkN64DepthCompareEnabled";
	case Config::HotKey::hkOsdVis:
		return "hkOsdVisEnabled";
	case Config::HotKey::hkOsdFps:
		return "hkOsdFpsEnabled";
	case Config::HotKey::hkOsdPercent:
		return "hkOsdPercentEnabled";
	case Config::HotKey::hkOsdInternalResolution:
		return "hkOsdInternalResolutionEnabled";
	case Config::HotKey::hkOsdRenderingResolution:
		return "hkOsdRenderingResolutionEnabled";
	case Config::HotKey::hkForceGammaCorrection:
		return "hkForceGammaCorrectionEnabled";
	case Config::HotKey::hkInaccurateTexCords:
		return "hkInaccurateTexCordsEnabled";
	case Config::HotKey::hkStrongCRC:
		return "hkStrongCRCEnabled";
	}
	return nullptr;
}

