#pragma once
#include <Types.h>

struct CachedTexture;

namespace graphics {

class ColorBufferReader
{
public:
	ColorBufferReader(CachedTexture * _pTexture)
		: m_pTexture(_pTexture) {
	}

	virtual ~ColorBufferReader() {}

	virtual void init() = 0;
//	virtual void initBuffers() = 0;
//	virtual void destroyBuffers() = 0;
	virtual u8 * readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync) = 0;
	virtual void cleanUp() = 0;

protected:
	CachedTexture * m_pTexture;
};

}
