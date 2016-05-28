#include <assert.h>
#include <algorithm>

#include "ColorBufferToRDRAM.h"
#include "WriteToRDRAM.h"

#include <FrameBuffer.h>
#include <Textures.h>
#include <Config.h>
#include <N64.h>
#include <VI.h>

ColorBufferToRDRAM::ColorBufferToRDRAM()
	: m_FBO(0)
	, m_pTexture(nullptr)
	, m_pCurFrameBuffer(nullptr)
	, m_curIndex(-1)
	, m_frameCount(-1)
	, m_startAddress(-1)
{
	m_PBO[0] = m_PBO[1] = m_PBO[2] = 0;
}

ColorBufferToRDRAM::~ColorBufferToRDRAM()
{
}

ColorBufferToRDRAM & ColorBufferToRDRAM::get()
{
	static ColorBufferToRDRAM cbCopy;
	return cbCopy;
}

#ifndef GLES2

void ColorBufferToRDRAM::init()
{
	// generate a framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

	m_pTexture = textureCache().addFrameBufferTexture();
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 640;
	m_pTexture->realHeight = 580;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight * 4;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTexture->glName, 0);
	// check if everything is OK
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

#ifdef ANDROID
	m_window = new GraphicBuffer(m_pTexture->realWidth, m_pTexture->realHeight,
		PIXEL_FORMAT_RGBA_8888, GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_HW_TEXTURE);

	EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
	m_image = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT,
		EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)m_window->getNativeBuffer(), eglImgAttrs);

	m_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
#else
	// Generate and initialize Pixel Buffer Objects
	glGenBuffers(3, m_PBO);
	for (u32 i = 0; i < 3; ++i) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, NULL, GL_DYNAMIC_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	m_curIndex = 0;
#endif
}

void ColorBufferToRDRAM::destroy() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (m_FBO != 0) {
		glDeleteFramebuffers(1, &m_FBO);
		m_FBO = 0;
	}
	if (m_pTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = NULL;
	}

#ifdef ANDROID
	eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_image);
#else
	glDeleteBuffers(3, m_PBO);
	m_PBO[0] = m_PBO[1] = m_PBO[2] = 0;
#endif
}

bool ColorBufferToRDRAM::_prepareCopy(u32 _startAddress)
{
	if (VI.width == 0 || frameBufferList().getCurrent() == NULL)
		return false;

	OGLVideo & ogl = video();
	const u32 curFrame = ogl.getBuffersSwapCount();
	FrameBuffer * pBuffer = frameBufferList().findBuffer(_startAddress);

	if (pBuffer == NULL || pBuffer->m_isOBScreen)
		return false;

	if (m_frameCount == curFrame && pBuffer == m_pCurFrameBuffer && m_startAddress != _startAddress)
		return true;

	const u32 numPixels = pBuffer->m_width * pBuffer->m_height;
	if (numPixels == 0)
		return false;

	const u32 stride = pBuffer->m_width << pBuffer->m_size >> 1;
	const u32 height = cutHeight(_startAddress, pBuffer->m_height, stride);
	if (height == 0)
		return false;

	m_pCurFrameBuffer = pBuffer;

	if ((config.generalEmulation.hacks & hack_subscreen) != 0 && m_pCurFrameBuffer->m_width == VI.width && m_pCurFrameBuffer->m_height == VI.height) {
		copyWhiteToRDRAM(m_pCurFrameBuffer);
		return false;
	}

	if (config.video.multisampling != 0) {
		m_pCurFrameBuffer->resolveMultisampledTexture();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pCurFrameBuffer->m_resolveFBO);
	}
	else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pCurFrameBuffer->m_FBO);

	if (m_pCurFrameBuffer->m_scaleX > 1.0f) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
		u32 x0 = 0;
		u32 width, height;
		if (config.frameBufferEmulation.nativeResFactor == 0) {
			height = ogl.getHeight();
			const u32 screenWidth = ogl.getWidth();
			width = screenWidth;
			if (ogl.isAdjustScreen()) {
				width = static_cast<u32>(screenWidth*ogl.getAdjustScale());
				x0 = (screenWidth - width) / 2;
			}
		}
		else {
			width = m_pCurFrameBuffer->m_pTexture->realWidth;
			height = m_pCurFrameBuffer->m_pTexture->realHeight;
		}
		glDisable(GL_SCISSOR_TEST);
		glBlitFramebuffer(
			x0, 0, x0 + width, height,
			0, 0, VI.width, VI.height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST
			);
		glEnable(GL_SCISSOR_TEST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
		frameBufferList().setCurrentDrawBuffer();
	}

	m_frameCount = curFrame;
	m_startAddress = _startAddress;
	return true;
}

u8 ColorBufferToRDRAM::_RGBAtoR8(u8 _c) {
	return _c;
}

u16 ColorBufferToRDRAM::_RGBAtoRGBA16(u32 _c) {
	RGBA c;
	c.raw = _c;
	return ((c.r >> 3) << 11) | ((c.g >> 3) << 6) | ((c.b >> 3) << 1) | (c.a == 0 ? 0 : 1);
}

u32 ColorBufferToRDRAM::_RGBAtoRGBA32(u32 _c) {
	RGBA c;
	c.raw = _c;
	return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
}

#ifdef ANDROID
GLubyte* ColorBufferToRDRAM::getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, bool _sync)
{
	GLenum colorFormat, colorType, colorFormatBytes;
	if (m_pCurFrameBuffer->m_size > G_IM_SIZ_8b) {
		colorFormat = fboFormats.colorFormat;
		colorType = fboFormats.colorType;
		colorFormatBytes = fboFormats.colorFormatBytes;
	}
	else {
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

		m_window->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, &ptr);
		memcpy(pixelData, ptr, m_pTexture->realWidth * m_pTexture->realHeight * colorFormatBytes);
		m_window->unlock();

		int widthBytes = _width*colorFormatBytes;
		int strideBytes = m_pTexture->realWidth*colorFormatBytes;
		for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex)
		{
			memmove(pixelData + lnIndex*widthBytes, pixelData + ((lnIndex + _y0)*strideBytes), widthBytes);
		}
	}
	else {
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, pixelData);
	}

	return pixelData;
}

void ColorBufferToRDRAM::cleanUpPixels(GLubyte* pixelData)
{
	free(pixelData);
}

#else

GLubyte* ColorBufferToRDRAM::getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, bool _sync)
{
	GLenum colorFormat, colorType, colorFormatBytes;
	if (m_pCurFrameBuffer->m_size > G_IM_SIZ_8b) {
		colorFormat = fboFormats.colorFormat;
		colorType = fboFormats.colorType;
		colorFormatBytes = fboFormats.colorFormatBytes;
	}
	else {
		colorFormat = fboFormats.monochromeFormat;
		colorType = fboFormats.monochromeType;
		colorFormatBytes = fboFormats.monochromeFormatBytes;
	}

	// If Sync, read pixels from the buffer, copy them to RDRAM.
	// If not Sync, read pixels from the buffer, copy pixels from the previous buffer to RDRAM.
	if (!_sync) {
		m_curIndex ^= 1;
		const u32 nextIndex = m_curIndex ^ 1;
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[m_curIndex]);
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, 0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[nextIndex]);
	}
	else {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[2]);
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, 0);
	}

	GLubyte* pixelData = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _width * _height * colorFormatBytes, GL_MAP_READ_BIT);
	if (pixelData == NULL)
		return NULL;

	return pixelData;
}

void ColorBufferToRDRAM::cleanUpPixels(GLubyte* pixelData)
{
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}
#endif //ANDROID


void ColorBufferToRDRAM::_copy(u32 _startAddress, u32 _endAddress, bool _sync)
{
	const u32 stride = m_pCurFrameBuffer->m_width << m_pCurFrameBuffer->m_size >> 1;
	const u32 max_height = cutHeight(_startAddress, m_pCurFrameBuffer->m_height, stride);

	u32 numPixels = (_endAddress - _startAddress) >> (m_pCurFrameBuffer->m_size - 1);
	if (numPixels / m_pCurFrameBuffer->m_width > max_height) {
		_endAddress = _startAddress + (max_height * stride);
		numPixels = (_endAddress - _startAddress) >> (m_pCurFrameBuffer->m_size - 1);
	}

	const GLsizei width = m_pCurFrameBuffer->m_width;
	const GLint x0 = 0;
	const GLint y0 = max_height - (_endAddress - m_pCurFrameBuffer->m_startAddress) / stride;
	const GLint y1 = max_height - (_startAddress - m_pCurFrameBuffer->m_startAddress) / stride;
	const GLsizei height = std::min(max_height, 1u + y1 - y0);

	GLubyte* pixelData = getPixels(x0, y0, width, height, _sync);

	if (m_pCurFrameBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_src = (u32*)pixelData;
		u32 *ptr_dst = (u32*)(RDRAM + _startAddress);
		writeToRdram<u32, u32>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoRGBA32, 0, 0, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}
	else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_16b) {
		u32 *ptr_src = (u32*)pixelData;
		u16 *ptr_dst = (u16*)(RDRAM + _startAddress);
		writeToRdram<u32, u16>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoRGBA16, 0, 1, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}
	else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_8b) {
		u8 *ptr_src = (u8*)pixelData;
		u8 *ptr_dst = RDRAM + _startAddress;
		writeToRdram<u8, u8>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoR8, 0, 3, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}

	m_pCurFrameBuffer->m_copiedToRdram = true;
	m_pCurFrameBuffer->copyRdram();
	m_pCurFrameBuffer->m_cleared = false;

	cleanUpPixels(pixelData);

	gDP.changed |= CHANGED_SCISSOR;
}

void ColorBufferToRDRAM::copyToRDRAM(u32 _address, bool _sync)
{
	if (!_prepareCopy(_address))
		return;
	const u32 numBytes = (m_pCurFrameBuffer->m_width*m_pCurFrameBuffer->m_height) << m_pCurFrameBuffer->m_size >> 1;
	_copy(m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_startAddress + numBytes, _sync);
}

void ColorBufferToRDRAM::copyChunkToRDRAM(u32 _address)
{
	if (!_prepareCopy(_address))
		return;
	_copy(_address, _address + 0x1000, true);
}

#endif // GLES2

void copyWhiteToRDRAM(FrameBuffer * _pBuffer)
{
	if (_pBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_dst = (u32*)(RDRAM + _pBuffer->m_startAddress);

		for (u32 y = 0; y < VI.height; ++y) {
			for (u32 x = 0; x < VI.width; ++x)
				ptr_dst[x + y*VI.width] = 0xFFFFFFFF;
		}
	}
	else {
		u16 *ptr_dst = (u16*)(RDRAM + _pBuffer->m_startAddress);

		for (u32 y = 0; y < VI.height; ++y) {
			for (u32 x = 0; x < VI.width; ++x) {
				ptr_dst[(x + y*VI.width) ^ 1] = 0xFFFF;
			}
		}
	}
	_pBuffer->m_copiedToRdram = true;
	_pBuffer->copyRdram();

	_pBuffer->m_cleared = false;
}
