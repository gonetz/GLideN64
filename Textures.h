#ifndef TEXTURES_H
#define TEXTURES_H

#ifdef ANDROID
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif //ANDROID

#include <map>

#include "CRC.h"
#include "convert.h"

struct CachedTexture
{
	CachedTexture(GLuint _glName) : glName(_glName) {}

	GLuint	glName;
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
	u32		frameBufferTexture;

	u32		lastDList;
	u32		address;
};


struct TextureCache
{
	CachedTexture * current[2];

	void init();
	void destroy();
	CachedTexture * addFrameBufferTexture();
	void addFrameBufferTextureSize(u32 _size) {m_cachedBytes += _size;}
	void removeFrameBufferTexture(CachedTexture * _pTexture);
	void activateTexture(u32 _t, CachedTexture *_pTexture);
	void activateDummy(u32 _t);
	void update(u32 _t);

	static TextureCache & get() {
		static TextureCache cache;
		return cache;
	}

private:
	TextureCache() : m_pDummy(NULL), m_maxBytes(0), m_cachedBytes(0), m_hits(0), m_misses(0), m_bitDepth(0)
	{
		current[0] = NULL;
		current[1] = NULL;
		CRC_BuildTable();
	}
	TextureCache(const TextureCache &);

	void _checkCacheSize();
	CachedTexture * _addTexture(u32 _crc32);
	void _load(CachedTexture *pTexture);
	void _loadBackground(CachedTexture *pTexture);
	void _updateBackground();

	typedef std::map<u32, CachedTexture> Textures;
	Textures m_textures;
	Textures m_fbTextures;
	CachedTexture * m_pDummy;
	u32 m_hits, m_misses;
	u32 m_bitDepth;
	u32 m_maxBytes;
	u32 m_cachedBytes;
};

inline TextureCache & textureCache()
{
	return TextureCache::get();
}

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
#endif
