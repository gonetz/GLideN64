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

DepthBuffer::DepthBuffer() : m_address(0), m_width(0), m_FBO(0), m_pDepthImageTexture(NULL), m_pDepthBufferTexture(NULL)
{
	glGenFramebuffers(1, &m_FBO);
}

DepthBuffer::DepthBuffer(DepthBuffer && _other) :
	m_address(_other.m_address), m_width(_other.m_width),
	m_FBO(_other.m_FBO), m_pDepthImageTexture(_other.m_pDepthImageTexture), m_pDepthBufferTexture(_other.m_pDepthBufferTexture)
{
	_other.m_FBO = 0;
	_other.m_pDepthImageTexture = NULL;
	_other.m_pDepthBufferTexture = NULL;
}

DepthBuffer::~DepthBuffer()
{
	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);
	if (m_pDepthImageTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pDepthImageTexture);
	if (m_pDepthBufferTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pDepthBufferTexture);
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

	depthBufferList().clearBuffer();
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

void DepthBuffer::initDepthBufferTexture(FrameBuffer * _pBuffer)
{
	if (m_pDepthBufferTexture != NULL)
		return;

	m_pDepthBufferTexture = textureCache().addFrameBufferTexture();

	if (_pBuffer != NULL) {
		m_pDepthBufferTexture->width = (u32)(_pBuffer->m_pTexture->width);
		m_pDepthBufferTexture->height = (u32)(_pBuffer->m_pTexture->height);
		m_pDepthBufferTexture->address = _pBuffer->m_startAddress;
		m_pDepthBufferTexture->clampWidth = _pBuffer->m_width;
		m_pDepthBufferTexture->clampHeight = _pBuffer->m_height;
	}
	else {
		m_pDepthBufferTexture->width = video().getWidth();
		m_pDepthBufferTexture->height = video().getHeight();
		m_pDepthBufferTexture->address = VI.lastOrigin;
		m_pDepthBufferTexture->clampWidth = VI.width;
		m_pDepthBufferTexture->clampHeight = VI.height;
	}
	m_pDepthBufferTexture->format = 0;
	m_pDepthBufferTexture->size = 2;
	m_pDepthBufferTexture->clampS = 1;
	m_pDepthBufferTexture->clampT = 1;
	m_pDepthBufferTexture->frameBufferTexture = TRUE;
	m_pDepthBufferTexture->maskS = 0;
	m_pDepthBufferTexture->maskT = 0;
	m_pDepthBufferTexture->mirrorS = 0;
	m_pDepthBufferTexture->mirrorT = 0;
	m_pDepthBufferTexture->realWidth = m_pDepthBufferTexture->width;
	m_pDepthBufferTexture->realHeight = m_pDepthBufferTexture->height;
	m_pDepthBufferTexture->textureBytes = m_pDepthBufferTexture->realWidth * m_pDepthBufferTexture->realHeight * sizeof(float);
	textureCache().addFrameBufferTextureSize(m_pDepthBufferTexture->textureBytes);

#ifndef GLES2
	const GLenum format = GL_DEPTH_COMPONENT;
#else
	const GLenum format = GL_DEPTH_COMPONENT24_OES;
#endif
	if (config.video.multisampling != 0) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pDepthBufferTexture->glName);
		if (_pBuffer != NULL)
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_DEPTH_COMPONENT, _pBuffer->m_pTexture->realWidth, _pBuffer->m_pTexture->realHeight, false);
		else
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_DEPTH_COMPONENT, video().getWidth(), video().getHeight(), false);
	}
	else {
		glBindTexture(GL_TEXTURE_2D, m_pDepthBufferTexture->glName);
		if (_pBuffer != NULL)
			glTexImage2D(GL_TEXTURE_2D, 0, format, _pBuffer->m_pTexture->realWidth, _pBuffer->m_pTexture->realHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, format, video().getWidth(), video().getHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glBindTexture( GL_TEXTURE_2D, 0);
}

void DepthBuffer::setDepthAttachment() {
	if (config.video.multisampling != 0)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_pDepthBufferTexture->glName, 0);
	else
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pDepthBufferTexture->glName, 0);
}

void DepthBuffer::activateDepthBufferTexture() {
	textureCache().activateTexture(0, m_pDepthBufferTexture);
}

void DepthBuffer::bindDepthImageTexture() {
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	glBindImageTexture(depthImageUnit, m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);
#endif
}

void DepthBufferList::init()
{
	m_pCurrent = NULL;

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

void DepthBufferList::destroy()
{
	delete[] m_pzLUT;
	m_pzLUT = NULL;
	m_pCurrent = NULL;
	m_list.clear();
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
	if (pFrameBuffer == NULL)
		pFrameBuffer = frameBufferList().getCurrent();

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

void DepthBufferList::clearBuffer()
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (!video().getRender().isImageTexturesSupported() || config.frameBufferEmulation.N64DepthCompare == 0)
		return;
	if (m_pCurrent == NULL || m_pCurrent->m_FBO == 0)
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
