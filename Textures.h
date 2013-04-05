#ifndef TEXTURES_H
#define TEXTURES_H

#include <GL/gl.h>
#include "convert.h"

struct CachedTexture
{
	GLuint	glName;
	u32		address;
	u32		crc;
//	float	fulS, fulT;
//	WORD	ulS, ulT, lrS, lrT;
	float	offsetS, offsetT;
	u32		maskS, maskT;
	u32		clampS, clampT;
	u32		mirrorS, mirrorT;
	u32		line;
	u32		size;
	u32		format;
	u32		tMem;
	u32		palette;
	u32		width, height;			  // N64 width and height
	u32		clampWidth, clampHeight;  // Size to clamp to
	u32		realWidth, realHeight;	  // Actual texture size
	f32		scaleS, scaleT;			  // Scale to map to 0.0-1.0
	f32		shiftScaleS, shiftScaleT; // Scale to shift
	u32		textureBytes;

	CachedTexture	*lower, *higher;
	u32		lastDList;

	u32		frameBufferTexture;
};


struct TextureCache
{
	CachedTexture	*bottom, *top;

	CachedTexture	*(current[2]);
	u32				maxBytes;
	u32				cachedBytes;
	u32				numCached;
	u32				hits, misses;
	GLuint			glNoiseNames[32];
	//GLuint			glDummyName;
	CachedTexture	*dummy;
	u32				enable2xSaI, bitDepth;
};

extern TextureCache cache;

inline u32 pow2( u32 dim )
{
	u32 i = 1;

	while (i < dim) i <<= 1;

	return i;
}

inline u32 powof( u32 dim )
{
	u32 num = 1;
	u32 i = 0;

	while (num < dim)
	{
		num <<= 1;
		i++;
	}

	return i;
}

CachedTexture *TextureCache_AddTop();
void TextureCache_MoveToTop( CachedTexture *newtop );
void TextureCache_Remove( CachedTexture *texture );
void TextureCache_RemoveBottom();
void TextureCache_Init();
void TextureCache_Destroy();
void TextureCache_Update( u32 t );
void TextureCache_ActivateTexture( u32 t, CachedTexture *texture );
void TextureCache_ActivateNoise( u32 t );
void TextureCache_ActivateDummy( u32 t );
BOOL TextureCache_Verify();

#endif
