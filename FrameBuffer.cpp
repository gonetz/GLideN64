#include <assert.h>
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
		m_aAddress[0] = m_aAddress[1] = 0;
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
	u32 m_aAddress[2];
	u32 m_curIndex;
	GLuint m_aPBO[2];
};

class DepthBufferToRDRAM
{
public:
	DepthBufferToRDRAM() :
	  m_FBO(0), m_pTexture(NULL), m_curIndex(0)
	{
		m_aPBO[0] = m_aPBO[1] = 0;
		m_aAddress[0] = m_aAddress[1] = 0;
	}

	void Init();
	void Destroy();

	void CopyToRDRAM( u32 address );

private:
	GLuint m_FBO;
	CachedTexture * m_pTexture;
	u32 m_aAddress[2];
	u32 m_curIndex;
	GLuint m_aPBO[2];
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
	struct PBOBinder {
#ifndef GLES2
		PBOBinder(GLuint _PBO)
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _PBO);
		}
		~PBOBinder() {
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}
#else
		PBOBinder(GLubyte* _ptr) : ptr(_ptr) {}
		~PBOBinder() {free(ptr);}
		GLubyte* ptr;
#endif
	};
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

FrameBuffer::FrameBuffer() : m_cleared(false), m_pLoadTile(NULL), m_pDepthBuffer(NULL)
{
	m_pTexture = textureCache().addFrameBufferTexture();
	glGenFramebuffers(1, &m_FBO);
}

FrameBuffer::FrameBuffer(FrameBuffer && _other) :
	m_startAddress(_other.m_startAddress), m_endAddress(_other.m_endAddress),
	m_size(_other.m_size), m_width(_other.m_width), m_height(_other.m_height), m_fillcolor(_other.m_fillcolor),
	m_scaleX(_other.m_scaleX), m_scaleY(_other.m_scaleY), m_cleared(_other.m_cleared),
	m_FBO(_other.m_FBO), m_pTexture(_other.m_pTexture), m_pLoadTile(_other.m_pLoadTile), m_pDepthBuffer(_other.m_pDepthBuffer)
{
	_other.m_FBO = 0;
	_other.m_pTexture = NULL;
	_other.m_pLoadTile = NULL;
	_other.m_pDepthBuffer = NULL;
}


FrameBuffer::~FrameBuffer()
{
	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);
	if (m_pTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pTexture);
}

void FrameBufferList::init()
{
	 m_pCurrent = NULL;
}

void FrameBufferList::destroy() {
	m_list.clear();
	m_pCurrent = NULL;
}

FrameBuffer * FrameBufferList::findBuffer(u32 _address)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress <= _address && iter->m_endAddress >= _address)
				return &(*iter);
	return NULL;
}

FrameBuffer * FrameBufferList::findTmpBuffer(u32 _address)
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress > _address || iter->m_endAddress < _address)
				return &(*iter);
	return NULL;
}

void FrameBufferList::saveBuffer(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height )
{
	OGLVideo & ogl = video();
	m_drawBuffer = GL_FRAMEBUFFER;
	if (m_pCurrent != NULL && gDP.colorImage.height > 1) {
		m_pCurrent->m_endAddress = min(RDRAMSize, m_pCurrent->m_startAddress + (((m_pCurrent->m_width * gDP.colorImage.height) << m_pCurrent->m_size >> 1) - 1));
		if (!config.frameBufferEmulation.copyToRDRAM && !m_pCurrent->m_cleared)
			gDPFillRDRAM(m_pCurrent->m_startAddress, 0, 0, m_pCurrent->m_width, gDP.colorImage.height, m_pCurrent->m_width, m_pCurrent->m_size, m_pCurrent->m_fillcolor);
	}

	if (m_pCurrent == NULL || m_pCurrent->m_startAddress != _address)
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

		buffer.m_startAddress = _address;
		buffer.m_endAddress = _address + ((_width * _height << _size >> 1) - 1);
		buffer.m_width = _width;
		buffer.m_height = _height;
		buffer.m_size = _size;
		buffer.m_scaleX = ogl.getScaleX();
		buffer.m_scaleY = ogl.getScaleY();
		buffer.m_fillcolor = 0;

		buffer.m_pTexture->width = (u32)(buffer.m_width * video().getScaleX());
		buffer.m_pTexture->height = (u32)(buffer.m_height * video().getScaleY());
		buffer.m_pTexture->format = _format;
		buffer.m_pTexture->size = _size;
		buffer.m_pTexture->clampS = 1;
		buffer.m_pTexture->clampT = 1;
		buffer.m_pTexture->address = buffer.m_startAddress;
		buffer.m_pTexture->clampWidth = buffer.m_width;
		buffer.m_pTexture->clampHeight = buffer.m_height;
		buffer.m_pTexture->frameBufferTexture = TRUE;
		buffer.m_pTexture->maskS = 0;
		buffer.m_pTexture->maskT = 0;
		buffer.m_pTexture->mirrorS = 0;
		buffer.m_pTexture->mirrorT = 0;
		buffer.m_pTexture->realWidth = (u32)pow2( buffer.m_pTexture->width );
		buffer.m_pTexture->realHeight = (u32)pow2( buffer.m_pTexture->height );
		buffer.m_pTexture->textureBytes = buffer.m_pTexture->realWidth * buffer.m_pTexture->realHeight * 4;
		textureCache().addFrameBufferTextureSize(buffer.m_pTexture->textureBytes);

		glBindTexture( GL_TEXTURE_2D, buffer.m_pTexture->glName );
		if (_size > G_IM_SIZ_8b)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer.m_pTexture->realWidth, buffer.m_pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, monohromeInternalformat, buffer.m_pTexture->realWidth, buffer.m_pTexture->realHeight, 0, monohromeformat, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, buffer.m_FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.m_pTexture->glName, 0);
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
	*(u32*)&RDRAM[m_pCurrent->m_startAddress] = m_pCurrent->m_startAddress;

	m_pCurrent->m_cleared = false;

	gSP.changed |= CHANGED_TEXTURE;
}

void FrameBufferList::removeBuffer(u32 _address )
{
	for (FrameBuffers::iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
		if (iter->m_startAddress == _address) {
			m_list.erase(iter);
			return;
		}
}

void FrameBufferList::attachDepthBuffer()
{
	DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
	if (m_pCurrent != NULL &&  m_pCurrent->m_FBO > 0 && pDepthBuffer != NULL && pDepthBuffer->m_renderbuf > 0) {
		if (pDepthBuffer->m_pDepthTexture == NULL || pDepthBuffer->m_pDepthTexture->width != m_pCurrent->m_pTexture->width)
			pDepthBuffer->initDepthTexture(m_pCurrent);
		m_pCurrent->m_pDepthBuffer = pDepthBuffer;
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pDepthBuffer->m_renderbuf);
#ifndef GLES2
		GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1,  attachments);
		glBindImageTexture(depthImageUnit, pDepthBuffer->m_pDepthTexture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
#endif
		assert(checkFBO());
	} else if (m_pCurrent != NULL) {
		m_pCurrent->m_pDepthBuffer = NULL;
#ifndef GLES2
		GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1,  attachments);
#endif
		assert(checkFBO());
	}
	currentCombiner()->updateDepthInfo(true);
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
	static u32 vStartPrev = 0;

	if (_SHIFTR( *REG.VI_H_START, 0, 10 ) == 0) // H width is zero. Don't draw
		return;
	FrameBuffer *pBuffer = findBuffer(_address);
	if (pBuffer == NULL)
		return;
	OGLVideo & ogl = video();
	GLint srcY0, srcY1, dstY0, dstY1;
	GLint partHeight = 0;
	dstY0 = 1;
	const u32 vStart = _SHIFTR( *REG.VI_V_START, 17, 9 );
	const u32 vEnd = _SHIFTR( *REG.VI_V_START, 1, 9 );
	bool isLowerField = false;
	if ((*REG.VI_STATUS & 0x40) != 0) {
		const bool isPAL = (*REG.VI_V_SYNC & 0x3ff) > 550;
		isLowerField = isPAL ? vStart < vStartPrev : vStart > vStartPrev;
	}
	vStartPrev = vStart;

	const float viScaleY = ogl.getHeight() / (float)VI.vHeight;

	if (vStart > VI.vStart)
		dstY0 += vStart - VI.vStart;
	dstY1 = dstY0 + vEnd - vStart;
	srcY0 = ((_address - pBuffer->m_startAddress) << 1 >> pBuffer->m_size) / (*REG.VI_WIDTH);
	if (isLowerField)
		--srcY0;

	srcY1 = srcY0 + VI.real_height;
	if (srcY1 > VI.height) {
		partHeight = srcY1 - VI.height;
		srcY1 = VI.height;
		dstY1 -= partHeight;
	}

	// glDisable(GL_SCISSOR_TEST) does not affect glBlitFramebuffer, at least on AMD
	glScissor(0, 0, ogl.getWidth(), ogl.getHeight());
	glDisable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//glDrawBuffer( GL_BACK );
	float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	ogl.getRender().clearColorBuffer(clearColor);
	glBlitFramebuffer(
		0, (GLint)(srcY0*ogl.getScaleY()), ogl.getWidth(), (GLint)(srcY1*ogl.getScaleY()),
		0, ogl.getHeightOffset() + (GLint)(dstY0*viScaleY), ogl.getWidth(), ogl.getHeightOffset() + (GLint)(dstY1*viScaleY),
		GL_COLOR_BUFFER_BIT, GL_LINEAR
	);

	if (partHeight > 0) {
		const u32 size = *REG.VI_STATUS & 3;
		pBuffer = findBuffer(_address + (((*REG.VI_WIDTH)*VI.height)<<size>>1));
		if (pBuffer != NULL) {
			srcY0 = 0;
			srcY1 = partHeight;
			dstY0 = dstY1;
			dstY1 = dstY0 + partHeight;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
			glBlitFramebuffer(
				0, (GLint)(srcY0*ogl.getScaleY()), ogl.getWidth(), (GLint)(srcY1*ogl.getScaleY()),
				0, ogl.getHeightOffset() + (GLint)(dstY0*viScaleY), ogl.getWidth(), ogl.getHeightOffset() + (GLint)(dstY1*viScaleY),
				GL_COLOR_BUFFER_BIT, GL_LINEAR
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
	if (_SHIFTR( *REG.VI_H_START, 0, 10 ) == 0) // H width is zero. Don't draw
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
	ogl.getRender().drawTexturedRect(0.0f, 0.0f, width, height, 0.0f, 0.0f, width - 1.0f, height - 1.0f, false);
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
	pBuffer->m_pTexture->scaleS = video().getScaleX() / (float)pBuffer->m_pTexture->realWidth;
	pBuffer->m_pTexture->scaleT = video().getScaleY() / (float)pBuffer->m_pTexture->realHeight;

	if (gSP.textureTile[t]->shifts > 10)
		pBuffer->m_pTexture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[t]->shifts));
	else if (gSP.textureTile[t]->shifts > 0)
		pBuffer->m_pTexture->shiftScaleS = 1.0f / (float)(1 << gSP.textureTile[t]->shifts);
	else
		pBuffer->m_pTexture->shiftScaleS = 1.0f;

	if (gSP.textureTile[t]->shiftt > 10)
		pBuffer->m_pTexture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[t]->shiftt));
	else if (gSP.textureTile[t]->shiftt > 0)
		pBuffer->m_pTexture->shiftScaleT = 1.0f / (float)(1 << gSP.textureTile[t]->shiftt);
	else
		pBuffer->m_pTexture->shiftScaleT = 1.0f;

	const u32 shift = gSP.textureTile[t]->imageAddress - pBuffer->m_startAddress;
	const u32 factor = pBuffer->m_width << pBuffer->m_size >> 1;
	if (gSP.textureTile[t]->loadType == LOADTYPE_TILE)
	{
		pBuffer->m_pTexture->offsetS = pBuffer->m_pLoadTile->uls;
		pBuffer->m_pTexture->offsetT = (float)(pBuffer->m_height - (pBuffer->m_pLoadTile->ult + shift/factor));
	}
	else
	{
		pBuffer->m_pTexture->offsetS = (float)(shift % factor);
		pBuffer->m_pTexture->offsetT = (float)(pBuffer->m_height - shift/factor);
	}

//	FrameBuffer_RenderBuffer(buffer->startAddress);
	textureCache().activateTexture(t, pBuffer->m_pTexture);
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_ActivateBufferTextureBG(s16 t, FrameBuffer *pBuffer )
{
	pBuffer->m_pTexture->scaleS = video().getScaleX() / (float)pBuffer->m_pTexture->realWidth;
	pBuffer->m_pTexture->scaleT = video().getScaleY() / (float)pBuffer->m_pTexture->realHeight;

	pBuffer->m_pTexture->shiftScaleS = 1.0f;
	pBuffer->m_pTexture->shiftScaleT = 1.0f;

	pBuffer->m_pTexture->offsetS = gSP.bgImage.imageX;
	pBuffer->m_pTexture->offsetT = (float)pBuffer->m_height - gSP.bgImage.imageY;

	//	FrameBuffer_RenderBuffer(buffer->startAddress);
	textureCache().activateTexture(t, pBuffer->m_pTexture);
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
	m_aAddress[0] = m_aAddress[1] = 0;
}

void FrameBufferToRDRAM::CopyToRDRAM( u32 address, bool bSync ) {
	FrameBuffer *pBuffer = frameBufferList().findBuffer(address);
	if (pBuffer == NULL)
		return;

	address = pBuffer->m_startAddress;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pBuffer->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	GLuint attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);
	glBlitFramebuffer(
		0, 0, video().getWidth(), video().getHeight(),
		0, 0, pBuffer->m_width, pBuffer->m_height,
		GL_COLOR_BUFFER_BIT, GL_LINEAR
	);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
#if 1 //ndef GLES2
	// If Sync, read pixels from the buffer, copy them to RDRAM.
	// If not Sync, read pixels from the buffer, copy pixels from the previous buffer to RDRAM.
	if (m_aAddress[m_curIndex] == 0)
		bSync = true;
	m_curIndex = (m_curIndex + 1) % 2;
	const u32 nextIndex = bSync ? m_curIndex : (m_curIndex + 1) % 2;
	m_aAddress[m_curIndex] = address;
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
	m_aAddress[0] = address;
	const u32 nextIndex = 0;
	GLubyte* pixelData = (GLubyte* )malloc(VI.width*VI.height*4);
	if(pixelData == NULL)
		return;
	glReadPixels( 0, 0, VI.width, VI.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelData );
#endif // GLES2

	if (pBuffer->m_size == G_IM_SIZ_32b) {
		u32 *ptr_dst = (u32*)(RDRAM + m_aAddress[nextIndex]);
		u32 *ptr_src = (u32*)pixelData;

		for (u32 y = 0; y < VI.height; ++y) {
			for (u32 x = 0; x < VI.width; ++x)
				ptr_dst[x + y*VI.width] = ptr_src[x + (VI.height - y - 1)*VI.width];
		}
	} else {
		u16 *ptr_dst = (u16*)(RDRAM + m_aAddress[nextIndex]);
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

	m_pTexture = textureCache().addFrameBufferTexture();
	m_pTexture->format = G_IM_FMT_IA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = TRUE;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 1024;
	m_pTexture->realHeight = 512;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight * 2;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);
	glBindTexture( GL_TEXTURE_2D, m_pTexture->glName );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);
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
	glBufferData(GL_PIXEL_PACK_BUFFER, 640*480*2, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[1]);
	glBufferData(GL_PIXEL_PACK_BUFFER, 640*480*2, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void DepthBufferToRDRAM::Destroy() {
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
	m_aAddress[0] = m_aAddress[1] = 0;
}

void DepthBufferToRDRAM::CopyToRDRAM( u32 address) {
	FrameBuffer *pBuffer = frameBufferList().findBuffer(address);
	if (pBuffer == NULL || pBuffer->m_pDepthBuffer == NULL)
		return;

	DepthBuffer * pDepthBuffer = pBuffer->m_pDepthBuffer;
	address = pDepthBuffer->m_address;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pDepthBuffer->m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	GLuint attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);
	glBlitFramebuffer(
		0, 0, video().getWidth(), video().getHeight(),
		0, 0, pBuffer->m_width, pBuffer->m_height,
		GL_COLOR_BUFFER_BIT, GL_LINEAR
	);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);

	m_curIndex = (m_curIndex + 1) % 2;
	const u32 nextIndex = m_aAddress[m_curIndex] == 0 ? m_curIndex : (m_curIndex + 1) % 2;
	m_aAddress[m_curIndex] = address;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[m_curIndex]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels( 0, 0, VI.width, VI.height, GL_RED, GL_UNSIGNED_SHORT, 0 );

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_aPBO[nextIndex]);
	GLubyte* pixelData = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if(pixelData == NULL) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		return;
	}

	u16 * ptr_src = (u16*)pixelData;
	u16 *ptr_dst = (u16*)(RDRAM + m_aAddress[nextIndex]);
	u16 col;

	for (u32 y = 0; y < VI.height; ++y) {
		for (u32 x = 0; x < VI.width; ++x) {
				col = ptr_src[x + (VI.height - y - 1)*VI.width];
				ptr_dst[(x + y*VI.width)^1] = col;
		}
	}

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}
#endif // GLES2

void FrameBuffer_CopyDepthBuffer( u32 address ) {
#ifndef GLES2
	g_dbToRDRAM.CopyToRDRAM(address);
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
				a = col&1 > 0 ? 0xff : 0;
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

#if 0
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
	const GLuint attachment = GL_COLOR_ATTACHMENT0;
	glReadBuffer(attachment);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pBuffer->m_FBO);
	glDrawBuffers(1, &attachment);
	glBlitFramebuffer(
		0, 0, width, height,
		0, 0, ogl.getWidth(), ogl.getHeight(),
		GL_COLOR_BUFFER_BIT, GL_LINEAR
		);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);
#else
	if (_bUseAlpha)
		CombinerInfo::get().setCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0 ) );
	else
		CombinerInfo::get().setCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1 ) );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
//	glDisable( GL_ALPHA_TEST );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_POLYGON_OFFSET_FILL );
	gSP.changed = gDP.changed = 0;

	m_pTexture->scaleS = 1.0f / (float)m_pTexture->realWidth;
	m_pTexture->scaleT = 1.0f / (float)m_pTexture->realHeight;
	m_pTexture->shiftScaleS = 1.0f;
	m_pTexture->shiftScaleT = 1.0f;
	m_pTexture->offsetS = 0;
	m_pTexture->offsetT = (float)m_pTexture->height;
	textureCache().activateTexture(0, m_pTexture);

	ogl.getRender().drawTexturedRect( 0.0f, 0.0f, width, height, 0.0f, 0.0f, width-1.0f, height-1.0f, false );
	gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_COMBINE;
#endif
}

void FrameBuffer_CopyFromRDRAM( u32 address, bool bUseAlpha )
{
	g_RDRAMtoFB.CopyFromRDRAM(address, bUseAlpha);
}
