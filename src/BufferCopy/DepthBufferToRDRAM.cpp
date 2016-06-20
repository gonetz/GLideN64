#include <assert.h>
#include <math.h>
#include <algorithm>

#include "DepthBufferToRDRAM.h"
#include "WriteToRDRAM.h"

#include <FBOTextureFormats.h>
#include <FrameBuffer.h>
#include <DepthBuffer.h>
#include <Textures.h>
#include <Config.h>
#include <N64.h>
#include <VI.h>

#ifndef GLES2

DepthBufferToRDRAM::DepthBufferToRDRAM()
	: m_FBO(0)
	, m_PBO(0)
	, m_frameCount(-1)
	, m_pColorTexture(nullptr)
	,	m_pDepthTexture(nullptr)
	, m_pCurDepthBuffer(nullptr)
{
}

DepthBufferToRDRAM::~DepthBufferToRDRAM()
{
}

DepthBufferToRDRAM & DepthBufferToRDRAM::get()
{
	static DepthBufferToRDRAM dbCopy;
	return dbCopy;
}

void DepthBufferToRDRAM::init()
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

	glBindTexture(GL_TEXTURE_2D, m_pColorTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.monochromeInternalFormat, m_pColorTexture->realWidth, m_pColorTexture->realHeight, 0, fboFormats.monochromeFormat, fboFormats.monochromeType, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, m_pDepthTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.depthInternalFormat, m_pDepthTexture->realWidth, m_pDepthTexture->realHeight, 0, GL_DEPTH_COMPONENT, fboFormats.depthType, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
	glBufferData(GL_PIXEL_PACK_BUFFER, m_pDepthTexture->realWidth * m_pDepthTexture->realHeight * sizeof(float), nullptr, GL_DYNAMIC_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void DepthBufferToRDRAM::destroy() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &m_FBO);
	m_FBO = 0;
	if (m_pColorTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pColorTexture);
		m_pColorTexture = nullptr;
	}
	if (m_pDepthTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pDepthTexture);
		m_pDepthTexture = nullptr;
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
	if (pBuffer == nullptr || pBuffer->isAuxiliary() || pBuffer->m_pDepthBuffer == nullptr || !pBuffer->m_pDepthBuffer->m_cleared)
		return false;

	m_pCurDepthBuffer = pBuffer->m_pDepthBuffer;
	const u32 address = m_pCurDepthBuffer->m_address;
	if (address + numPixels * 2 > RDRAMSize)
		return false;

	const u32 height = cutHeight(address, std::min(VI.height, m_pCurDepthBuffer->m_lry), pBuffer->m_width * 2);
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
		idx = std::min(0x3FFFFU, u32(floorf(_z + 0.5f)));
	}
	return zLUT[idx];
}

bool DepthBufferToRDRAM::_copy(u32 _startAddress, u32 _endAddress)
{
	const u32 stride = m_pCurDepthBuffer->m_width << 1;
	const u32 max_height = cutHeight(_startAddress, std::min(VI.height, m_pCurDepthBuffer->m_lry), stride);

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
	if (pixelData == nullptr)
		return false;

	f32 * ptr_src = (f32*)pixelData;
	u16 *ptr_dst = (u16*)(RDRAM + _startAddress);

	std::vector<f32> srcBuf(width * height);
	memcpy(srcBuf.data(), ptr_src, width * height * sizeof(f32));
	writeToRdram<f32, u16>(srcBuf.data(), ptr_dst, &DepthBufferToRDRAM::_FloatToUInt16, 2.0f, 1, width, height, numPixels, _startAddress, m_pCurDepthBuffer->m_address, G_IM_SIZ_16b);

	m_pCurDepthBuffer->m_cleared = false;
	FrameBuffer * pBuffer = frameBufferList().findBuffer(m_pCurDepthBuffer->m_address);
	if (pBuffer != nullptr)
		pBuffer->m_cleared = false;

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	gDP.changed |= CHANGED_SCISSOR;
	return true;
}

bool DepthBufferToRDRAM::copyToRDRAM(u32 _address)
{
	if (!_prepareCopy(_address, false))
		return false;

	const u32 endAddress = m_pCurDepthBuffer->m_address + (std::min(VI.height, m_pCurDepthBuffer->m_lry) * m_pCurDepthBuffer->m_width * 2);
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
