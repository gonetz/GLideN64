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

DepthBuffer::DepthBuffer() : m_address(0), m_width(0), m_renderbuf(0), m_FBO(0), m_pDepthTexture(NULL)
{
	glGenRenderbuffers(1, &m_renderbuf);
	glGenFramebuffers(1, &m_FBO);
}

DepthBuffer::DepthBuffer(DepthBuffer && _other) :
	m_address(_other.m_address), m_width(_other.m_width),
	m_renderbuf(_other.m_renderbuf), m_FBO(_other.m_FBO), m_pDepthTexture(_other.m_pDepthTexture)
{
	_other.m_renderbuf = 0;
	_other.m_FBO = 0;
	_other.m_pDepthTexture = NULL;
}

DepthBuffer::~DepthBuffer()
{
	if (m_renderbuf != 0)
		glDeleteRenderbuffers(1, &m_renderbuf);
	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);
	if (m_pDepthTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pDepthTexture);
}

void DepthBuffer::initDepthTexture(FrameBuffer * _pBuffer)
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (m_pDepthTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pDepthTexture);
	m_pDepthTexture = textureCache().addFrameBufferTexture();

	m_pDepthTexture->width = (u32)(_pBuffer->m_pTexture->width);
	m_pDepthTexture->height = (u32)(_pBuffer->m_pTexture->height);
	m_pDepthTexture->format = 0;
	m_pDepthTexture->size = 2;
	m_pDepthTexture->clampS = 1;
	m_pDepthTexture->clampT = 1;
	m_pDepthTexture->address = _pBuffer->m_startAddress;
	m_pDepthTexture->clampWidth = _pBuffer->m_width;
	m_pDepthTexture->clampHeight = _pBuffer->m_height;
	m_pDepthTexture->frameBufferTexture = TRUE;
	m_pDepthTexture->maskS = 0;
	m_pDepthTexture->maskT = 0;
	m_pDepthTexture->mirrorS = 0;
	m_pDepthTexture->mirrorT = 0;
	m_pDepthTexture->realWidth = (u32)pow2( m_pDepthTexture->width );
	m_pDepthTexture->realHeight = (u32)pow2( m_pDepthTexture->height );
	m_pDepthTexture->textureBytes = m_pDepthTexture->realWidth * m_pDepthTexture->realHeight * 4 * 4; // Width*Height*RGBA*sizeof(GL_RGBA32F)
	textureCache().addFrameBufferTextureSize(m_pDepthTexture->textureBytes);

	glBindTexture( GL_TEXTURE_2D, m_pDepthTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_pDepthTexture->realWidth, m_pDepthTexture->realHeight, 0, GL_RGBA, GL_FLOAT,	NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glBindTexture( GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pDepthTexture->glName, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);
	_pBuffer->m_pDepthBuffer = this;
	depthBufferList().clearBuffer();
#endif // GL_IMAGE_TEXTURES_SUPPORT
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
		buffer.m_pDepthTexture = NULL;
		glBindRenderbuffer(GL_RENDERBUFFER, buffer.m_renderbuf);
#ifndef GLES2
			const GLenum format = GL_DEPTH_COMPONENT;
#else
			const GLenum format = GL_DEPTH_COMPONENT24_OES;
#endif
		if (pFrameBuffer != NULL)
			glRenderbufferStorage(GL_RENDERBUFFER, format, pFrameBuffer->m_pTexture->realWidth, pFrameBuffer->m_pTexture->realHeight);
		else
			glRenderbufferStorage(GL_RENDERBUFFER, format, (u32)pow2(video().getWidth()), (u32)pow2(video().getHeight()));

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
	if (!video().getRender().isImageTexturesSupported())
		return;
	if (m_pCurrent == NULL || m_pCurrent->m_FBO == 0)
		return;
	float color[4] = {1.0f, 1.0f, 0.0f, 1.0f};
	glBindImageTexture(depthImageUnit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_FBO);
	video().getRender().drawRect(0,0,VI.width, VI.height, color);
	glBindImageTexture(depthImageUnit, m_pCurrent->m_pDepthTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
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
