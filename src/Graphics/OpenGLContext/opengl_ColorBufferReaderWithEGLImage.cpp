#if defined(EGL) && defined(OS_ANDROID)

#include <GBI.h>
#include <Graphics/Context.h>
#include "opengl_ColorBufferReaderWithEGLImage.h"

using namespace opengl;
using namespace graphics;

ColorBufferReaderWithEGLImage::ColorBufferReaderWithEGLImage(CachedTexture *_pTexture, CachedBindTexture *_bindTexture)
	: graphics::ColorBufferReader(_pTexture)
	, m_bindTexture(_bindTexture)
	, m_image(0)
{
	m_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
	_initBuffers();
}


ColorBufferReaderWithEGLImage::~ColorBufferReaderWithEGLImage()
{
}


void ColorBufferReaderWithEGLImage::_initBuffers()
{
	m_window.reallocate(m_pTexture->realWidth, m_pTexture->realHeight,
		HAL_PIXEL_FORMAT_RGBA_8888, GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_TEXTURE);
	EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };

	if(m_image == 0)
	{
		m_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT,
			EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)m_window.getNativeBuffer(), eglImgAttrs);
	}
}

u8 * ColorBufferReaderWithEGLImage::readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync)
{
	const graphics::FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();
	GLenum colorFormat, colorType, colorFormatBytes;
	if (_size > G_IM_SIZ_8b) {
		colorFormat = GLenum(fbTexFormat.colorFormat);
		colorType = GLenum(fbTexFormat.colorType);
		colorFormatBytes = GLenum(fbTexFormat.colorFormatBytes);
	}
	else {
		colorFormat = GLenum(fbTexFormat.monochromeFormat);
		colorType = GLenum(fbTexFormat.monochromeType);
		colorFormatBytes = GLenum(fbTexFormat.monochromeFormatBytes);
	}

	u8* pixelData = m_pixelData.data();

	if (!_sync) {
		void* ptr;

		m_bindTexture->bind(graphics::Parameter(0), graphics::Parameter(GL_TEXTURE_2D), m_pTexture->name);
		m_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_image);
		m_bindTexture->bind(graphics::Parameter(0), graphics::Parameter(GL_TEXTURE_2D), ObjectHandle());
		int widthBytes = _width*colorFormatBytes;
		int strideBytes = m_pTexture->realWidth * colorFormatBytes;

		m_window.lock(GRALLOC_USAGE_SW_READ_OFTEN, &ptr);

		for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
			memcpy(pixelData + lnIndex*widthBytes, reinterpret_cast<char*>(ptr) + ((lnIndex + _y0)*strideBytes), widthBytes);
		}

		m_window.unlock();
	} else {
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, pixelData);
	}

	return pixelData;
}

void ColorBufferReaderWithEGLImage::cleanUp()
{
}

#endif // EGL
