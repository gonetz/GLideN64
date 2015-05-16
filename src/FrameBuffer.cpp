#include <assert.h>
#include <math.h>
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

using namespace std;

#ifndef GLES2
const GLint monohromeInternalformat = GL_R8;
const GLenum monohromeformat = GL_RED;
#else
const GLint monohromeInternalformat = GL_LUMINANCE;
const GLenum monohromeformat = GL_LUMINANCE;
#endif // GLES2

#ifndef GLES2
class FrameBufferToRDRAM
{
public:
	FrameBufferToRDRAM() :
		m_FBO(0), m_PBO(0), m_pTexture(NULL)
	{}

	void Init();
	void Destroy();

	void CopyToRDRAM(u32 _address);

private:
	void _copyWhite(FrameBuffer * _pBuffer);

	struct RGBA {
		u8 r, g, b, a;
	};

	GLuint m_FBO;
	GLuint m_PBO;
	CachedTexture * m_pTexture;
};

class DepthBufferToRDRAM
{
public:
	DepthBufferToRDRAM() :
		m_FBO(0), m_PBO(0), m_pColorTexture(NULL), m_pDepthTexture(NULL), m_lastDList(0)
	{}

	void Init();
	void Destroy();

	bool CopyToRDRAM( u32 address );

private:
	GLuint m_FBO;
	GLuint m_PBO;
	CachedTexture * m_pColorTexture;
	CachedTexture * m_pDepthTexture;
	u32 m_lastDList;
};
#endif // GLES2

class RDRAMtoFrameBuffer
{
public:
	RDRAMtoFrameBuffer() : m_pTexture(NULL), m_PBO(0) {}

	void Init();
	void Destroy();

	void CopyFromRDRAM( u32 _address, bool _bUseAlpha);

private:
	CachedTexture * m_pTexture;
#ifndef GLES2
	GLuint m_PBO;
#else
	GLubyte* m_PBO;
#endif
};

#ifndef GLES2
FrameBufferToRDRAM g_fbToRDRAM;
DepthBufferToRDRAM g_dbToRDRAM;
#endif
RDRAMtoFrameBuffer g_RDRAMtoFB;

FrameBuffer::FrameBuffer() : m_validityChecked(0), m_cleared(false), m_changed(false), m_isDepthBuffer(false),
	m_needHeightCorrection(false), m_postProcessed(false), m_pLoadTile(NULL), m_pDepthBuffer(NULL),
	m_pResolveTexture(NULL), m_resolveFBO(0), m_copiedToRdram(false), m_resolved(false)
{
	m_pTexture = textureCache().addFrameBufferTexture();
	glGenFramebuffers(1, &m_FBO);
}

FrameBuffer::FrameBuffer(FrameBuffer && _other) :
	m_startAddress(_other.m_startAddress), m_endAddress(_other.m_endAddress),
	m_size(_other.m_size), m_width(_other.m_width), m_height(_other.m_height), m_fillcolor(_other.m_fillcolor),
	m_scaleX(_other.m_scaleX), m_scaleY(_other.m_scaleY), m_cleared(_other.m_cleared), m_changed(_other.m_changed), m_cfb(_other.m_cfb), m_isDepthBuffer(_other.m_isDepthBuffer),
	m_copiedToRdram(_other.m_copiedToRdram), m_needHeightCorrection(_other.m_needHeightCorrection), m_postProcessed(_other.m_postProcessed), m_validityChecked(_other.m_validityChecked),
	m_FBO(_other.m_FBO), m_pLoadTile(_other.m_pLoadTile), m_pTexture(_other.m_pTexture), m_pDepthBuffer(_other.m_pDepthBuffer),
	m_pResolveTexture(_other.m_pResolveTexture), m_resolveFBO(_other.m_resolveFBO), m_resolved(_other.m_resolved), m_RdramCopy(_other.m_RdramCopy)
{
	_other.m_FBO = 0;
	_other.m_pTexture = NULL;
	_other.m_pLoadTile = NULL;
	_other.m_pDepthBuffer = NULL;
	_other.m_pResolveTexture = NULL;
	_other.m_resolveFBO = 0;
	_other.m_RdramCopy.clear();
}


FrameBuffer::~FrameBuffer()
{
	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);
	if (m_pTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pTexture);
	if (m_resolveFBO != 0)
		glDeleteFramebuffers(1, &m_resolveFBO);
	if (m_pResolveTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pResolveTexture);
}

void FrameBuffer::_initTexture(u16 _format, u16 _size, CachedTexture *_pTexture)
{
	_pTexture->width = (u32)(m_width * m_scaleX);
	_pTexture->height = (u32)(m_height * m_scaleY);
	_pTexture->format = _format;
	_pTexture->size = _size;
	_pTexture->clampS = 1;
	_pTexture->clampT = 1;
	_pTexture->address = m_startAddress;
	_pTexture->clampWidth = m_width;
	_pTexture->clampHeight = m_height;
	_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	_pTexture->maskS = 0;
	_pTexture->maskT = 0;
	_pTexture->mirrorS = 0;
	_pTexture->mirrorT = 0;
	_pTexture->realWidth = _pTexture->width;
	_pTexture->realHeight = _pTexture->height;
	_pTexture->textureBytes = _pTexture->realWidth * _pTexture->realHeight;
	if (_size > G_IM_SIZ_8b)
		_pTexture->textureBytes *= 4;
	textureCache().addFrameBufferTextureSize(_pTexture->textureBytes);
}

void FrameBuffer::_setAndAttachTexture(u16 _size, CachedTexture *_pTexture)
{
	glBindTexture(GL_TEXTURE_2D, _pTexture->glName);
#ifdef GLES2
	if (_size > G_IM_SIZ_8b)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _pTexture->realWidth, _pTexture->realHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, monohromeInternalformat, _pTexture->realWidth, _pTexture->realHeight, 0, monohromeformat, GL_UNSIGNED_BYTE, NULL);
#else
	if (_size > G_IM_SIZ_8b)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _pTexture->realWidth, _pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, monohromeInternalformat, _pTexture->realWidth, _pTexture->realHeight, 0, monohromeformat, GL_UNSIGNED_BYTE, NULL);
#endif
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

void FrameBuffer::init(u32 _address, u32 _endAddress, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb)
{
	OGLVideo & ogl = video();
	m_startAddress = _address;
	m_endAddress = _endAddress;
	m_width = _width;
	m_height = _height;
	m_size = _size;
	m_scaleX = ogl.getScaleX();
	m_scaleY = ogl.getScaleY();
	m_fillcolor = 0;
	m_cfb = _cfb;
	m_needHeightCorrection = _width != VI.width && _width != *REG.VI_WIDTH;
	m_cleared = false;

	_initTexture(_format, _size, m_pTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling != 0) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pTexture->glName);
#if defined(GLES3_1)
		if (_size > G_IM_SIZ_8b)
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_RGBA8, m_pTexture->realWidth, m_pTexture->realHeight, false);
		else
			glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, monohromeInternalformat, m_pTexture->realWidth, m_pTexture->realHeight, false);
#else
		if (_size > G_IM_SIZ_8b)
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_RGBA8, m_pTexture->realWidth, m_pTexture->realHeight, false);
		else
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, monohromeInternalformat, m_pTexture->realWidth, m_pTexture->realHeight, false);
#endif
		m_pTexture->frameBufferTexture = CachedTexture::fbMultiSample;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_pTexture->glName, 0);

		m_pResolveTexture = textureCache().addFrameBufferTexture();
		_initTexture(_format, _size, m_pResolveTexture);
		glGenFramebuffers(1, &m_resolveFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFBO);
		_setAndAttachTexture(_size, m_pResolveTexture);
		assert(checkFBO());

		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	} else
#endif // GL_MULTISAMPLING_SUPPORT
		_setAndAttachTexture(_size, m_pTexture);
}

void FrameBuffer::reinit(u16 _height)
{
	const u16 format = m_pTexture->format;
	const u32 endAddress = m_startAddress + ((m_width * (_height - 1)) << m_size >> 1) - 1;
	if (m_pTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pTexture);
	if (m_resolveFBO != 0)
		glDeleteFramebuffers(1, &m_resolveFBO);
	if (m_pResolveTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pResolveTexture);
	m_pTexture = textureCache().addFrameBufferTexture();
	init(m_startAddress, endAddress, format, m_size, m_width, _height, m_cfb);
}

inline
u32 _cutHeight(u32 _address, u32 _height, u32 _stride)
{
	if (_address + _stride * _height > RDRAMSize)
		_height = (RDRAMSize - _address) / _stride;
	return _height;
}

void FrameBuffer::copyRdram()
{
	const u32 stride = m_width << m_size >> 1;
	const u32 height = _cutHeight(m_startAddress, m_height, stride);
	const u32 dataSize = stride * height;

	// Auxiliary frame buffer
	if (m_width != VI.width) {
		if (config.frameBufferEmulation.validityCheckMethod == Config::vcFill) {
			gDPFillRDRAM(m_startAddress, 0, 0, m_width, height, m_width, m_size, m_fillcolor, false);
			return;
		}
		if (config.frameBufferEmulation.validityCheckMethod == Config::vcFingerprint) {
			// Write small amount of data to the start of the buffer.
			// This is necessary for auxilary buffers: game can restore content of RDRAM when buffer is not needed anymore
			// Thus content of RDRAM on moment of buffer creation will be the same as when buffer becomes obsolete.
			// Validity check will see that the RDRAM is the same and thus the buffer is valid, which is false.
			// It can be enough to write data just little more than treshold level, but more safe to write twice as much in case that some values in buffer match our fingerprint.
			const u32 twoPercent = dataSize / 200;
			u32 start = m_startAddress >> 2;
			u32 * pData = (u32*)RDRAM;
			for (u32 i = 0; i < twoPercent; ++i)
				pData[start++] = m_startAddress;
		}
	}

	m_RdramCopy.resize(dataSize);
	memcpy(m_RdramCopy.data(), RDRAM + m_startAddress, dataSize);
}

bool FrameBuffer::isValid() const
{
	if (m_validityChecked == RSP.DList)
		return true; // Already checked

	const u32 * const pData = (const u32*)RDRAM;

	if (m_cleared) {
		const u32 color = m_fillcolor & 0xFFFEFFFE;
		const u32 start = m_startAddress >> 2;
		const u32 end = m_endAddress >> 2;
		u32 wrongPixels = 0;
		for (u32 i = start; i < end; ++i) {
			if ((pData[i] & 0xFFFEFFFE) != color)
				++wrongPixels;
		}
		return wrongPixels < (m_endAddress - m_startAddress) / 400; // treshold level 1% of dwords
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
		return wrongPixels < size / 400; // treshold level 1% of dwords
	}
	return true; // No data to decide
}

void FrameBuffer::resolveMultisampledTexture()
{
#ifdef GL_MULTISAMPLING_SUPPORT
	if (m_resolved)
		return;
	glScissor(0, 0, m_pTexture->realWidth, m_pTexture->realHeight);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFBO);
	glBlitFramebuffer(
		0, 0, m_pTexture->realWidth, m_pTexture->realHeight,
		0, 0, m_pResolveTexture->realWidth, m_pResolveTexture->realHeight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST
		);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);
	gDP.changed |= CHANGED_SCISSOR;
	m_resolved = true;
#endif
}

CachedTexture * FrameBuffer::getTexture()
{
	if (config.video.multisampling == 0)
		return m_pTexture;
	if (m_resolved)
		return m_pResolveTexture;

	resolveMultisampledTexture();
	return m_pResolveTexture;
}

FrameBufferList & FrameBufferList::get()
{
	static FrameBufferList frameBufferList;
	return frameBufferList;
}

void FrameBufferList::init()
{
	 m_pCurrent = NULL;
}

void FrameBufferList::destroy() {
	m_list.clear();
	m_pCurrent = NULL;
}

void FrameBufferList::setBufferChanged()
{
	gDP.colorImage.changed = TRUE;
	if (m_pCurrent != NULL) {
		m_pCurrent->m_changed = true;
		m_pCurrent->m_copiedToRdram = false;
		m_pCurrent->m_validityChecked = 0;
	}
}

void FrameBufferList::correctHeight()
{
	if (m_pCurrent == NULL)
		return;
	if (m_pCurrent->m_changed) {
		m_pCurrent->m_needHeightCorrection = false;
		return;
	}
	if (m_pCurrent->m_needHeightCorrection && m_pCurrent->m_width == gDP.scissor.lrx) {
		if (m_pCurrent->m_height < (u32)gDP.scissor.lry)
			m_pCurrent->reinit((u32)gDP.scissor.lry);
		else
			m_pCurrent->m_height = (u32)gDP.scissor.lry;

		if (m_pCurrent->_isMarioTennisScoreboard())
			g_RDRAMtoFB.CopyFromRDRAM(m_pCurrent->m_startAddress + 4, false);

		m_pCurrent->m_needHeightCorrection = false;
		gSP.changed |= CHANGED_VIEWPORT;
	}
}

void FrameBufferList::clearBuffersChanged()
{
	gDP.colorImage.changed = FALSE;
	FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
	if (pBuffer != NULL)
		pBuffer->m_changed = false;
}

FrameBuffer * FrameBufferList::findBuffer(u32 _startAddress)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
	if (iter->m_startAddress <= _startAddress && iter->m_endAddress >= _startAddress) // [  {  ]
		return &(*iter);
	return NULL;
}

FrameBuffer * FrameBufferList::_findBuffer(u32 _startAddress, u32 _endAddress, u32 _width)
{
	if (m_list.empty())
		return NULL;

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
	return NULL;
}

FrameBuffer * FrameBufferList::findTmpBuffer(u32 _address)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress > _address || iter->m_endAddress < _address)
				return &(*iter);
	return NULL;
}

void FrameBufferList::saveBuffer(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb)
{
	if (VI.width == 0 || _height == 0)
		return;

	OGLVideo & ogl = video();
	if (m_pCurrent != NULL) {
		if (gDP.colorImage.height > 0) {
			if (m_pCurrent->m_width == VI.width)
				gDP.colorImage.height = min(gDP.colorImage.height, VI.height);
			m_pCurrent->m_endAddress = min(RDRAMSize, m_pCurrent->m_startAddress + (((m_pCurrent->m_width * gDP.colorImage.height) << m_pCurrent->m_size >> 1) - 1));
			if (!config.frameBufferEmulation.copyFromRDRAM && !m_pCurrent->_isMarioTennisScoreboard() && !m_pCurrent->m_isDepthBuffer && !m_pCurrent->m_copiedToRdram && !m_pCurrent->m_cfb && !m_pCurrent->m_cleared && m_pCurrent->m_RdramCopy.empty() && gDP.colorImage.height > 1) {
				m_pCurrent->copyRdram();
			}
		}
		m_pCurrent = _findBuffer(m_pCurrent->m_startAddress, m_pCurrent->m_endAddress, m_pCurrent->m_width);
	}

	const u32 endAddress = _address + ((_width * (_height - 1)) << _size >> 1) - 1;
	if (m_pCurrent == NULL || m_pCurrent->m_startAddress != _address || m_pCurrent->m_width != _width)
		m_pCurrent = findBuffer(_address);
	if (m_pCurrent != NULL) {
		if ((m_pCurrent->m_startAddress != _address) ||
			(m_pCurrent->m_width != _width) ||
			//(current->height != height) ||
			//(current->size != size) ||  // TODO FIX ME
			(m_pCurrent->m_scaleX != ogl.getScaleX()) ||
			(m_pCurrent->m_scaleY != ogl.getScaleY()))
		{
			removeBuffer(m_pCurrent->m_startAddress);
			m_pCurrent = NULL;
		} else {
			m_pCurrent->m_resolved = false;
			glBindFramebuffer(GL_FRAMEBUFFER, m_pCurrent->m_FBO);
			if (m_pCurrent->m_size != _size) {
				f32 fillColor[4];
				gDPGetFillColor(fillColor);
				ogl.getRender().clearColorBuffer(fillColor);
				m_pCurrent->m_size = _size;
				m_pCurrent->m_pTexture->format = _format;
				m_pCurrent->m_pTexture->size = _size;
				if (m_pCurrent->m_pResolveTexture != NULL) {
					m_pCurrent->m_pResolveTexture->format = _format;
					m_pCurrent->m_pResolveTexture->size = _size;
				}
				if (m_pCurrent->m_copiedToRdram)
					m_pCurrent->copyRdram();
			}
		}
	}
	const bool bNew = m_pCurrent == NULL;
	if  (bNew) {
		// Wasn't found or removed, create a new one
		m_list.emplace_front();
		FrameBuffer & buffer = m_list.front();
		buffer.init(_address, endAddress, _format, _size, _width, _height, _cfb);
		m_pCurrent = &buffer;

		if (m_pCurrent->_isMarioTennisScoreboard() || ((config.generalEmulation.hacks & hack_legoRacers) != 0 && _width == VI.width))
			g_RDRAMtoFB.CopyFromRDRAM(m_pCurrent->m_startAddress + 4, false);
	}

	if (_address == gDP.depthImageAddress)
		depthBufferList().saveBuffer(_address);
	else
		attachDepthBuffer();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "FrameBuffer_SaveBuffer( 0x%08X ); depth buffer is 0x%08X\n",
		address, (depthBuffer.top != NULL && depthBuffer.top->renderbuf > 0) ? depthBuffer.top->address : 0
	);
#endif

	m_pCurrent->m_isDepthBuffer = _address == gDP.depthImageAddress;
	m_pCurrent->m_isPauseScreen = m_pCurrent->m_isOBScreen = false;
	m_pCurrent->m_postProcessed = false;
}

void FrameBufferList::removeBuffer(u32 _address )
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress == _address) {
			if (&(*iter) == m_pCurrent)
				m_pCurrent = NULL;
			m_list.erase(iter);
			return;
		}
}

void FrameBufferList::removeBuffers(u32 _width)
{
	m_pCurrent = NULL;
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
		while (iter->m_width == _width) {
			if (&(*iter) == m_pCurrent)
				m_pCurrent = NULL;
			iter = m_list.erase(iter);
			if (iter == m_list.end())
				return;
		}
	}
}

void FrameBufferList::attachDepthBuffer()
{
	if (m_pCurrent == NULL)
		return;

	DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
	if (m_pCurrent->m_FBO > 0 && pDepthBuffer != NULL) {
		pDepthBuffer->initDepthImageTexture(m_pCurrent);
		pDepthBuffer->initDepthBufferTexture(m_pCurrent);
		if (pDepthBuffer->m_pDepthBufferTexture->realWidth >= m_pCurrent->m_pTexture->realWidth) {
			m_pCurrent->m_pDepthBuffer = pDepthBuffer;
			pDepthBuffer->setDepthAttachment(GL_DRAW_FRAMEBUFFER);
			if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
				pDepthBuffer->bindDepthImageTexture();
		} else
			m_pCurrent->m_pDepthBuffer = NULL;
	} else
		m_pCurrent->m_pDepthBuffer = NULL;

#ifndef GLES2
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);
#endif
	assert(checkFBO());
}

void FrameBuffer_Init()
{
	frameBufferList().init();
	if (config.frameBufferEmulation.enable != 0) {
#ifndef GLES2
	g_fbToRDRAM.Init();
	g_dbToRDRAM.Init();
#endif
	g_RDRAMtoFB.Init();
	}
}

void FrameBuffer_Destroy()
{
	if (config.frameBufferEmulation.enable != 0) {
	g_RDRAMtoFB.Destroy();
#ifndef GLES2
	g_dbToRDRAM.Destroy();
	g_fbToRDRAM.Destroy();
#endif
	}
	frameBufferList().destroy();
}

#ifndef GLES2
void FrameBufferList::renderBuffer(u32 _address)
{
	static s32 vStartPrev = 0;

	if (VI.width == 0 || *REG.VI_WIDTH == 0 || *REG.VI_H_START == 0) // H width is zero. Don't draw
		return;

	FrameBuffer *pBuffer = findBuffer(_address);
	if (pBuffer == NULL)
		return;

	OGLVideo & ogl = video();
	GLint srcY0, srcY1, dstY0, dstY1;
	GLint X0, X1, Xwidth;
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

	srcY0 = ((_address - pBuffer->m_startAddress) << 1 >> pBuffer->m_size) / (*REG.VI_WIDTH);
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

	const f32 scaleX = _FIXED2FLOAT(_SHIFTR(*REG.VI_X_SCALE, 0, 12), 10);
	const s32 h0 = (isPAL ? 128 : 108);
	const s32 hx0 = max(0, hStart - h0);
	const s32 hx1 = max(0, h0 + 640 - hEnd);
	X0 = (GLint)(hx0 * scaleX * ogl.getScaleX());
	Xwidth = (GLint)((min((f32)VI.width, (hEnd - hStart)*scaleX)) * ogl.getScaleX());
	X1 = ogl.getWidth() - (GLint)(hx1 *scaleX * ogl.getScaleX());

	const float srcScaleY = ogl.getScaleY();
	const GLint hOffset = (ogl.getScreenWidth() - ogl.getWidth()) / 2;
	const GLint vOffset = (ogl.getScreenHeight() - ogl.getHeight()) / 2 + ogl.getHeightOffset();
	CachedTexture * pBufferTexture = pBuffer->m_pTexture;
	GLint srcCoord[4] = { 0, (GLint)(srcY0*srcScaleY), Xwidth, min((GLint)(srcY1*srcScaleY), (GLint)pBufferTexture->realHeight) };
	if (srcCoord[2] > pBufferTexture->realWidth || srcCoord[3] > pBufferTexture->realHeight) {
		removeBuffer(pBuffer->m_startAddress);
		return;
	}
	GLint dstCoord[4] = { X0 + hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + X1, vOffset + (GLint)(dstY1*dstScaleY) };

	ogl.getRender().updateScissor(pBuffer);
	PostProcessor::get().process(pBuffer);
	// glDisable(GL_SCISSOR_TEST) does not affect glBlitFramebuffer, at least on AMD
	glScissor(0, 0, ogl.getScreenWidth(), ogl.getScreenHeight() + ogl.getHeightOffset());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//glDrawBuffer( GL_BACK );
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ogl.getRender().clearColorBuffer(clearColor);

	GLenum filter = GL_LINEAR;
	if (config.video.multisampling != 0) {
		if (X0 > 0 || dstPartHeight > 0 ||
			(srcCoord[2] - srcCoord[0]) != (dstCoord[2] - dstCoord[0]) ||
			(srcCoord[3] - srcCoord[1]) != (dstCoord[3] - dstCoord[1])) {
			pBuffer->resolveMultisampledTexture();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		} else {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
			filter = GL_NEAREST;
		}
	} else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);

	glBlitFramebuffer(
		srcCoord[0], srcCoord[1], srcCoord[2], srcCoord[3],
		dstCoord[0], dstCoord[1], dstCoord[2], dstCoord[3],
		GL_COLOR_BUFFER_BIT, filter
	);

	if (dstPartHeight > 0) {
		const u32 size = *REG.VI_STATUS & 3;
		pBuffer = findBuffer(_address + (((*REG.VI_WIDTH)*VI.height)<<size>>1));
		if (pBuffer != NULL) {
			srcY0 = 0;
			srcY1 = srcPartHeight;
			dstY0 = dstY1;
			dstY1 = dstY0 + dstPartHeight;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
			glBlitFramebuffer(
				0, (GLint)(srcY0*srcScaleY), ogl.getWidth(), min((GLint)(srcY1*srcScaleY), (GLint)pBuffer->m_pTexture->realHeight),
				hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + ogl.getWidth(), vOffset + (GLint)(dstY1*dstScaleY),
				GL_COLOR_BUFFER_BIT, filter
			);
		}
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	if (m_pCurrent != NULL)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
	ogl.swapBuffers();
	gDP.changed |= CHANGED_SCISSOR;
}
#else

void FrameBufferList::renderBuffer(u32 _address)
{
	if (VI.width == 0) // H width is zero. Don't draw
		return;
	FrameBuffer *pBuffer = findBuffer(_address);
	if (pBuffer == NULL)
		return;

	CombinerInfo::get().setCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1 ) );
	glDisable( GL_BLEND );
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable( GL_CULL_FACE );
	glDisable( GL_POLYGON_OFFSET_FILL );
	gSP.changed = gDP.changed = 0;

	const u32 width = pBuffer->m_width;
	const u32 height = pBuffer->m_height;

	OGLVideo & ogl = video();
	pBuffer->m_pTexture->scaleS = ogl.getScaleX() / (float)pBuffer->m_pTexture->realWidth;
	pBuffer->m_pTexture->scaleT = ogl.getScaleY() / (float)pBuffer->m_pTexture->realHeight;
	pBuffer->m_pTexture->shiftScaleS = 1.0f;
	pBuffer->m_pTexture->shiftScaleT = 1.0f;
	pBuffer->m_pTexture->offsetS = 0;
	pBuffer->m_pTexture->offsetT = (float)height;
	textureCache().activateTexture(0, pBuffer->m_pTexture);
	gSP.textureTile[0]->fuls = gSP.textureTile[0]->fult = 0.0f;
	currentCombiner()->updateTextureInfo(true);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	OGLRender::TexturedRectParams params(0.0f, 0.0f, width, height, 0.0f, 0.0f, width - 1.0f, height - 1.0f, false);
	ogl.getRender().drawTexturedRect(params);
	ogl.swapBuffers();
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, m_pCurrent->m_FBO);
	gSP.changed |= CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_COMBINE;
}
#endif

void FrameBuffer_ActivateBufferTexture(s16 t, FrameBuffer *pBuffer)
{
	if (pBuffer == NULL || pBuffer->m_pTexture == NULL)
		return;

	CachedTexture *pTexture = pBuffer->m_pTexture;

	pTexture->scaleS = pBuffer->m_scaleX / (float)pTexture->realWidth;
	pTexture->scaleT = pBuffer->m_scaleY / (float)pTexture->realHeight;

	if (gSP.textureTile[t]->shifts > 10)
		pTexture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[t]->shifts));
	else if (gSP.textureTile[t]->shifts > 0)
		pTexture->shiftScaleS = 1.0f / (float)(1 << gSP.textureTile[t]->shifts);
	else
		pTexture->shiftScaleS = 1.0f;

	if (gSP.textureTile[t]->shiftt > 10)
		pTexture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[t]->shiftt));
	else if (gSP.textureTile[t]->shiftt > 0)
		pTexture->shiftScaleT = 1.0f / (float)(1 << gSP.textureTile[t]->shiftt);
	else
		pTexture->shiftScaleT = 1.0f;

	const u32 shift = gSP.textureTile[t]->imageAddress - pBuffer->m_startAddress;
	const u32 factor = pBuffer->m_width << pBuffer->m_size >> 1;
	if (gSP.textureTile[t]->loadType == LOADTYPE_TILE)
	{
		pTexture->offsetS = (float)pBuffer->m_pLoadTile->uls;
		pTexture->offsetT = (float)(pBuffer->m_height - (pBuffer->m_pLoadTile->ult + shift/factor));
	}
	else
	{
		pTexture->offsetS = (float)((shift % factor) >> pBuffer->m_size << 1);
		pTexture->offsetT = (float)(pBuffer->m_height - shift/factor);
	}

//	frameBufferList().renderBuffer(pBuffer->m_startAddress);
	textureCache().activateTexture(t, pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_ActivateBufferTextureBG(s16 t, FrameBuffer *pBuffer )
{
	if (pBuffer == NULL || pBuffer->m_pTexture == NULL)
		return;

	CachedTexture *pTexture = pBuffer->m_pTexture;
	pTexture->scaleS = video().getScaleX() / (float)pTexture->realWidth;
	pTexture->scaleT = video().getScaleY() / (float)pTexture->realHeight;

	pTexture->shiftScaleS = 1.0f;
	pTexture->shiftScaleT = 1.0f;

	pTexture->offsetS = gSP.bgImage.imageX;
	pTexture->offsetT = (float)pBuffer->m_height - gSP.bgImage.imageY;

	//	FrameBuffer_RenderBuffer(buffer->startAddress);
	textureCache().activateTexture(t, pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

#ifndef GLES2
void FrameBufferToRDRAM::Init()
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
	glBindTexture( GL_TEXTURE_2D, m_pTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTexture->glName, 0);
	// check if everything is OK
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// Generate and initialize Pixel Buffer Objects
	glGenBuffers(1, &m_PBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, NULL, GL_DYNAMIC_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void FrameBufferToRDRAM::Destroy() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (m_FBO != 0) {
		glDeleteFramebuffers(1, &m_FBO);
		m_FBO = 0;
	}
	if (m_pTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = NULL;
	}
	if (m_PBO != 0) {
		glDeleteBuffers(1, &m_PBO);
		m_PBO = 0;
	}
}

void FrameBufferToRDRAM::_copyWhite(FrameBuffer * _pBuffer)
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

void FrameBufferToRDRAM::CopyToRDRAM(u32 _address)
{
	if (VI.width == 0 || frameBufferList().getCurrent() == NULL) // H width is zero or no current buffer. Don't copy
		return;
	FrameBuffer *pBuffer = frameBufferList().findBuffer(_address);
	if (pBuffer == NULL || pBuffer->m_width < VI.width || pBuffer->m_isOBScreen)
		return;

	if ((config.generalEmulation.hacks & hack_subscreen) != 0) {
		_copyWhite(pBuffer);
		return;
	}

	_address = pBuffer->m_startAddress;
	if (config.video.multisampling != 0) {
		pBuffer->resolveMultisampledTexture();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
	} else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glScissor(0, 0, pBuffer->m_pTexture->realWidth, pBuffer->m_pTexture->realHeight);
	glBlitFramebuffer(
		0, 0, video().getWidth(), video().getHeight(),
		0, 0, VI.width, VI.height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST
	);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#ifndef GLES2
	PBOBinder binder(GL_PIXEL_PACK_BUFFER, m_PBO);
	glReadPixels(0, 0, VI.width, VI.height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	isGLError();
	GLubyte* pixelData = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, VI.width * VI.height * 4, GL_MAP_READ_BIT);
	isGLError();
	if(pixelData == NULL)
		return;
#else
	m_curIndex = 0;
	const u32 nextIndex = 0;
	GLubyte* pixelData = (GLubyte* )malloc(VI.width*VI.height*4);
	if(pixelData == NULL)
		return;
	glReadPixels( 0, 0, VI.width, VI.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelData );
#endif // GLES2

	const u32 stride = pBuffer->m_width << pBuffer->m_size >> 1;
	const u32 height = _cutHeight(_address, VI.height, stride);

	if (pBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_dst = (u32*)(RDRAM + _address);
		u32 *ptr_src = (u32*)pixelData;

		for (u32 y = 0; y < height; ++y) {
			for (u32 x = 0; x < VI.width; ++x)
				ptr_dst[x + y*VI.width] = ptr_src[x + (height - y - 1)*VI.width];
		}
	} else {
		u16 *ptr_dst = (u16*)(RDRAM + _address);
		RGBA * ptr_src = (RGBA*)pixelData;

		for (u32 y = 0; y < height; ++y) {
			for (u32 x = 0; x < VI.width; ++x) {
				const RGBA & c = ptr_src[x + (height - y - 1)*VI.width];
				ptr_dst[(x + y*VI.width)^1] = ((c.r>>3)<<11) | ((c.g>>3)<<6) | ((c.b>>3)<<1) | (c.a == 0 ? 0 : 1);
			}
		}
	}
	pBuffer->m_copiedToRdram = true;
	pBuffer->copyRdram();
	pBuffer->m_cleared = false;
#ifndef GLES2
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
#else
	free(pixelData);
#endif
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gDP.changed |= CHANGED_SCISSOR;
}
#endif // GLES2

void FrameBuffer_CopyToRDRAM(u32 _address)
{
#ifndef GLES2
	g_fbToRDRAM.CopyToRDRAM(_address);
#endif
}

#ifndef GLES2
void DepthBufferToRDRAM::Init()
{
	// generate a framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

	m_pColorTexture = textureCache().addFrameBufferTexture();
	m_pColorTexture->format = G_IM_FMT_I;
	m_pColorTexture->clampS = 1;
	m_pColorTexture->clampT = 1;
	m_pColorTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pColorTexture->maskS = 0;
	m_pColorTexture->maskT = 0;
	m_pColorTexture->mirrorS = 0;
	m_pColorTexture->mirrorT = 0;
	m_pColorTexture->realWidth = 640;
	m_pColorTexture->realHeight = 580;
	m_pColorTexture->textureBytes = m_pColorTexture->realWidth * m_pColorTexture->realHeight;
	textureCache().addFrameBufferTextureSize(m_pColorTexture->textureBytes);

	m_pDepthTexture = textureCache().addFrameBufferTexture();
	m_pDepthTexture->format = G_IM_FMT_I;
	m_pDepthTexture->clampS = 1;
	m_pDepthTexture->clampT = 1;
	m_pDepthTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pDepthTexture->maskS = 0;
	m_pDepthTexture->maskT = 0;
	m_pDepthTexture->mirrorS = 0;
	m_pDepthTexture->mirrorT = 0;
	m_pDepthTexture->realWidth = 640;
	m_pDepthTexture->realHeight = 580;
	m_pDepthTexture->textureBytes = m_pDepthTexture->realWidth * m_pDepthTexture->realHeight * sizeof(float);
	textureCache().addFrameBufferTextureSize(m_pDepthTexture->textureBytes);

	glBindTexture( GL_TEXTURE_2D, m_pColorTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_pColorTexture->realWidth, m_pColorTexture->realHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D, m_pDepthTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, DEPTH_COMPONENT_FORMAT, m_pDepthTexture->realWidth, m_pDepthTexture->realHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pColorTexture->glName, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthTexture->glName, 0);
	// check if everything is OK
	assert(checkFBO());
	assert(!isGLError());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// Generate and initialize Pixel Buffer Objects
	glGenBuffers(1, &m_PBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_PACK_BUFFER, m_pDepthTexture->realWidth * m_pDepthTexture->realHeight * sizeof(float), NULL, GL_DYNAMIC_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void DepthBufferToRDRAM::Destroy() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &m_FBO);
	m_FBO = 0;
	if (m_pColorTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pColorTexture);
		m_pColorTexture = NULL;
	}
	if (m_pDepthTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pDepthTexture);
		m_pDepthTexture = NULL;
	}
	glDeleteBuffers(1, &m_PBO);
	m_PBO = 0;
}

bool DepthBufferToRDRAM::CopyToRDRAM( u32 _address) {
	if (VI.width == 0) // H width is zero. Don't copy
		return false;
	if (m_lastDList == RSP.DList) // Already read;
		return true;
	FrameBuffer *pBuffer = frameBufferList().findBuffer(_address);
	if (pBuffer == NULL || pBuffer->m_width < VI.width || pBuffer->m_pDepthBuffer == NULL || !pBuffer->m_pDepthBuffer->m_cleared)
		return false;

	m_lastDList = RSP.DList;
	DepthBuffer * pDepthBuffer = pBuffer->m_pDepthBuffer;
	const u32 address = pDepthBuffer->m_address;
	if (address + VI.width*VI.height*2 > RDRAMSize)
		return false;

	if (config.video.multisampling == 0)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	else {
		pDepthBuffer->resolveDepthBufferTexture(pBuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glScissor(0, 0, pBuffer->m_pTexture->realWidth, pBuffer->m_pTexture->realHeight);
	glBlitFramebuffer(
		0, 0, video().getWidth(), video().getHeight(),
		0, 0, pBuffer->m_width, pBuffer->m_height,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
	);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	PBOBinder binder(GL_PIXEL_PACK_BUFFER, m_PBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadPixels(0, 0, VI.width, VI.height, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	GLubyte* pixelData = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, VI.width * VI.height * 4, GL_MAP_READ_BIT);
	if(pixelData == NULL)
		return false;

	f32 * ptr_src = (f32*)pixelData;
	u16 *ptr_dst = (u16*)(RDRAM + address);
	const u16 * const zLUT = depthBufferList().getZLUT();
	const u32 height = _cutHeight(address, min(VI.height, pDepthBuffer->m_lry), pBuffer->m_width * 2);

	for (u32 y = pDepthBuffer->m_uly; y < height; ++y) {
		for (u32 x = 0; x < VI.width; ++x) {
			f32 z = ptr_src[x + (height - y - 1)*VI.width];
			u32 idx = 0x3FFFF;
			if (z < 1.0f) {
				z *= 262144.0f;
				idx = min(0x3FFFFU, u32(floorf(z + 0.5f)));
			}
			ptr_dst[(x + y*VI.width) ^ 1] = zLUT[idx];
		}
	}

	pDepthBuffer->m_cleared = false;
	pBuffer = frameBufferList().findBuffer(pDepthBuffer->m_address);
	if (pBuffer != NULL)
		pBuffer->m_cleared = false;

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gDP.changed |= CHANGED_SCISSOR;
	return true;
}
#endif // GLES2

bool FrameBuffer_CopyDepthBuffer( u32 address ) {
#ifndef GLES2
	return g_dbToRDRAM.CopyToRDRAM(address);
#else
	return false;
#endif
}

void RDRAMtoFrameBuffer::Init()
{
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
	glBindTexture( GL_TEXTURE_2D, m_pTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glBindTexture(GL_TEXTURE_2D, 0);

	// Generate Pixel Buffer Object. Initialize it later
#ifndef GLES2
	glGenBuffers(1, &m_PBO);
#endif
}

void RDRAMtoFrameBuffer::Destroy()
{
	if (m_pTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = NULL;
	}
#ifndef GLES2
	glDeleteBuffers(1, &m_PBO);
	m_PBO = 0;
#endif
}

void RDRAMtoFrameBuffer::CopyFromRDRAM( u32 _address, bool _bUseAlpha)
{
	FrameBuffer *pBuffer = frameBufferList().findBuffer(_address);
	if (pBuffer == NULL || pBuffer->m_size < G_IM_SIZ_16b)
		return;
	if (pBuffer->m_startAddress == _address && gDP.colorImage.changed != 0)
		return;

	const bool bUseAlpha = _bUseAlpha && pBuffer->m_changed;
	const u32 address = pBuffer->m_startAddress;
	const u32 width = pBuffer->m_width;
	const u32 height = pBuffer->m_startAddress == _address ? VI.real_height : pBuffer->m_height;
	m_pTexture->width = width;
	m_pTexture->height = height;
	const u32 dataSize = width*height*4;
#ifndef GLES2
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, NULL, GL_DYNAMIC_DRAW);
	GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
#else
	m_PBO = (GLubyte*)malloc(dataSize);
	GLubyte* ptr = m_PBO;
	PBOBinder binder(m_PBO);
#endif // GLES2
	if (ptr == NULL)
		return;

	u8 * image = RDRAM + address;
	u32 * dst = (u32*)ptr;

	u32 empty = 0;
	u32 r, g, b, a, idx;
	if (pBuffer->m_size == G_IM_SIZ_16b) {
		u16 * src = (u16*)image;
		u16 col;
		const u32 bound = (RDRAMSize + 1 - address) >> 1;
		for (u32 y = 0; y < height; y++)
		{
			for (u32 x = 0; x < width; x++)
			{
				idx = (x + (height - y - 1)*width)^1;
				if (idx >= bound)
					break;
				col = src[idx];
				if (bUseAlpha)
					src[idx] = 0;
				empty |= col;
				r = ((col >> 11)&31)<<3;
				g = ((col >> 6)&31)<<3;
				b = ((col >> 1)&31)<<3;
				a = (col&1) > 0 && (r|g|b) > 0 ? 0xff : 0U;
				dst[x + y*width] = (a << 24) | (b << 16) | (g << 8) | r;
			}
		}
	} else {
		// 32 bit
		u32 * src = (u32*)image;
		u32 col;
		const u32 bound = (RDRAMSize + 1 - address) >> 2;
		for (u32 y=0; y < height; y++)
		{
			for (u32 x=0; x < width; x++)
			{
				idx = x + (height - y - 1)*width;
				if (idx >= bound)
					break;
				col = src[idx];
				if (bUseAlpha)
					src[idx] = 0;
				empty |= col;
				r = (col >> 24) & 0xff;
				g = (col >> 16) & 0xff;
				b = (col >> 8) & 0xff;
				a = (r|g|b) > 0 ? col & 0xff : 0U;
				dst[x + y*width] = (a<<24)|(b<<16)|(g<<8)|r;
			}
		}
	}
#ifndef GLES2
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
#endif
	if (empty == 0)
		return;

	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
#ifndef GLES2
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, m_PBO);
#endif

	m_pTexture->scaleS = 1.0f / (float)m_pTexture->realWidth;
	m_pTexture->scaleT = 1.0f / (float)m_pTexture->realHeight;
	m_pTexture->shiftScaleS = 1.0f;
	m_pTexture->shiftScaleT = 1.0f;
	m_pTexture->offsetS = 0;
	m_pTexture->offsetT = (float)m_pTexture->height;
	textureCache().activateTexture(0, m_pTexture);

	gDPTile tile0;
	tile0.fuls = tile0.fult = 0.0f;
	gDPTile * pTile0 = gSP.textureTile[0];
	gSP.textureTile[0] = &tile0;

	if (_bUseAlpha) {
		CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else {
		CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1));
		glDisable(GL_BLEND);
	}
	currentCombiner()->updateFBInfo();

	glDisable(GL_DEPTH_TEST);
	const u32 gdpChanged = gDP.changed & CHANGED_CPU_FB_WRITE;
	gSP.changed = gDP.changed = 0;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pBuffer->m_FBO);
	OGLRender::TexturedRectParams params(0.0f, 0.0f, (float)width, (float)height, 0.0f, 0.0f, width - 1.0f, height - 1.0f, false);
	video().getRender().drawTexturedRect(params);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	gSP.textureTile[0] = pTile0;

	gDP.changed |= gdpChanged | CHANGED_RENDERMODE | CHANGED_COMBINE;
}

void FrameBuffer_CopyFromRDRAM( u32 address, bool bUseAlpha )
{
	g_RDRAMtoFB.CopyFromRDRAM(address, bUseAlpha);
}
