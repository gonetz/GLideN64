#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif
#include <memory.h>
#include "OpenGL.h"
#include "Textures.h"
#include "GBI.h"
#include "RSP.h"
#include "gDP.h"
#include "gSP.h"
#include "N64.h"
#include "CRC.h"
#include "convert.h"
#include "2xSAI.h"
#include "FrameBuffer.h"

TextureCache	cache;

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
		{	GetCI4RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1_EXT,	GL_RGB5_A1,			GetCI4RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGB5_A1, 4, 4096 }, // CI (Banjo-Kazooie uses this, doesn't make sense, but it works...)
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 4, 8192 }, // YUV
		{	GetCI4RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1_EXT,	GL_RGB5_A1,			GetCI4RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGB5_A1, 4, 4096 }, // CI
		{	GetIA31_RGBA4444,		GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetIA31_RGBA8888,		GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 4, 8192 }, // IA
		{	GetI4_RGBA4444,			GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetI4_RGBA8888,			GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 4, 8192 }, // I
	},
	{ // 8-bit
		{	GetCI8RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1_EXT,	GL_RGB5_A1,			GetCI8RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGB5_A1, 3, 2048 }, // RGBA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 4096 }, // YUV
		{	GetCI8RGBA_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1_EXT,	GL_RGB5_A1,			GetCI8RGBA_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGB5_A1, 3, 2048 }, // CI
		{	GetIA44_RGBA4444,		GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetIA44_RGBA8888,		GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 3, 4096 }, // IA
		{	GetI8_RGBA4444,			GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetI8_RGBA8888,			GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA8, 3, 4096 }, // I
	},
	{ // 16-bit
		{	GetRGBA5551_RGBA5551,	GL_UNSIGNED_SHORT_5_5_5_1_EXT,	GL_RGB5_A1,			GetRGBA5551_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGB5_A1, 2, 2048 }, // RGBA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 2, 2048 }, // YUV
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 2048 }, // CI
		{	GetIA88_RGBA4444,		GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetIA88_RGBA8888,		GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA8, 2, 2048 }, // IA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 2048 }, // I
	},
	{ // 32-bit
		{	GetRGBA8888_RGBA4444,	GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetRGBA8888_RGBA8888,	GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA8, 2, 1024 }, // RGBA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 1024 }, // YUV
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 1024 }, // CI
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 1024 }, // IA
		{	GetNone,				GL_UNSIGNED_SHORT_4_4_4_4_EXT,	GL_RGBA4,			GetNone,				GL_UNSIGNED_BYTE,				GL_RGBA8,			GL_RGBA4, 0, 1024 }, // I
	}
};

void TextureCache_Init()
{
	u32 dummyTexture[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	cache.current[0] = NULL;
	cache.current[1] = NULL;
	cache.top = NULL;
	cache.bottom = NULL;
	cache.numCached = 0;
	cache.cachedBytes = 0;
	cache.enable2xSaI = OGL.enable2xSaI;
	cache.bitDepth = OGL.textureBitDepth;

	glGenTextures( 32, cache.glNoiseNames );

	u8 noise[64*64*4];
	for (s16 i = 0; i < 32; i++)
	{
		glBindTexture( GL_TEXTURE_2D, cache.glNoiseNames[i] );

		srand( timeGetTime() );

		for (s16 y = 0; y < 64; y++)
		{
			for (s16 x = 0; x < 64; x++)
			{
				u8 random = rand();
				noise[y*64*4+x*4] = random;
				noise[y*64*4+x*4+1] = random;
				noise[y*64*4+x*4+2] = random;
				noise[y*64*4+x*4+3] = random;
			}
		}
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise );
	}

	cache.dummy = TextureCache_AddTop();

	cache.dummy->address = 0;
	cache.dummy->clampS = 1;
	cache.dummy->clampT = 1;
	cache.dummy->clampWidth = 2;
	cache.dummy->clampHeight = 2;
	cache.dummy->crc = 0;
	cache.dummy->format = 0;
	cache.dummy->size = 0;
	cache.dummy->frameBufferTexture = FALSE;
	cache.dummy->width = 2;
	cache.dummy->height = 2;
	cache.dummy->realWidth = 0;
	cache.dummy->realHeight = 0;
	cache.dummy->maskS = 0;
	cache.dummy->maskT = 0;
	cache.dummy->scaleS = 0.5f;
	cache.dummy->scaleT = 0.5f;
	cache.dummy->shiftScaleS = 1.0f;
	cache.dummy->shiftScaleT = 1.0f;
	cache.dummy->textureBytes = 64;
	cache.dummy->tMem = 0;

	glBindTexture( GL_TEXTURE_2D, cache.dummy->glName );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummyTexture );

	cache.cachedBytes = cache.dummy->textureBytes;

	TextureCache_ActivateDummy( 0 );
	TextureCache_ActivateDummy( 1 );

	CRC_BuildTable();
}

BOOL TextureCache_Verify()
{
	s16 i = 0;
	CachedTexture *current;
	
	current = cache.top;

	while (current)
	{
		i++;
		current = current->lower;
	}
	if (i != cache.numCached) return FALSE;

	i = 0;
	current = cache.bottom;
	while (current)
	{
		i++;
		current = current->higher;
	}
	if (i != cache.numCached) return FALSE;

	return TRUE;
}

void TextureCache_RemoveBottom()
{
	CachedTexture *newBottom = cache.bottom->higher;

	glDeleteTextures( 1, &cache.bottom->glName );
	cache.cachedBytes -= cache.bottom->textureBytes;

	if (cache.bottom->frameBufferTexture)
		FrameBuffer_RemoveBuffer( cache.bottom->address );

	if (cache.bottom == cache.top)
		cache.top = NULL;

	free( cache.bottom );

    cache.bottom = newBottom;
	
	if (cache.bottom)
		cache.bottom->lower = NULL;

	cache.numCached--;
}

void TextureCache_Remove( CachedTexture *texture )
{
	if ((texture == cache.bottom) &&
		(texture == cache.top))
	{
		cache.top = NULL;
		cache.bottom = NULL;
	}
	else if (texture == cache.bottom)
	{
		cache.bottom = texture->higher;

		if (cache.bottom)
			cache.bottom->lower = NULL;
	}
	else if (texture == cache.top)
	{
		cache.top = texture->lower;

		if (cache.top)
			cache.top->higher = NULL;
	}
	else
	{
		texture->higher->lower = texture->lower;
		texture->lower->higher = texture->higher;
	}

	glDeleteTextures( 1, &texture->glName );
	cache.cachedBytes -= texture->textureBytes;
	free( texture );

	cache.numCached--;
}

CachedTexture *TextureCache_AddTop()
{
	while (cache.cachedBytes > cache.maxBytes)
	{
		if (cache.bottom != cache.dummy)
			TextureCache_RemoveBottom();
		else if (cache.dummy->higher)
			TextureCache_Remove( cache.dummy->higher );
	}

	CachedTexture *newtop = (CachedTexture*)malloc( sizeof( CachedTexture ) );

	glGenTextures( 1, &newtop->glName );

	newtop->lower = cache.top;
	newtop->higher = NULL;

	if (cache.top)
		cache.top->higher = newtop;

	if (!cache.bottom)
		cache.bottom = newtop;

    cache.top = newtop;

	cache.numCached++;

	return newtop;
}

void TextureCache_MoveToTop( CachedTexture *newtop )
{
	if (newtop == cache.top) return;

	if (newtop == cache.bottom)
	{
		cache.bottom = newtop->higher;
		cache.bottom->lower = NULL;
	}
	else
	{
		newtop->higher->lower = newtop->lower;
		newtop->lower->higher = newtop->higher;
	}

	newtop->higher = NULL;
	newtop->lower = cache.top;
	cache.top->higher = newtop;
	cache.top = newtop;
}

void TextureCache_Destroy()
{
	while (cache.bottom)
		TextureCache_RemoveBottom();

	glDeleteTextures( 32, cache.glNoiseNames );
//	glDeleteTextures( 1, &cache.glDummyName );

	cache.top = NULL;
	cache.bottom = NULL;
}

void TextureCache_LoadBackground( CachedTexture *texInfo )
{
	u32 *dest, *scaledDest;

	u8 *swapped, *src;
	u32 numBytes, bpl;
	u32 x, y, i, j, tx, ty;
	u16 clampSClamp;
	u16 clampTClamp;
	GetTexelFunc	GetTexel;
	GLuint			glInternalFormat;
	GLenum			glType;

	if (((imageFormat[texInfo->size][texInfo->format].autoFormat == GL_RGBA8) || 
		((texInfo->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) || (cache.bitDepth == 2)) && (cache.bitDepth != 0))
	{
	 	texInfo->textureBytes = (texInfo->realWidth * texInfo->realHeight) << 2;
		if ((texInfo->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16))
		{
			if (texInfo->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA8888;
			else
				GetTexel = GetCI8IA_RGBA8888;

			glInternalFormat = GL_RGBA8;
			glType = GL_UNSIGNED_BYTE;
		}
		else
		{
			GetTexel = imageFormat[texInfo->size][texInfo->format].Get32;
			glInternalFormat = imageFormat[texInfo->size][texInfo->format].glInternalFormat32;
			glType = imageFormat[texInfo->size][texInfo->format].glType32;
		}
	}
	else
	{
	 	texInfo->textureBytes = (texInfo->realWidth * texInfo->realHeight) << 1;
		if ((texInfo->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16))
		{
			if (texInfo->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA4444;
			else
				GetTexel = GetCI8IA_RGBA4444;

			glInternalFormat = GL_RGBA4;
			glType = GL_UNSIGNED_SHORT_4_4_4_4_EXT;
		}
		else
		{
			GetTexel = imageFormat[texInfo->size][texInfo->format].Get16;
			glInternalFormat = imageFormat[texInfo->size][texInfo->format].glInternalFormat16;
			glType = imageFormat[texInfo->size][texInfo->format].glType16;
		}
	}

	bpl = gSP.bgImage.width << gSP.bgImage.size >> 1;
	numBytes = bpl * gSP.bgImage.height;
	swapped = (u8*)malloc( numBytes );
	UnswapCopy( &RDRAM[gSP.bgImage.address], swapped, numBytes );
	dest = (u32*)malloc( texInfo->textureBytes );

	clampSClamp = texInfo->width - 1;
	clampTClamp = texInfo->height - 1;

	j = 0;
	for (y = 0; y < texInfo->realHeight; y++)
	{
		ty = min(y, clampTClamp);

		src = &swapped[bpl * ty];

		for (x = 0; x < texInfo->realWidth; x++)
		{
			tx = min(x, clampSClamp);

			if (glInternalFormat == GL_RGBA8)
				((u32*)dest)[j++] = GetTexel( (u64*)src, tx, 0, texInfo->palette );
			else
				((u16*)dest)[j++] = GetTexel( (u64*)src, tx, 0, texInfo->palette );
		}
	}

	if (cache.enable2xSaI)
	{
		texInfo->textureBytes <<= 2;

		scaledDest = (u32*)malloc( texInfo->textureBytes );

		if (glInternalFormat == GL_RGBA8)
			_2xSaI8888( (u32*)dest, (u32*)scaledDest, texInfo->realWidth, texInfo->realHeight, texInfo->clampS, texInfo->clampT );
		else if (glInternalFormat == GL_RGBA4)
			_2xSaI4444( (u16*)dest, (u16*)scaledDest, texInfo->realWidth, texInfo->realHeight, texInfo->clampS, texInfo->clampT );
		else
			_2xSaI5551( (u16*)dest, (u16*)scaledDest, texInfo->realWidth, texInfo->realHeight, texInfo->clampS, texInfo->clampT );

		glTexImage2D( GL_TEXTURE_2D, 0, glInternalFormat, texInfo->realWidth << 1, texInfo->realHeight << 1, 0, GL_RGBA, glType, scaledDest );

		free( dest );
		free( scaledDest );
	}
	else
	{
		glTexImage2D( GL_TEXTURE_2D, 0, glInternalFormat, texInfo->realWidth, texInfo->realHeight, 0, GL_RGBA, glType, dest );
		free( dest );
	}
}

void TextureCache_Load( CachedTexture *texInfo )
{
	u32 *dest, *scaledDest;

	u64 *src;
	u16 x, y, i, j, tx, ty, line;
	u16 mirrorSBit, maskSMask, clampSClamp;
	u16 mirrorTBit, maskTMask, clampTClamp;
	GetTexelFunc	GetTexel;
	GLuint			glInternalFormat;
	GLenum			glType;

	if (((imageFormat[texInfo->size][texInfo->format].autoFormat == GL_RGBA8) || 
		((texInfo->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16)) || (cache.bitDepth == 2)) && (cache.bitDepth != 0))
	{
	 	texInfo->textureBytes = (texInfo->realWidth * texInfo->realHeight) << 2;
		if ((texInfo->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16))
		{
			if (texInfo->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA8888;
			else
				GetTexel = GetCI8IA_RGBA8888;

			glInternalFormat = GL_RGBA8;
			glType = GL_UNSIGNED_BYTE;
		}
		else
		{
			GetTexel = imageFormat[texInfo->size][texInfo->format].Get32;
			glInternalFormat = imageFormat[texInfo->size][texInfo->format].glInternalFormat32;
			glType = imageFormat[texInfo->size][texInfo->format].glType32;
		}
	}
	else
	{
	 	texInfo->textureBytes = (texInfo->realWidth * texInfo->realHeight) << 1;
		if ((texInfo->format == G_IM_FMT_CI) && (gDP.otherMode.textureLUT == G_TT_IA16))
		{
			if (texInfo->size == G_IM_SIZ_4b)
				GetTexel = GetCI4IA_RGBA4444;
			else
				GetTexel = GetCI8IA_RGBA4444;

			glInternalFormat = GL_RGBA4;
			glType = GL_UNSIGNED_SHORT_4_4_4_4_EXT;
		}
		else
		{
			GetTexel = imageFormat[texInfo->size][texInfo->format].Get16;
			glInternalFormat = imageFormat[texInfo->size][texInfo->format].glInternalFormat16;
			glType = imageFormat[texInfo->size][texInfo->format].glType16;
		}
	}

	dest = (u32*)malloc( texInfo->textureBytes );

	line = texInfo->line;

	if (texInfo->size == G_IM_SIZ_32b)
		line <<= 1;

	if (texInfo->maskS)
	{
		clampSClamp = texInfo->clampS ? texInfo->clampWidth - 1 : (texInfo->mirrorS ? (texInfo->width << 1) - 1 : texInfo->width - 1);
		maskSMask = (1 << texInfo->maskS) - 1;
		mirrorSBit = texInfo->mirrorS ? 1 << texInfo->maskS : 0;
	}
	else
	{
		clampSClamp = min( texInfo->clampWidth, texInfo->width ) - 1;
		maskSMask = 0xFFFF;
		mirrorSBit = 0x0000;
	}

	if (texInfo->maskT)
	{
		clampTClamp = texInfo->clampT ? texInfo->clampHeight - 1 : (texInfo->mirrorT ? (texInfo->height << 1) - 1: texInfo->height - 1);
		maskTMask = (1 << texInfo->maskT) - 1;
		mirrorTBit = texInfo->mirrorT ?	1 << texInfo->maskT : 0;
	}
	else
	{
		clampTClamp = min( texInfo->clampHeight, texInfo->height ) - 1;
		maskTMask = 0xFFFF;
		mirrorTBit = 0x0000;
	}

	// Hack for Zelda warp texture
	if (((texInfo->tMem << 3) + (texInfo->width * texInfo->height << texInfo->size >> 1)) > 4096)
		texInfo->tMem = 0;

	j = 0;
	for (y = 0; y < texInfo->realHeight; y++)
	{
		ty = min(y, clampTClamp) & maskTMask;

		if (y & mirrorTBit)
			ty ^= maskTMask;

		src = &TMEM[texInfo->tMem] + line * ty;

		i = (ty & 1) << 1;
		for (x = 0; x < texInfo->realWidth; x++)
		{
			tx = min(x, clampSClamp) & maskSMask;

			if (x & mirrorSBit)
				tx ^= maskSMask;

			if (glInternalFormat == GL_RGBA8)
				((u32*)dest)[j++] = GetTexel( src, tx, i, texInfo->palette );
			else
				((u16*)dest)[j++] = GetTexel( src, tx, i, texInfo->palette );
		}
	}

	if (cache.enable2xSaI)
	{
		texInfo->textureBytes <<= 2;

		scaledDest = (u32*)malloc( texInfo->textureBytes );

		if (glInternalFormat == GL_RGBA8)
			_2xSaI8888( (u32*)dest, (u32*)scaledDest, texInfo->realWidth, texInfo->realHeight, 1, 1 );//texInfo->clampS, texInfo->clampT );
		else if (glInternalFormat == GL_RGBA4)
			_2xSaI4444( (u16*)dest, (u16*)scaledDest, texInfo->realWidth, texInfo->realHeight, 1, 1 );//texInfo->clampS, texInfo->clampT );
		else
			_2xSaI5551( (u16*)dest, (u16*)scaledDest, texInfo->realWidth, texInfo->realHeight, 1, 1 );//texInfo->clampS, texInfo->clampT );

		glTexImage2D( GL_TEXTURE_2D, 0, glInternalFormat, texInfo->realWidth << 1, texInfo->realHeight << 1, 0, GL_RGBA, glType, scaledDest );

		free( dest );
		free( scaledDest );
	}
	else
	{
		glTexImage2D( GL_TEXTURE_2D, 0, glInternalFormat, texInfo->realWidth, texInfo->realHeight, 0, GL_RGBA, glType, dest );
		free( dest );
	}
}

u32 TextureCache_CalculateCRC( u32 t, u32 width, u32 height )
{
	u32 crc;
	u32 y, i, bpl, lineBytes, line;
	u64 *src;

	src = (u64*)&TMEM[gSP.textureTile[t]->tmem];
	bpl = width << gSP.textureTile[t]->size >> 1;
	lineBytes = gSP.textureTile[t]->line << 3;

	line = gSP.textureTile[t]->line;
 	if (gSP.textureTile[t]->size == G_IM_SIZ_32b)
		line <<= 1;

	crc = 0xFFFFFFFF;
 	for (y = 0; y < height; y++)
	{
		crc = CRC_Calculate( crc, src, bpl );

		src += line;
	}

   	if (gSP.textureTile[t]->format == G_IM_FMT_CI)
	{
		if (gSP.textureTile[t]->size == G_IM_SIZ_4b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC16[gSP.textureTile[t]->palette], 4 );
		else if (gSP.textureTile[t]->size == G_IM_SIZ_8b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC256, 4 );
	}
	return crc;
}

void TextureCache_ActivateTexture( u32 t, CachedTexture *texture )
{
	// If multitexturing, set the appropriate texture
	if (OGL.ARB_multitexture)
		glActiveTextureARB( GL_TEXTURE0_ARB + t );

	// Bind the cached texture
	glBindTexture( GL_TEXTURE_2D, texture->glName );

	// Set filter mode. Almost always bilinear, but check anyways
	if ((gDP.otherMode.textureFilter == G_TF_BILERP) || (gDP.otherMode.textureFilter == G_TF_AVERAGE) || (OGL.forceBilinear))
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
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture->clampS ? GL_CLAMP_TO_EDGE : GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture->clampT ? GL_CLAMP_TO_EDGE : GL_REPEAT );

	texture->lastDList = RSP.DList;

	TextureCache_MoveToTop( texture );

	cache.current[t] = texture;
}

void TextureCache_ActivateDummy( u32 t )
{
//TextureCache_ActivateTexture( t, cache.dummy );
	if (OGL.ARB_multitexture)
		glActiveTextureARB( GL_TEXTURE0_ARB + t );

	glBindTexture( GL_TEXTURE_2D, cache.dummy->glName );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

void TextureCache_UpdateBackground()
{
	u32 numBytes = gSP.bgImage.width * gSP.bgImage.height << gSP.bgImage.size >> 1;
	u32 crc;

	crc = CRC_Calculate( 0xFFFFFFFF, &RDRAM[gSP.bgImage.address], numBytes );

   	if (gSP.bgImage.format == G_IM_FMT_CI)
	{
		if (gSP.bgImage.size == G_IM_SIZ_4b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC16[gSP.bgImage.palette], 4 );
		else if (gSP.bgImage.size == G_IM_SIZ_8b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC256, 4 );
	}

	CachedTexture *current = cache.top;

 	while (current)
  	{
		if ((current->crc == crc) &&
			(current->width == gSP.bgImage.width) &&
			(current->height == gSP.bgImage.height) &&
			(current->format == gSP.bgImage.format) &&
			(current->size == gSP.bgImage.size))
		{
			TextureCache_ActivateTexture( 0, current );

			cache.hits++;
			return;
		}

		current = current->lower;
	}

	cache.misses++;

	// If multitexturing, set the appropriate texture
	if (OGL.ARB_multitexture)
		glActiveTextureARB( GL_TEXTURE0_ARB );

	cache.current[0] = TextureCache_AddTop();

	glBindTexture( GL_TEXTURE_2D, cache.current[0]->glName );

	cache.current[0]->address = gSP.bgImage.address;
	cache.current[0]->crc = crc;

	cache.current[0]->format = gSP.bgImage.format;
	cache.current[0]->size = gSP.bgImage.size;

	cache.current[0]->width = gSP.bgImage.width;
	cache.current[0]->height = gSP.bgImage.height;

	cache.current[0]->clampWidth = gSP.bgImage.width;
	cache.current[0]->clampHeight = gSP.bgImage.height;
	cache.current[0]->palette = gSP.bgImage.palette;
	cache.current[0]->maskS = 0;
	cache.current[0]->maskT = 0;
	cache.current[0]->mirrorS = 0;
	cache.current[0]->mirrorT = 0;
 	cache.current[0]->clampS = 1;
	cache.current[0]->clampT = 1;
	cache.current[0]->line = 0;
	cache.current[0]->tMem = 0;
	cache.current[0]->lastDList = RSP.DList;
	cache.current[0]->frameBufferTexture = FALSE;

	cache.current[0]->realWidth = pow2( gSP.bgImage.width );
	cache.current[0]->realHeight = pow2( gSP.bgImage.height );

	cache.current[0]->scaleS = 1.0f / (f32)(cache.current[0]->realWidth);
	cache.current[0]->scaleT = 1.0f / (f32)(cache.current[0]->realHeight);

	cache.current[0]->shiftScaleS = 1.0f;
	cache.current[0]->shiftScaleT = 1.0f;

	TextureCache_LoadBackground( cache.current[0] );
	TextureCache_ActivateTexture( 0, cache.current[0] );

	cache.cachedBytes += cache.current[0]->textureBytes;
}

void TextureCache_Update( u32 t )
{
	CachedTexture *current;
	s32 i, j, k;
	u32 crc, bpl, cacheNum, maxTexels;
	u32 tileWidth, maskWidth, loadWidth, lineWidth, clampWidth, height;
	u32 tileHeight, maskHeight, loadHeight, lineHeight, clampHeight, width;

	if (cache.enable2xSaI != OGL.enable2xSaI)
	{
		TextureCache_Destroy();
		TextureCache_Init();
	}

	if (cache.bitDepth != OGL.textureBitDepth)
	{
		TextureCache_Destroy();
		TextureCache_Init();
	}

	if (gDP.textureMode == TEXTUREMODE_BGIMAGE)
	{
		TextureCache_UpdateBackground();
		return;
	}
	else if (gDP.textureMode == TEXTUREMODE_FRAMEBUFFER)
	{
		FrameBuffer_ActivateBufferTexture( t, gDP.loadTile->frameBuffer );
		return;
	}

	maxTexels = imageFormat[gSP.textureTile[t]->size][gSP.textureTile[t]->format].maxTexels;

	// Here comes a bunch of code that just calculates the texture size...I wish there was an easier way...
	tileWidth = gSP.textureTile[t]->lrs - gSP.textureTile[t]->uls + 1;
	tileHeight = gSP.textureTile[t]->lrt - gSP.textureTile[t]->ult + 1;

	maskWidth = 1 << gSP.textureTile[t]->masks;
	maskHeight = 1 << gSP.textureTile[t]->maskt;

	loadWidth = gDP.loadTile->lrs - gDP.loadTile->uls + 1;
	loadHeight = gDP.loadTile->lrt - gDP.loadTile->ult + 1;

	lineWidth = gSP.textureTile[t]->line << imageFormat[gSP.textureTile[t]->size][gSP.textureTile[t]->format].lineShift;

	if (lineWidth) // Don't allow division by zero
		lineHeight = min( maxTexels / lineWidth, tileHeight );
	else
		lineHeight = 0;

	if (gDP.textureMode == TEXTUREMODE_TEXRECT)
	{
		u16 texRectWidth = gDP.texRect.width - gSP.textureTile[t]->uls;
		u16 texRectHeight = gDP.texRect.height - gSP.textureTile[t]->ult;

//		if ((tileWidth == (maskWidth + 1)) && (gDP.loadType == LOADTYPE_TILE) && (gDP.loadTile->lrs - gDP.loadTile->uls + 1 == tileWidth))
//			gSP.textureTile[t]->masks = 0;

		if (gSP.textureTile[t]->masks && ((maskWidth * maskHeight) <= maxTexels))
			width = maskWidth;
		else if ((tileWidth * tileHeight) <= maxTexels)
			width = tileWidth;
		else if ((tileWidth * texRectHeight) <= maxTexels)
			width = tileWidth;
		else if ((texRectWidth * tileHeight) <= maxTexels)
			width = gDP.texRect.width;
		else if ((texRectWidth * texRectHeight) <= maxTexels)
			width = gDP.texRect.width;
		else if (gDP.loadType == LOADTYPE_TILE)
			width = loadWidth;
		else
			width = lineWidth;

//		if ((tileHeight == (maskHeight + 1)) && (gDP.loadType == LOADTYPE_TILE) && (gDP.loadTile->lrt - gDP.loadTile->ult + 1 == tileHeight))
//			gSP.textureTile[t]->maskt = 0;

		if (gSP.textureTile[t]->maskt && ((maskWidth * maskHeight) <= maxTexels))
			height = maskHeight;
		else if ((tileWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else if ((tileWidth * texRectHeight) <= maxTexels)
			height = gDP.texRect.height;
		else if ((texRectWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else if ((texRectWidth * texRectHeight) <= maxTexels)
			height = gDP.texRect.height;
		else if (gDP.loadType == LOADTYPE_TILE)
			height = loadHeight;
		else
			height = lineHeight;

//		gSP.textureTile[t]->masks = 0;
//		gSP.textureTile[t]->maskt = 0;
	}
	else
	{
		if (gSP.textureTile[t]->masks && ((maskWidth * maskHeight) <= maxTexels))
			width = maskWidth; // Use mask width if set and valid
		else if ((tileWidth * tileHeight) <= maxTexels)
			width = tileWidth; // else use tile width if valid
		else if (gDP.loadType == LOADTYPE_TILE)
			width = loadWidth; // else use load width if load done with LoadTile
		else
			width = lineWidth; // else use line-based width

		if (gSP.textureTile[t]->maskt && ((maskWidth * maskHeight) <= maxTexels))
			height = maskHeight;
		else if ((tileWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else if (gDP.loadType == LOADTYPE_TILE)
			height = loadHeight;
		else
			height = lineHeight;
	}

/*	if (gDP.loadTile->frameBuffer)
	{
		FrameBuffer_ActivateBufferTexture( t, gDP.loadTile->frameBuffer );
		return;
	}*/

 	clampWidth = gSP.textureTile[t]->clamps ? tileWidth : width;
	clampHeight = gSP.textureTile[t]->clampt ? tileHeight : height;

	if (clampWidth > 256)
		gSP.textureTile[t]->clamps = 0;
	if (clampHeight > 256)
		gSP.textureTile[t]->clampt = 0;

	// Make sure masking is valid
	if (maskWidth > width) 
	{
		gSP.textureTile[t]->masks = powof( width );
		maskWidth = 1 << gSP.textureTile[t]->masks;
	}

	if (maskHeight > height)
	{
		gSP.textureTile[t]->maskt = powof( height );
		maskHeight = 1 << gSP.textureTile[t]->maskt;
	}

	crc = TextureCache_CalculateCRC( t, width, height );

//	if (!TextureCache_Verify())
//		current = cache.top;

	current = cache.top;
 	while (current)
  	{
		if ((current->crc == crc) &&
//			(current->address == gDP.textureImage.address) &&
//			(current->palette == gSP.textureTile[t]->palette) &&
			(current->width == width) &&
			(current->height == height) &&
			(current->clampWidth == clampWidth) &&
			(current->clampHeight == clampHeight) &&
			(current->maskS == gSP.textureTile[t]->masks) &&
			(current->maskT == gSP.textureTile[t]->maskt) &&
			(current->mirrorS == gSP.textureTile[t]->mirrors) &&
			(current->mirrorT == gSP.textureTile[t]->mirrort) &&
			(current->clampS == gSP.textureTile[t]->clamps) &&
			(current->clampT == gSP.textureTile[t]->clampt) &&
//			(current->tMem == gSP.textureTile[t]->tMem) &&
/*			(current->ulS == gSP.textureTile[t]->ulS) &&
			(current->ulT == gSP.textureTile[t]->ulT) &&
			(current->lrS == gSP.textureTile[t]->lrS) &&
			(current->lrT == gSP.textureTile[t]->lrT) &&*/
			(current->format == gSP.textureTile[t]->format) &&
			(current->size == gSP.textureTile[t]->size))
		{
			TextureCache_ActivateTexture( t, current );

			cache.hits++;
			return;
		}

		current = current->lower;
	}

	cache.misses++;

	// If multitexturing, set the appropriate texture
	if (OGL.ARB_multitexture)
		glActiveTextureARB( GL_TEXTURE0_ARB + t );

	cache.current[t] = TextureCache_AddTop();

	glBindTexture( GL_TEXTURE_2D, cache.current[t]->glName );

	cache.current[t]->address = gDP.textureImage.address;
	cache.current[t]->crc = crc;

	cache.current[t]->format = gSP.textureTile[t]->format;
	cache.current[t]->size = gSP.textureTile[t]->size;

	cache.current[t]->width = width;
	cache.current[t]->height = height;

	cache.current[t]->clampWidth = clampWidth;
	cache.current[t]->clampHeight = clampHeight;

	cache.current[t]->palette = gSP.textureTile[t]->palette;
/*	cache.current[t]->fulS = gSP.textureTile[t]->fulS;
	cache.current[t]->fulT = gSP.textureTile[t]->fulT;
	cache.current[t]->ulS = gSP.textureTile[t]->ulS;
	cache.current[t]->ulT = gSP.textureTile[t]->ulT;
	cache.current[t]->lrS = gSP.textureTile[t]->lrS;
	cache.current[t]->lrT = gSP.textureTile[t]->lrT;*/
	cache.current[t]->maskS = gSP.textureTile[t]->masks;
	cache.current[t]->maskT = gSP.textureTile[t]->maskt;
	cache.current[t]->mirrorS = gSP.textureTile[t]->mirrors;
	cache.current[t]->mirrorT = gSP.textureTile[t]->mirrort;
 	cache.current[t]->clampS = gSP.textureTile[t]->clamps;
	cache.current[t]->clampT = gSP.textureTile[t]->clampt;
	cache.current[t]->line = gSP.textureTile[t]->line;
	cache.current[t]->tMem = gSP.textureTile[t]->tmem;
	cache.current[t]->lastDList = RSP.DList;
	cache.current[t]->frameBufferTexture = FALSE;

/*	if (cache.current[t]->clampS)
		cache.current[t]->realWidth = pow2( clampWidth );
	else if (cache.current[t]->mirrorS)
		cache.current[t]->realWidth = maskWidth << 1;
	else
		cache.current[t]->realWidth = pow2( width );

	if (cache.current[t]->clampT)
		cache.current[t]->realHeight = pow2( clampHeight );
	else if (cache.current[t]->mirrorT)
		cache.current[t]->realHeight = maskHeight << 1;
	else
		cache.current[t]->realHeight = pow2( height );*/

	if (cache.current[t]->clampS)
		cache.current[t]->realWidth = pow2( clampWidth );
	else if (cache.current[t]->mirrorS)
		cache.current[t]->realWidth = maskWidth << 1;
	else
		cache.current[t]->realWidth = pow2( width );

	if (cache.current[t]->clampT)
		cache.current[t]->realHeight = pow2( clampHeight );
	else if (cache.current[t]->mirrorT)
		cache.current[t]->realHeight = maskHeight << 1;
	else
		cache.current[t]->realHeight = pow2( height );

	cache.current[t]->scaleS = 1.0f / (f32)(cache.current[t]->realWidth);
	cache.current[t]->scaleT = 1.0f / (f32)(cache.current[t]->realHeight);

	cache.current[t]->shiftScaleS = 1.0f;
	cache.current[t]->shiftScaleT = 1.0f;

	cache.current[t]->offsetS = OGL.enable2xSaI ? 0.25f : 0.5f;
	cache.current[t]->offsetT = OGL.enable2xSaI ? 0.25f : 0.5f;

	if (gSP.textureTile[t]->shifts > 10)
		cache.current[t]->shiftScaleS = (f32)(1 << (16 - gSP.textureTile[t]->shifts));
	else if (gSP.textureTile[t]->shifts > 0)
		cache.current[t]->shiftScaleS /= (f32)(1 << gSP.textureTile[t]->shifts);

	if (gSP.textureTile[t]->shiftt > 10)
		cache.current[t]->shiftScaleT = (f32)(1 << (16 - gSP.textureTile[t]->shiftt));
	else if (gSP.textureTile[t]->shiftt > 0)
		cache.current[t]->shiftScaleT /= (f32)(1 << gSP.textureTile[t]->shiftt);

	TextureCache_Load( cache.current[t] );
	TextureCache_ActivateTexture( t, cache.current[t] );

	cache.cachedBytes += cache.current[t]->textureBytes;
}

void TextureCache_ActivateNoise( u32 t )
{
	if (OGL.ARB_multitexture)
		glActiveTextureARB( GL_TEXTURE0_ARB + t );

	glBindTexture( GL_TEXTURE_2D, cache.glNoiseNames[RSP.DList & 0x1F] );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}
