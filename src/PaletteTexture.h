#pragma once
#include <memory>

namespace graphics {
	class PixelWriteBuffer;
}

struct CachedTexture;

class PaletteTexture
{
public:
	PaletteTexture();

	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture;
	u32 m_tlut_tex;
	std::unique_ptr<graphics::PixelWriteBuffer> m_pbuf;
	u32 m_paletteCRC256;
};

extern PaletteTexture g_paletteTexture;
