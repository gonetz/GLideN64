#include <assert.h>
#include <math.h>
#include <algorithm>
#include <vector>
#include "OpenGL.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gDP.h"
#include "VI.h"
#include "Textures.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "Types.h"
#include "Config.h"
#include "Debug.h"
#include "PostProcessor.h"
#include "FrameBufferInfo.h"
#include "FBOTextureFormats.h"
#include "Log.h"

#include "BufferCopy/ColorBufferToRDRAM.h"
#include "BufferCopy/DepthBufferToRDRAM.h"
#include "BufferCopy/RDRAMtoColorBuffer.h"

using namespace std;

FrameBuffer::FrameBuffer() :
	m_startAddress(0), m_endAddress(0), m_size(0), m_width(0), m_height(0), m_validityChecked(0),
	m_scaleX(0), m_scaleY(0),
	m_copiedToRdram(false), m_fingerprint(false), m_cleared(false), m_changed(false), m_cfb(false),
	m_isDepthBuffer(false), m_isPauseScreen(false), m_isOBScreen(false),
	m_needHeightCorrection(false), m_readable(false),
	m_loadType(LOADTYPE_BLOCK), m_pDepthBuffer(nullptr),
	m_resolveFBO(0), m_pResolveTexture(nullptr), m_resolved(false),
	m_SubFBO(0), m_pSubTexture(nullptr)
{
	m_loadTileOrigin.uls = m_loadTileOrigin.ult = 0;
	m_pTexture = textureCache().addFrameBufferTexture();
	glGenFramebuffers(1, &m_FBO);
}

FrameBuffer::~FrameBuffer()
{
	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);
	if (m_pTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pTexture);
	if (m_resolveFBO != 0)
		glDeleteFramebuffers(1, &m_resolveFBO);
	if (m_pResolveTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pResolveTexture);
	if (m_SubFBO != 0)
		glDeleteFramebuffers(1, &m_SubFBO);
	if (m_pSubTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pSubTexture);
}

void FrameBuffer::_initTexture(u16 _width, u16 _height, u16 _format, u16 _size, CachedTexture *_pTexture)
{
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
		_pTexture->textureBytes *= fboFormats.colorFormatBytes;
	else
		_pTexture->textureBytes *= fboFormats.monochromeFormatBytes;
	textureCache().addFrameBufferTextureSize(_pTexture->textureBytes);
}

void FrameBuffer::_setAndAttachTexture(u16 _size, CachedTexture *_pTexture)
{
	glBindTexture(GL_TEXTURE_2D, _pTexture->glName);
	if (_size > G_IM_SIZ_8b)
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, _pTexture->realWidth, _pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, nullptr);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.monochromeInternalFormat, _pTexture->realWidth, _pTexture->realHeight, 0, fboFormats.monochromeFormat, fboFormats.monochromeType, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _pTexture->glName, 0);
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

void FrameBuffer::init(u32 _address, u32 _endAddress, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb)
{
	OGLVideo & ogl = video();
	m_startAddress = _address;
	m_endAddress = _endAddress;
	m_width = _width;
	m_height = _height;
	m_size = _size;
	if (isAuxiliary() && config.frameBufferEmulation.copyAuxToRDRAM != 0) {
		m_scaleX = 1.0f;
		m_scaleY = 1.0f;
	} else if (config.frameBufferEmulation.nativeResFactor != 0) {
		m_scaleX = m_scaleY = static_cast<float>(config.frameBufferEmulation.nativeResFactor);
	} else {
		m_scaleX = ogl.getScaleX();
		m_scaleY = ogl.getScaleY();
	}
	m_cfb = _cfb;
	m_needHeightCorrection = _width != VI.width && _width != *REG.VI_WIDTH;
	m_cleared = false;
	m_fingerprint = false;

	_initTexture(_width, _height, _format, _size, m_pTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling != 0) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pTexture->glName);
#if defined(GLES3_1)
		if (_size > G_IM_SIZ_8b)
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_RGBA8, m_pTexture->realWidth, m_pTexture->realHeight, false);
		else
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, fboFormats.monochromeInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, false);
#else
		if (_size > G_IM_SIZ_8b)
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_RGBA8, m_pTexture->realWidth, m_pTexture->realHeight, false);
		else
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, fboFormats.monochromeInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, false);
#endif
		m_pTexture->frameBufferTexture = CachedTexture::fbMultiSample;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_pTexture->glName, 0);

		m_pResolveTexture = textureCache().addFrameBufferTexture();
		_initTexture(_width, _height, _format, _size, m_pResolveTexture);
		glGenFramebuffers(1, &m_resolveFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFBO);
		_setAndAttachTexture(_size, m_pResolveTexture);
		assert(checkFBO());

		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	} else
#endif // GL_MULTISAMPLING_SUPPORT
		_setAndAttachTexture(_size, m_pTexture);

	ogl.getRender().clearColorBuffer(nullptr);
}

void FrameBuffer::reinit(u16 _height)
{
	const u16 format = m_pTexture->format;
	const u32 endAddress = m_startAddress + ((m_width * _height) << m_size >> 1) - 1;
	if (m_pTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pTexture);
	if (m_resolveFBO != 0)
		glDeleteFramebuffers(1, &m_resolveFBO);
	if (m_pResolveTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pResolveTexture);
	m_pTexture = textureCache().addFrameBufferTexture();
	init(m_startAddress, endAddress, format, m_size, m_width, _height, m_cfb);
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
		if (m_validityChecked == video().getBuffersSwapCount())
			return true; // Already checked
		m_validityChecked = video().getBuffersSwapCount();
	}

	const u32 * const pData = (const u32*)RDRAM;

	if (m_cleared) {
		const u32 testColor = m_clearParams.fillcolor & 0xFFFEFFFE;
		const u32 ci_width_in_dwords = m_width >> (3 - m_size);
		const u32 start = (m_startAddress >> 2) + m_clearParams.uly * ci_width_in_dwords;
		const u32 * dst = pData + start;
		u32 wrongPixels = 0;
		for (u32 y = m_clearParams.uly; y < m_clearParams.lry; ++y) {
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
#ifdef GL_MULTISAMPLING_SUPPORT
	if (m_resolved && !_bForce)
		return;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFBO);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		0, 0, m_pTexture->realWidth, m_pTexture->realHeight,
		0, 0, m_pResolveTexture->realWidth, m_pResolveTexture->realHeight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST
		);
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	frameBufferList().setCurrentDrawBuffer();
	m_resolved = true;
#endif
}

bool FrameBuffer::_initSubTexture(u32 _t)
{
	if (m_SubFBO == 0)
		glGenFramebuffers(1, &m_SubFBO);

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

	m_pSubTexture = textureCache().addFrameBufferTexture();
	_initTexture(width, height, m_pTexture->format, m_pTexture->size, m_pSubTexture);

	m_pSubTexture->clampS = pTile->clamps;
	m_pSubTexture->clampT = pTile->clampt;
	m_pSubTexture->offsetS = 0.0f;
	m_pSubTexture->offsetT = m_pSubTexture->clampHeight;

	glActiveTexture(GL_TEXTURE0 + _t);
	glBindTexture(GL_TEXTURE_2D, m_pSubTexture->glName);
	if (m_pSubTexture->size > G_IM_SIZ_8b)
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pSubTexture->realWidth, m_pSubTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, nullptr);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.monochromeInternalFormat, m_pSubTexture->realWidth, m_pSubTexture->realHeight, 0, fboFormats.monochromeFormat, fboFormats.monochromeType, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_SubFBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pSubTexture->glName, 0);
	return true;
}

CachedTexture * FrameBuffer::_getSubTexture(u32 _t)
{
#ifdef GLES2
	return m_pTexture;
#else
	if (!_initSubTexture(_t))
		return m_pTexture;
	GLint x0 = (GLint)(m_pTexture->offsetS * m_scaleX);
	GLint y0 = (GLint)(m_pTexture->offsetT * m_scaleY) - m_pSubTexture->realHeight;
	GLint copyWidth = m_pSubTexture->realWidth;
	if (x0 + copyWidth > m_pTexture->realWidth)
		copyWidth = m_pTexture->realWidth - x0;
	GLint copyHeight = m_pSubTexture->realHeight;
	if (y0 + copyHeight > m_pTexture->realHeight)
		copyHeight = m_pTexture->realHeight - y0;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_SubFBO);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		x0, y0, x0 + copyWidth, y0 + copyHeight,
		0, 0, copyWidth, copyHeight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST
		);
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	frameBufferList().setCurrentDrawBuffer();

	return m_pSubTexture;
#endif
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
	 glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	 m_prevColorImageHeight = 0;
}

void FrameBufferList::destroy() {
	m_list.clear();
	m_pCurrent = nullptr;
	m_pCopy = nullptr;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
}

FrameBuffer * FrameBufferList::findBuffer(u32 _startAddress)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
	if (iter->m_startAddress <= _startAddress && iter->m_endAddress >= _startAddress) // [  {  ]
		return &(*iter);
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
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress > _address || iter->m_endAddress < _address)
				return &(*iter);
	return nullptr;
}

void FrameBufferList::saveBuffer(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb)
{
	if (m_pCurrent != nullptr && config.frameBufferEmulation.copyAuxToRDRAM != 0) {
		if (m_pCurrent->isAuxiliary()) {
			FrameBuffer_CopyToRDRAM(m_pCurrent->m_startAddress, true);
			removeBuffer(m_pCurrent->m_startAddress);
		}
	}

	if (VI.width == 0 || _height == 0) {
		m_pCurrent = nullptr;
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		return;
	}

	OGLVideo & ogl = video();
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

	const u32 endAddress = _address + ((_width * _height) << _size >> 1) - 1;
	if (m_pCurrent == nullptr || m_pCurrent->m_startAddress != _address || m_pCurrent->m_width != _width)
		m_pCurrent = findBuffer(_address);
	const float scaleX = config.frameBufferEmulation.nativeResFactor == 0 ? ogl.getScaleX() : static_cast<float>(config.frameBufferEmulation.nativeResFactor);
	const float scaleY = config.frameBufferEmulation.nativeResFactor == 0 ? ogl.getScaleY() : scaleX;
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
#ifdef VC
			const GLenum discards[]  = {GL_DEPTH_ATTACHMENT};
			glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
#endif
			glBindFramebuffer(GL_FRAMEBUFFER, m_pCurrent->m_FBO);
			if (m_pCurrent->m_size != _size) {
				f32 fillColor[4];
				gDPGetFillColor(fillColor);
				ogl.getRender().clearColorBuffer(fillColor);
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
		buffer.init(_address, endAddress, _format, _size, _width, _height, _cfb);
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

	if (m_pCurrent->isAuxiliary() && m_pCurrent->m_pDepthBuffer != nullptr && bPrevIsDepth) {
		// N64 games may use partial depth buffer clear for aux buffers
		// It will not work for GL, so we have to force clear depth buffer for aux buffer
		const DepthBuffer * pDepth = m_pCurrent->m_pDepthBuffer;
		ogl.getRender().clearDepthBuffer(pDepth->m_ulx, pDepth->m_uly, pDepth->m_lrx, pDepth->m_lry);
	}

	m_pCurrent->m_isDepthBuffer = _address == gDP.depthImageAddress;
	m_pCurrent->m_isPauseScreen = m_pCurrent->m_isOBScreen = false;
}

void FrameBufferList::copyAux()
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
		if (iter->m_width != VI.width && iter->m_height != VI.height)
			FrameBuffer_CopyToRDRAM(iter->m_startAddress, true);
	}
}

void FrameBufferList::removeAux()
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
		while (iter->m_width != VI.width && iter->m_height != VI.height) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = nullptr;
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			}
			iter = m_list.erase(iter);
			if (iter == m_list.end())
				return;
		}
	}
}

void FrameBufferList::removeBuffer(u32 _address )
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress == _address) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = nullptr;
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			}
			m_list.erase(iter);
			return;
		}
}

void FrameBufferList::removeBuffers(u32 _width)
{
	m_pCurrent = nullptr;
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
		while (iter->m_width == _width) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = nullptr;
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
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
	if (m_pCurrent == nullptr)
		return;

#ifdef VC
	const GLenum discards[]  = {GL_DEPTH_ATTACHMENT};
	glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
#endif
	DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
	if (m_pCurrent->m_FBO > 0 && pDepthBuffer != nullptr) {
		pDepthBuffer->initDepthImageTexture(m_pCurrent);
		pDepthBuffer->initDepthBufferTexture(m_pCurrent);
#ifndef USE_DEPTH_RENDERBUFFER
#ifdef GLESX
		if (pDepthBuffer->m_pDepthBufferTexture->realWidth == m_pCurrent->m_pTexture->realWidth) {
#else
		if (pDepthBuffer->m_pDepthBufferTexture->realWidth >= m_pCurrent->m_pTexture->realWidth) {
#endif // GLES2
#else
		if (pDepthBuffer->m_depthRenderbufferWidth == m_pCurrent->m_pTexture->realWidth) {
#endif // USE_DEPTH_RENDERBUFFER
			m_pCurrent->m_pDepthBuffer = pDepthBuffer;
			pDepthBuffer->setDepthAttachment(GL_DRAW_FRAMEBUFFER);
			if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
				pDepthBuffer->bindDepthImageTexture();
		} else
			m_pCurrent->m_pDepthBuffer = nullptr;
	} else
		m_pCurrent->m_pDepthBuffer = nullptr;

#ifndef GLES2
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);
#endif
	assert(checkFBO());
}

void FrameBufferList::clearDepthBuffer(DepthBuffer * _pDepthBuffer)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
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
#ifndef GLES2
	DepthBufferToRDRAM::get().init();
#endif
	RDRAMtoColorBuffer::get().init();
	}
}

void FrameBuffer_Destroy()
{
	RDRAMtoColorBuffer::get().destroy();
#ifndef GLES2
	DepthBufferToRDRAM::get().destroy();
	ColorBufferToRDRAM::get().destroy();
#endif
	frameBufferList().destroy();
}

void FrameBufferList::renderBuffer(u32 _address)
{
	static s32 vStartPrev = 0;

	if (VI.width == 0 || *REG.VI_WIDTH == 0 || *REG.VI_H_START == 0) // H width is zero. Don't draw
		return;

	FrameBuffer *pBuffer = findBuffer(_address);
	if (pBuffer == nullptr)
		return;

	OGLVideo & ogl = video();
	OGLRender & render = ogl.getRender();
	GLint srcY0, srcY1, dstY0, dstY1;
	GLint X0, X1, Xwidth;
	GLint Xoffset = 0;
	GLint Xdivot = 0;
	GLint srcPartHeight = 0;
	GLint dstPartHeight = 0;

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
	const float dstScaleY = (float)ogl.getHeight() / float(vFullHeight);

	bool isLowerField = false;
	if (interlaced)
		isLowerField = vStart > vStartPrev;
	vStartPrev = vStart;

	const u32 addrOffset = ((_address - pBuffer->m_startAddress) << 1 >> pBuffer->m_size);
	srcY0 = addrOffset / (*REG.VI_WIDTH);
	if ((*REG.VI_WIDTH != addrOffset * 2) && (addrOffset % (*REG.VI_WIDTH) != 0))
		Xoffset = (*REG.VI_WIDTH) - addrOffset % (*REG.VI_WIDTH);
	if (isLowerField) {
		if (srcY0 > 0)
			--srcY0;
		if (dstY0 > 0)
			--dstY0;
	}

	if (srcY0 + vCurrentHeight > vFullHeight) {
		dstPartHeight = srcY0;
		srcY0 = (GLint)(srcY0*yScale);
		srcPartHeight = srcY0;
		srcY1 = VI.real_height;
		dstY1 = dstY0 + vCurrentHeight - dstPartHeight;
	} else {
		dstY0 += srcY0;
		dstY1 = dstY0 + vCurrentHeight;
		srcY0 = (GLint)(srcY0*yScale);
		srcY1 = srcY0 + VI.real_height;
	}

	FrameBuffer * pFilteredBuffer = PostProcessor::get().doBlur(PostProcessor::get().doGammaCorrection(pBuffer));

	const bool vi_fsaa = (*REG.VI_STATUS & 512) == 0;
	const bool vi_divot = (*REG.VI_STATUS & 16) != 0;
	if (vi_fsaa && vi_divot)
		Xdivot = 1;

	const f32 viScaleX = _FIXED2FLOAT(_SHIFTR(*REG.VI_X_SCALE, 0, 12), 10);
	const f32 srcScaleX = pFilteredBuffer->m_scaleX;
	const f32 dstScaleX = ogl.getScaleX();
	const s32 h0 = (isPAL ? 128 : 108);
	const s32 hx0 = max(0, hStart - h0);
	const s32 hx1 = max(0, h0 + 640 - hEnd);
	X0 = (GLint)((hx0 * viScaleX + Xoffset) * dstScaleX);
	Xwidth = (GLint)((min((f32)VI.width, (hEnd - hStart)*viScaleX - Xoffset - Xdivot)) * srcScaleX);
	X1 = ogl.getWidth() - (GLint)(hx1 *viScaleX * dstScaleX);

	const f32 srcScaleY = pFilteredBuffer->m_scaleY;
	CachedTexture * pBufferTexture = pFilteredBuffer->m_pTexture;
	const GLint hCrop = config.video.cropMode == Config::cmDisable ? 0 : GLint(config.video.cropWidth * srcScaleX);
	const GLint vCrop = config.video.cropMode == Config::cmDisable ? 0 : GLint(config.video.cropHeight * srcScaleY);
	GLint srcCoord[4] = { hCrop,
						  vCrop + (GLint)(srcY0*srcScaleY),
						  Xwidth - hCrop,
						  min((GLint)(srcY1*srcScaleY), (GLint)pBufferTexture->realHeight) - vCrop };
	if (srcCoord[2] > pBufferTexture->realWidth || srcCoord[3] > pBufferTexture->realHeight) {
		removeBuffer(pBuffer->m_startAddress);
		return;
	}

	const GLint hOffset = (ogl.getScreenWidth() - ogl.getWidth()) / 2;
	const GLint vOffset = (ogl.getScreenHeight() - ogl.getHeight()) / 2 + ogl.getHeightOffset();
	GLint dstCoord[4] = { X0 + hOffset,
						  vOffset + (GLint)(dstY0*dstScaleY),
						  hOffset + X1,
						  vOffset + (GLint)(dstY1*dstScaleY) };
#ifdef GLESX
	if (render.getRenderer() == OGLRender::glrAdreno)
		dstCoord[0] += 1; // workaround for Adreno's issue with glBindFramebuffer;
#endif // GLESX

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//glDrawBuffer( GL_BACK );
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	render.clearColorBuffer(clearColor);

	GLenum filter = GL_LINEAR;
	if (pFilteredBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
		if (X0 > 0 || dstPartHeight > 0 ||
			(srcCoord[2] - srcCoord[0]) != (dstCoord[2] - dstCoord[0]) ||
			(srcCoord[3] - srcCoord[1]) != (dstCoord[3] - dstCoord[1])) {
			pFilteredBuffer->resolveMultisampledTexture(true);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_resolveFBO);
		} else {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_FBO);
			filter = GL_NEAREST;
		}
	} else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_FBO);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	ogl.getRender().copyTexturedRect(srcCoord[0], srcCoord[1], srcCoord[2], srcCoord[3],
									 pBufferTexture->realWidth, pBufferTexture->realHeight, pBufferTexture->glName,
									 dstCoord[0], dstCoord[1], dstCoord[2], dstCoord[3],
									 ogl.getScreenWidth(), ogl.getScreenHeight() + ogl.getHeightOffset(), filter);

	if (dstPartHeight > 0) {
		const u32 size = *REG.VI_STATUS & 3;
		pBuffer = findBuffer(_address + (((*REG.VI_WIDTH)*VI.height)<<size>>1));
		if (pBuffer != nullptr) {
			pFilteredBuffer = PostProcessor::get().doBlur(PostProcessor::get().doGammaCorrection(pBuffer));
			srcY0 = 0;
			srcY1 = srcPartHeight;
			dstY0 = dstY1;
			dstY1 = dstY0 + dstPartHeight;
			if (pFilteredBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
				pFilteredBuffer->resolveMultisampledTexture();
				glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_resolveFBO);
			} else
				glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_FBO);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			pBufferTexture = pFilteredBuffer->m_pTexture;
			ogl.getRender().copyTexturedRect(0, (GLint)(srcY0*srcScaleY), Xwidth, min((GLint)(srcY1*srcScaleY), (GLint)pFilteredBuffer->m_pTexture->realHeight),
											 pBufferTexture->realWidth, pBufferTexture->realHeight, pBufferTexture->glName,
											 hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + X1, vOffset + (GLint)(dstY1*dstScaleY),
											 ogl.getScreenWidth(), ogl.getScreenHeight() + ogl.getHeightOffset(), filter);
		}
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	ogl.swapBuffers();
	if (m_pCurrent != nullptr) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
#ifdef VC
		const GLenum discards[]  = {GL_DEPTH_ATTACHMENT};
		glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
#endif
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
#ifndef GLES2
	FrameBuffer * pCopyBuffer = frameBufferList().getCopyBuffer();
	if (pCopyBuffer != nullptr) {
		// This code is mainly to emulate Zelda MM camera.
		ColorBufferToRDRAM::get().copyToRDRAM(pCopyBuffer->m_startAddress, true);
		pCopyBuffer->m_RdramCopy.resize(0); // To disable validity check by RDRAM content. CPU may change content of the buffer for some unknown reason.
		frameBufferList().setCopyBuffer(nullptr);
		return true;
	} else
		return DepthBufferToRDRAM::get().copyToRDRAM(address);
#else
	return false;
#endif
}

bool FrameBuffer_CopyDepthBufferChunk(u32 address)
{
#ifndef GLES2
	return DepthBufferToRDRAM::get().copyChunkToRDRAM(address);
#else
	return false;
#endif
}

void FrameBuffer_CopyFromRDRAM(u32 _address, bool _bCFB)
{
	RDRAMtoColorBuffer::get().copyFromRDRAM(_address, _bCFB);
}

void FrameBuffer_AddAddress(u32 address, u32 _size)
{
	RDRAMtoColorBuffer::get().addAddress(address, _size);
}
