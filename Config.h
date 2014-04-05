#ifndef CONFIG_H
#define CONFIG_H

#include "Types.h"

struct Config
{
	u32 version;

	struct
	{
		u32 fullscreenWidth, fullscreenHeight, windowedWidth, windowedHeight;
		u32 fullscreenBits, fullscreenRefresh;
	} video;

	struct
	{
		u32 maxAnisotropy;
		u32 textureBitDepth;
		u32 enableLOD;
		u32 forceBilinear;
		u32 enable2xSaI;
		u32 pow2;
	} texture;

	u32 frameBufferEmulation;
	u32 enableFog;
	u32 enableNoise;
	u32 enableHWLighting;
};

extern Config config;

void Config_LoadConfig();
void Config_DoConfig();
#endif

