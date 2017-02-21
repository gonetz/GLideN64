#pragma once
#include <vector>
#include <Types.h>
#include <Textures.h>

namespace graphics {

class ColorBufferReader
{
public:
	ColorBufferReader(CachedTexture * _pTexture)
		: m_pTexture(_pTexture) {
		m_pixelData.resize(m_pTexture->textureBytes);
	}
	virtual ~ColorBufferReader() {}

	virtual u8 * readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync) = 0;
	virtual void cleanUp() = 0;

protected:
	CachedTexture * m_pTexture;
	std::vector<u8> m_pixelData;
};

}
