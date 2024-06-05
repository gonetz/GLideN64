#ifndef CONFIG_DECL_H
#define CONFIG_DECL_H

#include <string>
#include "Types.h"

#define CONFIG_WITH_PROFILES 23U
#define CONFIG_VERSION_CURRENT 26U

#define BILINEAR_3POINT   0
#define BILINEAR_STANDARD 1
#define BILINEAR_3POINT_WITH_COLOR_BLEEDING 2
#define BILINEAR_STANDARD_WITH_COLOR_BLEEDING_AND_PREMULTIPLIED_ALPHA 3

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
		u32 fxaa;
		u32 multisampling;
		u32 verticalSync;
	} video;

	struct
	{
		u32 maxAnisotropy;
		f32 maxAnisotropyF;
		u32 bilinearMode;
		u32 enableHalosRemoval;
		u32 screenShotFormat;
	} texture;

	enum TexrectCorrectionMode {
		tcDisable = 0,
		tcSmart,
		tcForce
	};

	struct {
		u32 enableNoise;
		u32 enableLOD;
		u32 enableHWLighting;
		u32 enableCustomSettings;
		u32 enableShadersStorage;
		u32 enableLegacyBlending;
		u32 enableFragmentDepthWrite;
		u32 enableBlitScreenWorkaround;
		u32 hacks;
#if defined(OS_ANDROID) || defined(OS_IOS)
		u32 forcePolygonOffset;
		f32 polygonOffsetFactor;
		f32 polygonOffsetUnits;
#endif
	} generalEmulation;

	enum BGMode {
		bgOnePiece = 0,
		bgStripped = 1
	};

	struct {
		u32 correctTexrectCoords;
		u32 enableNativeResTexrects;
		u32 bgMode;
	} graphics2D;

	enum Aspect {
		aStretch = 0,
		a43 = 1,
		a169 = 2,
		aAdjust = 3,
		aTotal = 4
	};

	enum CopyToRDRAM {
		ctDisable = 0,
		ctSync = 1,
		ctDoubleBuffer = 2,
		ctTripleBuffer = 3
	};

	enum BufferSwapMode {
		bsOnVerticalInterrupt = 0,
		bsOnVIOriginChange,
		bsOnColorImageChange
	};

	enum CopyDepthMode {
		cdDisable = 0,
		cdCopyFromVRam = 1,
		cdSoftwareRender = 2
	};

	struct {
		u32 enable;
		u32 aspect; // 0: stretch ; 1: 4/3 ; 2: 16/9; 3: adjust
		u32 bufferSwapMode; // 0: on VI update call; 1: on VI origin change; 2: on main frame buffer update
		u32 nativeResFactor;
		u32 N64DepthCompare;
		u32 forceDepthBufferClear;
		u32 copyAuxToRDRAM;
		// Buffer read/write
		u32 copyToRDRAM;
		u32 copyDepthToRDRAM;
		u32 copyFromRDRAM;

		// FBInfo
		u32 fbInfoSupported;
		u32 fbInfoDisabled;
		u32 fbInfoReadColorChunk;
		u32 fbInfoReadDepthChunk;

		// Overscan
		u32 enableOverscan;
		struct {
			s32 left = 0;
			s32 right = 0;
			s32 top = 0;
			s32 bottom = 0;
		} overscanPAL, overscanNTSC;
	} frameBufferEmulation;

	struct
	{
		u32 txFilterMode;				// Texture filtering mode, eg Sharpen
		u32 txEnhancementMode;			// Texture enhancement mode, eg 2xSAI
		u32 txDeposterize;				// Deposterize texture before enhancement
		u32 txFilterIgnoreBG;			// Do not apply filtering to backgrounds textures
		u32 txCacheSize;				// Cache size in Mbytes

		u32 txHiresEnable;				// Use high-resolution texture packs
		u32 txHiresFullAlphaChannel;	// Use alpha channel fully
		u32 txHresAltCRC;				// Use alternative method of paletted textures CRC calculation
		u32 txDump;						// Dump textures

		u32 txForce16bpp;				// Force use 16bit color textures
		u32 txCacheCompression;			// Zip textures cache
		u32 txSaveCache;				// Save texture cache to hard disk

		wchar_t txPath[PLUGIN_PATH_SIZE]; // Path to texture packs
		wchar_t txCachePath[PLUGIN_PATH_SIZE]; // Path to store texture cache, that is .htc files
		wchar_t txDumpPath[PLUGIN_PATH_SIZE]; // Path to store texture dumps
	} textureFilter;

	struct
	{
		std::string name;
		u32 size;
		u8 color[4];
		float colorf[4];
	} font;

	struct {
		u32 force;
		f32 level;
	} gammaCorrection;

	enum CountersPosition {
		posTopLeft = 1,
		posTopCenter = 2,
		posTopRight = 4,
		posTop = posTopLeft | posTopCenter | posTopRight,
		posBottomLeft = 8,
		posBottomCenter = 16,
		posBottomRight = 32,
		posBottom = posBottomLeft | posBottomCenter | posBottomRight
	};

	struct {
		u32 vis;
		u32 fps;
		u32 percent;
		u32 internalResolution;
		u32 renderingResolution;
		u32 pos;
	} onScreenDisplay;

	struct {
		u32 dumpMode;
	} debug;

	enum AngleRenderer {
		arDirectX11 = 0,
		arVulkan = 1,
		arOpenGL = 2,
	};

	struct {
		u32 renderer;
		u32 directComposition;
		u32 shadowDelimiter;
	} angle;

	void resetToDefaults();
	void validate();
};

#endif // CONFIG_DECL_H
