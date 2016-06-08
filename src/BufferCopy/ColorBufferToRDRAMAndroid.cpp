#include <algorithm>
#include "ColorBufferToRDRAM.h"

#include <Textures.h>

#include <ui/GraphicBuffer.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, EGLImageKHR image);
typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) (GLenum target, EGLImageKHR image);

using namespace android;

class ColorBufferToRDRAMAndroid : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAMAndroid();
	~ColorBufferToRDRAMAndroid() {};

private:
	void _init() override;
	void _destroy() override;
	GLubyte* _getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)  override;
	void _cleanUpPixels(GLubyte* pixelData)  override;
	void _initBuffers(void) override;
	void _destroyBuffers(void) override;

	GraphicBuffer* m_window;
	EGLImageKHR m_image;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES;
};

ColorBufferToRDRAM & ColorBufferToRDRAM::get()
{
	static ColorBufferToRDRAMAndroid cbCopy;
	return cbCopy;
}

ColorBufferToRDRAMAndroid::ColorBufferToRDRAMAndroid() 
	: ColorBufferToRDRAM(),
	m_window(nullptr),
	m_image(0)
{
}

void ColorBufferToRDRAMAndroid::_init()
{
	m_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

	m_window = new GraphicBuffer();
}

void ColorBufferToRDRAMAndroid::_destroy()
{
}

void ColorBufferToRDRAMAndroid::_initBuffers(void)
{
	m_window->reallocate(m_pTexture->realWidth, m_pTexture->realHeight,
		PIXEL_FORMAT_RGBA_8888, GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_HW_TEXTURE);
	EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
	m_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT,
		EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)m_window->getNativeBuffer(), eglImgAttrs);
}

void ColorBufferToRDRAMAndroid::_destroyBuffers(void)
{
	if(m_image != 0)
	{
	    eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_image);
	    m_image = 0;
	}
}

GLubyte* ColorBufferToRDRAMAndroid::_getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)
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

	GLubyte* pixelData = (GLubyte*)malloc(m_pTexture->realWidth * m_pTexture->realHeight * colorFormatBytes);

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

	return pixelData;
}

void ColorBufferToRDRAMAndroid::_cleanUpPixels(GLubyte* pixelData)
{
	free(pixelData);
}
