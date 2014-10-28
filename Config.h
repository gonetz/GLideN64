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
