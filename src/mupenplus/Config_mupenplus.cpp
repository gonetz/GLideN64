#include "GLideN64_mupenplus.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../Config.h"
#include "../GLideN64.h"
#include "../OpenGL.h"
#include "../GBI.h"
#include "../RSP.h"
#include "../Log.h"

Config config;

static
const u32 uMegabyte = 1024U*1024U;

static m64p_handle g_configVideoGeneral = nullptr;
static m64p_handle g_configVideoGliden64 = nullptr;

static
bool Config_SetDefault()
{
	if (ConfigOpenSection("Video-General", &g_configVideoGeneral) != M64ERR_SUCCESS) {
		LOG(LOG_ERROR, "Unable to open Video-General configuration section");
		return false;
	}
	if (ConfigOpenSection("Video-GLideN64", &g_configVideoGliden64) != M64ERR_SUCCESS) {
		LOG(LOG_ERROR, "Unable to open GLideN64 configuration section");
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

	res = ConfigSetDefaultInt(g_configVideoGliden64, "MultiSampling", config.video.multisampling, "Enable/Disable MultiSampling (0=off, 2,4,8,16=quality)");
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
	res = ConfigSetDefaultBool(g_configVideoGliden64, "MaxAnisotropy", config.texture.maxAnisotropy, "Max level of Anisotropic Filtering, 0 for off");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "CacheSize", config.texture.maxBytes / uMegabyte, "Size of texture cache in megabytes. Good value is VRAM*3/4");
	assert(res == M64ERR_SUCCESS);
	//#Emulation Settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableNoise", config.generalEmulation.enableNoise, "Enable color noise emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableLOD", config.generalEmulation.enableLOD, "Enable LOD emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableHWLighting", config.generalEmulation.enableHWLighting, "Enable hardware per-pixel lighting.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableShadersStorage", config.generalEmulation.enableShadersStorage, "Use persistent storage for compiled shaders.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "CorrectTexrectCoords", config.generalEmulation.correctTexrectCoords, "Make texrect coordinates continuous to avoid black lines between them. (0=Off, 1=Auto, 2=Force)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableNativeResTexrects", config.generalEmulation.enableNativeResTexrects, "Render 2D texrects in native resolution to fix misalignment between parts of 2D image.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableLegacyBlending", config.generalEmulation.enableLegacyBlending, "Do not use shaders to emulate N64 blending modes. Works faster on slow GPU. Can cause glitches.");
	assert(res == M64ERR_SUCCESS);
#ifndef GLES2
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableFragmentDepthWrite", config.generalEmulation.enableFragmentDepthWrite, "Enable writing of fragment depth. Some mobile GPUs do not support it, thus it made optional. Leave enabled.");
	assert(res == M64ERR_SUCCESS);
#endif
#ifdef ANDROID
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ForcePolygonOffset", config.generalEmulation.forcePolygonOffset, "If true, use polygon offset values specified below");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "PolygonOffsetFactor", config.generalEmulation.polygonOffsetFactor, "Specifies a scale factor that is used to create a variable depth offset for each polygon");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "PolygonOffsetUnits", config.generalEmulation.polygonOffsetUnits, "Is multiplied by an implementation-specific value to create a constant depth offset");
	assert(res == M64ERR_SUCCESS);
#endif
	//#Frame Buffer Settings:"
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableFBEmulation", config.frameBufferEmulation.enable, "Enable frame and|or depth buffer emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyAuxiliaryToRDRAM", config.frameBufferEmulation.copyAuxToRDRAM, "Copy auxiliary buffers to RDRAM");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableN64DepthCompare", config.frameBufferEmulation.N64DepthCompare, "Enable N64 depth compare instead of OpenGL standard one. Experimental.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "DisableFBInfo", config.frameBufferEmulation.fbInfoDisabled, "Disable buffers read/write with FBInfo. Use for games, which do not work with FBInfo.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "FBInfoReadColorChunk", config.frameBufferEmulation.fbInfoReadColorChunk, "Read color buffer by 4kb chunks (strict follow to FBRead specification)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "FBInfoReadDepthChunk", config.frameBufferEmulation.fbInfoReadDepthChunk, "Read depth buffer by 4kb chunks (strict follow to FBRead specification)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableCopyColorToRDRAM", config.frameBufferEmulation.copyToRDRAM, "Enable color buffer copy to RDRAM (0=do not copy, 1=copy in sync mode, 2=copy in async mode)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableCopyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM, "Enable depth buffer copy to RDRAM  (0=do not copy, 1=copy from video memory, 2=use software render)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyColorFromRDRAM", config.frameBufferEmulation.copyFromRDRAM, "Enable color buffer copy from RDRAM.");
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
	res = ConfigSetDefaultInt(g_configVideoGliden64, "txCacheSize", config.textureFilter.txCacheSize/uMegabyte, "Size of filtered textures cache in megabytes.");
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
	// Convert to multibyte
	char txPath[PLUGIN_PATH_SIZE * 2];
	wcstombs(txPath, config.textureFilter.txPath, PLUGIN_PATH_SIZE * 2);
	res = ConfigSetDefaultString(g_configVideoGliden64, "txPath", txPath, "Path to folder with hi-res texture packs.");
	assert(res == M64ERR_SUCCESS);

	res = ConfigSetDefaultString(g_configVideoGliden64, "fontName", config.font.name.c_str(), "File name of True Type Font for text messages.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "fontSize", config.font.size, "Font size.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultString(g_configVideoGliden64, "fontColor", "B5E61D", "Font color in RGB format.");
	assert(res == M64ERR_SUCCESS);

	//#Bloom filter settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableBloom", config.bloomFilter.enable, "Enable bloom filter");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "bloomThresholdLevel", config.bloomFilter.thresholdLevel, "Brightness threshold level for bloom. Values [2, 6]");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "bloomBlendMode", config.bloomFilter.blendMode, "Bloom blend mode (0=Strong, 1=Mild, 2=Light)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "blurAmount", config.bloomFilter.blurAmount, "Blur radius. Values [2, 10]");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "blurStrength", config.bloomFilter.blurStrength, "Blur strength. Values [10, 100]");
	assert(res == M64ERR_SUCCESS);

	//#Gamma correction settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ForceGammaCorrection", config.gammaCorrection.force, "Force gamma correction.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "GammaCorrectionLevel", config.gammaCorrection.level, "Gamma correction level.");
	assert(res == M64ERR_SUCCESS);

	return ConfigSaveSection("Video-GLideN64") == M64ERR_SUCCESS;
}

void Config_LoadConfig()
{
	const u32 hacks = config.generalEmulation.hacks;

	if (!Config_SetDefault()) {
		config.generalEmulation.hacks = hacks;
		return;
	}

	config.version = ConfigGetParamInt(g_configVideoGliden64, "configVersion");
	if (config.version != CONFIG_VERSION_CURRENT) {
		m64p_error res = ConfigDeleteSection("Video-GLideN64");
		assert(res == M64ERR_SUCCESS);
		ConfigSaveFile();
		if (!Config_SetDefault()) {
			config.generalEmulation.hacks = hacks;
			return;
		}
	}

	config.video.fullscreen = ConfigGetParamBool(g_configVideoGeneral, "Fullscreen");
	config.video.windowedWidth = ConfigGetParamInt(g_configVideoGeneral, "ScreenWidth");
	config.video.windowedHeight = ConfigGetParamInt(g_configVideoGeneral, "ScreenHeight");
	config.video.verticalSync = ConfigGetParamBool(g_configVideoGeneral, "VerticalSync");

#ifdef GL_MULTISAMPLING_SUPPORT
	config.video.multisampling = ConfigGetParamInt(g_configVideoGliden64, "MultiSampling");
#else
	config.video.multisampling = 0;
#endif
	config.frameBufferEmulation.aspect = ConfigGetParamInt(g_configVideoGliden64, "AspectRatio");
	config.frameBufferEmulation.bufferSwapMode = ConfigGetParamInt(g_configVideoGliden64, "BufferSwapMode");
	config.frameBufferEmulation.nativeResFactor = ConfigGetParamInt(g_configVideoGliden64, "UseNativeResolutionFactor");

	//#Texture Settings
	config.texture.bilinearMode = ConfigGetParamBool(g_configVideoGliden64, "bilinearMode");
	config.texture.maxAnisotropy = ConfigGetParamInt(g_configVideoGliden64, "MaxAnisotropy");
	config.texture.maxBytes = ConfigGetParamInt(g_configVideoGliden64, "CacheSize") * uMegabyte;
	//#Emulation Settings
	config.generalEmulation.enableNoise = ConfigGetParamBool(g_configVideoGliden64, "EnableNoise");
	config.generalEmulation.enableLOD = ConfigGetParamBool(g_configVideoGliden64, "EnableLOD");
	config.generalEmulation.enableHWLighting = ConfigGetParamBool(g_configVideoGliden64, "EnableHWLighting");
	config.generalEmulation.enableShadersStorage = ConfigGetParamBool(g_configVideoGliden64, "EnableShadersStorage");
	config.generalEmulation.correctTexrectCoords = ConfigGetParamInt(g_configVideoGliden64, "CorrectTexrectCoords");
	config.generalEmulation.enableNativeResTexrects = ConfigGetParamBool(g_configVideoGliden64, "EnableNativeResTexrects");
	config.generalEmulation.enableLegacyBlending = ConfigGetParamBool(g_configVideoGliden64, "EnableLegacyBlending");
#ifndef GLES2
	config.generalEmulation.enableFragmentDepthWrite = ConfigGetParamBool(g_configVideoGliden64, "EnableFragmentDepthWrite");
#endif
#ifdef ANDROID
	config.generalEmulation.forcePolygonOffset = ConfigGetParamBool(g_configVideoGliden64, "ForcePolygonOffset");
	config.generalEmulation.polygonOffsetFactor = ConfigGetParamFloat(g_configVideoGliden64, "PolygonOffsetFactor");
	config.generalEmulation.polygonOffsetUnits = ConfigGetParamFloat(g_configVideoGliden64, "PolygonOffsetUnits");
#endif
	//#Frame Buffer Settings:"
	config.frameBufferEmulation.enable = ConfigGetParamBool(g_configVideoGliden64, "EnableFBEmulation");
	config.frameBufferEmulation.copyAuxToRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyAuxiliaryToRDRAM");
	config.frameBufferEmulation.copyToRDRAM = ConfigGetParamInt(g_configVideoGliden64, "EnableCopyColorToRDRAM");
	config.frameBufferEmulation.copyDepthToRDRAM = ConfigGetParamInt(g_configVideoGliden64, "EnableCopyDepthToRDRAM");
	config.frameBufferEmulation.copyFromRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyColorFromRDRAM");
	config.frameBufferEmulation.N64DepthCompare = ConfigGetParamBool(g_configVideoGliden64, "EnableN64DepthCompare");
	config.frameBufferEmulation.fbInfoDisabled = ConfigGetParamBool(g_configVideoGliden64, "DisableFBInfo");
	config.frameBufferEmulation.fbInfoReadColorChunk = ConfigGetParamBool(g_configVideoGliden64, "FBInfoReadColorChunk");
	config.frameBufferEmulation.fbInfoReadDepthChunk = ConfigGetParamBool(g_configVideoGliden64, "FBInfoReadDepthChunk");
	//#Texture filter settings
	config.textureFilter.txFilterMode = ConfigGetParamInt(g_configVideoGliden64, "txFilterMode");
	config.textureFilter.txEnhancementMode = ConfigGetParamInt(g_configVideoGliden64, "txEnhancementMode");
	config.textureFilter.txDeposterize = ConfigGetParamInt(g_configVideoGliden64, "txDeposterize");
	config.textureFilter.txFilterIgnoreBG = ConfigGetParamBool(g_configVideoGliden64, "txFilterIgnoreBG");
	config.textureFilter.txCacheSize = ConfigGetParamInt(g_configVideoGliden64, "txCacheSize") * uMegabyte;
	config.textureFilter.txHiresEnable = ConfigGetParamBool(g_configVideoGliden64, "txHiresEnable");
	config.textureFilter.txHiresFullAlphaChannel = ConfigGetParamBool(g_configVideoGliden64, "txHiresFullAlphaChannel");
	config.textureFilter.txHresAltCRC = ConfigGetParamBool(g_configVideoGliden64, "txHresAltCRC");
	config.textureFilter.txDump = ConfigGetParamBool(g_configVideoGliden64, "txDump");
	config.textureFilter.txForce16bpp = ConfigGetParamBool(g_configVideoGliden64, "txForce16bpp");
	config.textureFilter.txCacheCompression = ConfigGetParamBool(g_configVideoGliden64, "txCacheCompression");
	config.textureFilter.txSaveCache = ConfigGetParamBool(g_configVideoGliden64, "txSaveCache");
	::mbstowcs(config.textureFilter.txPath, ConfigGetParamString(g_configVideoGliden64, "txPath"), PLUGIN_PATH_SIZE);

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
	//#Bloom filter settings
	config.bloomFilter.enable = ConfigGetParamBool(g_configVideoGliden64, "EnableBloom");
	config.bloomFilter.thresholdLevel = ConfigGetParamInt(g_configVideoGliden64, "bloomThresholdLevel");
	config.bloomFilter.blendMode = ConfigGetParamInt(g_configVideoGliden64, "bloomBlendMode");
	config.bloomFilter.blurAmount = ConfigGetParamInt(g_configVideoGliden64, "blurAmount");
	config.bloomFilter.blurStrength = ConfigGetParamInt(g_configVideoGliden64, "blurStrength");

	//#Gamma correction settings
	config.gammaCorrection.force = ConfigGetParamBool(g_configVideoGliden64, "ForceGammaCorrection");
	config.gammaCorrection.level = ConfigGetParamFloat(g_configVideoGliden64, "GammaCorrectionLevel");

	config.generalEmulation.hacks = hacks;
}
