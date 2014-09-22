#include <memory.h>
#include <algorithm>
#include "OpenGL.h"
#include "Textures.h"
#include "GBI.h"
#include "RSP.h"
#include "gDP.h"
#include "gSP.h"
#include "N64.h"
#include "CRC.h"
#include "convert.h"
#include "FrameBuffer.h"
#include "Config.h"
#include <assert.h>

using namespace std;

typedef u32 (*GetTexelFunc)( u64 *src, u16 x, u16 i, u8 palette );

inline u32 GetNone( u64 *src, u16 x, u16 i, u8 palette )
{
	return 0x00000000;
}

inline u32 GetCI4IA_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return IA88_RGBA4444( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return IA88_RGBA4444( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetCI4IA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return IA88_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return IA88_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetCI4RGBA_RGBA5551( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return RGBA5551_RGBA5551( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return RGBA5551_RGBA5551( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetCI4RGBA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return RGBA5551_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return RGBA5551_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetIA31_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return IA31_RGBA8888( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetIA31_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return IA31_RGBA4444( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetI4_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return I4_RGBA8888( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetI4_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return I4_RGBA4444( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetCI8IA_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA4444( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetCI8IA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA8888( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetCI8RGBA_RGBA5551( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA5551_RGBA5551( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetCI8RGBA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA5551_RGBA8888( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetIA44_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA44_RGBA8888(((u8*)src)[x^(i<<1)]);
}

inline u32 GetIA44_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA44_RGBA4444(((u8*)src)[x^(i<<1)]);
}

inline u32 GetI8_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return I8_RGBA8888(((u8*)src)[x^(i<<1)]);
}

inline u32 GetI8_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return I8_RGBA4444(((u8*)src)[x^(i<<1)]);
}

inline u32 GetRGBA5551_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA5551_RGBA8888( ((u16*)src)[x^i] );
}

inline u32 GetRGBA5551_RGBA5551( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA5551_RGBA5551( ((u16*)src)[x^i] );
}

inline u32 GetIA88_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA8888(((u16*)src)[x^i]);
}

inline u32 GetIA88_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA4444(((u16*)src)[x^i]);
}

inline u32 GetRGBA8888_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return ((u32*)src)[x^i];
}

inline u32 GetRGBA8888_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA8888_RGBA4444(((u32*)src)[x^i]);
}

const struct
{
	GetTexelFunc	Get16;
	GLenum			glType16;
	GLint			glInternalFormat16;
	GetTexelFunc	Get32;
	GLenum			glType32;
	GLint			glInternalFormat32;
	u32				autoFormat, lineShift, maxTexels;
} imageFormat[4][5] =
{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
	{ // 4-bit
		{	GetCI4RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1,	GL_RGB5_A1,			GetCI4RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGB5_A1, 4, 4096 }, // CI (Banjo-Kazooie uses this, doesn't make sense, but it works...)
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 4, 8192 }, // YUV
		{	GetCI4RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1,	GL_RGB5_A1,			GetCI4RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGB5_A1, 4, 4096 }, // CI
		{	GetIA31_RGBA4444,		GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetIA31_RGBA8888,		GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 4, 8192 }, // IA
		{	GetI4_RGBA4444,			GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetI4_RGBA8888,			GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 4, 8192 }, // I
	},
	{ // 8-bit
		{	GetCI8RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1,	GL_RGB5_A1,			GetCI8RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGB5_A1, 3, 2048 }, // RGBA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 4096 }, // YUV
		{	GetCI8RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1,	GL_RGB5_A1,			GetCI8RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGB5_A1, 3, 2048 }, // CI
		{	GetIA44_RGBA4444,		GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetIA44_RGBA8888,		GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 3, 4096 }, // IA
		{	GetI8_RGBA4444,			GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetI8_RGBA8888,			GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA, 3, 4096 }, // I
	},
	{ // 16-bit
		{	GetRGBA5551_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1,	GL_RGB5_A1,			GetRGBA5551_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGB5_A1, 2, 2048 }, // RGBA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 2, 2048 }, // YUV
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 2048 }, // CI
		{	GetIA88_RGBA4444,		GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetIA88_RGBA8888,		GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA, 2, 2048 }, // IA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 2048 }, // I
	},
	{ // 32-bit
		{	GetRGBA8888_RGBA4444,	GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetRGBA8888_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA, 2, 1024 }, // RGBA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 1024 }, // YUV
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 1024 }, // CI
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 1024 }, // IA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA,			GL_RGBA4, 0, 1024 }, // I
	}
};

void TextureCache::init()
{
	u32 dummyTexture[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	m_bitDepth = config.texture.textureBitDepth;
	m_maxBytes = config.texture.maxBytes;

	m_pDummy = addFrameBufferTexture(); // we don't want to remove dummy texture

	m_pDummy->address = 0;
	m_pDummy->clampS = 1;
	m_pDummy->clampT = 1;
	m_pDummy->clampWidth = 2;
	m_pDummy->clampHeight = 2;
	m_pDummy->crc = 0;
	m_pDummy->format = 0;
	m_pDummy->size = 0;
	m_pDummy->frameBufferTexture = FALSE;
	m_pDummy->width = 2;
	m_pDummy->height = 2;
	m_pDummy->realWidth = 0;
	m_pDummy->realHeight = 0;
	m_pDummy->maskS = 0;
	m_pDummy->maskT = 0;
	m_pDummy->scaleS = 0.5f;
	m_pDummy->scaleT = 0.5f;
	m_pDummy->shiftScaleS = 1.0f;
	m_pDummy->shiftScaleT = 1.0f;
	m_pDummy->textureBytes = 64;
	m_pDummy->tMem = 0;

	glBindTexture( GL_TEXTURE_2D, m_pDummy->glName );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummyTexture );

	m_cachedBytes = m_pDummy->textureBytes;
	activateDummy( 0 );
	activateDummy( 1 );

	CRC_BuildTable();
}

void TextureCache::destroy()
{
	current[0] = current[1] = NULL;

	for (Textures::const_iterator cur = m_textures.cbegin(); cur != m_textures.cend(); ++cur)
		glDeleteTextures( 1, &cur->second.glName );
	m_textures.clear();

	for (Textures::const_iterator cur = m_fbTextures.cbegin(); cur != m_fbTextures.cend(); ++cur)
		glDeleteTextures( 1, &cur->second.glName );
	m_fbTextures.clear();

	m_cachedBytes = 0;
}

void TextureCache::_checkCacheSize()
{
	if (m_cachedBytes <= m_maxBytes)
		return;
	Textures::const_iterator iter = m_textures.cend();
	do {
		--iter;
		m_cachedBytes -= iter->second.textureBytes;
		glDeleteTextures( 1, &iter->second.glName );
	} while (m_cachedBytes > m_maxBytes && iter != m_textures.cbegin());
	m_textures.erase(iter, m_textures.cend());
}

CachedTexture * TextureCache::_addTexture(u32 _crc32)
{
	_checkCacheSize();
	GLuint glName;
	glGenTextures(1, &glName);
	m_textures.emplace(_crc32, glName);
	CachedTexture & texture = m_textures.at(_crc32);
	texture.crc = _crc32;
	return &texture;
}

void TextureCache::removeFrameBufferTexture(CachedTexture * _pTexture)
{
	Textures::const_iterator iter = m_fbTextures.find(_pTexture->glName);
	assert(iter != m_fbTextures.end());
	m_cachedBytes -= iter->second.textureBytes;
	glDeleteTextures( 1, &iter->second.glName );
	m_fbTextures.erase(iter);
}

CachedTexture * TextureCache::addFrameBufferTexture()
{
	_checkCacheSize();
	GLuint glName;
	glGenTextures(1, &glName);
	m_fbTextures.emplace(glName, glName);
	return &m_fbTextures.at(glName);
}

void TextureCache::_loadBackground( CachedTexture *pTexture )
{
	u32 *pDest;

	u8 *pSwapped, *pSrc;
	u32 numBytes, bpl;
	u32 x, y, j, tx, ty;
	u16 clampSClamp;
	u16 clampTClamp;
	GetTexelFunc GetTexel;
	GLuint glInternalFormat;
	GLenum glType;

	if (((imageFormat[pTexture->size][pTexture->format].autoFormat == GL_RGBA) ||
		((pTexture->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) || (m_bitDepth == 2)) && (m_bitDepth != 0))
	{
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 2;
		if ((pTexture->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) {
			if (pTexture->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA8888;
			else
				GetTexel = GetCI8IA_RGBA8888;

			glInternalFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;
		} else {
			GetTexel = imageFormat[pTexture->size][pTexture->format].Get32;
			glInternalFormat = imageFormat[pTexture->size][pTexture->format].glInternalFormat32;
			glType = imageFormat[pTexture->size][pTexture->format].glType32;
		}
	} else {
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 1;
		if ((pTexture->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) {
			if (pTexture->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA4444;
			else
				GetTexel = GetCI8IA_RGBA4444;

			glInternalFormat = GL_RGBA4;
			glType = GL_UNSIGNED_SHORT_4_4_4_4;
		} else {
			GetTexel = imageFormat[pTexture->size][pTexture->format].Get16;
			glInternalFormat = imageFormat[pTexture->size][pTexture->format].glInternalFormat16;
			glType = imageFormat[pTexture->size][pTexture->format].glType16;
		}
	}

	bpl = gSP.bgImage.width << gSP.bgImage.size >> 1;
	numBytes = bpl * gSP.bgImage.height;
	pSwapped = (u8*)malloc( numBytes );
	UnswapCopy( &RDRAM[gSP.bgImage.address], pSwapped, numBytes );
	pDest = (u32*)malloc( pTexture->textureBytes );

	clampSClamp = pTexture->width - 1;
	clampTClamp = pTexture->height - 1;

	j = 0;
	for (y = 0; y < pTexture->realHeight; y++) {
		ty = min(y, (u32)clampTClamp);

		pSrc = &pSwapped[bpl * ty];

		for (x = 0; x < pTexture->realWidth; x++) {
			tx = min(x, (u32)clampSClamp);

			if (glInternalFormat == GL_RGBA)
				((u32*)pDest)[j++] = GetTexel( (u64*)pSrc, tx, 0, pTexture->palette );
			else
				((u16*)pDest)[j++] = GetTexel( (u64*)pSrc, tx, 0, pTexture->palette );
		}
	}

	glTexImage2D( GL_TEXTURE_2D, 0, glInternalFormat, pTexture->realWidth, pTexture->realHeight, 0, GL_RGBA, glType, pDest );
	free(pDest);
}

void TextureCache::_load(CachedTexture *pTexture)
{
	u32 *pDest;

	u64 *pSrc;
	u16 x, y, i, j, tx, ty, line;
	u16 mirrorSBit, maskSMask, clampSClamp;
	u16 mirrorTBit, maskTMask, clampTClamp;
	GetTexelFunc GetTexel;
	GLuint glInternalFormat;
	GLenum glType;

	if (((imageFormat[pTexture->size][pTexture->format].autoFormat == GL_RGBA) ||
		((pTexture->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) || (m_bitDepth == 2)) && (m_bitDepth != 0))
	{
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 2;
		if ((pTexture->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) {
			if (pTexture->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA8888;
			else
				GetTexel = GetCI8IA_RGBA8888;

			glInternalFormat = GL_RGBA;
			glType = GL_UNSIGNED_BYTE;
		} else {
			GetTexel = imageFormat[pTexture->size][pTexture->format].Get32;
			glInternalFormat = imageFormat[pTexture->size][pTexture->format].glInternalFormat32;
			glType = imageFormat[pTexture->size][pTexture->format].glType32;
		}
	} else {
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 1;
		if ((pTexture->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) {
			if (pTexture->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA4444;
			else
				GetTexel = GetCI8IA_RGBA4444;

			glInternalFormat = GL_RGBA4;
			glType = GL_UNSIGNED_SHORT_4_4_4_4;
		} else {
			GetTexel = imageFormat[pTexture->size][pTexture->format].Get16;
			glInternalFormat = imageFormat[pTexture->size][pTexture->format].glInternalFormat16;
			glType = imageFormat[pTexture->size][pTexture->format].glType16;
		}
	}

	pDest = (u32*)malloc( pTexture->textureBytes );

	line = pTexture->line;

	if (pTexture->size == G_IM_SIZ_32b)
		line <<= 1;

	if (pTexture->maskS) {
		clampSClamp = pTexture->clampS ? pTexture->clampWidth - 1 : (pTexture->mirrorS ? (pTexture->width << 1) - 1 : pTexture->width - 1);
		maskSMask = (1 << pTexture->maskS) - 1;
		mirrorSBit = pTexture->mirrorS ? 1 << pTexture->maskS : 0;
	} else {
		clampSClamp = min( pTexture->clampWidth, pTexture->width ) - 1;
		maskSMask = 0xFFFF;
		mirrorSBit = 0x0000;
	}

	if (pTexture->maskT) {
		clampTClamp = pTexture->clampT ? pTexture->clampHeight - 1 : (pTexture->mirrorT ? (pTexture->height << 1) - 1: pTexture->height - 1);
		maskTMask = (1 << pTexture->maskT) - 1;
		mirrorTBit = pTexture->mirrorT ?	1 << pTexture->maskT : 0;
	} else {
		clampTClamp = min( pTexture->clampHeight, pTexture->height ) - 1;
		maskTMask = 0xFFFF;
		mirrorTBit = 0x0000;
	}

	// Hack for Zelda warp texture
	if (((pTexture->tMem << 3) + (pTexture->width * pTexture->height << pTexture->size >> 1)) > 4096)
		pTexture->tMem = 0;

	j = 0;
	for (y = 0; y < pTexture->realHeight; y++) {
		ty = min(y, clampTClamp) & maskTMask;

		if (y & mirrorTBit)
			ty ^= maskTMask;

		pSrc = &TMEM[pTexture->tMem] + line * ty;

		i = (ty & 1) << 1;
		for (x = 0; x < pTexture->realWidth; x++) {
			tx = min(x, clampSClamp) & maskSMask;

			if (x & mirrorSBit)
				tx ^= maskSMask;

			if (glInternalFormat == GL_RGBA)
				((u32*)pDest)[j++] = GetTexel( pSrc, tx, i, pTexture->palette );
			else
				((u16*)pDest)[j++] = GetTexel( pSrc, tx, i, pTexture->palette );
		}
	}

	glTexImage2D( GL_TEXTURE_2D, 0, glInternalFormat, pTexture->realWidth, pTexture->realHeight, 0, GL_RGBA, glType, pDest );
	free(pDest);
}

struct TextureParams
{
	u16 width;
	u16 height;
	u16 clampWidth;
	u16 clampHeight;
	u8 maskS;
	u8 maskT;
	u8 mirrorS;
	u8 mirrorT;
	u8 clampS;
	u8 clampT;
	u8 format;
	u8 size;
};

static
u32 _calculateCRC(u32 t, const TextureParams & _params)
{
	u32 crc;
	u32 y, bpl, lineBytes, line;
	u64 *src;

	src = (u64*)&TMEM[gSP.textureTile[t]->tmem];
	bpl = _params.width << gSP.textureTile[t]->size >> 1;
	lineBytes = gSP.textureTile[t]->line << 3;

	line = gSP.textureTile[t]->line;
	if (gSP.textureTile[t]->size == G_IM_SIZ_32b)
		line <<= 1;

	crc = 0xFFFFFFFF;
	for (y = 0; y < _params.height; y++)
	{
		crc = CRC_Calculate( crc, src, bpl );

		src += line;
	}

	if (gDP.otherMode.textureLUT != G_TT_NONE || gSP.textureTile[t]->format == G_IM_FMT_CI) {
		if (gSP.textureTile[t]->size == G_IM_SIZ_4b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC16[gSP.textureTile[t]->palette], 4 );
		else if (gSP.textureTile[t]->size == G_IM_SIZ_8b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC256, 4 );
	}

	crc = CRC_Calculate(crc, &_params, sizeof(_params));
	return crc;
}

void TextureCache::activateTexture(u32 _t, CachedTexture *_pTexture)
{
	glActiveTexture( GL_TEXTURE0 + _t );

	// Bind the cached texture
	glBindTexture( GL_TEXTURE_2D, _pTexture->glName );

	// Set filter mode. Almost always bilinear, but check anyways
	if ((gDP.otherMode.textureFilter == G_TF_BILERP) || (gDP.otherMode.textureFilter == G_TF_AVERAGE) || (config.texture.forceBilinear))
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	// Set clamping modes
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _pTexture->clampS ? GL_CLAMP_TO_EDGE : GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _pTexture->clampT ? GL_CLAMP_TO_EDGE : GL_REPEAT );

	_pTexture->lastDList = RSP.DList;

	current[_t] = _pTexture;
}

void TextureCache::activateDummy(u32 _t)
{
	glActiveTexture( GL_TEXTURE0 + _t );

	glBindTexture( GL_TEXTURE_2D, m_pDummy->glName );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

void TextureCache::_updateBackground()
{
	u32 numBytes = gSP.bgImage.width * gSP.bgImage.height << gSP.bgImage.size >> 1;
	u32 crc;

	crc = CRC_Calculate( 0xFFFFFFFF, &RDRAM[gSP.bgImage.address], numBytes );

	if (gDP.otherMode.textureLUT != G_TT_NONE || gSP.bgImage.format == G_IM_FMT_CI) {
		if (gSP.bgImage.size == G_IM_SIZ_4b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC16[gSP.bgImage.palette], 4 );
		else if (gSP.bgImage.size == G_IM_SIZ_8b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC256, 4 );
	}

	u32 params[4] = {gSP.bgImage.width, gSP.bgImage.height, gSP.bgImage.format, gSP.bgImage.size};
	crc = CRC_Calculate(crc, params, sizeof(u32)*4);

	Textures::iterator iter = m_textures.find(crc);
	if (iter != m_textures.end()) {
		CachedTexture & current = iter->second;
		assert((current.width == gSP.bgImage.width) &&
			(current.height == gSP.bgImage.height) &&
			(current.format == gSP.bgImage.format) &&
			(current.size == gSP.bgImage.size));

		activateTexture(0, &current);
		m_hits++;
		return;
	}

	m_misses++;

	glActiveTexture( GL_TEXTURE0 );
	CachedTexture * pCurrent = _addTexture(crc);

	glBindTexture( GL_TEXTURE_2D, pCurrent->glName );

	pCurrent->address = gSP.bgImage.address;

	pCurrent->format = gSP.bgImage.format;
	pCurrent->size = gSP.bgImage.size;

	pCurrent->width = gSP.bgImage.width;
	pCurrent->height = gSP.bgImage.height;

	pCurrent->clampWidth = gSP.bgImage.width;
	pCurrent->clampHeight = gSP.bgImage.height;
	pCurrent->palette = gSP.bgImage.palette;
	pCurrent->maskS = 0;
	pCurrent->maskT = 0;
	pCurrent->mirrorS = 0;
	pCurrent->mirrorT = 0;
	pCurrent->clampS = 1;
	pCurrent->clampT = 1;
	pCurrent->line = 0;
	pCurrent->tMem = 0;
	pCurrent->lastDList = RSP.DList;
	pCurrent->frameBufferTexture = FALSE;

	pCurrent->realWidth = pow2( gSP.bgImage.width );
	pCurrent->realHeight = pow2( gSP.bgImage.height );

	pCurrent->scaleS = 1.0f / (f32)(pCurrent->realWidth);
	pCurrent->scaleT = 1.0f / (f32)(pCurrent->realHeight);

	pCurrent->shiftScaleS = 1.0f;
	pCurrent->shiftScaleT = 1.0f;

	_loadBackground(pCurrent);
	activateTexture(0, pCurrent);

	m_cachedBytes += pCurrent->textureBytes;
	current[0] = pCurrent;
}

void TextureCache::update(u32 _t)
{
	u32 crc, maxTexels;
	u32 tileWidth, maskWidth, loadWidth, lineWidth, clampWidth, height;
	u32 tileHeight, maskHeight, loadHeight, lineHeight, clampHeight, width;

	if (m_bitDepth != config.texture.textureBitDepth)
	{
		destroy();
		init();
	}

	switch(gSP.textureTile[_t]->textureMode) {
	case TEXTUREMODE_BGIMAGE:
		_updateBackground();
		return;
	case TEXTUREMODE_FRAMEBUFFER:
		FrameBuffer_ActivateBufferTexture( _t, gSP.textureTile[_t]->frameBuffer );
		return;
	case TEXTUREMODE_FRAMEBUFFER_BG:
		FrameBuffer_ActivateBufferTextureBG( _t, gSP.textureTile[_t]->frameBuffer );
		return;
	}

	maxTexels = imageFormat[gSP.textureTile[_t]->size][gSP.textureTile[_t]->format].maxTexels;

	// Here comes a bunch of code that just calculates the texture size...I wish there was an easier way...
	tileWidth = gSP.textureTile[_t]->lrs - gSP.textureTile[_t]->uls + 1;
	tileHeight = gSP.textureTile[_t]->lrt - gSP.textureTile[_t]->ult + 1;

	maskWidth = 1 << gSP.textureTile[_t]->masks;
	maskHeight = 1 << gSP.textureTile[_t]->maskt;

	loadWidth = gDP.loadTile->lrs - gDP.loadTile->uls + 1;
	loadHeight = gDP.loadTile->lrt - gDP.loadTile->ult + 1;

	lineWidth = gSP.textureTile[_t]->line << imageFormat[gSP.textureTile[_t]->size][gSP.textureTile[_t]->format].lineShift;

	if (lineWidth) // Don't allow division by zero
		lineHeight = min( maxTexels / lineWidth, tileHeight );
	else
		lineHeight = 0;

	if (gSP.textureTile[_t]->textureMode == TEXTUREMODE_TEXRECT)
	{
		u16 texRectWidth = gDP.texRect.width - gSP.textureTile[_t]->uls;
		u16 texRectHeight = gDP.texRect.height - gSP.textureTile[_t]->ult;

//		if ((tileWidth == (maskWidth + 1)) && (gDP.loadType == LOADTYPE_TILE) && (gDP.loadTile->lrs - gDP.loadTile->uls + 1 == tileWidth))
//			gSP.textureTile[t]->masks = 0;

		if (gSP.textureTile[_t]->masks && ((maskWidth * maskHeight) <= maxTexels))
			width = maskWidth;
		else if ((tileWidth * tileHeight) <= maxTexels)
			width = tileWidth;
		else if ((tileWidth * texRectHeight) <= maxTexels)
			width = tileWidth;
		else if ((texRectWidth * tileHeight) <= maxTexels)
			width = gDP.texRect.width;
		else if ((texRectWidth * texRectHeight) <= maxTexels)
			width = gDP.texRect.width;
		else if (gSP.textureTile[_t]->loadType == LOADTYPE_TILE)
			width = loadWidth;
		else
			width = lineWidth;

//		if ((tileHeight == (maskHeight + 1)) && (gDP.loadType == LOADTYPE_TILE) && (gDP.loadTile->lrt - gDP.loadTile->ult + 1 == tileHeight))
//			gSP.textureTile[t]->maskt = 0;

		if (gSP.textureTile[_t]->maskt && ((maskWidth * maskHeight) <= maxTexels))
			height = maskHeight;
		else if ((tileWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else if ((tileWidth * texRectHeight) <= maxTexels)
			height = gDP.texRect.height;
		else if ((texRectWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else if ((texRectWidth * texRectHeight) <= maxTexels)
			height = gDP.texRect.height;
		else if (gSP.textureTile[_t]->loadType == LOADTYPE_TILE)
			height = loadHeight;
		else
			height = lineHeight;

//		gSP.textureTile[t]->masks = 0;
//		gSP.textureTile[t]->maskt = 0;
	}
	else
	{
		if (gSP.textureTile[_t]->masks && ((maskWidth * maskHeight) <= maxTexels))
			width = maskWidth; // Use mask width if set and valid
		else if ((tileWidth * tileHeight) <= maxTexels)
			width = tileWidth; // else use tile width if valid
		else if (gSP.textureTile[_t]->loadType == LOADTYPE_TILE)
			width = loadWidth; // else use load width if load done with LoadTile
		else
			width = lineWidth; // else use line-based width

		if (gSP.textureTile[_t]->maskt && ((maskWidth * maskHeight) <= maxTexels))
			height = maskHeight;
		else if ((tileWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else if (gSP.textureTile[_t]->loadType == LOADTYPE_TILE)
			height = loadHeight;
		else
			height = lineHeight;
	}

/*	if (gDP.loadTile->frameBuffer)
	{
		FrameBuffer_ActivateBufferTexture( t, gDP.loadTile->frameBuffer );
		return;
	}*/

	clampWidth = gSP.textureTile[_t]->clamps ? tileWidth : width;
	clampHeight = gSP.textureTile[_t]->clampt ? tileHeight : height;

	if (clampWidth > 256)
		gSP.textureTile[_t]->clamps = 0;
	if (clampHeight > 256)
		gSP.textureTile[_t]->clampt = 0;

	// Make sure masking is valid
	if (maskWidth > width)
	{
		gSP.textureTile[_t]->masks = powof( width );
		maskWidth = 1 << gSP.textureTile[_t]->masks;
	}

	if (maskHeight > height)
	{
		gSP.textureTile[_t]->maskt = powof( height );
		maskHeight = 1 << gSP.textureTile[_t]->maskt;
	}

	{
	TextureParams params;
	params.width = width;
	params.height = height;
	params.clampWidth = clampWidth;
	params.clampHeight = clampHeight;
	params.maskS = gSP.textureTile[_t]->masks;
	params.maskT = gSP.textureTile[_t]->maskt;
	params.mirrorS = gSP.textureTile[_t]->mirrors;
	params.mirrorT = gSP.textureTile[_t]->mirrort;
	params.clampS = gSP.textureTile[_t]->clamps;
	params.clampT = gSP.textureTile[_t]->clampt;
	params.format = gSP.textureTile[_t]->format;
	params.size = gSP.textureTile[_t]->size;

	crc = _calculateCRC( _t, params );
	}

	if (current[_t] != NULL && current[_t]->crc == crc) {
		activateTexture(_t, current[_t]);
		return;
	}

	Textures::iterator iter = m_textures.find(crc);
	if (iter != m_textures.end()) {
		CachedTexture & current = iter->second;
		assert((current.width == width) &&
			(current.height == height) &&
			(current.clampWidth == clampWidth) &&
			(current.clampHeight == clampHeight) &&
			(current.maskS == gSP.textureTile[_t]->masks) &&
			(current.maskT == gSP.textureTile[_t]->maskt) &&
			(current.mirrorS == gSP.textureTile[_t]->mirrors) &&
			(current.mirrorT == gSP.textureTile[_t]->mirrort) &&
			(current.clampS == gSP.textureTile[_t]->clamps) &&
			(current.clampT == gSP.textureTile[_t]->clampt) &&
			(current.format == gSP.textureTile[_t]->format) &&
			(current.size == gSP.textureTile[_t]->size)
		);

		activateTexture(_t, &current);
		m_hits++;
		return;
	}

	m_misses++;

	glActiveTexture( GL_TEXTURE0 + _t );

	CachedTexture * pCurrent = _addTexture(crc);

	glBindTexture( GL_TEXTURE_2D, pCurrent->glName );

	pCurrent->address = gDP.textureImage.address;

	pCurrent->format = gSP.textureTile[_t]->format;
	pCurrent->size = gSP.textureTile[_t]->size;

	pCurrent->width = width;
	pCurrent->height = height;

	pCurrent->clampWidth = clampWidth;
	pCurrent->clampHeight = clampHeight;

	pCurrent->palette = gSP.textureTile[_t]->palette;
/*	pCurrent->fulS = gSP.textureTile[t]->fulS;
	pCurrent->fulT = gSP.textureTile[t]->fulT;
	pCurrent->ulS = gSP.textureTile[t]->ulS;
	pCurrent->ulT = gSP.textureTile[t]->ulT;
	pCurrent->lrS = gSP.textureTile[t]->lrS;
	pCurrent->lrT = gSP.textureTile[t]->lrT;*/
	pCurrent->maskS = gSP.textureTile[_t]->masks;
	pCurrent->maskT = gSP.textureTile[_t]->maskt;
	pCurrent->mirrorS = gSP.textureTile[_t]->mirrors;
	pCurrent->mirrorT = gSP.textureTile[_t]->mirrort;
	pCurrent->clampS = gSP.textureTile[_t]->clamps;
	pCurrent->clampT = gSP.textureTile[_t]->clampt;
	pCurrent->line = gSP.textureTile[_t]->line;
	pCurrent->tMem = gSP.textureTile[_t]->tmem;
	pCurrent->lastDList = RSP.DList;
	pCurrent->frameBufferTexture = FALSE;

/*	if (pCurrent->clampS)
		pCurrent->realWidth = pow2( clampWidth );
	else if (pCurrent->mirrorS)
		pCurrent->realWidth = maskWidth << 1;
	else
		pCurrent->realWidth = pow2( width );

	if (pCurrent->clampT)
		pCurrent->realHeight = pow2( clampHeight );
	else if (pCurrent->mirrorT)
		pCurrent->realHeight = maskHeight << 1;
	else
		pCurrent->realHeight = pow2( height );*/

	if (pCurrent->clampS)
		pCurrent->realWidth = pow2( clampWidth );
	else if (pCurrent->mirrorS)
		pCurrent->realWidth = maskWidth << 1;
	else
		pCurrent->realWidth = pow2( width );

	if (pCurrent->clampT)
		pCurrent->realHeight = pow2( clampHeight );
	else if (pCurrent->mirrorT)
		pCurrent->realHeight = maskHeight << 1;
	else
		pCurrent->realHeight = pow2( height );

	pCurrent->scaleS = 1.0f / (f32)(pCurrent->realWidth);
	pCurrent->scaleT = 1.0f / (f32)(pCurrent->realHeight);

	pCurrent->shiftScaleS = 1.0f;
	pCurrent->shiftScaleT = 1.0f;

	pCurrent->offsetS = 0.5f;
	pCurrent->offsetT = 0.5f;

	if (gSP.textureTile[_t]->shifts > 10)
		pCurrent->shiftScaleS = (f32)(1 << (16 - gSP.textureTile[_t]->shifts));
	else if (gSP.textureTile[_t]->shifts > 0)
		pCurrent->shiftScaleS /= (f32)(1 << gSP.textureTile[_t]->shifts);

	if (gSP.textureTile[_t]->shiftt > 10)
		pCurrent->shiftScaleT = (f32)(1 << (16 - gSP.textureTile[_t]->shiftt));
	else if (gSP.textureTile[_t]->shiftt > 0)
		pCurrent->shiftScaleT /= (f32)(1 << gSP.textureTile[_t]->shiftt);

	_load( pCurrent );
	activateTexture( _t, pCurrent );

	m_cachedBytes += pCurrent->textureBytes;
	current[_t] = pCurrent;
}
