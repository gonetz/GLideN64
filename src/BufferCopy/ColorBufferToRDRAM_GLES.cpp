#include <algorithm>
#include "ColorBufferToRDRAM.h"
#include "ColorBufferToRDRAM_GLES.h"

#include <Textures.h>
#include <FBOTextureFormats.h>

#include <ui/GraphicBuffer.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

ColorBufferToRDRAM_GLES::ColorBufferToRDRAM_GLES()
		: ColorBufferToRDRAM(),
		  m_window(nullptr),
		  m_image(0)
{
}

void ColorBufferToRDRAM_GLES::_init()
{
	m_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

	m_window = new GraphicBuffer();
}

void ColorBufferToRDRAM_GLES::_destroy()
{
}

void ColorBufferToRDRAM_GLES::_initBuffers(void)
{
	m_window->reallocate(m_pTexture->realWidth, m_pTexture->realHeight,
		PIXEL_FORMAT_RGBA_8888, GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_HW_TEXTURE);
	EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };

	if(m_image == 0)
	{
		m_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT,
			EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)m_window->getNativeBuffer(), eglImgAttrs);
	}
}

bool ColorBufferToRDRAM_GLES::_readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)
{
	GLenum colorFormat, colorType, colorFormatBytes;
	if (_size > G_IM_SIZ_8b) {
		colorFormat = fboFormats.colorFormat;
		colorType = fboFormats.colorType;
		colorFormatBytes = fboFormats.colorFormatBytes;
	} else {
		colorFormat = fboFormats.monochromeFormat;
		colorType = fboFormats.monochromeType;
		colorFormatBytes = fboFormats.monochromeFormatBytes;
	}

	GLubyte* pixelData = m_pixelData.data();

	if (!_sync) {
		void* ptr;

		glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
		m_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_image);
		glBindTexture(GL_TEXTURE_2D, 0);
		int widthBytes = _width*colorFormatBytes;
		int strideBytes = m_pTexture->realWidth * colorFormatBytes;

		m_window->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, &ptr);

		for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
			memcpy(pixelData + lnIndex*widthBytes, reinterpret_cast<char*>(ptr) + ((lnIndex + _y0)*strideBytes), widthBytes);
		}

		m_window->unlock();


	} else {
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, pixelData);
	}

	return true;
}

void ColorBufferToRDRAM_GLES::_cleanUp()
{
}
