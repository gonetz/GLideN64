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
#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include "DisplayWindow.h"

const GLuint ZlutImageUnit = 0;
const GLuint TlutImageUnit = 1;
const GLuint depthImageUnit = 2;

using namespace graphics;

DepthBuffer::DepthBuffer() : m_address(0), m_width(0), m_ulx(0), m_uly(0), m_lrx(0), m_lry(0),
	m_depthImageFBO(0), m_pDepthImageTexture(nullptr), m_pDepthBufferTexture(nullptr),
	m_depthRenderbuffer(0), m_depthRenderbufferWidth(0),
	m_cleared(false), m_pResolveDepthBufferTexture(nullptr), m_resolved(false),
	m_pDepthBufferCopyTexture(nullptr), m_copied(false)
{
	m_copyFBO = GLuint(gfxContext.createFramebuffer());
	if (Context::imageTextures && config.frameBufferEmulation.N64DepthCompare != 0)
		m_depthImageFBO = GLuint(gfxContext.createFramebuffer());
}

DepthBuffer::DepthBuffer(DepthBuffer && _other) :
	m_address(_other.m_address), m_width(_other.m_width),
	m_ulx(_other.m_ulx), m_uly(_other.m_uly), m_lrx(_other.m_lrx), m_lry(_other.m_lry),
	m_depthImageFBO(_other.m_depthImageFBO), m_pDepthImageTexture(_other.m_pDepthImageTexture), m_pDepthBufferTexture(_other.m_pDepthBufferTexture),
	m_depthRenderbuffer(_other.m_depthRenderbuffer), m_depthRenderbufferWidth(_other.m_depthRenderbufferWidth),
	m_cleared(_other.m_cleared), m_pResolveDepthBufferTexture(_other.m_pResolveDepthBufferTexture), m_resolved(_other.m_resolved),
	m_pDepthBufferCopyTexture(_other.m_pDepthBufferCopyTexture), m_copied(_other.m_copied)
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
	gfxContext.deleteFramebuffer(graphics::ObjectHandle(m_depthImageFBO));
	gfxContext.deleteFramebuffer(graphics::ObjectHandle(m_depthRenderbuffer));
	gfxContext.deleteFramebuffer(graphics::ObjectHandle(m_copyFBO));

	textureCache().removeFrameBufferTexture(m_pDepthImageTexture);
	textureCache().removeFrameBufferTexture(m_pDepthBufferTexture);
	textureCache().removeFrameBufferTexture(m_pResolveDepthBufferTexture);
	textureCache().removeFrameBufferTexture(m_pDepthBufferCopyTexture);
}

void DepthBuffer::initDepthImageTexture(FrameBuffer * _pBuffer)
{
	if (!Context::imageTextures || config.frameBufferEmulation.N64DepthCompare == 0 || m_pDepthImageTexture != nullptr)
		return;

	const graphics::FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();
	m_pDepthImageTexture = textureCache().addFrameBufferTexture(false);

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
	m_pDepthImageTexture->textureBytes = m_pDepthImageTexture->realWidth * m_pDepthImageTexture->realHeight * fbTexFormat.depthImageFormatBytes;
	textureCache().addFrameBufferTextureSize(m_pDepthImageTexture->textureBytes);

	{
		graphics::Context::InitTextureParams params;
		params.handle = graphics::ObjectHandle(m_pDepthImageTexture->glName);
		params.width = m_pDepthImageTexture->realWidth;
		params.height = m_pDepthImageTexture->realHeight;
		params.internalFormat = fbTexFormat.depthImageInternalFormat;
		params.format = fbTexFormat.depthImageFormat;
		params.dataType = fbTexFormat.depthImageType;
		gfxContext.init2DTexture(params);
	}
	{
		graphics::Context::TexParameters params;
		params.handle = graphics::ObjectHandle(m_pDepthImageTexture->glName);
		params.target = graphics::target::TEXTURE_2D;
		params.textureUnitIndex = graphics::textureIndices::Tex[0];
		params.minFilter = graphics::textureParameters::FILTER_NEAREST;
		params.magFilter = graphics::textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(params);
	}
	{
		graphics::Context::FrameBufferRenderTarget bufTarget;
		bufTarget.bufferHandle = graphics::ObjectHandle(m_depthImageFBO);
		bufTarget.bufferTarget = graphics::bufferTarget::DRAW_FRAMEBUFFER;
		bufTarget.attachment = graphics::bufferAttachment::COLOR_ATTACHMENT0;
		bufTarget.textureTarget = graphics::target::TEXTURE_2D;
		bufTarget.textureHandle = graphics::ObjectHandle(m_pDepthImageTexture->glName);
		gfxContext.addFrameBufferRenderTarget(bufTarget);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(_pBuffer->m_FBO));

	depthBufferList().clearBuffer(0, 0, VI.width, VI.height);
}

void DepthBuffer::_initDepthBufferTexture(FrameBuffer * _pBuffer, CachedTexture * _pTexture, bool _multisample)
{
	const graphics::FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();

	if (_pBuffer != nullptr) {
		_pTexture->width = (u32)(_pBuffer->m_pTexture->width);
		_pTexture->height = (u32)(_pBuffer->m_pTexture->height);
		_pTexture->address = _pBuffer->m_startAddress;
		_pTexture->clampWidth = _pBuffer->m_width;
		_pTexture->clampHeight = _pBuffer->m_height;
	} else {
		if (config.frameBufferEmulation.nativeResFactor == 0) {
			_pTexture->width = dwnd().getWidth();
			_pTexture->height = dwnd().getHeight();
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
	_pTexture->textureBytes = _pTexture->realWidth * _pTexture->realHeight * fbTexFormat.depthFormatBytes;
	textureCache().addFrameBufferTextureSize(_pTexture->textureBytes);

	{
		graphics::Context::InitTextureParams params;
		params.handle = graphics::ObjectHandle(_pTexture->glName);
		params.msaaLevel = _multisample ? config.video.multisampling : 0U;
		params.width = _pTexture->realWidth;
		params.height = _pTexture->realHeight;
		params.internalFormat = fbTexFormat.depthInternalFormat;
		params.format = fbTexFormat.depthFormat;
		params.dataType = fbTexFormat.depthType;
		gfxContext.init2DTexture(params);
	}
	_pTexture->frameBufferTexture = _multisample ? CachedTexture::fbMultiSample : CachedTexture::fbOneSample;
	{
		graphics::Context::TexParameters params;
		params.handle = graphics::ObjectHandle(_pTexture->glName);
		params.target = _multisample ? graphics::target::TEXTURE_2D_MULTISAMPLE : graphics::target::TEXTURE_2D;
		params.textureUnitIndex = graphics::textureIndices::Tex[0];
		params.minFilter = graphics::textureParameters::FILTER_NEAREST;
		params.magFilter = graphics::textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(params);
	}

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
			m_depthRenderbufferWidth = dwnd().getWidth();
			height = dwnd().getHeight();
		} else {
			m_depthRenderbufferWidth = VI.width * config.frameBufferEmulation.nativeResFactor;
			height = VI.height * config.frameBufferEmulation.nativeResFactor;
		}
	}

	graphics::ObjectHandle renderbufHandle = gfxContext.createRenderbuffer();
	m_depthRenderbuffer = GLuint(renderbufHandle);
	graphics::Context::InitRenderbufferParams params;
	params.handle = renderbufHandle;
	params.target = graphics::target::RENDERBUFFER;
	params.format = gfxContext.getFramebufferTextureFormats().depthInternalFormat;
	params.width = m_depthRenderbufferWidth;
	params.height = height;
	gfxContext.initRenderbuffer(params);
}

void DepthBuffer::setDepthAttachment(GLuint _fbo, GLenum _target)
{
	graphics::Context::FrameBufferRenderTarget params;
	params.attachment = graphics::bufferAttachment::DEPTH_ATTACHMENT;
	params.bufferHandle = graphics::ObjectHandle(_fbo);
	params.bufferTarget = _target;
#ifndef USE_DEPTH_RENDERBUFFER
	params.textureHandle = graphics::ObjectHandle(m_pDepthBufferTexture->glName);
	params.textureTarget = config.video.multisampling != 0 ? graphics::target::TEXTURE_2D_MULTISAMPLE : graphics::target::TEXTURE_2D;
#else
	params.textureHandle = graphics::ObjectHandle(m_depthRenderbuffer);
	params.textureTarget = graphics::target::RENDERBUFFER;
#endif // USE_DEPTH_RENDERBUFFER
	gfxContext.addFrameBufferRenderTarget(params);

	m_copied = false;
	m_resolved = false;
}

void DepthBuffer::initDepthBufferTexture(FrameBuffer * _pBuffer)
{
#ifndef USE_DEPTH_RENDERBUFFER
	if (m_pDepthBufferTexture == nullptr) {
		m_pDepthBufferTexture = textureCache().addFrameBufferTexture(config.video.multisampling != 0);
		_initDepthBufferTexture(_pBuffer, m_pDepthBufferTexture, config.video.multisampling != 0);
	}
#else
	_initDepthBufferRenderbuffer(_pBuffer);
#endif

#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling != 0 && m_pResolveDepthBufferTexture == nullptr) {
		m_pResolveDepthBufferTexture = textureCache().addFrameBufferTexture(false);
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
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(_pBuffer->m_FBO));
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	GLuint attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(_pBuffer->m_resolveFBO));
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(_pBuffer->m_FBO));
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
		m_pDepthBufferCopyTexture = textureCache().addFrameBufferTexture(false);
		_initDepthBufferTexture(_pBuffer, m_pDepthBufferCopyTexture, false);
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(_pBuffer->m_FBO));
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_copyFBO);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		_pBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample ? _pBuffer->m_pResolveTexture->glName : _pBuffer->m_pTexture->glName,
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
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(_pBuffer->m_FBO));
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
	glBindImageTexture(depthImageUnit, m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE,
		GLenum(gfxContext.getFramebufferTextureFormats().depthImageInternalFormat));
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

	DepthBuffer * pDepthBuffer = findBuffer(_address);

	if (pDepthBuffer != nullptr && pFrameBuffer != nullptr && pDepthBuffer->m_width != pFrameBuffer->m_width) {
		removeBuffer(_address);
		pDepthBuffer = nullptr;
	}

	if (pDepthBuffer == nullptr && VI.height != 0) {
		m_list.emplace_front();
		DepthBuffer & buffer = m_list.front();

		buffer.m_address = _address;
		buffer.m_width = pFrameBuffer != nullptr ? pFrameBuffer->m_width : VI.width;

		buffer.initDepthBufferTexture(pFrameBuffer);

		pDepthBuffer = &buffer;
	}

	//Check for null since the depth buffer will not be initialized if VI.height == 0
	if(pDepthBuffer != nullptr) {
		DepthBuffer * pCurrent = m_pCurrent;
		m_pCurrent = pDepthBuffer;
		frameBufferList().attachDepthBuffer();
		if (pDepthBuffer->m_address != gDP.depthImageAddress)
			m_pCurrent = pCurrent;
	}

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

	const graphics::FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();

	m_pCurrent->m_cleared = true;
	m_pCurrent->m_ulx = _ulx;
	m_pCurrent->m_uly = _uly;
	m_pCurrent->m_lrx = _lrx;
	m_pCurrent->m_lry = _lry;

	if (m_pCurrent->m_depthImageFBO == 0 || !Context::imageTextures || config.frameBufferEmulation.N64DepthCompare == 0)
		return;

	float color[4] = {1.0f, 1.0f, 0.0f, 1.0f};
	glBindImageTexture(depthImageUnit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GLenum(fbTexFormats.depthImageInternalFormat));
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pCurrent->m_depthImageFBO);
	const u32 cycleType = gDP.otherMode.cycleType;
	gDP.otherMode.cycleType = G_CYC_FILL;
	dwnd().getDrawer().drawRect(_ulx, _uly, _lrx, _lry, color);
	gDP.otherMode.cycleType = cycleType;
	glBindImageTexture(depthImageUnit, m_pCurrent->m_pDepthImageTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GLenum(fbTexFormats.depthImageInternalFormat));
	frameBufferList().setCurrentDrawBuffer();
}

void DepthBuffer_Init()
{
	depthBufferList().init();
}

void DepthBuffer_Destroy()
{
	depthBufferList().destroy();
}
