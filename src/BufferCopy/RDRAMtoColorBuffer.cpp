#include "RDRAMtoColorBuffer.h"

#include <FBOTextureFormats.h>
#include <FrameBufferInfo.h>
#include <GLSLCombiner.h>
#include <FrameBuffer.h>
#include <Combiner.h>
#include <Textures.h>
#include <Config.h>
#include <N64.h>
#include <VI.h>

RDRAMtoColorBuffer::RDRAMtoColorBuffer()
	: m_pCurBuffer(nullptr)
	, m_pTexture(nullptr)
	, m_PBO(0) {
}

RDRAMtoColorBuffer & RDRAMtoColorBuffer::get()
{
	static RDRAMtoColorBuffer toCB;
	return toCB;
}

void RDRAMtoColorBuffer::init()
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
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, nullptr);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glBindTexture(GL_TEXTURE_2D, 0);

	// Generate Pixel Buffer Object. Initialize it later
#ifndef GLES2
	glGenBuffers(1, &m_PBO);
#endif
}

void RDRAMtoColorBuffer::destroy()
{
	if (m_pTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = nullptr;
	}
#ifndef GLES2
	if (m_PBO != 0) {
		glDeleteBuffers(1, &m_PBO);
		m_PBO = 0;
	}
#endif
}

void RDRAMtoColorBuffer::addAddress(u32 _address, u32 _size)
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
bool _copyPixelsFromRdram(u32 _address, const std::vector<u32> & _vecAddress, u32* _dst, u32(*converter)(TSrc _c, bool _bCFB), u32 _xor, u32 _width, u32 _height, bool _bCFB)
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

static
u32 RGBA16ToABGR32(u16 col, bool _bCFB)
{
	u32 r, g, b, a;
	r = ((col >> 11) & 31) << 3;
	g = ((col >> 6) & 31) << 3;
	b = ((col >> 1) & 31) << 3;
	if (_bCFB)
		a = 0xFF;
	else
		a = (col & 1) > 0 ? 0xFF : 0U;
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

static
u32 RGBA32ToABGR32(u32 col, bool _bCFB)
{
	u32 r, g, b, a;
	r = (col >> 24) & 0xff;
	g = (col >> 16) & 0xff;
	b = (col >> 8) & 0xff;
	if (_bCFB)
		a = 0xFF;
	else
		a = col & 0xFF;
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

void RDRAMtoColorBuffer::copyFromRDRAM(u32 _address, bool _bCFB)
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

	const u32 height = cutHeight(address, m_pCurBuffer->m_startAddress == _address ? VI.real_height : m_pCurBuffer->m_height, m_pCurBuffer->m_width << m_pCurBuffer->m_size >> 1);
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
	glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
	GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
#else
	GLubyte* ptr = (GLubyte*)malloc(dataSize);
	PBOBinder binder(ptr);
#endif // GLES2
	if (ptr == nullptr)
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

void RDRAMtoColorBuffer::reset()
{
	m_pCurBuffer = nullptr;
	m_vecAddress.clear();
}
