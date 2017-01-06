#pragma once
#include <memory>
#include "Types.h"

namespace graphics {
	class PixelWriteBuffer;
}

struct CachedTexture;

class NoiseTexture
{
public:
	NoiseTexture();

	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture;
	std::unique_ptr<graphics::PixelWriteBuffer> m_pbuf;
	u32 m_DList;
};

extern NoiseTexture g_noiseTexture;