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
		m_FBO(0), m_pTexture(NULL), m_curIndex(0)
	{
		m_aAddress = 0;
		m_aPBO[0] = m_aPBO[1] = 0;
	}

	void Init();
	void Destroy();

	void CopyToRDRAM( u32 address, bool bSync );

private:
	struct RGBA {
		u8 r, g, b, a;
	};

	GLuint m_FBO;
	CachedTexture * m_pTexture;
	u32 m_aAddress;
	u32 m_curIndex;
	GLuint m_aPBO[2];
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

FrameBuffer::FrameBuffer() : m_cleared(false), m_pLoadTile(NULL), m_pDepthBuffer(NULL), m_pResolveTexture(NULL), m_resolveFBO(0), m_resolved(false)
{
	m_pTexture = textureCache().addFrameBufferTexture();
	glGenFramebuffers(1, &m_FBO);
}

FrameBuffer::FrameBuffer(FrameBuffer && _other) :
	m_startAddress(_other.m_startAddress), m_endAddress(_other.m_endAddress),
	m_size(_other.m_size), m_width(_other.m_width), m_height(_other.m_height), m_fillcolor(_other.m_fillcolor),
	m_scaleX(_other.m_scaleX), m_scaleY(_other.m_scaleY), m_cleared(_other.m_cleared), m_cfb(_other.m_cfb),
	m_FBO(_other.m_FBO), m_pLoadTile(_other.m_pLoadTile), m_pTexture(_other.m_pTexture), m_pDepthBuffer(_other.m_pDepthBuffer),
	m_pResolveTexture(_other.m_pResolveTexture), m_resolveFBO(_other.m_resolveFBO), m_resolved(_other.m_resolved)
{
	_other.m_FBO = 0;
	_other.m_pTexture = NULL;
	_other.m_pLoadTile = NULL;
	_other.m_pDepthBuffer = NULL;
	_other.m_pResolveTexture = NULL;
	_other.m_resolveFBO = 0;
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
	_pTexture->width = (u32)(m_width * video().getScaleX());
	_pTexture->height = (u32)(m_height * video().getScaleY());
	_pTexture->format = _format;
	_pTexture->size = _size;
	_pTexture->clampS = 1;
	_pTexture->clampT = 1;
	_pTexture->address = m_startAddress;
	_pTexture->clampWidth = m_width;
	_pTexture->clampHeight = m_height;
	_pTexture->frameBufferTexture = TRUE;
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
	if (_size > G_IM_SIZ_8b)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _pTexture->realWidth, _pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, monohromeInternalformat, _pTexture->realWidth, _pTexture->realHeight, 0, monohromeformat, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _pTexture->glName, 0);
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

	_initTexture(_format, _size, m_pTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	if (config.video.multisampling != 0) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pTexture->glName);
		if (_size > G_IM_SIZ_8b)
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, GL_RGBA8, m_pTexture->realWidth, m_pTexture->realHeight, false);
		else
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling, monohromeInternalformat, m_pTexture->realWidth, m_pTexture->realHeight, false);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_pTexture->glName, 0);

		m_pResolveTexture = textureCache().addFrameBufferTexture();
		_initTexture(_format, _size, m_pResolveTexture);
		glGenFramebuffers(1, &m_resolveFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFBO);
		_setAndAttachTexture(_size, m_pResolveTexture);
		assert(checkFBO());

		glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	} else
		_setAndAttachTexture(_size, m_pTexture);
}

void FrameBuffer::resolveMultisampledTexture()
{
	if (m_resolved)
		return;
	glDisable(GL_SCISSOR_TEST);
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
	glEnable(GL_SCISSOR_TEST);
	m_resolved = true;
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

void FrameBufferList::init()
{
	 m_pCurrent = NULL;
}

void FrameBufferList::destroy() {
	m_list.clear();
	m_pCurrent = NULL;
	m_drawBuffer = GL_BACK;
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
	if (VI.width == 0 || VI.height == 0) // H width is zero. Don't save
		return;
	OGLVideo & ogl = video();
	m_drawBuffer = GL_FRAMEBUFFER;
	if (m_pCurrent != NULL && gDP.colorImage.height > 0) {
		m_pCurrent->m_endAddress = min(RDRAMSize, m_pCurrent->m_startAddress + (((m_pCurrent->m_width * gDP.colorImage.height) << m_pCurrent->m_size >> 1) - 1));
		if (!config.frameBufferEmulation.copyToRDRAM && !m_pCurrent->m_cfb && !m_pCurrent->m_cleared && gDP.colorImage.height > 1)
			gDPFillRDRAM(m_pCurrent->m_startAddress, 0, 0, m_pCurrent->m_width, gDP.colorImage.height, m_pCurrent->m_width, m_pCurrent->m_size, m_pCurrent->m_fillcolor, false);
	}

	const u32 endAddress = _address + ((_width * (_height - 1)) << _size >> 1) - 1;
	if (m_pCurrent == NULL || m_pCurrent->m_startAddress != _address || m_pCurrent->m_width != _width)
		m_pCurrent = _findBuffer(_address, endAddress, _width);
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
	}

	attachDepthBuffer();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "FrameBuffer_SaveBuffer( 0x%08X ); depth buffer is 0x%08X\n",
		address, (depthBuffer.top != NULL && depthBuffer.top->renderbuf > 0) ? depthBuffer.top->address : 0
	);
#endif
	// HACK ALERT: Dirty hack for Mario Tennis score board
	if (bNew && (m_pCurrent->m_startAddress == 0x13ba50 || m_pCurrent->m_startAddress == 0x264430))
		g_RDRAMtoFB.CopyFromRDRAM(m_pCurrent->m_startAddress, false);
	if (!_cfb)
		*(u32*)&RDRAM[m_pCurrent->m_startAddress] = m_pCurrent->m_startAddress;

	m_pCurrent->m_cleared = false;
	m_pCurrent->m_isDepthBuffer = false;

	gSP.changed |= CHANGED_TEXTURE;
}

void FrameBufferList::removeBuffer(u32 _address )
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress == _address) {
			if (m_pCurrent != NULL && m_pCurrent->m_startAddress == _address)
				m_pCurrent = NULL;
			m_list.erase(iter);
			return;
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
		m_pCurrent->m_pDepthBuffer = pDepthBuffer;
		pDepthBuffer->setDepthAttachment();
		if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
			pDepthBuffer->bindDepthImageTexture();
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
#ifndef GLES2
	g_fbToRDRAM.Init();
	g_dbToRDRAM.Init();
#endif
	g_RDRAMtoFB.Init();
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

	if (VI.width == 0 || *REG.VI_WIDTH == 0) // H width is zero. Don't draw
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
	X0 = (GLint)(max(0, hStart - (isPAL ? 128 : 108)) * scaleX * ogl.getScaleX());
	Xwidth = (GLint)((min(VI.width, (hEnd - hStart)*scaleX)) * ogl.getScaleX());
	X1 = X0 + Xwidth;

	PostProcessor::get().process(pBuffer);
	// glDisable(GL_SCISSOR_TEST) does not affect glBlitFramebuffer, at least on AMD
	glScissor(0, 0, ogl.getScreenWidth(), ogl.getScreenHeight());
	glDisable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//glDrawBuffer( GL_BACK );
	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	ogl.getRender().clearColorBuffer(clearColor);
	const float srcScaleY = ogl.getScaleY();
	const GLint hOffset = (ogl.getScreenWidth() - ogl.getWidth()) / 2;
	GLint vOffset = (ogl.getScreenHeight() - ogl.getHeight()) / 2;
	if (!ogl.isFullscreen())
		vOffset += ogl.getHeightOffset();

	GLint srcCoord[4] = { X0, (GLint)(srcY0*srcScaleY), X1, (GLint)(srcY1*srcScaleY) };
	GLint dstCoord[4] = { X0 + hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + X1, vOffset + (GLint)(dstY1*dstScaleY) };

	GLenum filter = GL_LINEAR;
	if (config.video.multisampling != 0) {
		if (dstPartHeight > 0 ||
			(srcCoord[2] - srcCoord[0]) != (dstCoord[2] - dstCoord[0]) ||
			(srcCoord[3] - srcCoord[1]) != (dstCoord[3] - dstCoord[1])) {
			pBuffer->resolveMultisampledTexture();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		} else
			filter = GL_NEAREST;
	}

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
				0, (GLint)(srcY0*srcScaleY), ogl.getWidth(), (GLint)(srcY1*srcScaleY),
				hOffset, vOffset + (GLint)(dstY0*dstScaleY), hOffset + ogl.getWidth(), vOffset + (GLint)(dstY1*dstScaleY),
				GL_COLOR_BUFFER_BIT, filter
			);
		}
	}

	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
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
	m_drawBuffer = GL_BACK;
	OGLRender::TexturedRectParams params(0.0f, 0.0f, width, height, 0.0f, 0.0f, width - 1.0f, height - 1.0f, false);
	ogl.getRender().drawTexturedRect(params);
	ogl.swapBuffers();
	m_drawBuffer = GL_FRAMEBUFFER;
	glEnable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, m_pCurrent->m_FBO);
	gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_COMBINE;
}
#endif

void FrameBuffer_ActivateBufferTexture(s16 t, FrameBuffer *pBuffer)
{
	if (pBuffer == NULL || pBuffer->m_pTexture == NULL)
		return;

	CachedTexture *pTexture = pBuffer->getTexture();

	pTexture->scaleS = video().getScaleX() / (float)pTexture->realWidth;
	pTexture->scaleT = video().getScaleY() / (float)pTexture->realHeight;

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

	CachedTexture *pTexture = pBuffer->getTexture();
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
	m_pTexture->frameBufferTexture = TRUE;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 1024;
	m_pTexture->realHeight = 512;
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
	glGenBuffers(2, m_aPBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[0]);
	glBufferData(GL_PIXEL_PACK_BUFFER, 640*480*4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[1]);
	glBufferData(GL_PIXEL_PACK_BUFFER, 640*480*4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void FrameBufferToRDRAM::Destroy() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &m_FBO);
	m_FBO = 0;
	if (m_pTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = NULL;
	}
	glDeleteBuffers(2, m_aPBO);
	m_aPBO[0] = m_aPBO[1] = 0;
	m_curIndex = 0;
	m_aAddress = 0;
}

void FrameBufferToRDRAM::CopyToRDRAM( u32 address, bool bSync ) {
	if (VI.width == 0) // H width is zero. Don't copy
		return;
	FrameBuffer *pBuffer = frameBufferList().findBuffer(address);
	if (pBuffer == NULL)
		return;

	address = pBuffer->m_startAddress;
	if (config.video.multisampling != 0) {
		pBuffer->resolveMultisampledTexture();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_resolveFBO);
	} else
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	glDisable(GL_SCISSOR_TEST);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glBlitFramebuffer(
		0, 0, video().getWidth(), video().getHeight(),
		0, 0, pBuffer->m_width, pBuffer->m_height,
		GL_COLOR_BUFFER_BIT, GL_LINEAR
	);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#ifndef GLES2
	// If Sync, read pixels from the buffer, copy them to RDRAM.
	// If not Sync, read pixels from the buffer, copy pixels from the previous buffer to RDRAM.
	if (m_aAddress == 0)
		bSync = true;
	m_curIndex = (m_curIndex + 1) % 2;
	const u32 nextIndex = bSync ? m_curIndex : (m_curIndex + 1) % 2;
	m_aAddress = address;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[m_curIndex]);
	glReadPixels( 0, 0, VI.width, VI.height, GL_RGBA, GL_UNSIGNED_BYTE, 0 );

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[nextIndex]);
	GLubyte* pixelData = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if(pixelData == NULL) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		return;
	}
#else
	m_curIndex = 0;
	m_aAddress = address;
	const u32 nextIndex = 0;
	GLubyte* pixelData = (GLubyte* )malloc(VI.width*VI.height*4);
	if(pixelData == NULL)
		return;
	glReadPixels( 0, 0, VI.width, VI.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelData );
#endif // GLES2

	if (pBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_dst = (u32*)(RDRAM + m_aAddress);
		u32 *ptr_src = (u32*)pixelData;

		for (u32 y = 0; y < VI.height; ++y) {
			for (u32 x = 0; x < VI.width; ++x)
				ptr_dst[x + y*VI.width] = ptr_src[x + (VI.height - y - 1)*VI.width];
		}
	} else {
		u16 *ptr_dst = (u16*)(RDRAM + m_aAddress);
		RGBA * ptr_src = (RGBA*)pixelData;

		for (u32 y = 0; y < VI.height; ++y) {
			for (u32 x = 0; x < VI.width; ++x) {
					const RGBA & c = ptr_src[x + (VI.height - y - 1)*VI.width];
					ptr_dst[(x + y*VI.width)^1] = ((c.r>>3)<<11) | ((c.g>>3)<<6) | ((c.b>>3)<<1) | (c.a == 0 ? 0 : 1);
			}
		}
	}
#if 1 //ndef GLES2
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
	free(pixelData);
#endif
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glEnable(GL_SCISSOR_TEST);
}
#endif // GLES2

void FrameBuffer_CopyToRDRAM( u32 address, bool bSync )
{
#ifndef GLES2
	g_fbToRDRAM.CopyToRDRAM(address, bSync);
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
	m_pColorTexture->frameBufferTexture = TRUE;
	m_pColorTexture->maskS = 0;
	m_pColorTexture->maskT = 0;
	m_pColorTexture->mirrorS = 0;
	m_pColorTexture->mirrorT = 0;
	m_pColorTexture->realWidth = 640;
	m_pColorTexture->realHeight = 480;
	m_pColorTexture->textureBytes = m_pColorTexture->realWidth * m_pColorTexture->realHeight;
	textureCache().addFrameBufferTextureSize(m_pColorTexture->textureBytes);

	m_pDepthTexture = textureCache().addFrameBufferTexture();
	m_pDepthTexture->format = G_IM_FMT_I;
	m_pDepthTexture->clampS = 1;
	m_pDepthTexture->clampT = 1;
	m_pDepthTexture->frameBufferTexture = TRUE;
	m_pDepthTexture->maskS = 0;
	m_pDepthTexture->maskT = 0;
	m_pDepthTexture->mirrorS = 0;
	m_pDepthTexture->mirrorT = 0;
	m_pDepthTexture->realWidth = 640;
	m_pDepthTexture->realHeight = 480;
	m_pDepthTexture->textureBytes = m_pDepthTexture->realWidth * m_pDepthTexture->realHeight * sizeof(float);
	textureCache().addFrameBufferTextureSize(m_pDepthTexture->textureBytes);

	glBindTexture( GL_TEXTURE_2D, m_pColorTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_pColorTexture->realWidth, m_pColorTexture->realHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D, m_pDepthTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_pDepthTexture->realWidth, m_pDepthTexture->realHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
	glBufferData(GL_PIXEL_PACK_BUFFER, 640*480*sizeof(float), NULL, GL_DYNAMIC_DRAW);
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
		return false;
	FrameBuffer *pBuffer = frameBufferList().findBuffer(_address);
	if (pBuffer == NULL || pBuffer->m_pDepthBuffer == NULL || !pBuffer->m_pDepthBuffer->m_cleared)
		return false;

	m_lastDList = RSP.DList;
	DepthBuffer * pDepthBuffer = pBuffer->m_pDepthBuffer;
	const u32 address = pDepthBuffer->m_address;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		0, 0, video().getWidth(), video().getHeight(),
		0, 0, pBuffer->m_width, pBuffer->m_height,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
	);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadPixels(0, 0, VI.width, VI.height, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO);
	GLubyte* pixelData = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if(pixelData == NULL) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		return false;
	}

	f32 * ptr_src = (f32*)pixelData;
	u16 *ptr_dst = (u16*)(RDRAM + address);
	const f32 scale = gSP.viewport.vscale[2] * 32768.0f;
	const f32 trans = gSP.viewport.vtrans[2] * 32768.0f;
	const u16 * const zLUT = depthBufferList().getZLUT();

	for (u32 y = 0; y < VI.height; ++y) {
		for (u32 x = 0; x < VI.width; ++x) {
			f32 z = ptr_src[x + (VI.height - y - 1)*VI.width];
			if (z == 1.0f)
				ptr_dst[(x + y*VI.width) ^ 1] = zLUT[0x3FFFF];
			else {
				z = z*2.0f - 1.0f;
				z = (z*scale + trans) * 8.0f;
				const u32 idx = min(0x3FFFFU, u32(floorf(z + 0.5f)));
				ptr_dst[(x + y*VI.width) ^ 1] = zLUT[idx];
			}
		}
	}

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glEnable(GL_SCISSOR_TEST);
	return true;
}
#endif // GLES2

bool FrameBuffer_CopyDepthBuffer( u32 address ) {
#ifndef GLES2
	return g_dbToRDRAM.CopyToRDRAM(address);
#endif
}

void RDRAMtoFrameBuffer::Init()
{
	m_pTexture = textureCache().addFrameBufferTexture();
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = TRUE;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 1024;
	m_pTexture->realHeight = 512;
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

	const u32 width = pBuffer->m_width;
	const u32 height = pBuffer->m_height;
	m_pTexture->width = width;
	m_pTexture->height = height;
	const u32 dataSize = width*height*4;
#ifndef GLES2
	PBOBinder binder(m_PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, NULL, GL_DYNAMIC_DRAW);
	GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
#else
	m_PBO = (GLubyte*)malloc(dataSize);
	GLubyte* ptr = m_PBO;
	PBOBinder binder(m_PBO);
#endif // GLES2
	if (ptr == NULL)
		return;

	u8 * image = RDRAM + _address;
	u32 * dst = (u32*)ptr;

	u32 empty = 0;
	u32 r, g, b,a, idx;
	if (pBuffer->m_size == G_IM_SIZ_16b) {
		u16 * src = (u16*)image;
		u16 col;
		const u32 bound = (RDRAMSize + 1 - _address) >> 1;
		for (u32 y = 0; y < height; y++)
		{
			for (u32 x = 0; x < width; x++)
			{
				idx = (x + (height - y - 1)*width)^1;
				if (idx >= bound)
					break;
				col = src[idx];
				empty |= col;
				r = ((col >> 11)&31)<<3;
				g = ((col >> 6)&31)<<3;
				b = ((col >> 1)&31)<<3;
				a = (col&1) > 0 ? 0xff : 0;
				//*(dst++) = RGBA5551_RGBA8888(c);
				dst[x + y*width] = (a<<24)|(b<<16)|(g<<8)|r;
			}
		}
	} else {
		// 32 bit
		u32 * src = (u32*)image;
		u32 col;
		const u32 bound = (RDRAMSize + 1 - _address) >> 2;
		for (u32 y=0; y < height; y++)
		{
			for (u32 x=0; x < width; x++)
			{
				idx = x + (height - y - 1)*width;
				if (idx >= bound)
					break;
				col = src[idx];
				empty |= col;
				r = (col >> 24) & 0xff;
				g = (col >> 16) & 0xff;
				b = (col >> 8) & 0xff;
				a = col & 0xff;
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

	OGLVideo & ogl = video();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pBuffer->m_FBO);
	if (_bUseAlpha) {
		CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}	else {
		CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1));
		glDisable(GL_BLEND);
	}
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	const u32 gspChanged = gSP.changed & CHANGED_CPU_FB_WRITE;
	gSP.changed = gDP.changed = 0;

	m_pTexture->scaleS = 1.0f / (float)m_pTexture->realWidth;
	m_pTexture->scaleT = 1.0f / (float)m_pTexture->realHeight;
	m_pTexture->shiftScaleS = 1.0f;
	m_pTexture->shiftScaleT = 1.0f;
	m_pTexture->offsetS = 0;
	m_pTexture->offsetT = (float)m_pTexture->height;
	textureCache().activateTexture(0, m_pTexture);

	OGLRender::TexturedRectParams params(0.0f, 0.0f, (float)width, (float)height, 0.0f, 0.0f, width - 1.0f, height - 1.0f, false);
	ogl.getRender().drawTexturedRect(params);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);
	gSP.changed |= gspChanged | CHANGED_TEXTURE | CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_COMBINE;
}

void FrameBuffer_CopyFromRDRAM( u32 address, bool bUseAlpha )
{
	g_RDRAMtoFB.CopyFromRDRAM(address, bUseAlpha);
}
