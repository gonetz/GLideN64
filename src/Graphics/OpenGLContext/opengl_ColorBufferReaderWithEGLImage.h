#pragma once
#ifdef EGL
#include <Graphics/ColorBufferReader.h>
#include "opengl_CachedFunctions.h"

#include <Graphics/OpenGLContext/GraphicBufferPrivateApi/GraphicBuffer.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef void (APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, EGLImageKHR image);


namespace opengl {

class ColorBufferReaderWithEGLImage : public graphics::ColorBufferReader
{
public:
	ColorBufferReaderWithEGLImage(CachedTexture * _pTexture,
								  CachedBindTexture * _bindTexture);
	~ColorBufferReaderWithEGLImage();

	u8 * readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync) override;
	void cleanUp() override;

private:
	void _initBuffers();

	CachedBindTexture * m_bindTexture;
	GraphicBuffer m_window{};
	EGLImageKHR m_image;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES;
};

}

#endif //EGL
