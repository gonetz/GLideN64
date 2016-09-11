#ifdef OS_MAC_OS_X
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include <assert.h>
#include "OpenGL.h"
#include "Combiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "VI.h"
#include "Config.h"
#include "Debug.h"
#include "FBOTextureFormats.h"

const GLuint ZlutImageUnit = 0;
const GLuint TlutImageUnit = 1;
const GLuint depthImageUnit = 2;

DepthBuffer::DepthBuffer() : m_address(0), m_width(0), m_ulx(0), m_uly(0), m_lrx(0), m_lry(0),
	m_depthImageFBO(0), m_pDepthImageTexture(nullptr), m_pDepthBufferTexture(nullptr),
	m_depthRenderbuffer(0), m_depthRenderbufferWidth(0),
	m_cleared(false), m_pResolveDepthBufferTexture(nullptr), m_resolved(false),
	m_pDepthBufferCopyTexture(nullptr), m_copied(false)
{
	glGenFramebuffers(1, &m_copyFBO);
	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		glGenFramebuffers(1, &m_depthImageFBO);
}

DepthBuffer::DepthBuffer(DepthBuffer && _other) :
	m_address(_other.m_address), m_width(_other.m_width),
	m_ulx(_other.m_ulx), m_uly(_other.m_uly), m_lrx(_other.m_lrx), m_lry(_other.m_lry),
	m_depthImageFBO(_other.m_depthImageFBO), m_pDepthImageTexture(_other.m_pDepthImageTexture), m_pDepthBufferTexture(_other.m_pDepthBufferTexture),
	m_depthRenderbuffer(_other.m_depthRenderbuffer), m_depthRenderbufferWidth(_other.m_depthRenderbufferWidth),
	m_cleared(_other.m_cleared), m_pResolveDepthBufferTexture(_other.m_pResolveDepthBufferTexture), m_resolved(_other.m_resolved),
	m_pDepthBufferCopyTexture(_other.m_pDepthBufferCopyTexture), m_copied(m_copied)
{
	_other.m_depthImageFBO = 0;
	_other.m_pDepthImageTexture = nullptr;
	_other.m_pDepthBufferTexture = nullptr;
	_other.m_depthRenderbuffer = 0;
	_other.m_pResolveDepthBufferTexture = nullptr;
	_other.m_resolved = false;
	_other.m_pDepthBufferCopyTexture = nullptr;
	_other.m_copied = false;
}

DepthBuffer::~DepthBuffer()
{
	if (m_depthImageFBO != 0)
		glDeleteFramebuffers(1, &m_depthImageFBO);
	if (m_pDepthImageTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pDepthImageTexture);
	if (m_pDepthBufferTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pDepthBufferTexture);
	if (m_depthRenderbuffer != 0)
		glDeleteRenderbuffers(1, &m_depthRenderbuffer);
	if (m_pResolveDepthBufferTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pResolveDepthBufferTexture);
	if (m_copyFBO != 0)
		glDeleteFramebuffers(1, &m_copyFBO);
	if (m_pDepthBufferCopyTexture != nullptr)
		textureCache().removeFrameBufferTexture(m_pDepthBufferCopyTexture);
}

void DepthBuffer::initDepthImageTexture(FrameBuffer * _pBuffer)
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (!video().getRender().isImageTexturesSupported() || config.frameBufferEmulation.N64DepthCompare == 0 || m_pDepthImageTexture != nullptr)
		return;

	m_pDepthImageTexture = textureCache().addFrameBufferTexture();

	m_pDepthImageTexture->width = (u32)(_pBuffer->m_pTexture->width);
	m_pDepthImageTexture->height = (u32)(_pBuffer->m_pTexture->height);
	m_pDepthImageTexture->format = 0;
	m_pDepthImageTexture->size = 2;
	m_pDepthImageTexture->clampS = 1;
	m_pDepthImageTexture->clampT = 1;
	m_pDepthImageTexture->address = _pBuffer->m_startAddress;
	m_pDepthImageTexture->clampWidth = _pBuffer->m_width;
	m_pDepthImageTexture->clampHeight = _pBuffer->m_height;
	m_pDepthImageTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pDepthImageTexture->maskS = 0;
	m_pDepthImageTexture->maskT = 0;
	m_pDepthImageTexture->mirrorS = 0;
	m_pDepthImageTexture->mirrorT = 0;
	m_pDepthImageTexture->realWidth = m_pDepthImageTexture->width;
	m_pDepthImageTexture->realHeight = m_pDepthImageTexture->height;
	m_pDepthImageTexture->textureBytes = m_pDepthImageTexture->realWidth * m_pDepthImageTexture->realHeight * fboFormats.depthImageFormatBytes;
	textureCache().addFrameBufferTextureSize(m_pDepthImageTexture->textureBytes);

	glBindTexture(GL_TEXTURE_2D, m_pDepthImageTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.depthImageInternalFormat, m_pDepthImageTexture->realWidth, m_pDepthImageTexture->realHeight, 0, fboFormats.depthImageFormat, fboFormats.depthImageType, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_depthImageFBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pDepthImageTexture->glName, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);

	depthBufferList().clearBuffer(0, 0, VI.width, VI.height);
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

void DepthBuffer::_initDepthBufferTexture(FrameBuffer * _pBuffer, CachedTexture * _pTexture, bool _multisample)
{
	if (_pBuffer != nullptr) {
		_pTexture->width = (u32)(_pBuffer->m_pTexture->width);
		_pTexture->height = (u32)(_pBuffer->m_pTexture->height);
		_pTexture->address = _pBuffer->m_startAddress;
		_pTexture->clampWidth = _pBuffer->m_width;
		_pTexture->clampHeight = _pBuffer->m_height;
	}
	else {
		if (config.frameBufferEmulation.nativeResFactor == 0) {
			_pTexture->width = video().getWidth();
			_pTexture->height = video().getHeight();
		} else {
			_pTexture->width = VI.width * config.frameBufferEmulation.nativeResFactor;
			_pTexture->height = VI.height * config.frameBufferEmulation.nativeResFactor;
		}
		_pTexture->address = gDP.depthImageAddress;
		_pTexture->clampWidth = VI.width;
		_pTexture->clampHeight = VI.height;
	}
	_pTexture->format = 0;
	_pTexture->size = 2;
	_pTexture->clampS = 1;
	_pTexture->clampT = 1;
	_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	_pTexture->maskS = 0;
	_pTexture->maskT = 0;
	_pTexture->mirrorS = 0;
	_pTexture->mirrorT = 0;
	_pTexture->realWidth = _pTexture->width;
	_pTexture->realHeight = _pTexture->height;
	_pTexture->textureBytes = _pTexture->realWidth * _pTexture->realHeight * fboFormats.depthFormatBytes;
	textureCache().addFrameBufferTextureSize(_pTexture->textureBytes);

#ifdef GL_MULTISAMPLING_SUPPORT
	if (_multisample) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _pTexture->glName);
#if defined(GLES3_1)
		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_DEPTH_COMPONENT, _pTexture->realWidth, _pTexture->realHeight, false);
#else
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_DEPTH_COMPONENT, _pTexture->realWidth, _pTexture->realHeight, false);
#endif
		_pTexture->frameBufferTexture = CachedTexture::fbMultiSample;
	} else
#endif // GL_MULTISAMPLING_SUPPORT
	{
		glBindTexture(GL_TEXTURE_2D, _pTexture->glName);
		glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.depthInternalFormat, _pTexture->realWidth, _pTexture->realHeight, 0, GL_DEPTH_COMPONENT, fboFormats.depthType, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void DepthBuffer::_initDepthBufferRenderbuffer(FrameBuffer * _pBuffer)
{
	if (m_depthRenderbuffer != 0)
		return;
	u32 height;
	if (_pBuffer != NULL) {
		m_depthRenderbufferWidth = (u32)(_pBuffer->m_pTexture->width);
		height = (u32)(_pBuffer->m_pTexture->height);
	} else {
		if (config.frameBufferEmulation.nativeResFactor == 0) {
			m_depthRenderbufferWidth = video().getWidth();
			height = video().getHeight();
		} else {
			m_depthRenderbufferWidth = VI.width * config.frameBufferEmulation.nativeResFactor;
			height = VI.height * config.frameBufferEmulation.nativeResFactor;
		}
	}

	glGenRenderbuffers(1, &m_depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, fboFormats.depthInternalFormat, m_depthRenderbufferWidth, height);
}

void DepthBuffer::setDepthAttachment(GLenum _target)
{
#ifndef USE_DEPTH_RENDERBUFFER
#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling != 0)
		glFramebufferTexture2D(_target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_pDepthBufferTexture->glName, 0);
	else
#endif
		glFramebufferTexture2D(_target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthBufferTexture->glName, 0);
#else
	glFramebufferRenderbuffer(_target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
#endif
	m_copied = false;
	m_resolved = false;
}

void DepthBuffer::initDepthBufferTexture(FrameBuffer * _pBuffer)
{
#ifndef USE_DEPTH_RENDERBUFFER
	if (m_pDepthBufferTexture == nullptr) {
		m_pDepthBufferTexture = textureCache().addFrameBufferTexture();
		_initDepthBufferTexture(_pBuffer, m_pDepthBufferTexture, config.video.multisampling != 0);
	}
#else
	_initDepthBufferRenderbuffer(_pBuffer);
#endif

#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling != 0 && m_pResolveDepthBufferTexture == nullptr) {
		m_pResolveDepthBufferTexture = textureCache().addFrameBufferTexture();
		_initDepthBufferTexture(_pBuffer, m_pResolveDepthBufferTexture, false);
	}
#endif
}

CachedTexture * DepthBuffer::resolveDepthBufferTexture(FrameBuffer * _pBuffer)
{
#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling == 0)
		return m_pDepthBufferTexture;
	if (m_resolved)
		return m_pResolveDepthBufferTexture;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _pBuffer->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	GLuint attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_resolveFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pResolveDepthBufferTexture->glName, 0);
	assert(checkFBO());
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		0, 0, m_pDepthBufferTexture->realWidth, m_pDepthBufferTexture->realHeight,
		0, 0, m_pResolveDepthBufferTexture->realWidth, m_pResolveDepthBufferTexture->realHeight,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);
	m_resolved = true;
	return m_pResolveDepthBufferTexture;
#else
	return m_pDepthBufferTexture;
#endif
}

#ifndef GLES2
CachedTexture * DepthBuffer::copyDepthBufferTexture(FrameBuffer * _pBuffer)
{
	if (m_copied)
		return m_pDepthBufferCopyTexture;

	if (m_pDepthBufferCopyTexture == nullptr) {
		m_pDepthBufferCopyTexture = textureCache().addFrameBufferTexture();
		_initDepthBufferTexture(_pBuffer, m_pDepthBufferCopyTexture, false);
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, _pBuffer->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_copyFBO);
#ifdef GL_MULTISAMPLING_SUPPORT
	GLenum textarget = config.video.multisampling != 0 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
#else
	GLenum textarget = GL_TEXTURE_2D;
#endif
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
						   GL_COLOR_ATTACHMENT0,
						   textarget,
						   _pBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample ?
						   _pBuffer->m_pResolveTexture->glName :
						   _pBuffer->m_pTexture->glName,
						   0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthBufferCopyTexture->glName, 0);
	assert(checkFBO());
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		0, 0, m_pDepthBufferTexture->realWidth, m_pDepthBufferTexture->realHeight,
		0, 0, m_pDepthBufferTexture->realWidth, m_pDepthBufferTexture->realHeight,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);
	m_copied = true;
	return m_pDepthBufferCopyTexture;
}
#endif

void DepthBuffer::activateDepthBufferTexture(FrameBuffer * _pBuffer)
{
	textureCache().activateTexture(0, resolveDepthBufferTexture(_pBuffer));
}

void DepthBuffer::bindDepthImageTexture()
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	glBindImageTexture(depthImageUnit, m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, fboFormats.depthImageInternalFormat);
#endif
}

DepthBufferList::DepthBufferList() : m_pCurrent(nullptr), m_pzLUT(nullptr)
{
	m_pzLUT = new u16[0x40000];
	for (int i = 0; i<0x40000; i++) {
		u32 exponent = 0;
		u32 testbit = 1 << 17;
		while ((i & testbit) && (exponent < 7)) {
			exponent++;
			testbit = 1 << (17 - exponent);
		}

		const u32 mantissa = (i >> (6 - (6 < exponent ? 6 : exponent))) & 0x7ff;
		m_pzLUT[i] = (u16)(((exponent << 11) | mantissa) << 2);
	}
}

DepthBufferList::~DepthBufferList()
{
	delete[] m_pzLUT;
	m_pzLUT = nullptr;
	m_list.clear();
}

DepthBufferList & DepthBufferList::get()
{
	static DepthBufferList depthBufferList;
	return depthBufferList;
}

void DepthBufferList::init()
{
	m_pCurrent = nullptr;
}

void DepthBufferList::destroy()
{
	m_pCurrent = nullptr;
	m_list.clear();
}

void DepthBufferList::setNotCleared()
{
	for (DepthBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		iter->m_cleared = false;
}

DepthBuffer * DepthBufferList::findBuffer(u32 _address)
{
	for (DepthBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_address == _address)
				return &(*iter);
	return nullptr;
}

void DepthBufferList::removeBuffer(u32 _address )
{
	for (DepthBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_address == _address) {
			frameBufferList().clearDepthBuffer(&(*iter));
			m_list.erase(iter);
			return;
		}
}

void DepthBufferList::saveBuffer(u32 _address)
{
	if (!config.frameBufferEmulation.enable)
		return;

	FrameBuffer * pFrameBuffer = frameBufferList().findBuffer(_address);
	if (pFrameBuffer != nullptr)
		pFrameBuffer->m_isDepthBuffer = true;

	if (m_pCurrent == nullptr || m_pCurrent->m_address != _address)
		m_pCurrent = findBuffer(_address);

	if (m_pCurrent != nullptr && pFrameBuffer != nullptr && m_pCurrent->m_width != pFrameBuffer->m_width) {
		removeBuffer(_address);
		m_pCurrent = nullptr;
	}

	if (m_pCurrent == nullptr) {
		m_list.emplace_front();
		DepthBuffer & buffer = m_list.front();

		buffer.m_address = _address;
		buffer.m_width = pFrameBuffer != nullptr ? pFrameBuffer->m_width : VI.width;

		buffer.initDepthBufferTexture(pFrameBuffer);

		m_pCurrent = &buffer;
	}

	frameBufferList().attachDepthBuffer();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "DepthBuffer_SetBuffer( 0x%08X ); color buffer is 0x%08X\n",
			address, ( pFrameBuffer != nullptr &&  pFrameBuffer->m_FBO > 0) ?  pFrameBuffer->m_startAddress : 0
		);
#endif

}

void DepthBufferList::clearBuffer(u32 _ulx, u32 _uly, u32 _lrx, u32 _lry)
{
	if (m_pCurrent == nullptr)
		return;
	m_pCurrent->m_cleared = true;
	m_pCurrent->m_ulx = _ulx;
	m_pCurrent->m_uly = _uly;
	m_pCurrent->m_lrx = _lrx;
	m_pCurrent->m_lry = _lry;
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (m_pCurrent->m_depthImageFBO == 0 || !video().getRender().isImageTexturesSupported() || config.frameBufferEmulation.N64DepthCompare == 0)
		return;
	float color[4] = {1.0f, 1.0f, 0.0f, 1.0f};
	glBindImageTexture(depthImageUnit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, fboFormats.depthImageInternalFormat);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_depthImageFBO);
	const u32 cycleType = gDP.otherMode.cycleType;
	gDP.otherMode.cycleType = G_CYC_FILL;
	video().getRender().drawRect(_ulx, _uly, _lrx, _lry, color);
	gDP.otherMode.cycleType = cycleType;
	glBindImageTexture(depthImageUnit, m_pCurrent->m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, fboFormats.depthImageInternalFormat);
	frameBufferList().setCurrentDrawBuffer();
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

void DepthBuffer_Init()
{
	depthBufferList().init();
}

void DepthBuffer_Destroy()
{
	depthBufferList().destroy();
}
