#include <Graphics/Context.h>
#include "opengl_ColorBufferReaderWithReadPixels.h"
#include <algorithm>

using namespace graphics;
using namespace opengl;

ColorBufferReaderWithReadPixels::ColorBufferReaderWithReadPixels(CachedTexture *_pTexture)
	: ColorBufferReader(_pTexture)
{

}

u8 * ColorBufferReaderWithReadPixels::readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync)
{
	const graphics::FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();
	GLenum colorFormat, colorType, colorFormatBytes;
	if (_size > G_IM_SIZ_8b) {
		colorFormat = GLenum(fbTexFormat.colorFormat);
		colorType = GLenum(fbTexFormat.colorType);
		colorFormatBytes = GLenum(fbTexFormat.colorFormatBytes);
	} else {
		colorFormat = GLenum(fbTexFormat.monochromeFormat);
		colorType = GLenum(fbTexFormat.monochromeType);
		colorFormatBytes = GLenum(fbTexFormat.monochromeFormatBytes);
	}

	// No async pixel buffer copies are supported in this class, this is a last resort fallback
	auto pixelData = std::unique_ptr<GLubyte[]>(new GLubyte[m_pTexture->realWidth * _height * colorFormatBytes]) ;
	glReadPixels(_x0, _y0, m_pTexture->realWidth, _height, colorFormat, colorType, pixelData.get());

	int widthBytes = _width*colorFormatBytes;
	int strideBytes = m_pTexture->realWidth * colorFormatBytes;

	for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
		std::copy_n(pixelData.get() + (lnIndex*strideBytes), widthBytes, m_pixelData.data() + lnIndex*widthBytes);
	}
	return m_pixelData.data();
}

void ColorBufferReaderWithReadPixels::cleanUp()
{
}
