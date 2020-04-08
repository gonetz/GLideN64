#include <assert.h>
#include <algorithm>

#include "ColorBufferToRDRAM.h"
#include "WriteToRDRAM.h"

#include <FrameBuffer.h>
#include <FrameBufferInfo.h>
#include <Config.h>
#include <N64.h>
#include <VI.h>
#include "Log.h"
#include "MemoryStatus.h"

/*
#include "ColorBufferToRDRAM_GL.h"
#include "ColorBufferToRDRAM_BufferStorageExt.h"
#elif defined(OS_ANDROID) && defined (GLES2)
#include "ColorBufferToRDRAM_GL.h"
#include "ColorBufferToRDRAM_GLES.h"
#else
#include "ColorBufferToRDRAMStub.h"
#endif
*/

#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include <Graphics/ColorBufferReader.h>
#include <DisplayWindow.h>
#include "BlueNoiseTexture.h"

using namespace graphics;

ColorBufferToRDRAM::ColorBufferToRDRAM()
	: m_FBO(0)
	, m_pTexture(nullptr)
	, m_pCurFrameBuffer(nullptr)
	, m_frameCount(-1)
	, m_startAddress(-1)
	, m_lastBufferWidth(-1)
{
	m_allowedRealWidths[0] = 320;
	m_allowedRealWidths[1] = 480;
	m_allowedRealWidths[2] = 640;
}

u32 ColorBufferToRDRAM::m_blueNoiseIdx = 0;

ColorBufferToRDRAM::~ColorBufferToRDRAM()
{
}

void ColorBufferToRDRAM::init()
{
	m_FBO = gfxContext.createFramebuffer();
}

void ColorBufferToRDRAM::destroy() {
	_destroyFBTexure();

	if (m_FBO.isNotNull()) {
		gfxContext.deleteFramebuffer(m_FBO);
		m_FBO.reset();
	}
}

void ColorBufferToRDRAM::_initFBTexture(void)
{
	const FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();

	m_pTexture = textureCache().addFrameBufferTexture(Context::EglImage ? textureTarget::TEXTURE_EXTERNAL : textureTarget::TEXTURE_2D);
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->size = 2;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	//The actual VI width is not used for texture width because most texture widths
	//cause slowdowns in the glReadPixels call, at least on Android
	m_pTexture->width = m_lastBufferWidth;
	m_pTexture->height = VI_GetMaxBufferHeight(m_lastBufferWidth);
	m_pTexture->textureBytes = m_pTexture->width * m_pTexture->height * fbTexFormat.colorFormatBytes;


	m_bufferReader.reset(gfxContext.createColorBufferReader(m_pTexture));

	// Skip this since texture is initialized in the EGL color buffer reader
	if (!Context::EglImage)
	{
		Context::InitTextureParams params;
		params.handle = m_pTexture->name;
		params.target = textureTarget::TEXTURE_2D;
		params.width = m_pTexture->width;
		params.height = m_pTexture->height;
		params.internalFormat = fbTexFormat.colorInternalFormat;
		params.format = fbTexFormat.colorFormat;
		params.dataType = fbTexFormat.colorType;
		gfxContext.init2DTexture(params);
	}

	{
		Context::TexParameters params;
		params.handle = m_pTexture->name;
		params.target = Context::EglImage ? textureTarget::TEXTURE_EXTERNAL : textureTarget::TEXTURE_2D;
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
		bufTarget.textureTarget = Context::EglImageFramebuffer ? textureTarget::TEXTURE_EXTERNAL : textureTarget::TEXTURE_2D;
		bufTarget.textureHandle = m_pTexture->name;
		gfxContext.addFrameBufferRenderTarget(bufTarget);
	}

	// check if everything is OK
	assert(!gfxContext.isFramebufferError());

	gfxContext.bindFramebuffer(graphics::bufferTarget::DRAW_FRAMEBUFFER, graphics::ObjectHandle::defaultFramebuffer);
}

void ColorBufferToRDRAM::_destroyFBTexure(void)
{
	m_bufferReader.reset();

	if (m_pTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = nullptr;
	}
}

bool ColorBufferToRDRAM::_prepareCopy(u32& _startAddress)
{
	if (VI.width == 0 || frameBufferList().getCurrent() == nullptr)
		return false;

	FrameBuffer * pBuffer = frameBufferList().findBuffer(_startAddress);
	if (pBuffer == nullptr || pBuffer->m_isOBScreen)
		return false;

	DisplayWindow & wnd = dwnd();
	const u32 curFrame = wnd.getBuffersSwapCount();

	_startAddress &= ~0xfff;
	if (_startAddress < pBuffer->m_startAddress)
		_startAddress = pBuffer->m_startAddress;

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
		m_pTexture->width != _getRealWidth(pBuffer->m_width) ||
		m_pTexture->height != VI_GetMaxBufferHeight(_getRealWidth(pBuffer->m_width)))
	{
		_destroyFBTexure();

		m_lastBufferWidth = _getRealWidth(pBuffer->m_width);
		_initFBTexture();
	}

	m_pCurFrameBuffer = pBuffer;

	if ((config.generalEmulation.hacks & hack_subscreen) != 0 && m_pCurFrameBuffer->m_width == VI.width) {
		copyWhiteToRDRAM(m_pCurFrameBuffer);
		return false;
	}

	ObjectHandle readBuffer;

	if (config.video.multisampling != 0) {
		m_pCurFrameBuffer->resolveMultisampledTexture();
		readBuffer = m_pCurFrameBuffer->m_resolveFBO;
	} else {
		readBuffer = m_pCurFrameBuffer->m_FBO;
	}

	if (m_pCurFrameBuffer->m_scale != 1.0f) {
		u32 x0 = 0;
		u32 width;
		if (config.frameBufferEmulation.nativeResFactor == 0) {
			const u32 screenWidth = wnd.getWidth();
			width = screenWidth;
			if (wnd.isAdjustScreen()) {
				width = static_cast<u32>(screenWidth*wnd.getAdjustScale());
				x0 = (screenWidth - width) / 2;
			}
		} else {
			width = m_pCurFrameBuffer->m_pTexture->width;
		}
		u32 height = (u32)(bufferHeight * m_pCurFrameBuffer->m_scale);

		CachedTexture * pInputTexture = m_pCurFrameBuffer->m_pTexture;
		GraphicsDrawer::BlitOrCopyRectParams blitParams;
		blitParams.srcX0 = x0;
		blitParams.srcY0 = 0;
		blitParams.srcX1 = x0 + width;
		blitParams.srcY1 = height;
		blitParams.srcWidth = pInputTexture->width;
		blitParams.srcHeight = pInputTexture->height;
		blitParams.dstX0 = 0;
		blitParams.dstY0 = 0;
		blitParams.dstX1 = m_pCurFrameBuffer->m_width;
		blitParams.dstY1 = bufferHeight;
		blitParams.dstWidth = m_pTexture->width;
		blitParams.dstHeight = m_pTexture->height;
		blitParams.filter = textureParameters::FILTER_NEAREST;
		blitParams.tex[0] = pInputTexture;
		blitParams.combiner = CombinerInfo::get().getTexrectCopyProgram();
		blitParams.readBuffer = readBuffer;
		blitParams.drawBuffer = m_FBO;
		blitParams.mask = blitMask::COLOR_BUFFER;
		wnd.getDrawer().blitOrCopyTexturedRect(blitParams);

		gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, m_FBO);
	} else {
		gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, readBuffer);
	}

	m_frameCount = curFrame;
	m_startAddress = _startAddress;
	return true;
}

u8 ColorBufferToRDRAM::_RGBAtoR8(u8 _c, u32 x, u32 y) {
	return _c;
}

u16 ColorBufferToRDRAM::_RGBAtoRGBA16(u32 _c, u32 x, u32 y) {
	// Precalculated 4x4 bayer matrix values for 5Bit
	static const s32 thresholdMapBayer[4][4] = {
		{ -4, 2, -3, 4 },
		{ 0, -2, 2, -1 },
		{ -3, 3, -4, 3 },
		{ 1, -1, 1, -2 }
	};

	// Precalculated 4x4 magic square matrix values for 5Bit
	static const s32 thresholdMapMagicSquare[4][4] = {
		{ -4, 2, 2, -1 },
		{ 3, -2, -3, 1 },
		{ -3, 0, 4, -2 },
		{ 3, -1, -4, 1 }
	};

	union RGBA c;
	c.raw = _c;
	
	if (config.generalEmulation.enableDitheringPattern == 0 || config.frameBufferEmulation.nativeResFactor != 1) {
		// Apply color dithering
		switch (config.generalEmulation.rdramImageDitheringMode) {
		case Config::BufferDitheringMode::bdmBayer:
		case Config::BufferDitheringMode::bdmMagicSquare:
		{
			s32 threshold = config.generalEmulation.rdramImageDitheringMode == Config::BufferDitheringMode::bdmBayer ?
				thresholdMapBayer[x & 3][y & 3] :
				thresholdMapMagicSquare[x & 3][y & 3];
			c.r = (u8)std::max(std::min((s32)c.r + threshold, 255), 0);
			c.g = (u8)std::max(std::min((s32)c.g + threshold, 255), 0);
			c.b = (u8)std::max(std::min((s32)c.b + threshold, 255), 0);
		}
		break;
		case Config::BufferDitheringMode::bdmBlueNoise:
		{
			const BlueNoiseItem& threshold = blueNoiseTex[m_blueNoiseIdx & 7][x & 63][y & 63];
			c.r = (u8)std::max(std::min((s32)c.r + threshold.r, 255), 0);
			c.g = (u8)std::max(std::min((s32)c.g + threshold.g, 255), 0);
			c.b = (u8)std::max(std::min((s32)c.b + threshold.b, 255), 0);
		}
		break;
		}
	}

	return ((c.r >> 3) << 11) | ((c.g >> 3) << 6) | ((c.b >> 3) << 1) | (c.a == 0 ? 0 : 1);	
}

u32 ColorBufferToRDRAM::_RGBAtoRGBA32(u32 _c, u32 x, u32 y) {
	RGBA c;
	c.raw = _c;
	return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
}

void ColorBufferToRDRAM::_copy(u32 _startAddress, u32 _endAddress, bool _sync)
{
	const u32 stride = m_pCurFrameBuffer->m_width << m_pCurFrameBuffer->m_size >> 1;
	const u32 max_height = std::min((u32)VI_GetMaxBufferHeight(m_pCurFrameBuffer->m_width), cutHeight(_startAddress, m_pCurFrameBuffer->m_height, stride));

	u32 numPixels = (_endAddress - _startAddress) >> (m_pCurFrameBuffer->m_size - 1);
	if (numPixels / m_pCurFrameBuffer->m_width > max_height) {
		_endAddress = _startAddress + (max_height * stride);
		numPixels = (_endAddress - _startAddress) >> (m_pCurFrameBuffer->m_size - 1);
	}

	const u32 width = m_pCurFrameBuffer->m_width;
	const s32 x0 = 0;
	const s32 y0 = (_startAddress - m_pCurFrameBuffer->m_startAddress) / stride;
	const u32 y1 = (_endAddress - m_pCurFrameBuffer->m_startAddress) / stride;
	const u32 height = std::min(max_height, 1u + y1 - y0);

	const u8* pPixels = m_bufferReader->readPixels(x0, y0, width, height, m_pCurFrameBuffer->m_size, _sync);
	frameBufferList().setCurrentDrawBuffer();
	if (pPixels == nullptr)
		return;

	if (m_pCurFrameBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_src = (u32*)pPixels;
		u32 *ptr_dst = (u32*)(RDRAM + _startAddress);

		if (!FBInfo::fbInfo.isSupported() && config.frameBufferEmulation.copyFromRDRAM != 0) {
			memset(ptr_dst, 0, numPixels * 4);
		}

		writeToRdram<u32, u32>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoRGBA32, 0, 0, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	} else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_16b) {
		u32 *ptr_src = (u32*)pPixels;
		u16 *ptr_dst = (u16*)(RDRAM + _startAddress);
		m_blueNoiseIdx++;

		if (!FBInfo::fbInfo.isSupported() && config.frameBufferEmulation.copyFromRDRAM != 0) {
			memset(ptr_dst, 0, numPixels * 2);
		}

		writeToRdram<u32, u16>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoRGBA16, 0, 1, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	} else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_8b) {
		u8 *ptr_src = (u8*)pPixels;
		u8 *ptr_dst = RDRAM + _startAddress;

		if (!FBInfo::fbInfo.isSupported() && config.frameBufferEmulation.copyFromRDRAM != 0) {
			memset(ptr_dst, 0, numPixels);
		}

		writeToRdram<u8, u8>(ptr_src, ptr_dst, &ColorBufferToRDRAM::_RGBAtoR8, 0, 3, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}

	m_pCurFrameBuffer->m_copiedToRdram = true;
	m_pCurFrameBuffer->copyRdram();
	m_pCurFrameBuffer->m_cleared = false;

	m_bufferReader->cleanUp();

	gDP.changed |= CHANGED_SCISSOR;
}

u32 ColorBufferToRDRAM::_getRealWidth(u32 _viWidth)
{
	u32 index = 0;
	const u32 maxIndex = static_cast<u32>(m_allowedRealWidths.size()) - 1;
	while (index < maxIndex && _viWidth > m_allowedRealWidths[index])
	{
		++index;
	}

	return m_allowedRealWidths[index];
}

void ColorBufferToRDRAM::copyToRDRAM(u32 _address, bool _sync)
{
	if (!isMemoryWritable(RDRAM + _address, gDP.colorImage.width << gDP.colorImage.size >> 1))
		return;
	if (!_prepareCopy(_address))
		return;
	if (config.frameBufferEmulation.copyToRDRAM == Config::CopyToRDRAM::ctDisable)
		return;

	const u32 numBytes = (m_pCurFrameBuffer->m_width*m_pCurFrameBuffer->m_height) << m_pCurFrameBuffer->m_size >> 1;
	_copy(m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_startAddress + numBytes, _sync);
}

void ColorBufferToRDRAM::copyChunkToRDRAM(u32 _startAddress)
{
	const u32 endAddress = (_startAddress & ~0xfff) + 0x1000;

	if (!isMemoryWritable(RDRAM + _startAddress, endAddress - _startAddress))
		return;
	if (!_prepareCopy(_startAddress))
		return;
	_copy(_startAddress, endAddress, true);
}


ColorBufferToRDRAM & ColorBufferToRDRAM::get()
{
	static ColorBufferToRDRAM cbCopy;
	return cbCopy;
}

void copyWhiteToRDRAM(FrameBuffer * _pBuffer)
{
	if (_pBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_dst = (u32*)(RDRAM + _pBuffer->m_startAddress);

		for (u32 y = 0; y < VI.height; ++y) {
			for (u32 x = 0; x < VI.width; ++x)
				ptr_dst[x + y*VI.width] = 0xFFFFFFFF;
		}
	} else {
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
