#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "Types.h"

#define CONFIG_VERSION_ONE 1U
#define CONFIG_VERSION_TWO 2U
#define CONFIG_VERSION_THREE 3U
#define CONFIG_VERSION_CURRENT CONFIG_VERSION_THREE

#define BILINEAR_3POINT   0
#define BILINEAR_STANDARD 1

const u32 gc_uMegabyte = 1024U * 1024U;

struct Config
{
	u32 version;

	std::string translationFile;

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
		f32 maxAnisotropyF;
		u32 bilinearMode;
		u32 maxBytes;
		u32 screenShotFormat;
	} texture;

	struct {
		u32 enableFog;
		u32 enableNoise;
		u32 enableLOD;
		u32 enableHWLighting;
		u32 enableCustomSettings;
		u32 hacks;
	} generalEmulation;

	enum Aspect {
		aStretch = 0,
		a43 = 1,
		a169 = 2,
		aAdjust = 3,
		aTotal = 4
	};

	enum ValidityCheckMethod {
		vcFingerprint = 0,
		vcFill = 1,
		vcTotal = 2
	};

	struct {
		u32 enable;
		u32 copyToRDRAM;
		u32 copyDepthToRDRAM;
		u32 copyFromRDRAM;
		u32 detectCFB;
		u32 N64DepthCompare;
		u32 aspect; // 0: stretch ; 1: 4/3 ; 2: 16/9; 3: adjust
		u32 validityCheckMethod; // 0=write fingerprint to the buffer, 1=fill whole buffer in RDRAM with test value
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

		wchar_t txPath[PLUGIN_PATH_SIZE];
	} textureFilter;

	struct
	{
		std::string name;
		u32 size;
		u8 color[4];
		float colorf[4];
	} font;

	struct {
		u32 enable;
		u32 thresholdLevel;
		u32 blendMode;
		u32 blurAmount;
		u32 blurStrength;
	} bloomFilter;

	void resetToDefaults();
};

#define hack_Ogre64					(1<<0)  //Ogre Battle 64 background copy
#define hack_noDepthFrameBuffers	(1<<1)  //Do not use depth buffers as texture
#define hack_blurPauseScreen		(1<<2)  //Game copies frame buffer to depth buffer area, CPU blurs it. That image is used as background for pause screen.
#define hack_scoreboard				(1<<3)  //Copy data from RDRAM to auxilary frame buffer. Scoreboard in Mario Tennis.
#define hack_scoreboardJ			(1<<4)  //Copy data from RDRAM to auxilary frame buffer. Scoreboard in Mario Tennis (J).
#define hack_pilotWings				(1<<5)  //Special blend mode for PilotWings.
#define hack_subscreen				(1<<6)  //Fix subscreen delay in Zelda OOT
#define hack_legoRacers				(1<<7)  //LEGO racers course map
#define hack_blastCorps				(1<<8)  //Blast Corps black polygons

extern Config config;

void Config_LoadConfig();
#ifndef MUPENPLUSAPI
void Config_DoConfig(/*HWND hParent*/);
#endif

#endif // CONFIG_H
