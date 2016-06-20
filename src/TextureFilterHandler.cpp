#include <stdarg.h>
#include "GLideNHQ/Ext_TxFilter.h"

#include "RSP.h"
#include "OpenGL.h"
#include "Config.h"
#include "PluginAPI.h"
#include "FrameBuffer.h"
#include "TextureFilterHandler.h"
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

void displayLoadProgress(const wchar_t *format, ...)
{
	va_list args;
	wchar_t wbuf[INFO_BUF];
	char buf[INFO_BUF];

	// process input
#ifdef ANDROID
	const u32 bufSize = 2048;
	char cbuf[bufSize];
	char fmt[bufSize];
	wcstombs(fmt, format, bufSize);
	va_start(args, format);
	vsprintf(cbuf, fmt, args);
	va_end(args);
	mbstowcs(wbuf, cbuf, INFO_BUF);
#else
	va_start(args, format);
	vswprintf(wbuf, INFO_BUF, format, args);
	va_end(args);
#endif

	// XXX: convert to multibyte
	wcstombs(buf, wbuf, INFO_BUF);

	FrameBuffer* pBuffer = frameBufferList().getCurrent();
	if (pBuffer != nullptr)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	OGLRender & render = video().getRender();
	render.clearColorBuffer(nullptr);
	render.drawText(buf, -0.9f, 0);
	video().swapBuffers();

	if (pBuffer != nullptr)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pBuffer->m_FBO);
}

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
	if (config.textureFilter.txDump)
		options |= DUMP_TEX;
	if (config.textureFilter.txDeposterize)
		options |= DEPOSTERIZE;
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

	GLint maxTextureSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	wchar_t wRomName[32];
	::mbstowcs(wRomName, RSP.romname, 32);
	wchar_t txPath[PLUGIN_PATH_SIZE + 16];
	wchar_t * pTexPackPath = config.textureFilter.txPath;
	if (::wcslen(config.textureFilter.txPath) == 0) {
		api().GetUserDataPath(txPath);
		gln_wcscat(txPath, wst("/hires_texture"));
		pTexPackPath = txPath;
	}
	wchar_t txCachePath[PLUGIN_PATH_SIZE];
	api().GetUserCachePath(txCachePath);

	m_inited = txfilter_init(maxTextureSize, // max texture width supported by hardware
		maxTextureSize, // max texture height supported by hardware
		32, // max texture bpp supported by hardware
		m_options,
		config.textureFilter.txCacheSize, // cache texture to system memory
		txCachePath, // path to store cache files
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

TextureFilterHandler TFH;
