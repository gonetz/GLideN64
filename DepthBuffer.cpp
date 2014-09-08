#include <malloc.h>
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

void DepthBuffer_Init()
{
	DepthBufferList & dbList = depthBufferList();
	dbList.current = NULL;
	dbList.top = NULL;
	dbList.bottom = NULL;
	dbList.numBuffers = 0;
}

void DepthBuffer_RemoveBottom()
{
	DepthBufferList & dbList = depthBufferList();
	DepthBuffer *newBottom = dbList.bottom->higher;

	if (dbList.bottom == dbList.top)
		dbList.top = NULL;

	if (dbList.bottom->renderbuf != 0)
		glDeleteRenderbuffers(1, &dbList.bottom->renderbuf);
	if (dbList.bottom->depth_texture != NULL)
		textureCache().removeFrameBufferTexture(dbList.bottom->depth_texture);
	free( dbList.bottom );

	dbList.bottom = newBottom;

	if (dbList.bottom != NULL)
		dbList.bottom->lower = NULL;

	dbList.numBuffers--;
}

void DepthBuffer_Remove( DepthBuffer *buffer )
{
	DepthBufferList & dbList = depthBufferList();
	if ((buffer == dbList.bottom) &&
		(buffer == dbList.top))
	{
		dbList.top = NULL;
		dbList.bottom = NULL;
	}
	else if (buffer == dbList.bottom)
	{
		dbList.bottom = buffer->higher;

		if (dbList.bottom)
			dbList.bottom->lower = NULL;
	}
	else if (buffer == dbList.top)
	{
		dbList.top = buffer->lower;

		if (dbList.top)
			dbList.top->higher = NULL;
	}
	else
	{
		buffer->higher->lower = buffer->lower;
		buffer->lower->higher = buffer->higher;
	}

	if (buffer->renderbuf != 0)
		glDeleteRenderbuffers(1, &buffer->renderbuf);
	if (buffer->fbo != 0)
		glDeleteFramebuffers(1, &buffer->fbo);
	if (buffer->depth_texture != NULL)
		textureCache().removeFrameBufferTexture(buffer->depth_texture);
	free( buffer );

	dbList.numBuffers--;
}

void DepthBuffer_RemoveBuffer( u32 address )
{
	DepthBuffer *current = depthBufferList().bottom;

	while (current != NULL)
	{
		if (current->address == address)
		{
			DepthBuffer_Remove( current );
			return;
		}
		current = current->higher;
	}
}

DepthBuffer *DepthBuffer_AddTop()
{
	DepthBufferList & dbList = depthBufferList();
	DepthBuffer *newtop = (DepthBuffer*)malloc( sizeof( DepthBuffer ) );

	newtop->lower = dbList.top;
	newtop->higher = NULL;
	newtop->renderbuf = 0;
	newtop->fbo = 0;

	if (dbList.top)
		dbList.top->higher = newtop;

	if (!dbList.bottom)
		dbList.bottom = newtop;

	dbList.top = newtop;

	dbList.numBuffers++;

	return newtop;
}

void DepthBuffer_MoveToTop( DepthBuffer *newtop )
{
	DepthBufferList & dbList = depthBufferList();
	if (newtop == dbList.top)
		return;

	if (newtop == dbList.bottom)
	{
		dbList.bottom = newtop->higher;
		dbList.bottom->lower = NULL;
	}
	else
	{
		newtop->higher->lower = newtop->lower;
		newtop->lower->higher = newtop->higher;
	}

	newtop->higher = NULL;
	newtop->lower = dbList.top;
	dbList.top->higher = newtop;
	dbList.top = newtop;
}

void DepthBuffer_Destroy()
{
	DepthBufferList & dbList = depthBufferList();
	while (dbList.bottom)
		DepthBuffer_RemoveBottom();

	dbList.top = dbList.bottom = dbList.current = NULL;
}

void DepthBuffer_SetBuffer( u32 address )
{
	DepthBufferList & dbList = depthBufferList();
	FrameBuffer * pFrameBuffer = frameBufferList().findBuffer(address);
	if (pFrameBuffer == NULL)
		pFrameBuffer = frameBufferList().getCurrent();

	DepthBuffer *current = dbList.top;

	// Search through saved depth buffers
	while (current != NULL)
	{
		if (current->address == address)
		{
			if (pFrameBuffer != NULL && current->width != pFrameBuffer->m_width) {
				DepthBuffer_Remove( current );
				current = NULL;
				break;
			}
			DepthBuffer_MoveToTop( current );
			break;
		}
		current = current->lower;
	}

	if (current == NULL) {
		current = DepthBuffer_AddTop();

		current->address = address;
		current->width = pFrameBuffer != NULL ? pFrameBuffer->m_width : VI.width;
		current->depth_texture = NULL;
		if (config.frameBufferEmulation.enable) {
			glGenRenderbuffers(1, &current->renderbuf);
			glBindRenderbuffer(GL_RENDERBUFFER, current->renderbuf);
#ifndef GLES2
			const GLenum format = GL_DEPTH_COMPONENT;
#else
			const GLenum format = GL_DEPTH_COMPONENT24_OES;
#endif
			if (pFrameBuffer != NULL)
				glRenderbufferStorage(GL_RENDERBUFFER, format, pFrameBuffer->m_pTexture->realWidth, pFrameBuffer->m_pTexture->realHeight);
			else
				glRenderbufferStorage(GL_RENDERBUFFER, format, (u32)pow2(OGL.width), (u32)pow2(OGL.height));
		}
	}

	if (config.frameBufferEmulation.enable) {
		frameBufferList().attachDepthBuffer();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "DepthBuffer_SetBuffer( 0x%08X ); color buffer is 0x%08X\n",
			address, ( pFrameBuffer != NULL &&  pFrameBuffer->m_FBO > 0) ?  pFrameBuffer->m_startAddress : 0
		);
#endif

	}
	dbList.current = current;
}

DepthBuffer *DepthBuffer_FindBuffer( u32 address )
{
	DepthBufferList & dbList = depthBufferList();
	DepthBuffer *current = dbList.top;

	while (current)
	{
		if (current->address == address)
			return current;
		current = current->lower;
	}

	return NULL;
}

void DepthBuffer_ClearBuffer() {
#ifndef GLES2
	if (!OGL.bImageTexture)
		return;
	DepthBuffer *current = depthBufferList().top;
	if (current == NULL || current->fbo == 0)
		return;
	float color[4] = {1.0f, 1.0f, 0.0f, 1.0f};
	glBindImageTexture(depthImageUnit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, current->fbo);
	OGL_DrawRect(0,0,VI.width, VI.height, color);
	glBindImageTexture(depthImageUnit, current->depth_texture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);
#endif // GLES2
}
