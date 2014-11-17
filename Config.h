#ifndef CONFIG_H
#define CONFIG_H

#include "Types.h"

struct Config
{
	u32 version;

	struct
	{
		u32 fullscreen;
		u32 fullscreenWidth, fullscreenHeight, windowedWidth, windowedHeight;
		u32 fullscreenBits, fullscreenRefresh;
		u32 multisampling, verticalSync;
		u32 aspect; // 0: stretch ; 1: 4/3 ; 2: 16/9
	} video;

	struct
	{
		u32 maxAnisotropy;
		u32 textureBitDepth;
		u32 forceBilinear;
		u32 pow2;
		u32 maxBytes;
	} texture;

	struct
	{
		u32 txFilterMode;				// Texture filtering mode, eg Sharpen
		u32 txEnhancementMode;			// Texture enhancement mode, eg 2xSAI
		u32 txFilterCompression;		// Apply texture compression to filtered textures
		//u32 txTileTextures;			// Split enchanced textures onto tiles with 256 width. Not used.
		u32 txFilterForce16bpp;			// Force use 16bit color textures
		u32 txFilterIgnoreBG;			// Do not apply filtering to backgrounds textures
		u32 txCacheSize;				// Cache size in Mbytes
		u32 txFilterCacheCompression;	// Zip cache of filtered textures
		u32 txSaveCache;				// Save texture cache to hard disk

		u32 txHiresEnable;				// Use high-resolution texture packs
		u32 txHiresCompression;			// Apply texture compression to high-res textures
		//u32 txTileHiresTextures;		// Split hires textures onto tiles with 256 width. Not used.
		u32 txHiresForce16bpp;			// Force use 16bit color textures
		u32 txHiresFullAlphaChannel;	// Use alpha channel fully
		u32 txHresAltCRC;				// Use alternative method of paletted textures CRC calculation
		u32 txHiresCacheCompression;	// Zip cache of hires textures
		u32 txDump;						// Dump textures
	} textureFilter;

	struct {
		u32 enable;
		u32 copyToRDRAM;
		u32 copyDepthToRDRAM;
		u32 copyFromRDRAM;
		u32 ignoreCFB;
		u32 N64DepthCompare;
	} frameBufferEmulation;

	u32 enableFog;
	u32 enableNoise;
	u32 enableLOD;
	u32 enableHWLighting;

	u32 hacks;
};

#define hack_Ogre64	(1<<0)  //Ogre Battle 64

extern Config config;

void Config_LoadConfig();
#ifndef MUPENPLUSAPI
void Config_DoConfig(HWND hParent);
#endif

#endif // CONFIG_H
