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
#include "Log.h"

#ifdef ANDROID
#include "ui/GraphicBuffer.h"
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, EGLImageKHR image);
typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) (GLenum target, EGLImageKHR image);

using namespace android;
#endif

using namespace std;

#ifndef GLES2
class FrameBufferToRDRAM
{
public:
	FrameBufferToRDRAM() :
		m_FBO(0),
		m_pTexture(nullptr),
		m_pCurFrameBuffer(nullptr),
		m_curIndex(-1),
		m_frameCount(-1),
		m_startAddress(-1)
	{
		m_PBO[0] = m_PBO[1] = m_PBO[2] = 0;
	}

	void Init();
	void Destroy();

	void copyToRDRAM(u32 _address, bool _sync);
	void copyChunkToRDRAM(u32 _address);

private:
	union RGBA {
		struct {
			u8 r, g, b, a;
		};
		u32 raw;
	};

	bool _prepareCopy(u32 _startAddress);

	GLubyte* getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, bool _sync);
	void cleanUpPixels(GLubyte* pixelData);
	void _copy(u32 _startAddress, u32 _endAddress, bool _sync);

	// Convert pixel from video memory to N64 buffer format.
	static u8 _RGBAtoR8(u8 _c);
	static u16 _RGBAtoRGBA16(u32 _c);
	static u32 _RGBAtoRGBA32(u32 _c);

	GLuint m_FBO;
	CachedTexture * m_pTexture;
	FrameBuffer * m_pCurFrameBuffer;
	u32 m_curIndex;
	u32 m_frameCount;
	u32 m_startAddress;
	GLuint m_PBO[3];

#ifdef ANDROID
	GraphicBuffer* m_window;
	EGLImageKHR m_image;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES;
#endif
};

class DepthBufferToRDRAM
{
public:
	DepthBufferToRDRAM() :
		m_FBO(0),
		m_PBO(0),
		m_frameCount(-1),
		m_pColorTexture(nullptr),
		m_pDepthTexture(nullptr),
		m_pCurDepthBuffer(nullptr)
	{}

	void Init();
	void Destroy();

	bool copyToRDRAM(u32 _address);
	bool copyChunkToRDRAM(u32 _address);

private:
	bool _prepareCopy(u32 _address, bool _copyChunk);
	bool _copy(u32 _startAddress, u32 _endAddress);

	// Convert pixel from video memory to N64 depth buffer format.
	static u16 _FloatToUInt16(f32 _z);

	GLuint m_FBO;
	GLuint m_PBO;
	u32 m_frameCount;
	CachedTexture * m_pColorTexture;
	CachedTexture * m_pDepthTexture;
	DepthBuffer * m_pCurDepthBuffer;
};
#endif // GLES2

class RDRAMtoFrameBuffer
{
public:
	RDRAMtoFrameBuffer()
		: m_pCurBuffer(nullptr)
		, m_pTexture(nullptr)
		, m_PBO(0) {
	}

	void Init();
	void Destroy();

	void AddAddress(u32 _address, u32 _size);
	void CopyFromRDRAM(u32 _address, bool _bCFB);

private:
	void Reset();
	class Cleaner
	{
	public:
		Cleaner(RDRAMtoFrameBuffer * _p) : m_p(_p) {}
		~Cleaner()
		{
			m_p->Reset();
		}
	private:
		RDRAMtoFrameBuffer * m_p;
	};

	FrameBuffer * m_pCurBuffer;
	CachedTexture * m_pTexture;
	GLuint m_PBO;
	std::vector<u32> m_vecAddress;
};

#ifndef GLES2
FrameBufferToRDRAM g_fbToRDRAM;
DepthBufferToRDRAM g_dbToRDRAM;
#endif
RDRAMtoFrameBuffer g_RDRAMtoFB;

FrameBuffer::FrameBuffer() :
	m_startAddress(0), m_endAddress(0), m_size(0), m_width(0), m_height(0), m_fillcolor(0), m_validityChecked(0),
	m_scaleX(0), m_scaleY(0),
	m_copiedToRdram(false), m_fingerprint(false), m_cleared(false), m_changed(false), m_cfb(false),
	m_isDepthBuffer(false), m_isPauseScreen(false), m_isOBScreen(false), m_needHeightCorrection(false),
	m_loadType(LOADTYPE_BLOCK), m_pDepthBuffer(NULL),
	m_resolveFBO(0), m_pResolveTexture(NULL), m_resolved(false),
	m_SubFBO(0), m_pSubTexture(NULL)
{
	m_loadTileOrigin.uls = m_loadTileOrigin.ult = 0;
	m_pTexture = textureCache().addFrameBufferTexture();
	glGenFramebuffers(1, &m_FBO);
}

FrameBuffer::FrameBuffer(FrameBuffer && _other) :
	m_startAddress(_other.m_startAddress), m_endAddress(_other.m_endAddress),
	m_size(_other.m_size), m_width(_other.m_width), m_height(_other.m_height), m_fillcolor(_other.m_fillcolor),
	m_validityChecked(_other.m_validityChecked), m_scaleX(_other.m_scaleX), m_scaleY(_other.m_scaleY), m_copiedToRdram(_other.m_copiedToRdram),
	m_fingerprint(_other.m_fingerprint), m_cleared(_other.m_cleared), m_changed(_other.m_changed),
	m_cfb(_other.m_cfb), m_isDepthBuffer(_other.m_isDepthBuffer), m_isPauseScreen(_other.m_isPauseScreen),
	m_isOBScreen(_other.m_isOBScreen), m_needHeightCorrection(_other.m_needHeightCorrection),
	m_loadTileOrigin(_other.m_loadTileOrigin), m_loadType(_other.m_loadType),
	m_FBO(_other.m_FBO), m_pTexture(_other.m_pTexture), m_pDepthBuffer(_other.m_pDepthBuffer),
	m_resolveFBO(_other.m_resolveFBO), m_pResolveTexture(_other.m_pResolveTexture), m_resolved(_other.m_resolved),
	m_SubFBO(_other.m_SubFBO), m_pSubTexture(_other.m_pSubTexture),
	m_RdramCopy(_other.m_RdramCopy)
{
	_other.m_FBO = 0;
	_other.m_pTexture = NULL;
	_other.m_pDepthBuffer = NULL;
	_other.m_pResolveTexture = NULL;
	_other.m_resolveFBO = 0;
	_other.m_RdramCopy.clear();
	_other.m_SubFBO = 0;
	_other.m_pSubTexture = NULL;
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
	if (m_SubFBO != 0)
		glDeleteFramebuffers(1, &m_SubFBO);
	if (m_pSubTexture != NULL)
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
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, _pTexture->realWidth, _pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.monochromeInternalFormat, _pTexture->realWidth, _pTexture->realHeight, 0, fboFormats.monochromeFormat, fboFormats.monochromeType, NULL);
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
	m_fillcolor = 0;
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
	if (_address > RDRAMSize)
		return 0;
	if (_address + _stride * _height > (RDRAMSize + 1))
		return (RDRAMSize + 1 - _address) / _stride;
	return _height;
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

bool FrameBuffer::isValid() const
{
	if (m_validityChecked == video().getBuffersSwapCount())
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

	if (m_pSubTexture != NULL) {
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
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pSubTexture->realWidth, m_pSubTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.monochromeInternalFormat, m_pSubTexture->realWidth, m_pSubTexture->realHeight, 0, fboFormats.monochromeFormat, fboFormats.monochromeType, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_SubFBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pSubTexture->glName, 0);
	return true;
}

CachedTexture * FrameBuffer::_getSubTexture(u32 _t)
{
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

#ifdef GLES2
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x0, y0, copyWidth, copyHeight, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#else
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_SubFBO);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(x0, y0, x0 + copyWidth, y0 + copyHeight,
		0, 0, copyWidth, copyHeight,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	frameBufferList().setCurrentDrawBuffer();
#endif

	return m_pSubTexture;
}

CachedTexture * FrameBuffer::getTexture(u32 _t)
{
	const u32 shift = (gSP.textureTile[_t]->imageAddress - m_startAddress) >> (m_size - 1);
	const u32 factor = m_width;
	if (m_loadType == LOADTYPE_TILE) {
		m_pTexture->offsetS = (float)(m_loadTileOrigin.uls + (shift % factor));
		m_pTexture->offsetT = (float)(m_height - (m_loadTileOrigin.ult + shift / factor));
	} else {
		m_pTexture->offsetS = (float)(shift % factor);
		m_pTexture->offsetT = (float)(m_height - shift / factor);
	}

	CachedTexture *pTexture = m_pTexture;
//	if (gSP.textureTile[_t]->loadType == LOADTYPE_TILE && pTexture->size > 1)
	if (gSP.textureTile[_t]->clamps == 0 || gSP.textureTile[_t]->clampt == 0)
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
	 m_pCurrent = NULL;
	 m_pCopy = NULL;
	 glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	 m_prevColorImageHeight = 0;
}

void FrameBufferList::destroy() {
	m_list.clear();
	m_pCurrent = NULL;
	m_pCopy = NULL;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FrameBufferList::setBufferChanged()
{
	gDP.colorImage.changed = TRUE;
	if (m_pCurrent != NULL) {
		m_pCurrent->m_changed = true;
		m_pCurrent->m_copiedToRdram = false;
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
		if (m_pCurrent->m_height != gDP.scissor.lry) {
			m_pCurrent->reinit((u32)gDP.scissor.lry);

			if (m_pCurrent->_isMarioTennisScoreboard())
				g_RDRAMtoFB.CopyFromRDRAM(m_pCurrent->m_startAddress + 4, true);
			gSP.changed |= CHANGED_VIEWPORT;
		}
		m_pCurrent->m_needHeightCorrection = false;
	}
}

void FrameBufferList::clearBuffersChanged()
{
	gDP.colorImage.changed = FALSE;
	FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
	if (pBuffer != NULL)
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
	if (m_pCurrent != NULL && config.frameBufferEmulation.copyAuxToRDRAM != 0) {
		if (m_pCurrent->isAuxiliary()) {
			FrameBuffer_CopyToRDRAM(m_pCurrent->m_startAddress, true);
			removeBuffer(m_pCurrent->m_startAddress);
		}
	}

	if (VI.width == 0 || _height == 0) {
		m_pCurrent = NULL;
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		return;
	}

	OGLVideo & ogl = video();
	if (m_pCurrent != NULL) {
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
	if (m_pCurrent == NULL || m_pCurrent->m_startAddress != _address || m_pCurrent->m_width != _width)
		m_pCurrent = findBuffer(_address);
	const float scaleX = config.frameBufferEmulation.nativeResFactor == 0 ? ogl.getScaleX() : static_cast<float>(config.frameBufferEmulation.nativeResFactor);
	const float scaleY = config.frameBufferEmulation.nativeResFactor == 0 ? ogl.getScaleY() : scaleX;
	if (m_pCurrent != NULL) {
		if ((m_pCurrent->m_startAddress != _address) ||
			(m_pCurrent->m_width != _width) ||
			//(current->height != height) ||
			//(current->size != size) ||  // TODO FIX ME
			(m_pCurrent->m_scaleX != scaleX) ||
			(m_pCurrent->m_scaleY != scaleY))
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
			g_RDRAMtoFB.CopyFromRDRAM(m_pCurrent->m_startAddress + 4, true);
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
				m_pCurrent = NULL;
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
				m_pCurrent = NULL;
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			}
			m_list.erase(iter);
			return;
		}
}

void FrameBufferList::removeBuffers(u32 _width)
{
	m_pCurrent = NULL;
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
		while (iter->m_width == _width) {
			if (&(*iter) == m_pCurrent) {
				m_pCurrent = NULL;
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
	if (m_pCurrent == NULL)
		return;

	DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
	if (m_pCurrent->m_FBO > 0 && pDepthBuffer != NULL) {
		pDepthBuffer->initDepthImageTexture(m_pCurrent);
		pDepthBuffer->initDepthBufferTexture(m_pCurrent);
#ifdef GLES2
		if (pDepthBuffer->m_pDepthBufferTexture->realWidth == m_pCurrent->m_pTexture->realWidth) {
#else
		if (pDepthBuffer->m_pDepthBufferTexture->realWidth >= m_pCurrent->m_pTexture->realWidth) {
#endif
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

void FrameBufferList::clearDepthBuffer(DepthBuffer * _pDepthBuffer)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter) {
		if (iter->m_pDepthBuffer == _pDepthBuffer) {
			iter->m_pDepthBuffer = NULL;
		}
	}
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
	g_RDRAMtoFB.Destroy();
#ifndef GLES2
	g_dbToRDRAM.Destroy();
	g_fbToRDRAM.Destroy();
#endif
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
	OGLRender & render = ogl.getRender();
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

	FrameBuffer * pFilteredBuffer = PostProcessor::get().doBlur(PostProcessor::get().doGammaCorrection(pBuffer));

	const f32 viScaleX = _FIXED2FLOAT(_SHIFTR(*REG.VI_X_SCALE, 0, 12), 10);
	const f32 srcScaleX = pFilteredBuffer->m_scaleX;
	const f32 dstScaleX = ogl.getScaleX();
	const s32 h0 = (isPAL ? 128 : 108);
	const s32 hx0 = max(0, hStart - h0);
	const s32 hx1 = max(0, h0 + 640 - hEnd);
	X0 = (GLint)(hx0 * viScaleX * dstScaleX);
	Xwidth = (GLint)((min((f32)VI.width, (hEnd - hStart)*viScaleX)) * srcScaleX);
	X1 = ogl.getWidth() - (GLint)(hx1 *viScaleX * dstScaleX);

	const f32 srcScaleY = pFilteredBuffer->m_scaleY;
	const GLint hOffset = (ogl.getScreenWidth() - ogl.getWidth()) / 2;
	const GLint vOffset = (ogl.getScreenHeight() - ogl.getHeight()) / 2 + ogl.getHeightOffset();
	CachedTexture * pBufferTexture = pFilteredBuffer->m_pTexture;
	GLint srcCoord[4] = { 0, (GLint)(srcY0*srcScaleY), Xwidth, min((GLint)(srcY1*srcScaleY), (GLint)pBufferTexture->realHeight) };
	if (srcCoord[2] > pBufferTexture->realWidth || srcCoord[3] > pBufferTexture->realHeight) {
		removeBuffer(pBuffer->m_startAddress);
		return;
	}
	GLint dstCoord[4] = { X0 + hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + X1, vOffset + (GLint)(dstY1*dstScaleY) };
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
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		} else {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_FBO);
			filter = GL_NEAREST;
		}
	} else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_FBO);

	// glDisable(GL_SCISSOR_TEST) does not affect glBlitFramebuffer, at least on AMD
	glScissor(0, 0, ogl.getScreenWidth(), ogl.getScreenHeight() + ogl.getHeightOffset());

	glBlitFramebuffer(
		srcCoord[0], srcCoord[1], srcCoord[2], srcCoord[3],
		dstCoord[0], dstCoord[1], dstCoord[2], dstCoord[3],
		GL_COLOR_BUFFER_BIT, filter
	);

	if (dstPartHeight > 0) {
		const u32 size = *REG.VI_STATUS & 3;
		pBuffer = findBuffer(_address + (((*REG.VI_WIDTH)*VI.height)<<size>>1));
		if (pBuffer != NULL) {
			pFilteredBuffer = PostProcessor::get().doBlur(PostProcessor::get().doGammaCorrection(pBuffer));
			srcY0 = 0;
			srcY1 = srcPartHeight;
			dstY0 = dstY1;
			dstY1 = dstY0 + dstPartHeight;
			if (pFilteredBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
				pFilteredBuffer->resolveMultisampledTexture();
				glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_resolveFBO);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			} else
				glBindFramebuffer(GL_READ_FRAMEBUFFER, pFilteredBuffer->m_FBO);
			glBlitFramebuffer(
				0, (GLint)(srcY0*srcScaleY), Xwidth, min((GLint)(srcY1*srcScaleY), (GLint)pFilteredBuffer->m_pTexture->realHeight),
				hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + X1, vOffset + (GLint)(dstY1*dstScaleY),
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
	if (VI.width == 0 || *REG.VI_WIDTH == 0 || *REG.VI_H_START == 0) // H width is zero. Don't draw
		return;

	FrameBuffer *pBuffer = findBuffer(_address);
	if (pBuffer == NULL)
		return;

	OGLVideo & ogl = video();
	ogl.getRender().updateScissor(pBuffer);
	FrameBuffer * pFilteredBuffer = PostProcessor::get().doBlur(PostProcessor::get().doGammaCorrection(pBuffer));
	ogl.getRender().dropRenderState();

	const u32 width = pFilteredBuffer->m_width;
	const u32 height = pFilteredBuffer->m_height;

	pFilteredBuffer->m_pTexture->scaleS = ogl.getScaleX() / (float)pFilteredBuffer->m_pTexture->realWidth;
	pFilteredBuffer->m_pTexture->scaleT = ogl.getScaleY() / (float)pFilteredBuffer->m_pTexture->realHeight;
	pFilteredBuffer->m_pTexture->shiftScaleS = 1.0f;
	pFilteredBuffer->m_pTexture->shiftScaleT = 1.0f;
	pFilteredBuffer->m_pTexture->offsetS = 0;
	pFilteredBuffer->m_pTexture->offsetT = (float)height;
	textureCache().activateTexture(0, pFilteredBuffer->m_pTexture);
	gSP.textureTile[0]->fuls = gSP.textureTile[0]->fult = 0.0f;
	gSP.textureTile[0]->shifts = gSP.textureTile[0]->shiftt = 0;

	gDP.otherMode.cycleType = G_CYC_COPY;
	CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
	CombinerInfo::get().updateParameters(OGLRender::rsTexRect);
	currentCombiner()->disableBlending();
	glDisable( GL_BLEND );
	glDisable(GL_DEPTH_TEST);
	glDisable( GL_CULL_FACE );
	glDisable( GL_POLYGON_OFFSET_FILL );

	glScissor(0, 0, ogl.getScreenWidth(), ogl.getScreenHeight() + ogl.getHeightOffset());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	OGLRender::TexturedRectParams params(0.0f, 0.0f, width, height, 0.0f, 0.0f, width - 1.0f, height - 1.0f, 1.0f, 1.0f, false, false, false, pFilteredBuffer);
	ogl.getRender().drawTexturedRect(params);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	if (m_pCurrent != NULL)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
	ogl.swapBuffers();

	gSP.changed |= CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_COMBINE | CHANGED_SCISSOR;
}
#endif



void FrameBuffer_ActivateBufferTexture(u32 t, FrameBuffer *pBuffer)
{
	if (pBuffer == NULL)
		return;

	CachedTexture *pTexture = pBuffer->getTexture(t);
	if (pTexture == NULL)
		return;

//	frameBufferList().renderBuffer(pBuffer->m_startAddress);
	textureCache().activateTexture(t, pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_ActivateBufferTextureBG(u32 t, FrameBuffer *pBuffer )
{
	if (pBuffer == NULL)
		return;

	CachedTexture *pTexture = pBuffer->getTextureBG(t);
	if (pTexture == NULL)
		return;

//	frameBufferList().renderBuffer(pBuffer->m_startAddress);
	textureCache().activateTexture(t, pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

static
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
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
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

#ifdef ANDROID
	eglDestroyImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY), m_image);
#else
	glDeleteBuffers(3, m_PBO);
	m_PBO[0] = m_PBO[1] = m_PBO[2] = 0;
#endif
}

bool FrameBufferToRDRAM::_prepareCopy(u32 _startAddress)
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
	const u32 height = _cutHeight(_startAddress, pBuffer->m_height, stride);
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
		} else {
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

template <typename TSrc, typename TDst>
void _writeToRdram(TSrc* _src, TDst* _dst, TDst(*converter)(TSrc _c), TSrc _testValue, u32 _xor, u32 _width, u32 _height, u32 _numPixels, u32 _startAddress, u32 _bufferAddress, u32 _bufferSize)
{
	u32 chunkStart = ((_startAddress - _bufferAddress) >> (_bufferSize - 1)) % _width;
	if (chunkStart % 2 != 0) {
		--chunkStart;
		--_dst;
		++_numPixels;
	}

	u32 numStored = 0;
	u32 y = 0;
	TSrc c;
	if (chunkStart > 0) {
		for (u32 x = chunkStart; x < _width; ++x) {
			c = _src[x + (_height - 1)*_width];
			if (c != _testValue)
				_dst[numStored ^ _xor] = converter(c);
			++numStored;
		}
		++y;
		_dst += numStored;
	}

	u32 dsty = 0;
	for (; y < _height; ++y) {
		for (u32 x = 0; x < _width && numStored < _numPixels; ++x) {
			c = _src[x + (_height - y - 1)*_width];
			if (c != _testValue)
				_dst[(x + dsty*_width) ^ _xor] = converter(c);
			++numStored;
		}
		++dsty;
	}
}

u8 FrameBufferToRDRAM::_RGBAtoR8(u8 _c) {
	return _c;
}

u16 FrameBufferToRDRAM::_RGBAtoRGBA16(u32 _c) {
	RGBA c;
	c.raw = _c;
	return ((c.r >> 3) << 11) | ((c.g >> 3) << 6) | ((c.b >> 3) << 1) | (c.a == 0 ? 0 : 1);
}

u32 FrameBufferToRDRAM::_RGBAtoRGBA32(u32 _c) {
	RGBA c;
	c.raw = _c;
	return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
}

#ifdef ANDROID
GLubyte* FrameBufferToRDRAM::getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, bool _sync)
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
		for(unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex)
		{
			memmove(pixelData + lnIndex*widthBytes, pixelData+((lnIndex+_y0)*strideBytes), widthBytes);
		}
	}
	else {
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, pixelData);
	}

	return pixelData;
}

void FrameBufferToRDRAM::cleanUpPixels(GLubyte* pixelData)
{
	free(pixelData);
}

#else

GLubyte* FrameBufferToRDRAM::getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, bool _sync)
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

#ifndef GLES2
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
#else
	GLubyte* pixelData = (GLubyte*)malloc(_width * _height * colorFormatBytes);
	if (pixelData == NULL)
		return NULL;
	glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, pixelData);
#endif // GLES2

	return pixelData;
}

void FrameBufferToRDRAM::cleanUpPixels(GLubyte* pixelData)
{
#ifndef GLES2
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
	free(pixelData);
#endif
}
#endif //ANDROID


void FrameBufferToRDRAM::_copy(u32 _startAddress, u32 _endAddress, bool _sync)
{
	const u32 stride = m_pCurFrameBuffer->m_width << m_pCurFrameBuffer->m_size >> 1;
	const u32 max_height = _cutHeight(_startAddress, m_pCurFrameBuffer->m_height, stride);

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
		_writeToRdram<u32, u32>(ptr_src, ptr_dst, &FrameBufferToRDRAM::_RGBAtoRGBA32, 0, 0, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	} else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_16b) {
		u32 *ptr_src = (u32*)pixelData;
		u16 *ptr_dst = (u16*)(RDRAM + _startAddress);
		_writeToRdram<u32, u16>(ptr_src, ptr_dst, &FrameBufferToRDRAM::_RGBAtoRGBA16, 0, 1, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}	else if (m_pCurFrameBuffer->m_size == G_IM_SIZ_8b) {
		u8 *ptr_src = (u8*)pixelData;
		u8 *ptr_dst = RDRAM + _startAddress;
		_writeToRdram<u8, u8>(ptr_src, ptr_dst, &FrameBufferToRDRAM::_RGBAtoR8, 0, 3, width, height, numPixels, _startAddress, m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_size);
	}

	m_pCurFrameBuffer->m_copiedToRdram = true;
	m_pCurFrameBuffer->copyRdram();
	m_pCurFrameBuffer->m_cleared = false;

	cleanUpPixels(pixelData);

	gDP.changed |= CHANGED_SCISSOR;
}

void FrameBufferToRDRAM::copyToRDRAM(u32 _address, bool _sync)
{
	if (!_prepareCopy(_address))
		return;
	const u32 numBytes = (m_pCurFrameBuffer->m_width*m_pCurFrameBuffer->m_height) << m_pCurFrameBuffer->m_size >> 1;
	_copy(m_pCurFrameBuffer->m_startAddress, m_pCurFrameBuffer->m_startAddress + numBytes, _sync);
}

void FrameBufferToRDRAM::copyChunkToRDRAM(u32 _address)
{
	if (!_prepareCopy(_address))
		return;
	_copy(_address, _address + 0x1000, true);
}
#endif // GLES2

void FrameBuffer_CopyToRDRAM(u32 _address, bool _sync)
{
#ifndef GLES2
	g_fbToRDRAM.copyToRDRAM(_address, _sync);
#else
	if ((config.generalEmulation.hacks & hack_subscreen) == 0)
		return;
	if (VI.width == 0 || frameBufferList().getCurrent() == NULL)
		return;
	FrameBuffer *pBuffer = frameBufferList().findBuffer(_address);
	if (pBuffer == NULL || pBuffer->m_width < VI.width || pBuffer->m_isOBScreen)
		return;
	copyWhiteToRDRAM(pBuffer);
#endif
}

void FrameBuffer_CopyChunkToRDRAM(u32 _address)
{
#ifndef GLES2
	g_fbToRDRAM.copyChunkToRDRAM(_address);
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
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.monochromeInternalFormat, m_pColorTexture->realWidth, m_pColorTexture->realHeight, 0, fboFormats.monochromeFormat, fboFormats.monochromeType, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D, m_pDepthTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.depthInternalFormat, m_pDepthTexture->realWidth, m_pDepthTexture->realHeight, 0, GL_DEPTH_COMPONENT, fboFormats.depthType, NULL);
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
	if (m_PBO != 0) {
		glDeleteBuffers(1, &m_PBO);
		m_PBO = 0;
	}
}

bool DepthBufferToRDRAM::_prepareCopy(u32 _address, bool _copyChunk)
{
	const u32 curFrame = video().getBuffersSwapCount();
	if (_copyChunk && m_frameCount == curFrame)
		return true;

	const u32 numPixels = VI.width * VI.height;
	if (numPixels == 0) // Incorrect buffer size. Don't copy
		return false;
	FrameBuffer *pBuffer = frameBufferList().findBuffer(_address);
	if (pBuffer == NULL || pBuffer->isAuxiliary() || pBuffer->m_pDepthBuffer == NULL || !pBuffer->m_pDepthBuffer->m_cleared)
		return false;

	m_pCurDepthBuffer = pBuffer->m_pDepthBuffer;
	const u32 address = m_pCurDepthBuffer->m_address;
	if (address + numPixels * 2 > RDRAMSize)
		return false;

	const u32 height = _cutHeight(address, min(VI.height, m_pCurDepthBuffer->m_lry), pBuffer->m_width * 2);
	if (height == 0)
		return false;

	if (config.video.multisampling == 0)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	else {
		m_pCurDepthBuffer->resolveDepthBufferTexture(pBuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		0, 0, pBuffer->m_pTexture->realWidth, pBuffer->m_pTexture->realHeight,
		0, 0, pBuffer->m_width, pBuffer->m_height,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
	);
	glEnable(GL_SCISSOR_TEST);
	frameBufferList().setCurrentDrawBuffer();
	m_frameCount = curFrame;
	return true;
}

u16 DepthBufferToRDRAM::_FloatToUInt16(f32 _z)
{
	static const u16 * const zLUT = depthBufferList().getZLUT();
	u32 idx = 0x3FFFF;
	if (_z < 1.0f) {
		_z *= 262144.0f;
		idx = min(0x3FFFFU, u32(floorf(_z + 0.5f)));
	}
	return zLUT[idx];
}

bool DepthBufferToRDRAM::_copy(u32 _startAddress, u32 _endAddress)
{
	const u32 stride = m_pCurDepthBuffer->m_width << 1;
	const u32 max_height = _cutHeight(_startAddress, min(VI.height, m_pCurDepthBuffer->m_lry), stride);

	u32 numPixels = (_endAddress - _startAddress) >> 1;
	if (numPixels / m_pCurDepthBuffer->m_width > max_height) {
		_endAddress = _startAddress + (max_height * stride);
		numPixels = (_endAddress - _startAddress) >> 1;
	}

	const GLsizei width = m_pCurDepthBuffer->m_width;
	const GLint x0 = 0;
	const GLint y0 = max_height - (_endAddress - m_pCurDepthBuffer->m_address) / stride;
	const GLint y1 = max_height - (_startAddress - m_pCurDepthBuffer->m_address) / stride;
	const GLsizei height = std::min(max_height, 1u + y1 - y0);

	PBOBinder binder(GL_PIXEL_PACK_BUFFER, m_PBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadPixels(x0, y0, width, height, fboFormats.depthFormat, fboFormats.depthType, 0);

	GLubyte* pixelData = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, width * height * fboFormats.depthFormatBytes, GL_MAP_READ_BIT);
	if (pixelData == NULL)
		return false;

	f32 * ptr_src = (f32*)pixelData;
	u16 *ptr_dst = (u16*)(RDRAM + _startAddress);

	std::vector<f32> srcBuf(width * height);
	memcpy(srcBuf.data(), ptr_src, width * height * sizeof(f32));
	_writeToRdram<f32, u16>(srcBuf.data(), ptr_dst, &DepthBufferToRDRAM::_FloatToUInt16, 2.0f, 1, width, height, numPixels, _startAddress, m_pCurDepthBuffer->m_address, G_IM_SIZ_16b);

	m_pCurDepthBuffer->m_cleared = false;
	FrameBuffer * pBuffer = frameBufferList().findBuffer(m_pCurDepthBuffer->m_address);
	if (pBuffer != NULL)
		pBuffer->m_cleared = false;

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	gDP.changed |= CHANGED_SCISSOR;
	return true;
}

bool DepthBufferToRDRAM::copyToRDRAM( u32 _address)
{
	if (!_prepareCopy(_address, false))
		return false;

	const u32 endAddress = m_pCurDepthBuffer->m_address + (min(VI.height, m_pCurDepthBuffer->m_lry) * m_pCurDepthBuffer->m_width * 2);
	return _copy(m_pCurDepthBuffer->m_address, endAddress);
}

bool DepthBufferToRDRAM::copyChunkToRDRAM(u32 _address)
{
	if (!_prepareCopy(_address, true))
		return false;

	const u32 endAddress = _address + 0x1000;
	return _copy(_address, endAddress);
}
#endif // GLES2

bool FrameBuffer_CopyDepthBuffer( u32 address )
{
#ifndef GLES2
	FrameBuffer * pCopyBuffer = frameBufferList().getCopyBuffer();
	if (pCopyBuffer != NULL) {
		// This code is mainly to emulate Zelda MM camera.
		g_fbToRDRAM.copyToRDRAM(pCopyBuffer->m_startAddress, true);
		pCopyBuffer->m_RdramCopy.resize(0); // To disable validity check by RDRAM content. CPU may change content of the buffer for some unknown reason.
		frameBufferList().setCopyBuffer(NULL);
		return true;
	} else
		return g_dbToRDRAM.copyToRDRAM(address);
#else
	return false;
#endif
}

bool FrameBuffer_CopyDepthBufferChunk(u32 address)
{
#ifndef GLES2
	return g_dbToRDRAM.copyChunkToRDRAM(address);
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
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, NULL);
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
	if (m_PBO != 0) {
		glDeleteBuffers(1, &m_PBO);
		m_PBO = 0;
	}
#endif
}

void RDRAMtoFrameBuffer::AddAddress(u32 _address, u32 _size)
{
	if (m_pCurBuffer == nullptr) {
		m_pCurBuffer = frameBufferList().findBuffer(_address);
		if (m_pCurBuffer == nullptr)
			return;
	}

	const u32 pixelSize = 1 << m_pCurBuffer->m_size >> 1;
	if (_size != pixelSize && (_address%pixelSize) > 0)
		return;
	m_vecAddress.push_back(_address);
	gDP.colorImage.changed = TRUE;
}

// Write the whole buffer
template <typename TSrc>
bool _copyBufferFromRdram(u32 _address, u32* _dst, u32(*converter)(TSrc _c, bool _bCFB), u32 _xor, u32 _x0, u32 _y0, u32 _width, u32 _height, bool _bCFB)
{
	TSrc * src = reinterpret_cast<TSrc*>(RDRAM + _address);
	const u32 bound = (RDRAMSize + 1 - _address) >> (sizeof(TSrc) / 2);
	TSrc col;
	u32 idx;
	u32 summ = 0;
	u32 dsty = 0;
	const u32 y1 = _y0 + _height;
	for (u32 y = _y0; y < y1; ++y) {
		for (u32 x = _x0; x < _width; ++x) {
			idx = (x + (_height - y - 1)*_width) ^ _xor;
			if (idx >= bound)
				break;
			col = src[idx];
			summ += col;
			_dst[x + dsty*_width] = converter(col, _bCFB);
		}
		++dsty;
	}

	return summ != 0;
}

// Write only pixels provided with FBWrite
template <typename TSrc>
bool _copyPixelsFromRdram(u32 _address, const vector<u32> & _vecAddress, u32* _dst, u32(*converter)(TSrc _c, bool _bCFB), u32 _xor, u32 _width, u32 _height, bool _bCFB)
{
	memset(_dst, 0, _width*_height*sizeof(u32));
	TSrc * src = reinterpret_cast<TSrc*>(RDRAM + _address);
	const u32 szPixel = sizeof(TSrc);
	const size_t numPixels = _vecAddress.size();
	TSrc col;
	u32 summ = 0;
	u32 idx, w, h;
	for (size_t i = 0; i < numPixels; ++i) {
		if (_vecAddress[i] < _address)
			return false;
		idx = (_vecAddress[i] - _address) / szPixel;
		w = idx % _width;
		h = idx / _width;
		if (h > _height)
			return false;
		col = src[idx];
		summ += col;
		_dst[(w + (_height - h)*_width) ^ _xor] = converter(col, _bCFB);
	}

	return summ != 0;
}

u32 RGBA16ToABGR32(u16 col, bool _bCFB)
{
	u32 r, g, b, a;
	r = ((col >> 11) & 31) << 3;
	g = ((col >> 6) & 31) << 3;
	b = ((col >> 1) & 31) << 3;
	if (_bCFB)
		a = 0xFF;
	else
		a = (col & 1) > 0 && (r | g | b) > 0 ? 0xFF : 0U;
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

u32 RGBA32ToABGR32(u32 col, bool _bCFB)
{
	u32 r, g, b, a;
	r = (col >> 24) & 0xff;
	g = (col >> 16) & 0xff;
	b = (col >> 8) & 0xff;
	if (_bCFB)
		a = 0xFF;
	else
		a = (r | g | b) > 0 ? col & 0xFF : 0U;
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

void RDRAMtoFrameBuffer::CopyFromRDRAM(u32 _address, bool _bCFB)
{
	Cleaner cleaner(this);

	if (m_pCurBuffer == nullptr) {
		if (_bCFB || (config.frameBufferEmulation.copyFromRDRAM != 0 && !FBInfo::fbInfo.isSupported()))
			m_pCurBuffer = frameBufferList().findBuffer(_address);
	} else if (m_vecAddress.empty())
		return;

	if (m_pCurBuffer == nullptr || m_pCurBuffer->m_size < G_IM_SIZ_16b)
		return;

	if (m_pCurBuffer->m_startAddress == _address && gDP.colorImage.changed != 0)
		return;

	const u32 address = m_pCurBuffer->m_startAddress;

	const u32 height = _cutHeight(address, m_pCurBuffer->m_startAddress == _address ? VI.real_height : m_pCurBuffer->m_height, m_pCurBuffer->m_width << m_pCurBuffer->m_size >> 1);
	if (height == 0)
		return;

	const u32 width = m_pCurBuffer->m_width;

	const u32 x0 = 0;
	const u32 y0 = 0;
	const u32 y1 = y0 + height;

	const bool bUseAlpha = !_bCFB && m_pCurBuffer->m_changed;

	m_pTexture->width = width;
	m_pTexture->height = height;
	const u32 dataSize = width*height * 4;
#ifndef GLES2
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, NULL, GL_DYNAMIC_DRAW);
	GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
#else
	GLubyte* ptr = (GLubyte*)malloc(dataSize);
	PBOBinder binder(ptr);
#endif // GLES2
	if (ptr == NULL)
		return;

	u32 * dst = (u32*)ptr;
	bool bCopy;
	if (m_vecAddress.empty()) {
		if (m_pCurBuffer->m_size == G_IM_SIZ_16b)
			bCopy = _copyBufferFromRdram<u16>(address, dst, RGBA16ToABGR32, 1, x0, y0, width, height, _bCFB);
		else
			bCopy = _copyBufferFromRdram<u32>(address, dst, RGBA32ToABGR32, 0, x0, y0, width, height, _bCFB);
	}
	else {
		if (m_pCurBuffer->m_size == G_IM_SIZ_16b)
			bCopy = _copyPixelsFromRdram<u16>(address, m_vecAddress, dst, RGBA16ToABGR32, 1, width, height, _bCFB);
		else
			bCopy = _copyPixelsFromRdram<u32>(address, m_vecAddress, dst, RGBA32ToABGR32, 0, width, height, _bCFB);
	}

	if (bUseAlpha) {
		u32 totalBytes = (width * height) << m_pCurBuffer->m_size >> 1;
		if (address + totalBytes > RDRAMSize + 1)
			totalBytes = RDRAMSize + 1 - address;
		memset(RDRAM + address, 0, totalBytes);
	}

#ifndef GLES2
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
#endif
	if (!bCopy)
		return;

	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
#ifndef GLES2
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
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

	const u32 cycleType = gDP.otherMode.cycleType;
	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
	currentCombiner()->disableBlending();
	gDP.otherMode.cycleType = cycleType;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	currentCombiner()->updateFrameBufferInfo();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurBuffer->m_FBO);
	OGLRender::TexturedRectParams params((float)x0, (float)y0, (float)width, (float)height,
										 0.0f, 0.0f, width - 1.0f, height - 1.0f, 1.0f, 1.0f,
										 false, true, false, m_pCurBuffer);
	video().getRender().drawTexturedRect(params);
	frameBufferList().setCurrentDrawBuffer();

	gSP.textureTile[0] = pTile0;

	gDP.changed |= CHANGED_RENDERMODE | CHANGED_COMBINE | CHANGED_SCISSOR;
}

void RDRAMtoFrameBuffer::Reset()
{
	m_pCurBuffer = nullptr;
	m_vecAddress.clear();
}

void FrameBuffer_CopyFromRDRAM(u32 _address, bool _bCFB)
{
	g_RDRAMtoFB.CopyFromRDRAM(_address, _bCFB);
}

void FrameBuffer_AddAddress(u32 address, u32 _size)
{
	g_RDRAMtoFB.AddAddress(address, _size);
}
