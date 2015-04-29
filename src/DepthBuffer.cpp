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

const GLuint ZlutImageUnit = 0;
const GLuint TlutImageUnit = 1;
const GLuint depthImageUnit = 2;

DepthBuffer::DepthBuffer() : m_address(0), m_width(0), m_uly(0), m_lry(0),
	m_FBO(0), m_pDepthImageTexture(NULL), m_pDepthBufferTexture(NULL),
	m_cleared(false), m_pResolveDepthBufferTexture(NULL), m_resolved(false)
{
	glGenFramebuffers(1, &m_FBO);
}

DepthBuffer::DepthBuffer(DepthBuffer && _other) :
	m_address(_other.m_address), m_width(_other.m_width), m_uly(_other.m_uly), m_lry(_other.m_lry),
	m_FBO(_other.m_FBO), m_pDepthImageTexture(_other.m_pDepthImageTexture), m_pDepthBufferTexture(_other.m_pDepthBufferTexture),
	m_pResolveDepthBufferTexture(_other.m_pResolveDepthBufferTexture), m_resolved(_other.m_resolved)
{
	_other.m_FBO = 0;
	_other.m_pDepthImageTexture = NULL;
	_other.m_pDepthBufferTexture = NULL;
	_other.m_pResolveDepthBufferTexture = NULL;
	_other.m_resolved = false;
}

DepthBuffer::~DepthBuffer()
{
	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);
	if (m_pDepthImageTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pDepthImageTexture);
	if (m_pDepthBufferTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pDepthBufferTexture);
	if (m_pResolveDepthBufferTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pResolveDepthBufferTexture);
}

void DepthBuffer::initDepthImageTexture(FrameBuffer * _pBuffer)
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (!video().getRender().isImageTexturesSupported() || config.frameBufferEmulation.N64DepthCompare == 0 || m_pDepthImageTexture != NULL)
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
	m_pDepthImageTexture->frameBufferTexture = TRUE;
	m_pDepthImageTexture->maskS = 0;
	m_pDepthImageTexture->maskT = 0;
	m_pDepthImageTexture->mirrorS = 0;
	m_pDepthImageTexture->mirrorT = 0;
	m_pDepthImageTexture->realWidth = m_pDepthImageTexture->width;
	m_pDepthImageTexture->realHeight = m_pDepthImageTexture->height;
	m_pDepthImageTexture->textureBytes = m_pDepthImageTexture->realWidth * m_pDepthImageTexture->realHeight * 2 * sizeof(float); // Width*Height*RG*sizeof(GL_RGBA32F)
	textureCache().addFrameBufferTextureSize(m_pDepthImageTexture->textureBytes);

	glBindTexture(GL_TEXTURE_2D, m_pDepthImageTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, m_pDepthImageTexture->realWidth, m_pDepthImageTexture->realHeight, 0, GL_RG, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pDepthImageTexture->glName, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);

	depthBufferList().clearBuffer(0, VI.height);
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

void DepthBuffer::_initDepthBufferTexture(FrameBuffer * _pBuffer, CachedTexture * _pTexture, bool _multisample)
{
	if (_pBuffer != NULL) {
		_pTexture->width = (u32)(_pBuffer->m_pTexture->width);
		_pTexture->height = (u32)(_pBuffer->m_pTexture->height);
		_pTexture->address = _pBuffer->m_startAddress;
		_pTexture->clampWidth = _pBuffer->m_width;
		_pTexture->clampHeight = _pBuffer->m_height;
	}
	else {
		_pTexture->width = video().getWidth();
		_pTexture->height = video().getHeight();
		_pTexture->address = gDP.depthImageAddress;
		_pTexture->clampWidth = VI.width;
		_pTexture->clampHeight = VI.height;
	}
	_pTexture->format = 0;
	_pTexture->size = 2;
	_pTexture->clampS = 1;
	_pTexture->clampT = 1;
	_pTexture->frameBufferTexture = TRUE;
	_pTexture->maskS = 0;
	_pTexture->maskT = 0;
	_pTexture->mirrorS = 0;
	_pTexture->mirrorT = 0;
	_pTexture->realWidth = _pTexture->width;
	_pTexture->realHeight = _pTexture->height;
	_pTexture->textureBytes = _pTexture->realWidth * _pTexture->realHeight * sizeof(float);
	textureCache().addFrameBufferTextureSize(_pTexture->textureBytes);

#ifndef GLES2
	const GLenum format = GL_DEPTH_COMPONENT;
#else
	const GLenum format = GL_DEPTH_COMPONENT24_OES;
#endif
	if (_multisample) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _pTexture->glName);
		if (_pBuffer != NULL)
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_DEPTH_COMPONENT, _pBuffer->m_pTexture->realWidth, _pBuffer->m_pTexture->realHeight, false);
		else
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_DEPTH_COMPONENT, video().getWidth(), video().getHeight(), false);
	}
	else {
		glBindTexture(GL_TEXTURE_2D, _pTexture->glName);
		if (_pBuffer != NULL)
			glTexImage2D(GL_TEXTURE_2D, 0, format, _pBuffer->m_pTexture->realWidth, _pBuffer->m_pTexture->realHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, format, video().getWidth(), video().getHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void DepthBuffer::setDepthAttachment(GLenum _target)
{
	if (config.video.multisampling != 0)
		glFramebufferTexture2D(_target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_pDepthBufferTexture->glName, 0);
	else
		glFramebufferTexture2D(_target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthBufferTexture->glName, 0);
	m_resolved = false;
}

void DepthBuffer::initDepthBufferTexture(FrameBuffer * _pBuffer)
{
	if (m_pDepthBufferTexture == NULL) {
		m_pDepthBufferTexture = textureCache().addFrameBufferTexture();
		_initDepthBufferTexture(_pBuffer, m_pDepthBufferTexture, config.video.multisampling != 0);
	}

	if (config.video.multisampling != 0 && m_pResolveDepthBufferTexture == NULL) {
		m_pResolveDepthBufferTexture = textureCache().addFrameBufferTexture();
		_initDepthBufferTexture(_pBuffer, m_pResolveDepthBufferTexture, false);
	}
}

CachedTexture * DepthBuffer::resolveDepthBufferTexture(FrameBuffer * _pBuffer)
{
	if (config.video.multisampling == 0)
		return m_pDepthBufferTexture;
	if (m_resolved)
		return m_pResolveDepthBufferTexture;
	glScissor(0, 0, m_pDepthBufferTexture->realWidth, m_pDepthBufferTexture->realHeight);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _pBuffer->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	GLuint attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_resolveFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pResolveDepthBufferTexture->glName, 0);
	assert(checkFBO());
	glBlitFramebuffer(
		0, 0, m_pDepthBufferTexture->realWidth, m_pDepthBufferTexture->realHeight,
		0, 0, m_pResolveDepthBufferTexture->realWidth, m_pResolveDepthBufferTexture->realHeight,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);
	m_resolved = true;
	gDP.changed |= CHANGED_SCISSOR;
	return m_pResolveDepthBufferTexture;
}

void DepthBuffer::activateDepthBufferTexture(FrameBuffer * _pBuffer)
{
	textureCache().activateTexture(0, resolveDepthBufferTexture(_pBuffer));
}

void DepthBuffer::bindDepthImageTexture()
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	glBindImageTexture(depthImageUnit, m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
#endif
}

DepthBufferList::DepthBufferList() : m_pCurrent(NULL), m_pzLUT(NULL) 
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
	m_pzLUT = NULL;
	m_list.clear();
}

DepthBufferList & DepthBufferList::get()
{
	static DepthBufferList depthBufferList;
	return depthBufferList;
}

void DepthBufferList::init()
{
	m_pCurrent = NULL;
}

void DepthBufferList::destroy()
{
	m_pCurrent = NULL;
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
	return NULL;
}

void DepthBufferList::removeBuffer(u32 _address )
{
	for (DepthBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_address == _address) {
			m_list.erase(iter);
			return;
		}
}

void DepthBufferList::saveBuffer(u32 _address)
{
	if (!config.frameBufferEmulation.enable)
		return;

	FrameBuffer * pFrameBuffer = frameBufferList().findBuffer(_address);
	if (pFrameBuffer != NULL)
		pFrameBuffer->m_isDepthBuffer = true;

	if (m_pCurrent == NULL || m_pCurrent->m_address != _address)
		m_pCurrent = findBuffer(_address);

	if (m_pCurrent != NULL && pFrameBuffer != NULL && m_pCurrent->m_width != pFrameBuffer->m_width) {
		removeBuffer(_address);
		m_pCurrent = NULL;
	}

	if (m_pCurrent == NULL) {
		m_list.emplace_front();
		DepthBuffer & buffer = m_list.front();

		buffer.m_address = _address;
		buffer.m_width = pFrameBuffer != NULL ? pFrameBuffer->m_width : VI.width;

		buffer.initDepthBufferTexture(pFrameBuffer);

		m_pCurrent = &buffer;
	}

	frameBufferList().attachDepthBuffer();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "DepthBuffer_SetBuffer( 0x%08X ); color buffer is 0x%08X\n",
			address, ( pFrameBuffer != NULL &&  pFrameBuffer->m_FBO > 0) ?  pFrameBuffer->m_startAddress : 0
		);
#endif

}

void DepthBufferList::clearBuffer(u32 _uly, u32 _lry)
{
	if (m_pCurrent == NULL)
		return;
	m_pCurrent->m_cleared = true;
	m_pCurrent->m_uly = _uly;
	m_pCurrent->m_lry = _lry;
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (m_pCurrent->m_FBO == 0 || !video().getRender().isImageTexturesSupported() || config.frameBufferEmulation.N64DepthCompare == 0)
		return;
	float color[4] = {1.0f, 1.0f, 0.0f, 1.0f};
	glBindImageTexture(depthImageUnit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
	const u32 cycleType = gDP.otherMode.cycleType;
	gDP.otherMode.cycleType = G_CYC_FILL;
	video().getRender().drawRect(0,0,VI.width, VI.height, color);
	gDP.otherMode.cycleType = cycleType;
	glBindImageTexture(depthImageUnit, m_pCurrent->m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);
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
