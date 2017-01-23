#include <assert.h>
#include <algorithm>

#include "ColorBufferToRDRAM.h"
#include "WriteToRDRAM.h"

#include <FrameBuffer.h>
#include <Config.h>
#include <N64.h>
#include <VI.h>
#include "Log.h"
#ifndef GLES2
#include "ColorBufferToRDRAM_GL.h"
#include "ColorBufferToRDRAM_BufferStorageExt.h"
#elif defined(ANDROID) && defined (GLES2)
#include "ColorBufferToRDRAM_GLES.h"
#else
#include "ColorBufferToRDRAMStub.h"
#endif
#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include <DisplayWindow.h>

using namespace graphics;

ColorBufferToRDRAM::ColorBufferToRDRAM()
	: m_FBO(0)
	, m_pTexture(nullptr)
	, m_pCurFrameBuffer(nullptr)
	, m_frameCount(-1)
	, m_startAddress(-1)
	, m_lastBufferWidth(-1)
	, m_lastBufferHeight(-1)
{
	m_allowedRealWidths[0] = 320;
	m_allowedRealWidths[1] = 480;
	m_allowedRealWidths[2] = 640;
}

ColorBufferToRDRAM::~ColorBufferToRDRAM()
{
}

void ColorBufferToRDRAM::init()
{
	// generate a framebuffer
//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//	glGenFramebuffers(1, &m_FBO);

	m_FBO = gfxContext.createFramebuffer();
	_init();
}

void ColorBufferToRDRAM::destroy() {
	_destroyFBTexure();

//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (m_FBO.isNotNull()) {
		gfxContext.deleteFramebuffer(m_FBO);
//		glDeleteFramebuffers(1, &m_FBO);
		m_FBO.reset();
	}
}

void ColorBufferToRDRAM::_initFBTexture(void)
{
	m_pTexture = textureCache().addFrameBufferTexture(false);
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	//The actual VI width is not used for texture width because most texture widths
	//cause slowdowns in the glReadPixels call, at least on Android
	m_pTexture->realWidth = _getRealWidth(m_lastBufferWidth);
	m_pTexture->realHeight = m_lastBufferHeight;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight * 4;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);

	{
		const FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();
		Context::InitTextureParams params;
		params.handle = m_pTexture->name;
		params.width = m_pTexture->realWidth;
		params.height = m_pTexture->realHeight;
		params.internalFormat = fbTexFormat.colorInternalFormat;
		params.format = fbTexFormat.colorFormat;
		params.dataType = fbTexFormat.colorType;
		gfxContext.init2DTexture(params);
	}
	{
		Context::TexParameters params;
		params.handle = m_pTexture->name;
		params.target = target::TEXTURE_2D;
		params.textureUnitIndex = textureIndices::Tex[0];
		params.minFilter = textureParameters::FILTER_LINEAR;
		params.magFilter = textureParameters::FILTER_LINEAR;
		gfxContext.setTextureParameters(params);
	}
	{
		Context::FrameBufferRenderTarget bufTarget;
		bufTarget.bufferHandle = ObjectHandle(m_FBO);
		bufTarget.bufferTarget = bufferTarget::DRAW_FRAMEBUFFER;
		bufTarget.attachment = bufferAttachment::COLOR_ATTACHMENT0;
		bufTarget.textureTarget = target::TEXTURE_2D;
		bufTarget.textureHandle = m_pTexture->name;
		gfxContext.addFrameBufferRenderTarget(bufTarget);
	}

	// check if everything is OK
	assert(checkFBO());

	gfxContext.bindFramebuffer(graphics::bufferTarget::DRAW_FRAMEBUFFER, graphics::ObjectHandle());
	//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	_initBuffers();
}

void ColorBufferToRDRAM::_destroyFBTexure(void)
{
	_destroyBuffers();

	if (m_pTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = nullptr;
	}
}

bool ColorBufferToRDRAM::_prepareCopy(u32 _startAddress)
{
	if (VI.width == 0 || frameBufferList().getCurrent() == nullptr)
		return false;

	DisplayWindow & wnd = dwnd();
	const u32 curFrame = wnd.getBuffersSwapCount();
	FrameBuffer * pBuffer = frameBufferList().findBuffer(_startAddress);

	if (pBuffer == nullptr || pBuffer->m_isOBScreen)
		return false;

	if (m_frameCount == curFrame && pBuffer == m_pCurFrameBuffer && m_startAddress != _startAddress)
		return true;

	const u32 numPixels = pBuffer->m_width * pBuffer->m_height;
	if (numPixels == 0)
		return false;

	const u32 stride = pBuffer->m_width << pBuffer->m_size >> 1;
	const u32 bufferHeight = cutHeight(_startAddress, pBuffer->m_height, stride);
	if (bufferHeight == 0)
		return false;

	if(m_pTexture == nullptr ||
		(m_lastBufferWidth != pBuffer->m_width || m_lastBufferHeight != pBuffer->m_height))
	{
		_destroyFBTexure();

		m_lastBufferWidth = pBuffer->m_width;
		m_lastBufferHeight = pBuffer->m_height;
		_initFBTexture();
		m_pixelData.resize(m_pTexture->realWidth * m_pTexture->realHeight * gfxContext.getFramebufferTextureFormats().colorFormatBytes);
	}

	m_pCurFrameBuffer = pBuffer;

	if ((config.generalEmulation.hacks & hack_subscreen) != 0 && m_pCurFrameBuffer->m_width == VI.width && m_pCurFrameBuffer->m_height == VI.height) {
		copyWhiteToRDRAM(m_pCurFrameBuffer);
		return false;
	}

	ObjectHandle readBuffer;

	if (config.video.multisampling != 0) {
		m_pCurFrameBuffer->resolveMultisampledTexture();
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pCurFrameBuffer->m_resolveFBO);
		readBuffer = ObjectHandle(m_pCurFrameBuffer->m_resolveFBO);
	} else {
		//		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pCurFrameBuffer->m_FBO);
		readBuffer = ObjectHandle(m_pCurFrameBuffer->m_FBO);
	}


	if (m_pCurFrameBuffer->m_scaleX != 1.0f || m_pCurFrameBuffer->m_scaleY != 1.0f) {
//		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
		ObjectHandle drawBuffer(m_FBO);

		u32 x0 = 0;
		u32 width, height;
		if (config.frameBufferEmulation.nativeResFactor == 0) {
			height = wnd.getHeight();
			const u32 screenWidth = wnd.getWidth();
			width = screenWidth;
			if (wnd.isAdjustScreen()) {
				width = static_cast<u32>(screenWidth*wnd.getAdjustScale());
				x0 = (screenWidth - width) / 2;
			}
		}
		else {
			width = m_pCurFrameBuffer->m_pTexture->realWidth;
			height = m_pCurFrameBuffer->m_pTexture->realHeight;
		}

		CachedTexture * pInputTexture = frameBufferList().getCurrent()->m_pTexture;
		GraphicsDrawer::BlitOrCopyRectParams blitParams;
		blitParams.srcX0 = x0;
		blitParams.srcY0 = 0;
		blitParams.srcX1 = x0 + width;
		blitParams.srcY1 = height;
		blitParams.srcWidth = pInputTexture->realWidth;
		blitParams.srcHeight = pInputTexture->realHeight;
		blitParams.dstX0 = 0;
		blitParams.dstY0 = 0;
		blitParams.dstX1 = VI.width;
		blitParams.dstY1 = VI.height;
		blitParams.dstWidth = m_pTexture->realWidth;
		blitParams.dstHeight = m_pTexture->realHeight;
		blitParams.filter = textureParameters::FILTER_NEAREST;
		blitParams.tex[0] = pInputTexture;
		blitParams.combiner = CombinerInfo::get().getTexrectCopyProgram();
		blitParams.readBuffer = readBuffer;
		blitParams.drawBuffer = drawBuffer;
		blitParams.mask = blitMask::COLOR_BUFFER;
		wnd.getDrawer().blitOrCopyTexturedRect(blitParams);

		gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, ObjectHandle(m_FBO));
//		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
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

void ColorBufferToRDRAM::_copy(u32 _startAddress, u32 _endAddress, bool _sync)
{
	const u32 stride = m_pCurFrameBuffer->m_width << m_pCurFrameBuffer->m_size >> 1;
	const u32 max_height = std::min(480U, cutHeight(_startAddress, m_pCurFrameBuffer->m_height, stride));

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

	const bool pixelsRead = _readPixels(x0, y0, width, height, m_pCurFrameBuffer->m_size, _sync);
	frameBufferList().setCurrentDrawBuffer();
	if (!pixelsRead)
		return;

	if (m_pCurFrameBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_src = (u32*)m_pixelData.data();
		u32 *ptr_dst = (u32*)(RDRAM + _startAddress);
		writeToRdram<u32, u32>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoRGBA32, 0, 0, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}
	else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_16b) {
		u32 *ptr_src = (u32*)m_pixelData.data();
		u16 *ptr_dst = (u16*)(RDRAM + _startAddress);
		writeToRdram<u32, u16>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoRGBA16, 0, 1, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}
	else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_8b) {
		u8 *ptr_src = (u8*)m_pixelData.data();
		u8 *ptr_dst = RDRAM + _startAddress;
		writeToRdram<u8, u8>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoR8, 0, 3, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}

	m_pCurFrameBuffer->m_copiedToRdram = true;
	m_pCurFrameBuffer->copyRdram();
	m_pCurFrameBuffer->m_cleared = false;

	_cleanUp();

	gDP.changed |= CHANGED_SCISSOR;
}

u32 ColorBufferToRDRAM::_getRealWidth(u32 _viWidth)
{
	u32 index = 0;
	while(index < m_allowedRealWidths.size() && _viWidth > m_allowedRealWidths[index])
	{
		++index;
	}

	return m_allowedRealWidths[index];
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


ColorBufferToRDRAM & ColorBufferToRDRAM::get()
{
#ifndef GLES2

	static bool supportsBufferStorage = OGLVideo::isExtensionSupported("GL_EXT_buffer_storage") ||
		OGLVideo::isExtensionSupported("GL_ARB_buffer_storage");

	if (supportsBufferStorage) {
		static ColorBufferToRDRAM_BufferStorageExt cbCopy;
		return cbCopy;
	} else {
		static ColorBufferToRDRAM_GL cbCopy;
		return cbCopy;
	}

#elif defined(ANDROID) && defined (GLES2)
	static ColorBufferToRDRAM_GLES cbCopy;
	return cbCopy;
#else
	static ColorBufferToRDRAMStub cbCopy;
	return cbCopy;
#endif
}

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
