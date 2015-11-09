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

static m64p_handle g_configVideoGeneral = NULL;
static m64p_handle g_configVideoGliden64 = NULL;

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

	//#Texture Settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "bilinearMode", config.texture.bilinearMode, "Bilinear filtering mode (0=N64 3point, 1=standard)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "MaxAnisotropy", config.texture.maxAnisotropy, "Max level of Anisotropic Filtering, 0 for off");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "CacheSize", config.texture.maxBytes / uMegabyte, "Size of texture cache in megabytes. Good value is VRAM*3/4");
	assert(res == M64ERR_SUCCESS);
	//#Emulation Settings
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableFog", config.generalEmulation.enableFog, "Enable fog emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableNoise", config.generalEmulation.enableNoise, "Enable color noise emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableLOD", config.generalEmulation.enableLOD, "Enable LOD emulation.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableHWLighting", config.generalEmulation.enableHWLighting, "Enable hardware per-pixel lighting.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableShadersStorage", config.generalEmulation.enableShadersStorage, "Use persistent storage for compiled shaders.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "ForceGammaCorrection", config.generalEmulation.forceGammaCorrection, "Force gamma correction.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultFloat(g_configVideoGliden64, "GammaCorrectionLevel", config.generalEmulation.gammaCorrectionLevel, "Gamma correction level.");
	assert(res == M64ERR_SUCCESS);
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
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableCopyColorToRDRAM", config.frameBufferEmulation.copyToRDRAM, "Enable color buffer copy to RDRAM (0=do not copy, 1=copy in sync mode, 2=copy in async mode)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyDepthToRDRAM", config.frameBufferEmulation.copyDepthToRDRAM, "Enable depth buffer copy to RDRAM.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableCopyColorFromRDRAM", config.frameBufferEmulation.copyFromRDRAM, "Enable color buffer copy from RDRAM.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableDetectCFB", config.frameBufferEmulation.detectCFB, "Detect CPU writes to frame buffer.");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultBool(g_configVideoGliden64, "EnableN64DepthCompare", config.frameBufferEmulation.N64DepthCompare, "Enable N64 depth compare instead of OpenGL standard one. Experimental.");
	assert(res == M64ERR_SUCCESS);
	//#Texture filter settings
	res = ConfigSetDefaultInt(g_configVideoGliden64, "txFilterMode", config.textureFilter.txFilterMode, "Texture filter (0=none, 1=Smooth filtering 1, 2=Smooth filtering 2, 3=Smooth filtering 3, 4=Smooth filtering 4, 5=Sharp filtering 1, 6=Sharp filtering 2)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "txEnhancementMode", config.textureFilter.txEnhancementMode, "Texture Enhancement (0=none, 1=store as is, 2=X2, 3=X2SAI, 4=HQ2X, 5=HQ2XS, 6=LQ2X, 7=LQ2XS, 8=HQ4X, 9=2xBRZ, 10=3xBRZ, 11=4xBRZ, 12=5xBRZ), 13=6xBRZ");
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
	res = ConfigSetDefaultInt(g_configVideoGliden64, "EnableBloom", config.bloomFilter.enable, "Enable bloom filter");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "bloomThresholdLevel", config.bloomFilter.thresholdLevel, "Brightness threshold level for bloom. Values [2, 6]");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "bloomBlendMode", config.bloomFilter.blendMode, "Bloom blend mode (0=Strong, 1=Mild, 2=Light)");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "blurAmount", config.bloomFilter.blurAmount, "Blur radius. Values [2, 10]");
	assert(res == M64ERR_SUCCESS);
	res = ConfigSetDefaultInt(g_configVideoGliden64, "blurStrength", config.bloomFilter.blurStrength, "Blur strength. Values [10, 100]");
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

	//#Texture Settings
	config.texture.bilinearMode = ConfigGetParamBool(g_configVideoGliden64, "bilinearMode");
	config.texture.maxAnisotropy = ConfigGetParamInt(g_configVideoGliden64, "MaxAnisotropy");
	config.texture.maxBytes = ConfigGetParamInt(g_configVideoGliden64, "CacheSize") * uMegabyte;
	//#Emulation Settings
	config.generalEmulation.enableFog = ConfigGetParamBool(g_configVideoGliden64, "EnableFog");
	config.generalEmulation.enableNoise = ConfigGetParamBool(g_configVideoGliden64, "EnableNoise");
	config.generalEmulation.enableLOD = ConfigGetParamBool(g_configVideoGliden64, "EnableLOD");
	config.generalEmulation.enableHWLighting = ConfigGetParamBool(g_configVideoGliden64, "EnableHWLighting");
	config.generalEmulation.enableShadersStorage = ConfigGetParamBool(g_configVideoGliden64, "EnableShadersStorage");
	config.generalEmulation.forceGammaCorrection = ConfigGetParamBool(g_configVideoGliden64, "ForceGammaCorrection");
	config.generalEmulation.gammaCorrectionLevel = ConfigGetParamFloat(g_configVideoGliden64, "GammaCorrectionLevel");
#ifdef ANDROID
	config.generalEmulation.forcePolygonOffset = ConfigGetParamBool(g_configVideoGliden64, "ForcePolygonOffset");
	config.generalEmulation.polygonOffsetFactor = ConfigGetParamFloat(g_configVideoGliden64, "PolygonOffsetFactor");
	config.generalEmulation.polygonOffsetUnits = ConfigGetParamFloat(g_configVideoGliden64, "PolygonOffsetUnits");
#endif
	//#Frame Buffer Settings:"
	config.frameBufferEmulation.enable = ConfigGetParamBool(g_configVideoGliden64, "EnableFBEmulation");
	config.frameBufferEmulation.copyAuxToRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyAuxiliaryToRDRAM");
	config.frameBufferEmulation.copyToRDRAM = ConfigGetParamInt(g_configVideoGliden64, "EnableCopyColorToRDRAM");
	config.frameBufferEmulation.copyDepthToRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyDepthToRDRAM");
	config.frameBufferEmulation.copyFromRDRAM = ConfigGetParamBool(g_configVideoGliden64, "EnableCopyColorFromRDRAM");
	config.frameBufferEmulation.detectCFB = ConfigGetParamBool(g_configVideoGliden64, "EnableDetectCFB");
	config.frameBufferEmulation.N64DepthCompare = ConfigGetParamBool(g_configVideoGliden64, "EnableN64DepthCompare");
	//#Texture filter settings
	config.textureFilter.txFilterMode = ConfigGetParamInt(g_configVideoGliden64, "txFilterMode");
	config.textureFilter.txEnhancementMode = ConfigGetParamInt(g_configVideoGliden64, "txEnhancementMode");
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
	long int uColor = strtol(buf, NULL, 16);
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
	config.bloomFilter.enable = ConfigGetParamInt(g_configVideoGliden64, "EnableBloom");
	config.bloomFilter.thresholdLevel = ConfigGetParamInt(g_configVideoGliden64, "bloomThresholdLevel");
	config.bloomFilter.blendMode = ConfigGetParamInt(g_configVideoGliden64, "bloomBlendMode");
	config.bloomFilter.blurAmount = ConfigGetParamInt(g_configVideoGliden64, "blurAmount");
	config.bloomFilter.blurStrength = ConfigGetParamInt(g_configVideoGliden64, "blurStrength");

	config.generalEmulation.hacks = hacks;
}
