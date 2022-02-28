#include <stdarg.h>
#include <osal_files.h>
#include "GLideNHQ/TxFilterExport.h"
#include <Graphics/Context.h>
#include <Graphics/Parameters.h>

#include "RSP.h"
#include "Config.h"
#include "PluginAPI.h"
#include "FrameBuffer.h"
#include "TextureFilterHandler.h"
#include "DisplayWindow.h"
#include "DisplayLoadProgress.h"
#include "wst.h"

static
u32 textureFilters[] = {
	NO_FILTER, //"None"
	SMOOTH_FILTER_1, //"Smooth filtering 1"
	SMOOTH_FILTER_2, //"Smooth filtering 2"
	SMOOTH_FILTER_3, //"Smooth filtering 3"
	SMOOTH_FILTER_4, //"Smooth filtering 4"
	SHARP_FILTER_1,  //"Sharp filtering 1"
	SHARP_FILTER_2,  //"Sharp filtering 2"
};

static
u32 textureEnhancements[] = {
	NO_ENHANCEMENT,    //"None"
	NO_ENHANCEMENT,    //"Store"
	X2_ENHANCEMENT,    //"X2"
	X2SAI_ENHANCEMENT, //"X2SAI"
	HQ2X_ENHANCEMENT,  //"HQ2X"
	HQ2XS_ENHANCEMENT, //"HQ2XS"
	LQ2X_ENHANCEMENT,  //"LQ2X"
	LQ2XS_ENHANCEMENT, //"LQ2XS"
	HQ4X_ENHANCEMENT,  //"HQ4X"
	BRZ2X_ENHANCEMENT, //"2XBRZ"
	BRZ3X_ENHANCEMENT, //"3XBRZ"
	BRZ4X_ENHANCEMENT, //"4XBRZ"
	BRZ5X_ENHANCEMENT, //"5XBRZ"
	BRZ6X_ENHANCEMENT  //"6XBRZ"
};

u32 TextureFilterHandler::_getConfigOptions() const
{
	u32 options = textureFilters[config.textureFilter.txFilterMode] | textureEnhancements[config.textureFilter.txEnhancementMode];
	if (config.textureFilter.txHiresEnable)
		options |= RICE_HIRESTEXTURES;
	if (config.textureFilter.txForce16bpp)
		options |= FORCE16BPP_TEX | FORCE16BPP_HIRESTEX;
	if (config.textureFilter.txCacheCompression)
		options |= GZ_TEXCACHE | GZ_HIRESTEXCACHE;
	if (config.textureFilter.txSaveCache)
		options |= (DUMP_TEXCACHE | DUMP_HIRESTEXCACHE);
	if (config.textureFilter.txHiresFullAlphaChannel)
		options |= LET_TEXARTISTS_FLY;
	if (config.hotkeys.enabledKeys[Config::HotKey::hkTexDump] != 0)
		options |= DUMP_TEX;
	if (config.textureFilter.txDeposterize)
		options |= DEPOSTERIZE;
	if (config.textureFilter.txEnhancedTextureFileStorage)
		options |= FILE_TEXCACHE;
	if (config.textureFilter.txHiresTextureFileStorage)
		options |= FILE_HIRESTEXCACHE;
	if (config.textureFilter.txNoTextureFileStorage)
		options |= FILE_NOTEXCACHE;
	return options;
}

void TextureFilterHandler::init()
{
	if (isInited())
		return;

	m_inited = config.textureFilter.txFilterMode | config.textureFilter.txEnhancementMode | config.textureFilter.txHiresEnable;
	if (m_inited == 0)
		return;

	m_options = _getConfigOptions();

	s32 maxTextureSize = gfxContext.getMaxTextureSize();
	wchar_t wRomName[32];
	::mbstowcs(wRomName, RSP.romname, 32);

	wchar_t txPath[PLUGIN_PATH_SIZE + 16];
	wchar_t * pTexPackPath = config.textureFilter.txPath;
	if (::wcslen(config.textureFilter.txPath) == 0 ||
		osal_is_absolute_path(config.textureFilter.txPath) == 0) {
		api().GetUserDataPath(txPath);
		gln_wcscat(txPath, wst("/hires_texture"));
		pTexPackPath = txPath;
	}

	wchar_t txCachePath[PLUGIN_PATH_SIZE + 16];
	wchar_t * pTexCachePath = config.textureFilter.txCachePath;
	if (::wcslen(config.textureFilter.txCachePath) == 0 ||
		osal_is_absolute_path(config.textureFilter.txCachePath) == 0) {
		api().GetUserCachePath(txCachePath);
		gln_wcscat(txCachePath, wst("/cache"));
		pTexCachePath = txCachePath;
	}

	wchar_t txDumpPath[PLUGIN_PATH_SIZE + 16];
	wchar_t * pTexDumpPath = config.textureFilter.txDumpPath;
	if (::wcslen(config.textureFilter.txDumpPath) == 0 ||
		osal_is_absolute_path(config.textureFilter.txDumpPath) == 0) {
		api().GetUserCachePath(txDumpPath);
		gln_wcscat(txDumpPath, wst("/texture_dump"));
		pTexDumpPath = txDumpPath;
	}

	m_inited = txfilter_init(maxTextureSize, // max texture width supported by hardware
		maxTextureSize, // max texture height supported by hardware
		32, // max texture bpp supported by hardware
		m_options,
		config.textureFilter.txCacheSize, // cache texture to system memory
		pTexCachePath, // path to store cache files
		pTexDumpPath, // path to folder with dumped textures
		pTexPackPath, // path to texture packs folder
		wRomName, // name of ROM. must be no longer than 256 characters
		displayLoadProgress);

}

void TextureFilterHandler::shutdown()
{
	if (isInited()) {
		txfilter_shutdown();
		m_inited = m_options = 0;
	}
}

void TextureFilterHandler::dumpcache()
{
	if (isInited())
		txfilter_dumpcache();
}

TextureFilterHandler TFH;
