#include "GLideN64_mupenplus.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <osal_files.h>
#include <algorithm>

#include "../Textures.h"
#include "../Config.h"
#include "../GLideN64.h"
#include "../GBI.h"
#include "../RSP.h"
#include "../Log.h"

Config config;

m64p_handle g_configVideoGeneral = nullptr;
m64p_handle g_configVideoGliden64 = nullptr;

bool Config_SetDefault()
{
	if (ConfigOpenSection("Video-General", &g_configVideoGeneral) != M64ERR_SUCCESS) {
		LOG(LOG_ERROR, "Unable to open Video-General configuration section");
		g_configVideoGeneral = nullptr;
		return false;
	}
	if (ConfigOpenSection("Video-GLideN64", &g_configVideoGliden64) != M64ERR_SUCCESS) {
		LOG(LOG_ERROR, "Unable to open GLideN64 configuration section");
		g_configVideoGliden64 = nullptr;
		return false;
	}

	config.resetToDefaults();
	// Set default values for "Video-General" section, if they are not set yet. Taken from RiceVideo
	m64p_error res = ConfigSetDefaultBool(g_configVideoGeneral, "Fullscreen", config.video.fullscreen, "Use fullscreen mode if True, or windowed mode if False ");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGeneral, "ScreenWidth", config.video.windowedWidth, "Width of output window or fullscreen width");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGeneral, "ScreenHeight", config.video.windowedHeight, "Height of output window or fullscreen height");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGeneral, "VerticalSync", config.video.verticalSync, "If true, activate the SDL_GL_SWAP_CONTROL attribute");
	assert(res == M64ERR_SUCCESS);

	res = ConfigSetDefaultInt(g_configVideoGliden64, "configVersion", CONFIG_VERSION_CURRENT, "Settings version. Don't touch it.");
	assert(res == M64ERR_SUCCESS);

	res = ConfigSetDefaultBool(g_configVideoGliden64, "ThreadedVideo", config.video.threadedVideo, "Enable threaded video backend");
	assert(res == M64ERR_SUCCESS);

	res = ConfigSetDefaultInt(g_configVideoGliden64, "MultiSampling", config.video.multisampling, "Enable/Disable MultiSampling (0=off, 2,4,8,16=quality)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "FXAA", config.video.fxaa, "Enable/Disable Fast Approximate Anti-Aliasing FXAA");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "AspectRatio", config.frameBufferEmulation.aspect, "Screen aspect ratio (0=stretch, 1=force 4:3, 2=force 16:9, 3=adjust)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "BufferSwapMode", config.frameBufferEmulation.bufferSwapMode, "Swap frame buffers (0=On VI update call, 1=On VI origin change, 2=On buffer update)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "UseNativeResolutionFactor", config.frameBufferEmulation.nativeResFactor, "Frame buffer size is the factor of N64 native resolution.");
	assert(res == M64ERR_SUCCESS);

	//#Texture Settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "bilinearMode", config.texture.bilinearMode, "Bilinear filtering mode (0=N64 3point, 1=standard)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "enableHalosRemoval", config.texture.enableHalosRemoval, "Remove halos around filtered textures.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "MaxAnisotropy", config.texture.maxAnisotropy, "Max level of Anisotropic Filtering, 0 for off");
	assert(res == M64ERR_SUCCESS);
	//#Emulation Settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableDitheringPattern", config.generalEmulation.enableDitheringPattern, "Enable dithering pattern on output image.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableHiresNoiseDithering", config.generalEmulation.enableHiresNoiseDithering, "Enable hi-res noise dithering.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "DitheringQuantization", config.generalEmulation.enableDitheringQuantization, "Dither with color quantization.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "RDRAMImageDitheringMode", config.generalEmulation.rdramImageDitheringMode, "Dithering mode for image in RDRAM. (0=disable, 1=bayer, 2=magic square, 3=blue noise)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableLOD", config.generalEmulation.enableLOD, "Enable LOD emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableHWLighting", config.generalEmulation.enableHWLighting, "Enable hardware per-pixel lighting.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableShadersStorage", config.generalEmulation.enableShadersStorage, "Use persistent storage for compiled shaders.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableLegacyBlending", config.generalEmulation.enableLegacyBlending, "Do not use shaders to emulate N64 blending modes. Works faster on slow GPU. Can cause glitches.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableHybridFilter", config.generalEmulation.enableHybridFilter, "Enable hybrid integer scaling filter, this can be slow with low-end GPUs");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableFragmentDepthWrite", config.generalEmulation.enableFragmentDepthWrite, "Enable writing of fragment depth. Some mobile GPUs do not support it, thus it made optional. Leave enabled.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCustomSettings", config.generalEmulation.enableCustomSettings, "Use GLideN64 per-game settings.");
	assert(res == M64ERR_SUCCESS);
#if defined(OS_ANDROID) || defined(OS_IOS)
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableBlitScreenWorkaround", config.generalEmulation.enableBlitScreenWorkaround, "Enable to render everything upside down");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ForcePolygonOffset", config.generalEmulation.forcePolygonOffset, "If true, use polygon offset values specified below");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "PolygonOffsetFactor", config.generalEmulation.polygonOffsetFactor, "Specifies a scale factor that is used to create a variable depth offset for each polygon");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "PolygonOffsetUnits", config.generalEmulation.polygonOffsetUnits, "Is multiplied by an implementation-specific value to create a constant depth offset");
	assert(res == M64ERR_SUCCESS);
#endif
	//#2D graphics Settings
	res = ConfigSetDefaultInt(g_configVideoGliden64, "CorrectTexrectCoords", config.graphics2D.correctTexrectCoords, "Make texrect coordinates continuous to avoid black lines between them. (0=Off, 1=Auto, 2=Force)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableNativeResTexrects", config.graphics2D.enableNativeResTexrects, "Render 2D texrects in native resolution to fix misalignment between parts of 2D image. (0=Off, 1=Optimized, 2=Unoptimized)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "BackgroundsMode", config.graphics2D.bgMode, "Render backgrounds mode (HLE only). (0=One piece (fast), 1=Stripped (precise))");
	assert(res == M64ERR_SUCCESS);

	//#Frame Buffer Settings:"
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableFBEmulation", config.frameBufferEmulation.enable, "Enable frame and|or depth buffer emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyAuxiliaryToRDRAM", config.frameBufferEmulation.copyAuxToRDRAM, "Copy auxiliary buffers to RDRAM");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableN64DepthCompare", config.frameBufferEmulation.N64DepthCompare, "Enable N64 depth compare instead of OpenGL standard one. Experimental. (0=Off, 1=Fast, 2=Compatible)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ForceDepthBufferClear", config.frameBufferEmulation.forceDepthBufferClear, "Force depth buffer clear. Hack. Needed for Eikou no Saint Andrews.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "DisableFBInfo", config.frameBufferEmulation.fbInfoDisabled, "Disable buffers read/write with FBInfo. Use for games, which do not work with FBInfo.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "FBInfoReadColorChunk", config.frameBufferEmulation.fbInfoReadColorChunk, "Read color buffer by 4kb chunks (strict follow to FBRead specification)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "FBInfoReadDepthChunk", config.frameBufferEmulation.fbInfoReadDepthChunk, "Read depth buffer by 4kb chunks (strict follow to FBRead specification)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableCopyColorToRDRAM", config.frameBufferEmulation.copyToRDRAM, "Enable color buffer copy to RDRAM (0=do not copy, 1=copy in sync mode, 2=Double Buffer, 3=Triple Buffer)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableCopyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM, "Enable depth buffer copy to RDRAM  (0=do not copy, 1=copy from video memory, 2=use software render)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyColorFromRDRAM", config.frameBufferEmulation.copyFromRDRAM, "Enable color buffer copy from RDRAM.");
	assert(res == M64ERR_SUCCESS);
#if defined(OS_WINDOWS)
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyDepthToMainDepthBuffer", config.frameBufferEmulation.copyDepthToMainDepthBuffer, "Enable copy of depth information from FBO to main depth buffer. Required for some Reshade shaders.");
	assert(res == M64ERR_SUCCESS);
#endif
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableOverscan", config.frameBufferEmulation.enableOverscan, "Enable resulted image crop by Overscan.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanPalLeft", config.frameBufferEmulation.overscanPAL.left, "PAL mode. Left bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanPalRight", config.frameBufferEmulation.overscanPAL.right, "PAL mode. Right bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanPalTop", config.frameBufferEmulation.overscanPAL.top, "PAL mode. Top bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanPalBottom", config.frameBufferEmulation.overscanPAL.bottom, "PAL mode. Bottom bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanNtscLeft", config.frameBufferEmulation.overscanNTSC.left, "NTSC mode. Left bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanNtscRight", config.frameBufferEmulation.overscanNTSC.right, "NTSC mode. Right bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanNtscTop", config.frameBufferEmulation.overscanNTSC.top, "NTSC mode. Top bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "OverscanNtscBottom", config.frameBufferEmulation.overscanNTSC.bottom, "NTSC mode. Bottom bound of Overscan");
	assert(res == M64ERR_SUCCESS);
	//#Texture filter settings
	res = ConfigSetDefaultInt(g_configVideoGliden64, "txFilterMode", config.textureFilter.txFilterMode, "Texture filter (0=none, 1=Smooth filtering 1, 2=Smooth filtering 2, 3=Smooth filtering 3, 4=Smooth filtering 4, 5=Sharp filtering 1, 6=Sharp filtering 2)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "txEnhancementMode", config.textureFilter.txEnhancementMode, "Texture Enhancement (0=none, 1=store as is, 2=X2, 3=X2SAI, 4=HQ2X, 5=HQ2XS, 6=LQ2X, 7=LQ2XS, 8=HQ4X, 9=2xBRZ, 10=3xBRZ, 11=4xBRZ, 12=5xBRZ), 13=6xBRZ");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txDeposterize", config.textureFilter.txDeposterize, "Deposterize texture before enhancement.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txFilterIgnoreBG", config.textureFilter.txFilterIgnoreBG, "Don't filter background textures.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "txCacheSize", config.textureFilter.txCacheSize/ gc_uMegabyte, "Size of memory cache for enhanced textures in megabytes.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txHiresEnable", config.textureFilter.txHiresEnable, "Use high-resolution texture packs if available.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txHiresFullAlphaChannel", config.textureFilter.txHiresFullAlphaChannel, "Allow to use alpha channel of high-res texture fully.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txHresAltCRC", config.textureFilter.txHresAltCRC, "Use alternative method of paletted textures CRC calculation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txDump", config.textureFilter.txDump, "Enable dump of loaded N64 textures.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txCacheCompression", config.textureFilter.txCacheCompression, "Zip textures cache.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txForce16bpp", config.textureFilter.txForce16bpp, "Force use 16bit texture formats for HD textures.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txSaveCache", config.textureFilter.txSaveCache, "Save texture cache to hard disk.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txEnhancedTextureFileStorage", config.textureFilter.txEnhancedTextureFileStorage, "Use file storage instead of memory cache for enhanced textures.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "txHiresTextureFileStorage", config.textureFilter.txHiresTextureFileStorage, "Use file storage instead of memory cache for HD textures.");
	assert(res == M64ERR_SUCCESS);
	// Convert to multibyte
	char txPath[PLUGIN_PATH_SIZE * 2];
	wcstombs(txPath, config.textureFilter.txPath, PLUGIN_PATH_SIZE * 2);
	res = ConfigSetDefaultString(g_configVideoGliden64, "txPath", txPath, "Path to folder with hi-res texture packs.");
	assert(res == M64ERR_SUCCESS);
	wcstombs(txPath, config.textureFilter.txCachePath, PLUGIN_PATH_SIZE * 2);
	res = ConfigSetDefaultString(g_configVideoGliden64, "txCachePath", txPath, "Path to folder where plugin saves texture cache files.");
	assert(res == M64ERR_SUCCESS);
	wcstombs(txPath, config.textureFilter.txDumpPath, PLUGIN_PATH_SIZE * 2);
	res = ConfigSetDefaultString(g_configVideoGliden64, "txDumpPath", txPath, "Path to folder where plugin saves dumped textures.");
	assert(res == M64ERR_SUCCESS);

	res = ConfigSetDefaultString(g_configVideoGliden64, "fontName", config.font.name.c_str(), "File name of True Type Font for text messages.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "fontSize", config.font.size, "Font size.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultString(g_configVideoGliden64, "fontColor", "B5E61D", "Font color in RGB format.");
	assert(res == M64ERR_SUCCESS);

	//#Gamma correction settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ForceGammaCorrection", config.gammaCorrection.force, "Force gamma correction.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "GammaCorrectionLevel", config.gammaCorrection.level, "Gamma correction level.");
	assert(res == M64ERR_SUCCESS);

	//#On screen display settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ShowFPS", config.onScreenDisplay.fps, "Show FPS counter.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ShowVIS", config.onScreenDisplay.vis, "Show VI/S counter.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ShowPercent", config.onScreenDisplay.percent, "Show percent counter.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ShowInternalResolution", config.onScreenDisplay.internalResolution, "Show internal resolution.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ShowRenderingResolution", config.onScreenDisplay.renderingResolution, "Show rendering resolution.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "CountersPos", config.onScreenDisplay.pos,
		"Counters position (1=top left, 2=top center, 4=top right, 8=bottom left, 16=bottom center, 32=bottom right)");
	assert(res == M64ERR_SUCCESS);

#ifdef DEBUG_DUMP
	//#Debug settings
	res = ConfigSetDefaultInt(g_configVideoGliden64, "DebugDumpMode", config.debug.dumpMode, "Enable debug dump. Set 3 to normal or 7 to detailed dump.");
	assert(res == M64ERR_SUCCESS);
#endif

	return ConfigSaveSection("Video-GLideN64") == M64ERR_SUCCESS;
}

void Config_LoadCustomConfig()
{
	if (ConfigExternalGetParameter == nullptr || ConfigExternalOpen == nullptr || ConfigExternalClose == nullptr)
		return;
	char value[PATH_MAX];
	m64p_error result;
	std::string ROMname = RSP.romname;
	const char* pathName = ConfigGetSharedDataFilepath("GLideN64.custom.ini");
	if (pathName == nullptr)
		return;
	for (size_t pos = ROMname.find(' '); pos != std::string::npos; pos = ROMname.find(' ', pos))
		ROMname.replace(pos, 1, "%20");
	for (size_t pos = ROMname.find('\''); pos != std::string::npos; pos = ROMname.find('\'', pos))
		ROMname.replace(pos, 1, "%27");
	std::transform(ROMname.begin(), ROMname.end(), ROMname.begin(), ::toupper);
	const char* sectionName = ROMname.c_str();
	m64p_handle fileHandle;
	result = ConfigExternalOpen(pathName, &fileHandle);
	if (result != M64ERR_SUCCESS)
		return;
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\fullscreenWidth", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.video.fullscreenWidth = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\fullscreenHeight", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.video.fullscreenHeight = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\windowedWidth", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.video.windowedWidth = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\windowedHeight", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.video.windowedHeight = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\fullscreenRefresh", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.video.fullscreenRefresh = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\multisampling", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.video.multisampling = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "video\\FXAA", value, sizeof(value));
	if (result == M64ERR_SUCCESS) {
		config.video.fxaa = atoi(value);
		if (config.video.fxaa != 0)
			config.video.multisampling = 0;
	}

	result = ConfigExternalGetParameter(fileHandle, sectionName, "texture\\maxAnisotropy", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.texture.maxAnisotropy = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "texture\\bilinearMode", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.texture.bilinearMode = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "texture\\enableHalosRemoval", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.texture.enableHalosRemoval = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "texture\\screenShotFormat", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.texture.screenShotFormat = atoi(value);

	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableDitheringPattern", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableDitheringPattern = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableHiresNoiseDithering", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableHiresNoiseDithering = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableDitheringQuantization", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableDitheringQuantization = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\rdramImageDitheringMode", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.rdramImageDitheringMode = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableLOD", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableLOD = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableHWLighting", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableHWLighting = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableShadersStorage", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableShadersStorage = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableLegacyBlending", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableLegacyBlending = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "generalEmulation\\enableFragmentDepthWrite", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.generalEmulation.enableFragmentDepthWrite = atoi(value);

	result = ConfigExternalGetParameter(fileHandle, sectionName, "graphics2D\\correctTexrectCoords", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.graphics2D.correctTexrectCoords = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "graphics2D\\enableNativeResTexrects", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.graphics2D.enableNativeResTexrects = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "graphics2D\\bgMode", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.graphics2D.bgMode = atoi(value);

	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\enable", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.enable = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\aspect", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.aspect = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\nativeResFactor", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.nativeResFactor = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\bufferSwapMode", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.bufferSwapMode = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\N64DepthCompare", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.N64DepthCompare = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\forceDepthBufferClear", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.forceDepthBufferClear = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\copyAuxToRDRAM", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.copyAuxToRDRAM = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\copyToRDRAM", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.copyToRDRAM = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\copyDepthToRDRAM", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.copyDepthToRDRAM = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\copyFromRDRAM", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.copyFromRDRAM = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\fbInfoDisabled", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.fbInfoDisabled = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\fbInfoReadColorChunk", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.fbInfoReadColorChunk = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\fbInfoReadDepthChunk", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.fbInfoReadDepthChunk = atoi(value);
#if defined(OS_WINDOWS)
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\EnableCopyDepthToMainDepthBuffer", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.copyDepthToMainDepthBuffer = atoi(value);
#endif
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\EnableOverscan", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.enableOverscan = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanPalLeft", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanPAL.left = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanPalRight", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanPAL.right = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanPalTop", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanPAL.top = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanPalBottom", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanPAL.bottom = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanNtscLeft", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanNTSC.left = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanNtscRight", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanNTSC.right = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanNtscTop", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanNTSC.top = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "frameBufferEmulation\\OverscanNtscBottom", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.frameBufferEmulation.overscanNTSC.bottom = atoi(value);

	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txFilterMode", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txFilterMode = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txEnhancementMode", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txEnhancementMode = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txDeposterize", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txDeposterize = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txFilterIgnoreBG", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txFilterIgnoreBG = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txCacheSize", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txCacheSize = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txHiresEnable", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txHiresEnable = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txHiresFullAlphaChannel", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txHiresFullAlphaChannel = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txHresAltCRC", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txHresAltCRC = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txDump", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txDump = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txForce16bpp", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txForce16bpp = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txCacheCompression", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txCacheCompression = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txSaveCache", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txSaveCache = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txEnhancedTextureFileStorage", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txEnhancedTextureFileStorage = atoi(value);
	result = ConfigExternalGetParameter(fileHandle, sectionName, "textureFilter\\txHiresTextureFileStorage", value, sizeof(value));
	if (result == M64ERR_SUCCESS) config.textureFilter.txHiresTextureFileStorage = atoi(value);
	ConfigExternalClose(fileHandle);
}

void Config_LoadConfig()
{
	if (g_configVideoGeneral == nullptr || g_configVideoGliden64 == nullptr)
		return;

	config.video.fullscreen = ConfigGetParamBool(g_configVideoGeneral, "Fullscreen");
	config.video.windowedWidth = ConfigGetParamInt(g_configVideoGeneral, "ScreenWidth");
	config.video.windowedHeight = ConfigGetParamInt(g_configVideoGeneral, "ScreenHeight");
	config.video.verticalSync = ConfigGetParamBool(g_configVideoGeneral, "VerticalSync");
	config.video.threadedVideo = ConfigGetParamBool(g_configVideoGliden64, "ThreadedVideo");
	const u32 multisampling = ConfigGetParamInt(g_configVideoGliden64, "MultiSampling");
	config.video.multisampling = multisampling == 0 ? 0 : pow2(multisampling);
	config.video.fxaa = ConfigGetParamBool(g_configVideoGliden64, "FXAA");
	if (config.video.fxaa != 0)
		config.video.multisampling = 0;
	config.frameBufferEmulation.aspect = ConfigGetParamInt(g_configVideoGliden64, "AspectRatio");
	config.frameBufferEmulation.bufferSwapMode = ConfigGetParamInt(g_configVideoGliden64, "BufferSwapMode");
	config.frameBufferEmulation.nativeResFactor = ConfigGetParamInt(g_configVideoGliden64, "UseNativeResolutionFactor");

	//#Texture Settings
	config.texture.bilinearMode = ConfigGetParamBool(g_configVideoGliden64, "bilinearMode");
	config.texture.maxAnisotropy = ConfigGetParamInt(g_configVideoGliden64, "MaxAnisotropy");
	config.texture.enableHalosRemoval = ConfigGetParamBool(g_configVideoGliden64, "enableHalosRemoval");
	//#Emulation Settings
	config.generalEmulation.enableDitheringPattern = ConfigGetParamBool(g_configVideoGliden64, "EnableDitheringPattern");
	config.generalEmulation.enableHiresNoiseDithering = ConfigGetParamBool(g_configVideoGliden64, "EnableHiresNoiseDithering");
	config.generalEmulation.enableDitheringQuantization = ConfigGetParamBool(g_configVideoGliden64, "DitheringQuantization");
	config.generalEmulation.rdramImageDitheringMode = ConfigGetParamInt(g_configVideoGliden64, "RDRAMImageDitheringMode");

	config.generalEmulation.enableLOD = ConfigGetParamBool(g_configVideoGliden64, "EnableLOD");
	config.generalEmulation.enableHWLighting = ConfigGetParamBool(g_configVideoGliden64, "EnableHWLighting");
	config.generalEmulation.enableShadersStorage = ConfigGetParamBool(g_configVideoGliden64, "EnableShadersStorage");
	config.generalEmulation.enableLegacyBlending = ConfigGetParamBool(g_configVideoGliden64, "EnableLegacyBlending");
	config.generalEmulation.enableHybridFilter = ConfigGetParamBool(g_configVideoGliden64, "EnableHybridFilter");
	config.generalEmulation.enableFragmentDepthWrite = ConfigGetParamBool(g_configVideoGliden64, "EnableFragmentDepthWrite");
	config.generalEmulation.enableCustomSettings = ConfigGetParamBool(g_configVideoGliden64, "EnableCustomSettings");
#if defined(OS_ANDROID) || defined(OS_IOS)
	config.generalEmulation.enableBlitScreenWorkaround = ConfigGetParamBool(g_configVideoGliden64, "EnableBlitScreenWorkaround");
	config.generalEmulation.forcePolygonOffset = ConfigGetParamBool(g_configVideoGliden64, "ForcePolygonOffset");
	config.generalEmulation.polygonOffsetFactor = ConfigGetParamFloat(g_configVideoGliden64, "PolygonOffsetFactor");
	config.generalEmulation.polygonOffsetUnits = ConfigGetParamFloat(g_configVideoGliden64, "PolygonOffsetUnits");
#endif
	//#2D graphics Settings
	config.graphics2D.correctTexrectCoords = ConfigGetParamInt(g_configVideoGliden64, "CorrectTexrectCoords");
	config.graphics2D.enableNativeResTexrects = ConfigGetParamInt(g_configVideoGliden64, "EnableNativeResTexrects");
	config.graphics2D.bgMode = ConfigGetParamInt(g_configVideoGliden64, "BackgroundsMode");
	//#Frame Buffer Settings:"
	config.frameBufferEmulation.enable = ConfigGetParamBool(g_configVideoGliden64, "EnableFBEmulation");
	config.frameBufferEmulation.copyAuxToRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyAuxiliaryToRDRAM");
	config.frameBufferEmulation.copyToRDRAM = ConfigGetParamInt(g_configVideoGliden64, "EnableCopyColorToRDRAM");
	config.frameBufferEmulation.copyDepthToRDRAM = ConfigGetParamInt(g_configVideoGliden64, "EnableCopyDepthToRDRAM");
	config.frameBufferEmulation.copyFromRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyColorFromRDRAM");
	config.frameBufferEmulation.N64DepthCompare = ConfigGetParamInt(g_configVideoGliden64, "EnableN64DepthCompare");
	config.frameBufferEmulation.forceDepthBufferClear = ConfigGetParamBool(g_configVideoGliden64, "ForceDepthBufferClear");
	config.frameBufferEmulation.fbInfoDisabled = ConfigGetParamBool(g_configVideoGliden64, "DisableFBInfo");
	config.frameBufferEmulation.fbInfoReadColorChunk = ConfigGetParamBool(g_configVideoGliden64, "FBInfoReadColorChunk");
	config.frameBufferEmulation.fbInfoReadDepthChunk = ConfigGetParamBool(g_configVideoGliden64, "FBInfoReadDepthChunk");
#if defined(OS_WINDOWS)
	config.frameBufferEmulation.copyDepthToMainDepthBuffer = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyDepthToMainDepthBuffer");
#endif
	config.frameBufferEmulation.enableOverscan = ConfigGetParamBool(g_configVideoGliden64, "EnableOverscan");
	config.frameBufferEmulation.overscanPAL.left = ConfigGetParamInt(g_configVideoGliden64, "OverscanPalLeft");
	config.frameBufferEmulation.overscanPAL.right = ConfigGetParamInt(g_configVideoGliden64, "OverscanPalRight");
	config.frameBufferEmulation.overscanPAL.top = ConfigGetParamInt(g_configVideoGliden64, "OverscanPalTop");
	config.frameBufferEmulation.overscanPAL.bottom = ConfigGetParamInt(g_configVideoGliden64, "OverscanPalBottom");
	config.frameBufferEmulation.overscanNTSC.left = ConfigGetParamInt(g_configVideoGliden64, "OverscanNtscLeft");
	config.frameBufferEmulation.overscanNTSC.right = ConfigGetParamInt(g_configVideoGliden64, "OverscanNtscRight");
	config.frameBufferEmulation.overscanNTSC.top = ConfigGetParamInt(g_configVideoGliden64, "OverscanNtscTop");
	config.frameBufferEmulation.overscanNTSC.bottom = ConfigGetParamInt(g_configVideoGliden64, "OverscanNtscBottom");
	//#Texture filter settings
	config.textureFilter.txFilterMode = ConfigGetParamInt(g_configVideoGliden64, "txFilterMode");
	config.textureFilter.txEnhancementMode = ConfigGetParamInt(g_configVideoGliden64, "txEnhancementMode");
	config.textureFilter.txDeposterize = ConfigGetParamInt(g_configVideoGliden64, "txDeposterize");
	config.textureFilter.txFilterIgnoreBG = ConfigGetParamBool(g_configVideoGliden64, "txFilterIgnoreBG");
	config.textureFilter.txCacheSize = ConfigGetParamInt(g_configVideoGliden64, "txCacheSize") * gc_uMegabyte;
	config.textureFilter.txHiresEnable = ConfigGetParamBool(g_configVideoGliden64, "txHiresEnable");
	config.textureFilter.txHiresFullAlphaChannel = ConfigGetParamBool(g_configVideoGliden64, "txHiresFullAlphaChannel");
	config.textureFilter.txHresAltCRC = ConfigGetParamBool(g_configVideoGliden64, "txHresAltCRC");
	config.textureFilter.txDump = ConfigGetParamBool(g_configVideoGliden64, "txDump");
	config.textureFilter.txForce16bpp = ConfigGetParamBool(g_configVideoGliden64, "txForce16bpp");
	config.textureFilter.txCacheCompression = ConfigGetParamBool(g_configVideoGliden64, "txCacheCompression");
	config.textureFilter.txSaveCache = ConfigGetParamBool(g_configVideoGliden64, "txSaveCache");
	config.textureFilter.txEnhancedTextureFileStorage = ConfigGetParamBool(g_configVideoGliden64, "txEnhancedTextureFileStorage");
	config.textureFilter.txHiresTextureFileStorage = ConfigGetParamBool(g_configVideoGliden64, "txHiresTextureFileStorage");
	::mbstowcs(config.textureFilter.txPath, ConfigGetParamString(g_configVideoGliden64, "txPath"), PLUGIN_PATH_SIZE);
	::mbstowcs(config.textureFilter.txCachePath, ConfigGetParamString(g_configVideoGliden64, "txCachePath"), PLUGIN_PATH_SIZE);
	::mbstowcs(config.textureFilter.txDumpPath, ConfigGetParamString(g_configVideoGliden64, "txDumpPath"), PLUGIN_PATH_SIZE);

	//#Font settings
	config.font.name = ConfigGetParamString(g_configVideoGliden64, "fontName");
	if (config.font.name.empty())
		config.font.name = "arial.ttf";
	char buf[16];
	sprintf(buf, "0x%s", ConfigGetParamString(g_configVideoGliden64, "fontColor"));
	long int uColor = strtol(buf, nullptr, 16);
	if (uColor != 0) {
		config.font.color[0] = _SHIFTR(uColor, 16, 8);
		config.font.color[1] = _SHIFTR(uColor, 8, 8);
		config.font.color[2] = _SHIFTR(uColor, 0, 8);
		config.font.color[3] = 0xFF;
		config.font.colorf[0] = _FIXED2FLOAT(config.font.color[0], 8);
		config.font.colorf[1] = _FIXED2FLOAT(config.font.color[1], 8);
		config.font.colorf[2] = _FIXED2FLOAT(config.font.color[2], 8);
		config.font.colorf[3] = 1.0f;
	}
	config.font.size = ConfigGetParamInt(g_configVideoGliden64, "fontSize");
	if (config.font.size == 0)
		config.font.size = 30;

	//#Gamma correction settings
	config.gammaCorrection.force = ConfigGetParamBool(g_configVideoGliden64, "ForceGammaCorrection");
	config.gammaCorrection.level = ConfigGetParamFloat(g_configVideoGliden64, "GammaCorrectionLevel");

	//#On screen display settings
	config.onScreenDisplay.fps = ConfigGetParamBool(g_configVideoGliden64, "ShowFPS");
	config.onScreenDisplay.vis = ConfigGetParamBool(g_configVideoGliden64, "ShowVIS");
	config.onScreenDisplay.percent = ConfigGetParamBool(g_configVideoGliden64, "ShowPercent");
	config.onScreenDisplay.internalResolution = ConfigGetParamBool(g_configVideoGliden64, "ShowInternalResolution");
	config.onScreenDisplay.renderingResolution = ConfigGetParamBool(g_configVideoGliden64, "ShowRenderingResolution");
	config.onScreenDisplay.pos = ConfigGetParamInt(g_configVideoGliden64, "CountersPos");

#ifdef DEBUG_DUMP
	config.debug.dumpMode = ConfigGetParamInt(g_configVideoGliden64, "DebugDumpMode");
#endif

	if (config.generalEmulation.enableCustomSettings)
		Config_LoadCustomConfig();

	config.validate();
}
