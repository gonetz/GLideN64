#include <assert.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gDP.h"
#include "VI.h"
#include "Textures.h"
#include "Combiner.h"
#include "Types.h"
#include "Config.h"
#include "Debug.h"
#include "PostProcessor.h"
#include "FrameBufferInfo.h"
#include "Log.h"

#include "BufferCopy/ColorBufferToRDRAM.h"
#include "BufferCopy/DepthBufferToRDRAM.h"
#include "BufferCopy/RDRAMtoColorBuffer.h"

#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include "DisplayWindow.h"

using namespace std;
using namespace graphics;

FrameBuffer::FrameBuffer() :
	m_startAddress(0), m_endAddress(0), m_size(0), m_width(0), m_height(0), m_validityChecked(0),
	m_scaleX(0), m_scaleY(0),
	m_copiedToRdram(false), m_fingerprint(false), m_cleared(false), m_changed(false), m_cfb(false),
	m_isDepthBuffer(false), m_isPauseScreen(false), m_isOBScreen(false),
	m_needHeightCorrection(false), m_readable(false),
	m_loadType(LOADTYPE_BLOCK), m_pDepthBuffer(nullptr),
	m_pResolveTexture(nullptr), m_resolved(false),
	m_pSubTexture(nullptr)
{
	m_loadTileOrigin.uls = m_loadTileOrigin.ult = 0;
	m_pTexture = textureCache().addFrameBufferTexture(config.video.multisampling != 0);
	m_FBO = gfxContext.createFramebuffer();
}

FrameBuffer::~FrameBuffer()
{
	gfxContext.deleteFramebuffer(m_FBO);
	gfxContext.deleteFramebuffer(m_resolveFBO);
	gfxContext.deleteFramebuffer(m_SubFBO);

	textureCache().removeFrameBufferTexture(m_pTexture);
	textureCache().removeFrameBufferTexture(m_pResolveTexture);
	textureCache().removeFrameBufferTexture(m_pSubTexture);
}

void FrameBuffer::_initTexture(u16 _width, u16 _height, u16 _format, u16 _size, CachedTexture *_pTexture)
{
	const FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();

	_pTexture->width = (u32)(_width * m_scaleX);
	_pTexture->height = (u32)(_height * m_scaleY);
	_pTexture->format = _format;
	_pTexture->size = _size;
	_pTexture->clampS = 1;
	_pTexture->clampT = 1;
	_pTexture->address = m_startAddress;
	_pTexture->clampWidth = _width;
	_pTexture->clampHeight = _height;
	_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	_pTexture->maskS = 0;
	_pTexture->maskT = 0;
	_pTexture->mirrorS = 0;
	_pTexture->mirrorT = 0;
	_pTexture->realWidth = _pTexture->width;
	_pTexture->realHeight = _pTexture->height;
	_pTexture->textureBytes = _pTexture->realWidth * _pTexture->realHeight;
	if (_size > G_IM_SIZ_8b)
		_pTexture->textureBytes *= fbTexFormats.colorFormatBytes;
	else
		_pTexture->textureBytes *= fbTexFormats.monochromeFormatBytes;
	textureCache().addFrameBufferTextureSize(_pTexture->textureBytes);
}

void FrameBuffer::_setAndAttachTexture(ObjectHandle _fbo, CachedTexture *_pTexture, u32 _t, bool _multisampling)
{
	const FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();
	Context::InitTextureParams initParams;
	initParams.handle = _pTexture->name;
	initParams.textureUnitIndex = textureIndices::Tex[_t];
	if (_multisampling)
		initParams.msaaLevel = config.video.multisampling;
	initParams.width = _pTexture->realWidth;
	initParams.height = _pTexture->realHeight;
	if (_pTexture->size > G_IM_SIZ_8b) {
		initParams.internalFormat = fbTexFormat.colorInternalFormat;
		initParams.format = fbTexFormat.colorFormat;
		initParams.dataType = fbTexFormat.colorType;
	} else {
		initParams.internalFormat = fbTexFormat.monochromeInternalFormat;
		initParams.format = fbTexFormat.monochromeFormat;
		initParams.dataType = fbTexFormat.monochromeType;
	}
	gfxContext.init2DTexture(initParams);

	if (!_multisampling) {
		Context::TexParameters texParams;
		texParams.handle = _pTexture->name;
		texParams.target = textureTarget::TEXTURE_2D;
		texParams.textureUnitIndex = textureIndices::Tex[_t];
		texParams.minFilter = textureParameters::FILTER_NEAREST;
		texParams.magFilter = textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(texParams);
	}

	Context::FrameBufferRenderTarget bufTarget;
	bufTarget.bufferHandle = _fbo;
	bufTarget.bufferTarget = bufferTarget::FRAMEBUFFER;
	bufTarget.attachment = bufferAttachment::COLOR_ATTACHMENT0;
	bufTarget.textureTarget = _multisampling ? textureTarget::TEXTURE_2D_MULTISAMPLE : textureTarget::TEXTURE_2D;
	bufTarget.textureHandle = _pTexture->name;
	gfxContext.addFrameBufferRenderTarget(bufTarget);
	assert(!gfxContext.isFramebufferError());
}

bool FrameBuffer::_isMarioTennisScoreboard() const
{
	if ((config.generalEmulation.hacks&hack_scoreboard) != 0) {
		if (VI.PAL)
			return m_startAddress == 0x13b480 || m_startAddress == 0x26a530;
		else
			return m_startAddress == 0x13ba50 || m_startAddress == 0x264430;
	}
	return (config.generalEmulation.hacks&hack_scoreboardJ) != 0 && (m_startAddress == 0x134080 || m_startAddress == 0x1332f8);
}

bool FrameBuffer::isAuxiliary() const
{
	return m_width != VI.width;
}

void FrameBuffer::init(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb)
{
	DisplayWindow & wnd = dwnd();
	m_startAddress = _address;
	m_endAddress = _address + ((_width * _height) << _size >> 1) - 1;
	m_width = _width;
	m_height = _height;
	m_size = _size;
	if (isAuxiliary() && config.frameBufferEmulation.copyAuxToRDRAM != 0) {
		m_scaleX = 1.0f;
		m_scaleY = 1.0f;
	} else if (config.frameBufferEmulation.nativeResFactor != 0 && config.frameBufferEmulation.enable != 0) {
		m_scaleX = m_scaleY = static_cast<float>(config.frameBufferEmulation.nativeResFactor);
	} else {
		m_scaleX = wnd.getScaleX();
		m_scaleY = wnd.getScaleY();
	}
	m_cfb = _cfb;
	m_needHeightCorrection = _width != VI.width && _width != *REG.VI_WIDTH;
	m_cleared = false;
	m_fingerprint = false;

	_initTexture(_width, _height, _format, _size, m_pTexture);

	if (config.video.multisampling != 0) {
		_setAndAttachTexture(m_FBO, m_pTexture, 0, true);
		m_pTexture->frameBufferTexture = CachedTexture::fbMultiSample;

		m_pResolveTexture = textureCache().addFrameBufferTexture(false);
		_initTexture(_width, _height, _format, _size, m_pResolveTexture);
		m_resolveFBO = gfxContext.createFramebuffer();
		_setAndAttachTexture(m_resolveFBO, m_pResolveTexture, 0, false);
		assert(!gfxContext.isFramebufferError());

		gfxContext.bindFramebuffer(bufferTarget::FRAMEBUFFER, m_FBO);
	} else
		_setAndAttachTexture(m_FBO, m_pTexture, 0, false);

	wnd.getDrawer().clearColorBuffer(nullptr);
}

void FrameBuffer::reinit(u16 _height)
{
	const u16 format = m_pTexture->format;
	textureCache().removeFrameBufferTexture(m_pTexture);

	gfxContext.deleteFramebuffer(ObjectHandle(m_resolveFBO));

	textureCache().removeFrameBufferTexture(m_pResolveTexture);
	m_pTexture = textureCache().addFrameBufferTexture(config.video.multisampling != 0);
	init(m_startAddress, format, m_size, m_width, _height, m_cfb);
}

inline
u32 _cutHeight(u32 _address, u32 _height, u32 _stride)
{
	if (_address > RDRAMSize)
		return 0;
	if (_address + _stride * _height > (RDRAMSize + 1))
		return (RDRAMSize + 1 - _address) / _stride;
	return _height;
}

u32 cutHeight(u32 _address, u32 _height, u32 _stride)
{
	return _cutHeight(_address, _height, _stride);
}

void FrameBuffer::setBufferClearParams(u32 _fillcolor, s32 _ulx, s32 _uly, s32 _lrx, s32 _lry)
{
	m_cleared = true;
	m_clearParams.fillcolor = _fillcolor;
	m_clearParams.ulx = _ulx;
	m_clearParams.lrx = _lrx;
	m_clearParams.uly = _uly;
	m_clearParams.lry = _lry;
}

void FrameBuffer::copyRdram()
{
	const u32 stride = m_width << m_size >> 1;
	const u32 height = _cutHeight(m_startAddress, m_height, stride);
	if (height == 0)
		return;
	const u32 dataSize = stride * height;

	// Auxiliary frame buffer
	if (isAuxiliary() && config.frameBufferEmulation.copyAuxToRDRAM == 0) {
		// Write small amount of data to the start of the buffer.
		// This is necessary for auxilary buffers: game can restore content of RDRAM when buffer is not needed anymore
		// Thus content of RDRAM on moment of buffer creation will be the same as when buffer becomes obsolete.
		// Validity check will see that the RDRAM is the same and thus the buffer is valid, which is false.
		const u32 twoPercent = max(4U, dataSize / 200);
		u32 start = m_startAddress >> 2;
		u32 * pData = (u32*)RDRAM;
		for (u32 i = 0; i < twoPercent; ++i) {
			if (i < 4)
				pData[start++] = fingerprint[i];
			else
				pData[start++] = 0;
		}
		m_cleared = false;
		m_fingerprint = true;
		return;
	}
	m_RdramCopy.resize(dataSize);
	memcpy(m_RdramCopy.data(), RDRAM + m_startAddress, dataSize);
}

bool FrameBuffer::isValid(bool _forceCheck) const
{
	if (!_forceCheck) {
		if (m_validityChecked == dwnd().getBuffersSwapCount())
			return true; // Already checked
		m_validityChecked = dwnd().getBuffersSwapCount();
	}

	const u32 * const pData = (const u32*)RDRAM;

	if (m_cleared) {
		const u32 testColor = m_clearParams.fillcolor & 0xFFFEFFFE;
		const u32 stride = m_width << m_size >> 1;
		const u32 lry = _cutHeight(m_startAddress, m_clearParams.lry, stride);
		if (lry == 0)
			return false;

		const u32 ci_width_in_dwords = m_width >> (3 - m_size);
		const u32 start = (m_startAddress >> 2) + m_clearParams.uly * ci_width_in_dwords;
		const u32 * dst = pData + start;
		u32 wrongPixels = 0;
		for (u32 y = m_clearParams.uly; y < lry; ++y) {
			for (u32 x = m_clearParams.ulx; x < m_clearParams.lrx; ++x) {
				if ((dst[x] & 0xFFFEFFFE) != testColor)
					++wrongPixels;
			}
			dst += ci_width_in_dwords;
		}
		return wrongPixels < (m_endAddress - m_startAddress) / 400; // threshold level 1% of dwords
	} else if (m_fingerprint) {
			//check if our fingerprint is still there
			u32 start = m_startAddress >> 2;
			for (u32 i = 0; i < 4; ++i)
				if ((pData[start++] & 0xFFFEFFFE) != (fingerprint[i] & 0xFFFEFFFE))
					return false;
			return true;
	} else if (!m_RdramCopy.empty()) {
		const u32 * const pCopy = (const u32*)m_RdramCopy.data();
		const u32 size = m_RdramCopy.size();
		const u32 size_dwords = size >> 2;
		u32 start = m_startAddress >> 2;
		u32 wrongPixels = 0;
		for (u32 i = 0; i < size_dwords; ++i) {
			if ((pData[start++] & 0xFFFEFFFE) != (pCopy[i] & 0xFFFEFFFE))
				++wrongPixels;
		}
		return wrongPixels < size / 400; // threshold level 1% of dwords
	}
	return true; // No data to decide
}

void FrameBuffer::resolveMultisampledTexture(bool _bForce)
{
	if (!Context::multisampling)
		return;

	if (m_resolved && !_bForce)
		return;

	Context::BlitFramebuffersParams blitParams;
	blitParams.readBuffer = m_FBO;
	blitParams.drawBuffer = m_resolveFBO;
	blitParams.srcX0 = 0;
	blitParams.srcY0 = 0;
	blitParams.srcX1 = m_pTexture->realWidth;
	blitParams.srcY1 = m_pTexture->realHeight;
	blitParams.dstX0 = 0;
	blitParams.dstY0 = 0;
	blitParams.dstX1 = m_pResolveTexture->realWidth;
	blitParams.dstY1 = m_pResolveTexture->realHeight;
	blitParams.mask = blitMask::COLOR_BUFFER;
	blitParams.filter = textureParameters::FILTER_NEAREST;

	gfxContext.blitFramebuffers(blitParams);

	gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, ObjectHandle::null);

	frameBufferList().setCurrentDrawBuffer();
	m_resolved = true;
}

bool FrameBuffer::_initSubTexture(u32 _t)
{
	if (!m_SubFBO.isNotNull())
		m_SubFBO = gfxContext.createFramebuffer();

	gDPTile * pTile = gSP.textureTile[_t];
	if (pTile->lrs < pTile->uls || pTile->lrt < pTile->ult)
		return false;
	const u32 width = pTile->lrs - pTile->uls + 1;
	const u32 height = pTile->lrt - pTile->ult + 1;

	if (m_pSubTexture != nullptr) {
		if (m_pSubTexture->size == m_pTexture->size &&
			m_pSubTexture->clampWidth == width &&
			m_pSubTexture->clampHeight == height)
			return true;
		textureCache().removeFrameBufferTexture(m_pSubTexture);
	}

	m_pSubTexture = textureCache().addFrameBufferTexture(false);
	_initTexture(width, height, m_pTexture->format, m_pTexture->size, m_pSubTexture);

	m_pSubTexture->clampS = pTile->clamps;
	m_pSubTexture->clampT = pTile->clampt;
	m_pSubTexture->offsetS = 0.0f;
	m_pSubTexture->offsetT = m_pSubTexture->clampHeight;


	_setAndAttachTexture(m_SubFBO, m_pSubTexture, _t, false);

	return true;
}

CachedTexture * FrameBuffer::_getSubTexture(u32 _t)
{
	if (!gfxContext.isSupported(SpecialFeatures::BlitFramebuffer))
		return m_pTexture;

	if (!_initSubTexture(_t))
		return m_pTexture;

	s32 x0 = (s32)(m_pTexture->offsetS * m_scaleX);
	s32 y0 = (s32)(m_pTexture->offsetT * m_scaleY) - m_pSubTexture->realHeight;
	s32 copyWidth = m_pSubTexture->realWidth;
	if (x0 + copyWidth > m_pTexture->realWidth)
		copyWidth = m_pTexture->realWidth - x0;
	s32 copyHeight = m_pSubTexture->realHeight;
	if (y0 + copyHeight > m_pTexture->realHeight)
		copyHeight = m_pTexture->realHeight - y0;

	ObjectHandle readFBO = m_FBO;
	if (gfxContext.isSupported(SpecialFeatures::WeakBlitFramebuffer) &&
			m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
		resolveMultisampledTexture(true);
		readFBO = m_resolveFBO;
	}

	Context::BlitFramebuffersParams blitParams;
	blitParams.readBuffer = readFBO;
	blitParams.drawBuffer = m_SubFBO;
	blitParams.srcX0 = x0;
	blitParams.srcY0 = y0;
	blitParams.srcX1 = x0 + copyWidth;
	blitParams.srcY1 = y0 + copyHeight;
	blitParams.dstX0 = 0;
	blitParams.dstY0 = 0;
	blitParams.dstX1 = copyWidth;
	blitParams.dstY1 = copyHeight;
	blitParams.mask = blitMask::COLOR_BUFFER;
	blitParams.filter = textureParameters::FILTER_NEAREST;

	gfxContext.blitFramebuffers(blitParams);

	gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, ObjectHandle::null);

	frameBufferList().setCurrentDrawBuffer();

	return m_pSubTexture;
}

CachedTexture * FrameBuffer::getTexture(u32 _t)
{
	const bool getDepthTexture = m_isDepthBuffer &&
								 gDP.colorImage.address == gDP.depthImageAddress &&
								 m_pDepthBuffer != nullptr &&
								 (config.generalEmulation.hacks & hack_ZeldaMM) == 0;
	CachedTexture *pTexture = getDepthTexture ? m_pDepthBuffer->m_pDepthBufferTexture : m_pTexture;

	const u32 shift = (gSP.textureTile[_t]->imageAddress - m_startAddress) >> (m_size - 1);
	const u32 factor = m_width;
	if (m_loadType == LOADTYPE_TILE) {
		pTexture->offsetS = (float)(m_loadTileOrigin.uls + (shift % factor));
		pTexture->offsetT = (float)(m_height - (m_loadTileOrigin.ult + shift / factor));
	} else {
		pTexture->offsetS = (float)(shift % factor);
		pTexture->offsetT = (float)(m_height - shift / factor);
	}

	if (!getDepthTexture && (gSP.textureTile[_t]->clamps == 0 || gSP.textureTile[_t]->clampt == 0))
		pTexture = _getSubTexture(_t);

	pTexture->scaleS = m_scaleX / (float)pTexture->realWidth;
	pTexture->scaleT = m_scaleY / (float)pTexture->realHeight;

	if (gSP.textureTile[_t]->shifts > 10)
		pTexture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[_t]->shifts));
	else if (gSP.textureTile[_t]->shifts > 0)
		pTexture->shiftScaleS = 1.0f / (float)(1 << gSP.textureTile[_t]->shifts);
	else
		pTexture->shiftScaleS = 1.0f;

	if (gSP.textureTile[_t]->shiftt > 10)
		pTexture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[_t]->shiftt));
	else if (gSP.textureTile[_t]->shiftt > 0)
		pTexture->shiftScaleT = 1.0f / (float)(1 << gSP.textureTile[_t]->shiftt);
	else
		pTexture->shiftScaleT = 1.0f;

	return pTexture;
}

CachedTexture * FrameBuffer::getTextureBG(u32 _t)
{
	m_pTexture->scaleS = m_scaleX / (float)m_pTexture->realWidth;
	m_pTexture->scaleT = m_scaleY / (float)m_pTexture->realHeight;

	m_pTexture->shiftScaleS = 1.0f;
	m_pTexture->shiftScaleT = 1.0f;

	m_pTexture->offsetS = gSP.bgImage.imageX;
	m_pTexture->offsetT = (float)m_height - gSP.bgImage.imageY;
	return m_pTexture;
}

FrameBufferList & FrameBufferList::get()
{
	static FrameBufferList frameBufferList;
	return frameBufferList;
}

void FrameBufferList::init()
{
	 m_pCurrent = nullptr;
	 m_pCopy = nullptr;
	 gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);
	 m_prevColorImageHeight = 0;
}

void FrameBufferList::destroy() {
	m_list.clear();
	m_pCurrent = nullptr;
	m_pCopy = nullptr;
	gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);
}

void FrameBufferList::setBufferChanged()
{
	gDP.colorImage.changed = TRUE;
	if (m_pCurrent != nullptr) {
		m_pCurrent->m_cfb = false;
		m_pCurrent->m_changed = true;
		m_pCurrent->m_copiedToRdram = false;
	}
}

void FrameBufferList::correctHeight()
{
	if (m_pCurrent == nullptr)
		return;
	if (m_pCurrent->m_changed) {
		m_pCurrent->m_needHeightCorrection = false;
		return;
	}
	if (m_pCurrent->m_needHeightCorrection && m_pCurrent->m_width == gDP.scissor.lrx) {
		if (m_pCurrent->m_height != gDP.scissor.lry) {
			m_pCurrent->reinit((u32)gDP.scissor.lry);

			if (m_pCurrent->_isMarioTennisScoreboard())
				RDRAMtoColorBuffer::get().copyFromRDRAM(m_pCurrent->m_startAddress + 4, true);
			gSP.changed |= CHANGED_VIEWPORT;
		}
		m_pCurrent->m_needHeightCorrection = false;
	}
}

void FrameBufferList::clearBuffersChanged()
{
	gDP.colorImage.changed = FALSE;
	FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
	if (pBuffer != nullptr)
		pBuffer->m_changed = false;
}

void FrameBufferList::setCurrentDrawBuffer() const
{
	if (m_pCurrent != nullptr)
		gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
}

FrameBuffer * FrameBufferList::findBuffer(u32 _startAddress)
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		if (iter->m_startAddress <= _startAddress && iter->m_endAddress >= _startAddress) // [  {  ]
			return &(*iter);
	}
	return nullptr;
}

FrameBuffer * FrameBufferList::_findBuffer(u32 _startAddress, u32 _endAddress, u32 _width)
{
	if (m_list.empty())
		return nullptr;

	FrameBuffers::iterator iter = m_list.end();
	do {
		--iter;
		if ((iter->m_startAddress <= _startAddress && iter->m_endAddress >= _startAddress) || // [  {  ]
			(_startAddress <= iter->m_startAddress && _endAddress >= iter->m_startAddress)) { // {  [  }

			if (_startAddress != iter->m_startAddress || _width != iter->m_width) {
				m_list.erase(iter);
				return _findBuffer(_startAddress, _endAddress, _width);
			}

			return &(*iter);
		}
	} while (iter != m_list.begin());
	return nullptr;
}

FrameBuffer * FrameBufferList::findTmpBuffer(u32 _address)
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress > _address || iter->m_endAddress < _address)
				return &(*iter);
	return nullptr;
}


void FrameBufferList::_createScreenSizeBuffer()
{
	if (VI.height == 0)
		return;
	m_list.emplace_front();
	FrameBuffer & buffer = m_list.front();
	buffer.init(VI.width * 2, G_IM_FMT_RGBA, G_IM_SIZ_16b, VI.width, VI.height, false);
}

void FrameBufferList::saveBuffer(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb)
{
	if (config.frameBufferEmulation.enable == 0) {
		if (m_list.empty())
			_createScreenSizeBuffer();
		return;
	}

	if (m_pCurrent != nullptr &&
		config.frameBufferEmulation.copyAuxToRDRAM != 0 &&
		(config.generalEmulation.hacks & hack_Snap) == 0) {
		if (m_pCurrent->isAuxiliary()) {
			FrameBuffer_CopyToRDRAM(m_pCurrent->m_startAddress, true);
			removeBuffer(m_pCurrent->m_startAddress);
		}
	}

	if (VI.width == 0 || _height == 0) {
		m_pCurrent = nullptr;
		gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);
		return;
	}

	DisplayWindow & wnd = dwnd();
	bool bPrevIsDepth = false;

	if (m_pCurrent != nullptr) {
		bPrevIsDepth = m_pCurrent->m_isDepthBuffer;
		m_pCurrent->m_readable = true;

		// Correct buffer's end address
		if (!m_pCurrent->isAuxiliary()) {
			if (gDP.colorImage.height > 200)
				m_prevColorImageHeight = gDP.colorImage.height;
			else if (gDP.colorImage.height == 0)
				gDP.colorImage.height = m_prevColorImageHeight;

			gDP.colorImage.height = min(gDP.colorImage.height, VI.height);
		}

		//Non-auxiliary buffers are always corrected, auxiliary buffers are correct only if they need correction.
		//Also, before making any adjustments, make sure gDP.colorImage.height has a valid value.
		if((!m_pCurrent->isAuxiliary() || m_pCurrent->m_needHeightCorrection) && gDP.colorImage.height != 0)
		{
			m_pCurrent->m_endAddress = min(RDRAMSize, m_pCurrent->m_startAddress + (((m_pCurrent->m_width * gDP.colorImage.height) << m_pCurrent->m_size >> 1) - 1));
		}

		if (!m_pCurrent->_isMarioTennisScoreboard() && !m_pCurrent->m_isDepthBuffer && !m_pCurrent->m_copiedToRdram && !m_pCurrent->m_cfb && !m_pCurrent->m_cleared && m_pCurrent->m_RdramCopy.empty() && gDP.colorImage.height > 1) {
			m_pCurrent->copyRdram();
		}

		m_pCurrent = _findBuffer(m_pCurrent->m_startAddress, m_pCurrent->m_endAddress, m_pCurrent->m_width);
	}

	if (m_pCurrent == nullptr || m_pCurrent->m_startAddress != _address || m_pCurrent->m_width != _width)
		m_pCurrent = findBuffer(_address);
	const float scaleX = config.frameBufferEmulation.nativeResFactor == 0 ? wnd.getScaleX() : static_cast<float>(config.frameBufferEmulation.nativeResFactor);
	const float scaleY = config.frameBufferEmulation.nativeResFactor == 0 ? wnd.getScaleY() : scaleX;
	if (m_pCurrent != nullptr) {
		if ((m_pCurrent->m_startAddress != _address) ||
			(m_pCurrent->m_width != _width) ||
			//(current->height != height) ||
			(m_pCurrent->m_size < _size) ||
			(m_pCurrent->m_scaleX != scaleX) ||
			(m_pCurrent->m_scaleY != scaleY))
		{
			removeBuffer(m_pCurrent->m_startAddress);
			m_pCurrent = nullptr;
		} else {
			m_pCurrent->m_resolved = false;
			gfxContext.bindFramebuffer(bufferTarget::FRAMEBUFFER, m_pCurrent->m_FBO);
			if (m_pCurrent->m_size != _size) {
				f32 fillColor[4];
				gDPGetFillColor(fillColor);
				wnd.getDrawer().clearColorBuffer(fillColor);
				m_pCurrent->m_size = _size;
				m_pCurrent->m_pTexture->format = _format;
				m_pCurrent->m_pTexture->size = _size;
				if (m_pCurrent->m_pResolveTexture != nullptr) {
					m_pCurrent->m_pResolveTexture->format = _format;
					m_pCurrent->m_pResolveTexture->size = _size;
				}
				if (m_pCurrent->m_copiedToRdram)
					m_pCurrent->copyRdram();
			}
		}
	}
	const bool bNew = m_pCurrent == nullptr;
	if  (bNew) {
		// Wasn't found or removed, create a new one
		m_list.emplace_front();
		FrameBuffer & buffer = m_list.front();
		buffer.init(_address, _format, _size, _width, _height, _cfb);
		m_pCurrent = &buffer;

		if (m_pCurrent->_isMarioTennisScoreboard() || ((config.generalEmulation.hacks & hack_legoRacers) != 0 && _width == VI.width))
			RDRAMtoColorBuffer::get().copyFromRDRAM(m_pCurrent->m_startAddress + 4, true);
	}

	if (_address == gDP.depthImageAddress)
		depthBufferList().saveBuffer(_address);
	else
		attachDepthBuffer();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "FrameBuffer_SaveBuffer( 0x%08X ); depth buffer is 0x%08X\n",
		address, (depthBuffer.top != nullptr && depthBuffer.top->renderbuf > 0) ? depthBuffer.top->address : 0
	);
#endif

	if (m_pCurrent->isAuxiliary() &&
		m_pCurrent->m_pDepthBuffer != nullptr &&
		bPrevIsDepth &&
		(config.generalEmulation.hacks&hack_LoadDepthTextures) == 0) {
		// N64 games may use partial depth buffer clear for aux buffers
		// It will not work for GL, so we have to force clear depth buffer for aux buffer
		const DepthBuffer * pDepth = m_pCurrent->m_pDepthBuffer;
		wnd.getDrawer().clearDepthBuffer(pDepth->m_ulx, pDepth->m_uly, pDepth->m_lrx, pDepth->m_lry);
	}

	m_pCurrent->m_isDepthBuffer = _address == gDP.depthImageAddress;
	m_pCurrent->m_isPauseScreen = m_pCurrent->m_isOBScreen = false;
}

void FrameBufferList::copyAux()
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		if (iter->isAuxiliary())
			FrameBuffer_CopyToRDRAM(iter->m_startAddress, true);
	}
}

void FrameBufferList::removeAux()
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		while (iter->isAuxiliary()) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = nullptr;
				gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);
			}
			iter = m_list.erase(iter);
			if (iter == m_list.end())
				return;
		}
	}
}

void FrameBufferList::removeBuffer(u32 _address )
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress == _address) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = nullptr;
				gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);
			}
			m_list.erase(iter);
			return;
		}
}

void FrameBufferList::removeBuffers(u32 _width)
{
	m_pCurrent = nullptr;
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		while (iter->m_width == _width) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = nullptr;
				gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);
			}
			iter = m_list.erase(iter);
			if (iter == m_list.end())
				return;
		}
	}
}

void FrameBufferList::fillBufferInfo(void * _pinfo, u32 _size)
{
	FBInfo::FrameBufferInfo* pInfo = reinterpret_cast<FBInfo::FrameBufferInfo*>(_pinfo);

	u32 idx = 0;
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		if (iter->m_width == VI.width && !iter->m_cfb && !iter->m_isDepthBuffer) {
			pInfo[idx].addr = iter->m_startAddress;
			pInfo[idx].width = iter->m_width;
			pInfo[idx].height = iter->m_height;
			pInfo[idx++].size = iter->m_size;
			if (idx >= _size)
				return;
		}
	}
}

void FrameBufferList::attachDepthBuffer()
{
	FrameBuffer * pCurrent = config.frameBufferEmulation.enable == 0 ? &m_list.back() : m_pCurrent;
	if (pCurrent == nullptr)
		return;

	DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
	if (pCurrent->m_pDepthBuffer == pDepthBuffer)
		return;

	if (pCurrent->m_FBO.isNotNull() && pDepthBuffer != nullptr) {
		pDepthBuffer->initDepthImageTexture(pCurrent);
		pDepthBuffer->initDepthBufferTexture(pCurrent);

		bool goodDepthBufferTexture = false;
		if (gfxContext.isSupported(SpecialFeatures::DepthFramebufferTextures)) {
			goodDepthBufferTexture = gfxContext.isSupported(SpecialFeatures::WeakBlitFramebuffer) ?
				pDepthBuffer->m_pDepthBufferTexture->realWidth == pCurrent->m_pTexture->realWidth :
				pDepthBuffer->m_pDepthBufferTexture->realWidth >= pCurrent->m_pTexture->realWidth;
		} else {
			goodDepthBufferTexture = pDepthBuffer->m_depthRenderbufferWidth == pCurrent->m_pTexture->realWidth;
		}

		if (goodDepthBufferTexture) {
			pCurrent->m_pDepthBuffer = pDepthBuffer;
			pDepthBuffer->setDepthAttachment(pCurrent->m_FBO, bufferTarget::DRAW_FRAMEBUFFER);
			if (config.frameBufferEmulation.N64DepthCompare != 0)
				pDepthBuffer->bindDepthImageTexture();
		} else
			pCurrent->m_pDepthBuffer = nullptr;
	} else
		pCurrent->m_pDepthBuffer = nullptr;

	assert(!gfxContext.isFramebufferError());
}

void FrameBufferList::clearDepthBuffer(DepthBuffer * _pDepthBuffer)
{
	for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
		if (iter->m_pDepthBuffer == _pDepthBuffer) {
			iter->m_pDepthBuffer = nullptr;
		}
	}
}

void FrameBuffer_Init()
{
	frameBufferList().init();
	if (config.frameBufferEmulation.enable != 0) {
	ColorBufferToRDRAM::get().init();
	DepthBufferToRDRAM::get().init();
	RDRAMtoColorBuffer::get().init();
	}
}

void FrameBuffer_Destroy()
{
	RDRAMtoColorBuffer::get().destroy();
	DepthBufferToRDRAM::get().destroy();
	ColorBufferToRDRAM::get().destroy();
	frameBufferList().destroy();
}

void FrameBufferList::_renderScreenSizeBuffer()
{
	if (m_list.empty())
		return;

	DisplayWindow & wnd = dwnd();
	GraphicsDrawer & drawer = wnd.getDrawer();
	FrameBuffer *pBuffer = &m_list.back();
	PostProcessor & postProcessor = PostProcessor::get();
	FrameBuffer * pFilteredBuffer = postProcessor.doBlur(postProcessor.doGammaCorrection(
		postProcessor.doOrientationCorrection(pBuffer)));
	CachedTexture * pBufferTexture = pFilteredBuffer->m_pTexture;


	s32 srcCoord[4] = { 0, 0, pBufferTexture->realWidth, pBufferTexture->realHeight };
	const s32 hOffset = (wnd.getScreenWidth() - wnd.getWidth()) / 2;
	const s32 vOffset = (wnd.getScreenHeight() - wnd.getHeight()) / 2 + wnd.getHeightOffset();
	s32 dstCoord[4] = { hOffset, vOffset, hOffset + pBufferTexture->realWidth, vOffset + pBufferTexture->realHeight };

	gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	drawer.clearColorBuffer(clearColor);

	TextureParam filter = textureParameters::FILTER_LINEAR;

	GraphicsDrawer::BlitOrCopyRectParams blitParams;
	blitParams.srcX0 = srcCoord[0];
	blitParams.srcY0 = srcCoord[1];
	blitParams.srcX1 = srcCoord[2];
	blitParams.srcY1 = srcCoord[3];
	blitParams.srcWidth = pBufferTexture->realWidth;
	blitParams.srcHeight = pBufferTexture->realHeight;
	blitParams.dstX0 = dstCoord[0];
	blitParams.dstY0 = dstCoord[1];
	blitParams.dstX1 = dstCoord[2];
	blitParams.dstY1 = dstCoord[3];
	blitParams.dstWidth = wnd.getScreenWidth();
	blitParams.dstHeight = wnd.getScreenHeight() + wnd.getHeightOffset();
	blitParams.filter = filter;
	blitParams.mask = blitMask::COLOR_BUFFER;
	blitParams.tex[0] = pBufferTexture;
	blitParams.combiner = CombinerInfo::get().getTexrectCopyProgram();
	blitParams.readBuffer = pFilteredBuffer->m_FBO;

	drawer.blitOrCopyTexturedRect(blitParams);

	gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, ObjectHandle::null);

	wnd.swapBuffers();
	gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, pBuffer->m_FBO);
	gDP.changed |= CHANGED_SCISSOR;
}

void FrameBufferList::renderBuffer(u32 _address)
{
	static s32 vStartPrev = 0;

	if (VI.width == 0 || *REG.VI_WIDTH == 0 || *REG.VI_H_START == 0) // H width is zero. Don't draw
		return;

	if (config.frameBufferEmulation.enable == 0) {
		_renderScreenSizeBuffer();
		return;
	}

	FrameBuffer *pBuffer = findBuffer(_address);
	if (pBuffer == nullptr)
		return;

	DisplayWindow & wnd = dwnd();
	GraphicsDrawer & drawer = wnd.getDrawer();
	s32 srcY0, srcY1, dstY0, dstY1;
	s32 X0, X1, Xwidth;
	s32 Xoffset = 0;
	s32 Xdivot = 0;
	s32 srcPartHeight = 0;
	s32 dstPartHeight = 0;

	const f32 yScale = _FIXED2FLOAT(_SHIFTR(*REG.VI_Y_SCALE, 0, 12), 10);
	s32 vEnd = _SHIFTR(*REG.VI_V_START, 0, 10);
	s32 vStart = _SHIFTR(*REG.VI_V_START, 16, 10);
	const s32 hEnd = _SHIFTR(*REG.VI_H_START, 0, 10);
	const s32 hStart = _SHIFTR(*REG.VI_H_START, 16, 10);
	const s32 vSync = (*REG.VI_V_SYNC) & 0x03FF;
	const bool interlaced = (*REG.VI_STATUS & 0x40) != 0;
	const bool isPAL = vSync > 550;
	const s32 vShift = (isPAL ? 47 : 37);
	dstY0 = vStart - vShift;
	dstY0 >>= 1;
	dstY0 &= -(dstY0 >= 0);
	vStart >>= 1;
	vEnd >>= 1;
	const u32 vFullHeight = isPAL ? 288 : 240;
	const u32 vCurrentHeight = vEnd - vStart;
	const float dstScaleY = (float)wnd.getHeight() / float(vFullHeight);

	bool isLowerField = false;
	if (interlaced)
		isLowerField = vStart > vStartPrev;
	vStartPrev = vStart;

	const u32 addrOffset = ((_address - pBuffer->m_startAddress) << 1 >> pBuffer->m_size);
	srcY0 = addrOffset / (*REG.VI_WIDTH);
	if ((*REG.VI_WIDTH != addrOffset * 2) && (addrOffset % (*REG.VI_WIDTH) != 0))
		Xoffset = (*REG.VI_WIDTH) - addrOffset % (*REG.VI_WIDTH);
	if (Xoffset == VI.width)
		Xoffset = 0;

	if (isLowerField) {
		if (*REG.VI_WIDTH < VI.width * 2) {
			if (srcY0 > 0)
				--srcY0;
		}
		if (dstY0 > 0)
			--dstY0;
	}

	if (srcY0 + vCurrentHeight > vFullHeight) {
		dstPartHeight = srcY0;
		srcY0 = (s32)(srcY0*yScale);
		srcPartHeight = srcY0;
		srcY1 = VI.real_height;
		dstY1 = dstY0 + vCurrentHeight - dstPartHeight;
	} else {
		dstY0 += srcY0;
		dstY1 = dstY0 + vCurrentHeight;
		srcY0 = (s32)(srcY0*yScale);
		srcY1 = srcY0 + VI.real_height;
	}
	PostProcessor & postProcessor = PostProcessor::get();
	FrameBuffer * pFilteredBuffer = postProcessor.doBlur(postProcessor.doGammaCorrection(
		postProcessor.doOrientationCorrection(pBuffer)));

	const bool vi_fsaa = (*REG.VI_STATUS & 512) == 0;
	const bool vi_divot = (*REG.VI_STATUS & 16) != 0;
	if (vi_fsaa && vi_divot)
		Xdivot = 1;

	const f32 viScaleX = _FIXED2FLOAT(_SHIFTR(*REG.VI_X_SCALE, 0, 12), 10);
	const f32 srcScaleX = pFilteredBuffer->m_scaleX;
	const f32 dstScaleX = wnd.getScaleX();
	const s32 h0 = (isPAL ? 128 : 108);
	const s32 hx0 = max(0, hStart - h0);
	const s32 hx1 = max(0, h0 + 640 - hEnd);
	X0 = (s32)((hx0 * viScaleX + Xoffset) * dstScaleX);
	Xwidth = (s32)((min((f32)VI.width, (hEnd - hStart)*viScaleX - Xoffset - Xdivot)) * srcScaleX);
	X1 = wnd.getWidth() - (s32)((hx1*viScaleX + Xdivot) * dstScaleX);

	const f32 srcScaleY = pFilteredBuffer->m_scaleY;
	CachedTexture * pBufferTexture = pFilteredBuffer->m_pTexture;
	const s32 hCrop = config.video.cropMode == Config::cmDisable ? 0 : s32(config.video.cropWidth * srcScaleX);
	const s32 vCrop = config.video.cropMode == Config::cmDisable ? 0 : s32(config.video.cropHeight * srcScaleY);
	s32 srcCoord[4] = { hCrop,
						  vCrop + (s32)(srcY0*srcScaleY),
						  Xwidth - hCrop,
						  min((s32)(srcY1*srcScaleY), (s32)pBufferTexture->realHeight) - vCrop };
	if (srcCoord[2] > pBufferTexture->realWidth || srcCoord[3] > pBufferTexture->realHeight) {
		removeBuffer(pBuffer->m_startAddress);
		return;
	}

	const s32 hOffset = (wnd.getScreenWidth() - wnd.getWidth()) / 2;
	const s32 vOffset = (wnd.getScreenHeight() - wnd.getHeight()) / 2 + wnd.getHeightOffset();
	s32 dstCoord[4] = { X0 + hOffset,
						  vOffset + (s32)(dstY0*dstScaleY),
						  hOffset + X1,
						  vOffset + (s32)(dstY1*dstScaleY) };

	gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::null);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	drawer.clearColorBuffer(clearColor);


	TextureParam filter = textureParameters::FILTER_LINEAR;
	ObjectHandle readBuffer;

	if (pFilteredBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
		if (X0 > 0 || dstPartHeight > 0 ||
			(srcCoord[2] - srcCoord[0]) != (dstCoord[2] - dstCoord[0]) ||
			(srcCoord[3] - srcCoord[1]) != (dstCoord[3] - dstCoord[1])) {
			pFilteredBuffer->resolveMultisampledTexture(true);
			readBuffer = pFilteredBuffer->m_resolveFBO;
		} else {
			readBuffer = pFilteredBuffer->m_FBO;
			filter = textureParameters::FILTER_NEAREST;
		}
	} else {
		readBuffer = pFilteredBuffer->m_FBO;
	}

	GraphicsDrawer::BlitOrCopyRectParams blitParams;
	blitParams.srcX0 = srcCoord[0];
	blitParams.srcY0 = srcCoord[1];
	blitParams.srcX1 = srcCoord[2];
	blitParams.srcY1 = srcCoord[3];
	blitParams.srcWidth = pBufferTexture->realWidth;
	blitParams.srcHeight = pBufferTexture->realHeight;
	blitParams.dstX0 = dstCoord[0];
	blitParams.dstY0 = dstCoord[1];
	blitParams.dstX1 = dstCoord[2];
	blitParams.dstY1 = dstCoord[3];
	blitParams.dstWidth = wnd.getScreenWidth();
	blitParams.dstHeight = wnd.getScreenHeight() + wnd.getHeightOffset();
	blitParams.filter = filter;
	blitParams.mask = blitMask::COLOR_BUFFER;
	blitParams.tex[0] = pBufferTexture;
	blitParams.combiner = CombinerInfo::get().getTexrectCopyProgram();
	blitParams.readBuffer = readBuffer;

	drawer.blitOrCopyTexturedRect(blitParams);

	if (dstPartHeight > 0) {
		const u32 size = *REG.VI_STATUS & 3;
		pBuffer = findBuffer(_address + (((*REG.VI_WIDTH)*VI.height)<<size>>1));
		if (pBuffer != nullptr) {
			pFilteredBuffer = postProcessor.doBlur(postProcessor.doGammaCorrection(
				postProcessor.doOrientationCorrection(pBuffer)));
			srcY0 = 0;
			srcY1 = srcPartHeight;
			dstY0 = dstY1;
			dstY1 = dstY0 + dstPartHeight;
			if (pFilteredBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
				pFilteredBuffer->resolveMultisampledTexture();
				readBuffer = pFilteredBuffer->m_resolveFBO;
			} else {
				readBuffer = pFilteredBuffer->m_FBO;
			}

			pBufferTexture = pFilteredBuffer->m_pTexture;

			blitParams.srcX0 = 0;
			blitParams.srcY0 = (s32)(srcY0*srcScaleY);
			blitParams.srcX1 = Xwidth;
			blitParams.srcY1 = min((s32)(srcY1*srcScaleY), (s32)pFilteredBuffer->m_pTexture->realHeight);
			blitParams.srcWidth = pBufferTexture->realWidth;
			blitParams.srcHeight = pBufferTexture->realHeight;
			blitParams.dstX0 = hOffset;
			blitParams.dstY0 = vOffset + (s32)(dstY0*dstScaleY);
			blitParams.dstX1 = hOffset + X1;
			blitParams.dstY1 = vOffset + (s32)(dstY1*dstScaleY);
			blitParams.dstWidth = wnd.getScreenWidth();
			blitParams.dstHeight = wnd.getScreenHeight() + wnd.getHeightOffset();
			blitParams.tex[0] = pBufferTexture;
			blitParams.readBuffer = readBuffer;

			drawer.blitOrCopyTexturedRect(blitParams);
		}
	}

	gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, ObjectHandle::null);

	wnd.swapBuffers();
	if (m_pCurrent != nullptr) {
		gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
	}
	gDP.changed |= CHANGED_SCISSOR;
}

void FrameBufferList::fillRDRAM(s32 ulx, s32 uly, s32 lrx, s32 lry)
{
	if (m_pCurrent == nullptr)
		return;

	ulx = min(max((float)ulx, gDP.scissor.ulx), gDP.scissor.lrx);
	lrx = min(max((float)lrx, gDP.scissor.ulx), gDP.scissor.lrx);
	uly = min(max((float)uly, gDP.scissor.uly), gDP.scissor.lry);
	lry = min(max((float)lry, gDP.scissor.uly), gDP.scissor.lry);

	const u32 stride = gDP.colorImage.width << gDP.colorImage.size >> 1;
	const u32 lowerBound = gDP.colorImage.address + lry*stride;
	if (lowerBound > RDRAMSize)
		lry -= (lowerBound - RDRAMSize) / stride;
	u32 ci_width_in_dwords = gDP.colorImage.width >> (3 - gDP.colorImage.size);
	ulx >>= (3 - gDP.colorImage.size);
	lrx >>= (3 - gDP.colorImage.size);
	u32 * dst = (u32*)(RDRAM + gDP.colorImage.address);
	dst += uly * ci_width_in_dwords;
	for (u32 y = uly; y < lry; ++y) {
		for (u32 x = ulx; x < lrx; ++x) {
			dst[x] = gDP.fillColor.color;
		}
		dst += ci_width_in_dwords;
	}

	m_pCurrent->setBufferClearParams(gDP.fillColor.color, ulx, uly, lrx, lry);
}

void FrameBuffer_ActivateBufferTexture(u32 t, FrameBuffer *pBuffer)
{
	if (pBuffer == nullptr)
		return;

	CachedTexture *pTexture = pBuffer->getTexture(t);
	if (pTexture == nullptr)
		return;

//	frameBufferList().renderBuffer(pBuffer->m_startAddress);
	textureCache().activateTexture(t, pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_ActivateBufferTextureBG(u32 t, FrameBuffer *pBuffer )
{
	if (pBuffer == nullptr)
		return;

	CachedTexture *pTexture = pBuffer->getTextureBG(t);
	if (pTexture == nullptr)
		return;

//	frameBufferList().renderBuffer(pBuffer->m_startAddress);
	textureCache().activateTexture(t, pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_CopyToRDRAM(u32 _address, bool _sync)
{
	ColorBufferToRDRAM::get().copyToRDRAM(_address, _sync);
}

void FrameBuffer_CopyChunkToRDRAM(u32 _address)
{
	ColorBufferToRDRAM::get().copyChunkToRDRAM(_address);
}

bool FrameBuffer_CopyDepthBuffer( u32 address )
{
	FrameBuffer * pCopyBuffer = frameBufferList().getCopyBuffer();
	if (pCopyBuffer != nullptr) {
		// This code is mainly to emulate Zelda MM camera.
		ColorBufferToRDRAM::get().copyToRDRAM(pCopyBuffer->m_startAddress, true);
		pCopyBuffer->m_RdramCopy.resize(0); // To disable validity check by RDRAM content. CPU may change content of the buffer for some unknown reason.
		frameBufferList().setCopyBuffer(nullptr);
		return true;
	} else
		return DepthBufferToRDRAM::get().copyToRDRAM(address);
}

bool FrameBuffer_CopyDepthBufferChunk(u32 address)
{
	return DepthBufferToRDRAM::get().copyChunkToRDRAM(address);
}

void FrameBuffer_CopyFromRDRAM(u32 _address, bool _bCFB)
{
	RDRAMtoColorBuffer::get().copyFromRDRAM(_address, _bCFB);
}

void FrameBuffer_AddAddress(u32 address, u32 _size)
{
	RDRAMtoColorBuffer::get().addAddress(address, _size);
}
