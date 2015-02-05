#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "Types.h"

#define CONFIG_VERSION_ONE 1U
#define CONFIG_VERSION_CURRENT CONFIG_VERSION_ONE

const u32 gc_uMegabyte = 1024U * 1024U;

struct Config
{
	u32 version;

	struct
	{
		u32 fullscreen;
		u32 windowedWidth, windowedHeight;
		u32 fullscreenWidth, fullscreenHeight, fullscreenRefresh;
		u32 multisampling;
		u32 verticalSync;
	} video;

	struct
	{
		u32 maxAnisotropy;
		u32 forceBilinear;
		u32 maxBytes;
		u32 screenShotFormat;
	} texture;

	struct {
		u32 enableFog;
		u32 enableNoise;
		u32 enableLOD;
		u32 enableHWLighting;
		u32 hacks;
	} generalEmulation;

	struct {
		u32 enable;
		u32 copyToRDRAM;
		u32 copyDepthToRDRAM;
		u32 copyFromRDRAM;
		u32 ignoreCFB;
		u32 N64DepthCompare;
		u32 aspect; // 0: stretch ; 1: 4/3 ; 2: 16/9
	} frameBufferEmulation;

	struct
	{
		u32 txFilterMode;				// Texture filtering mode, eg Sharpen
		u32 txEnhancementMode;			// Texture enhancement mode, eg 2xSAI
		u32 txFilterIgnoreBG;			// Do not apply filtering to backgrounds textures
		u32 txCacheSize;				// Cache size in Mbytes

		u32 txHiresEnable;				// Use high-resolution texture packs
		u32 txHiresFullAlphaChannel;	// Use alpha channel fully
		u32 txHresAltCRC;				// Use alternative method of paletted textures CRC calculation
		u32 txDump;						// Dump textures

		u32 txForce16bpp;				// Force use 16bit color textures
		u32 txCacheCompression;			// Zip textures cache
		u32 txSaveCache;				// Save texture cache to hard disk
	} textureFilter;

	struct
	{
		std::string name;
		u32 size;
		u8 color[4];
		float colorf[4];
	} font;

	struct {
		u32 mode;
		u32 thresholdLevel;
		u32 blendMode;
		u32 blurAmount;
		u32 blurStrength;
	} bloomFilter;

	void resetToDefaults()
	{
		version = CONFIG_VERSION_CURRENT;

		video.fullscreen = 0;
		video.fullscreenWidth = video.windowedWidth = 640;
		video.fullscreenHeight = video.windowedHeight = 480;
		video.fullscreenRefresh = 60;
		video.multisampling = 0;
		video.verticalSync = 0;

		texture.maxAnisotropy = 0;
		texture.forceBilinear = 0;
		texture.maxBytes = 500 * gc_uMegabyte;
		texture.screenShotFormat = 0;

		generalEmulation.enableFog = 1;
		generalEmulation.enableLOD = 1;
		generalEmulation.enableNoise = 1;
		generalEmulation.enableHWLighting = 0;
		generalEmulation.hacks = 0;

		frameBufferEmulation.enable = 1;
		frameBufferEmulation.copyDepthToRDRAM = 1;
		frameBufferEmulation.copyFromRDRAM = 0;
		frameBufferEmulation.copyToRDRAM = 0;
		frameBufferEmulation.ignoreCFB = 0;
		frameBufferEmulation.N64DepthCompare = 0;

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
			font.colorf[i] = font.color[i] /255.0f;

		bloomFilter.mode = 0;
		bloomFilter.thresholdLevel = 4;
		bloomFilter.blendMode = 0;
		bloomFilter.blurAmount = 10;
		bloomFilter.blurStrength = 20;
	}
};

#define hack_Ogre64	(1<<0)  //Ogre Battle 64

extern Config config;

void Config_LoadConfig();
#ifndef MUPENPLUSAPI
void Config_DoConfig(HWND hParent);
#endif

#endif // CONFIG_H
