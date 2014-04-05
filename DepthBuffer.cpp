#include <malloc.h>
#include <assert.h>
#include "OpenGL.h"
#include "Combiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "VI.h"
#include "Config.h"
#include "Debug.h"

DepthBufferInfo depthBuffer;
const GLuint ZlutImageUnit = 0;
const GLuint TlutImageUnit = 1;
const GLuint depthImageUnit = 2;

bool g_bN64DepthCompare = false;

	void DepthBuffer_Init()
{
	depthBuffer.current = NULL;
	depthBuffer.top = NULL;
	depthBuffer.bottom = NULL;
	depthBuffer.numBuffers = 0;
}

void DepthBuffer_RemoveBottom()
{
	DepthBuffer *newBottom = depthBuffer.bottom->higher;

	if (depthBuffer.bottom == depthBuffer.top)
		depthBuffer.top = NULL;

	if (depthBuffer.bottom->renderbuf != 0)
		glDeleteRenderbuffers(1, &depthBuffer.bottom->renderbuf);
	if (depthBuffer.bottom->depth_texture != NULL)
		TextureCache_Remove( depthBuffer.bottom->depth_texture );
	free( depthBuffer.bottom );

    depthBuffer.bottom = newBottom;
	
	if (depthBuffer.bottom != NULL)
		depthBuffer.bottom->lower = NULL;

	depthBuffer.numBuffers--;
}

void DepthBuffer_Remove( DepthBuffer *buffer )
{
	if ((buffer == depthBuffer.bottom) &&
		(buffer == depthBuffer.top))
	{
		depthBuffer.top = NULL;
		depthBuffer.bottom = NULL;
	}
	else if (buffer == depthBuffer.bottom)
	{
		depthBuffer.bottom = buffer->higher;

		if (depthBuffer.bottom)
			depthBuffer.bottom->lower = NULL;
	}
	else if (buffer == depthBuffer.top)
	{
		depthBuffer.top = buffer->lower;

		if (depthBuffer.top)
			depthBuffer.top->higher = NULL;
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
		TextureCache_Remove( buffer->depth_texture );
	free( buffer );

	depthBuffer.numBuffers--;
}

void DepthBuffer_RemoveBuffer( u32 address )
{
	DepthBuffer *current = depthBuffer.bottom;

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
	DepthBuffer *newtop = (DepthBuffer*)malloc( sizeof( DepthBuffer ) );

	newtop->lower = depthBuffer.top;
	newtop->higher = NULL;
	newtop->renderbuf = 0;
	newtop->fbo = 0;

	if (depthBuffer.top)
		depthBuffer.top->higher = newtop;

	if (!depthBuffer.bottom)
		depthBuffer.bottom = newtop;

    depthBuffer.top = newtop;

	depthBuffer.numBuffers++;

	return newtop;
}

void DepthBuffer_MoveToTop( DepthBuffer *newtop )
{
	if (newtop == depthBuffer.top)
		return;

	if (newtop == depthBuffer.bottom)
	{
		depthBuffer.bottom = newtop->higher;
		depthBuffer.bottom->lower = NULL;
	}
	else
	{
		newtop->higher->lower = newtop->lower;
		newtop->lower->higher = newtop->higher;
	}

	newtop->higher = NULL;
	newtop->lower = depthBuffer.top;
	depthBuffer.top->higher = newtop;
	depthBuffer.top = newtop;
}

void DepthBuffer_Destroy()
{
	while (depthBuffer.bottom)
		DepthBuffer_RemoveBottom();

	depthBuffer.top = NULL;
}

void DepthBuffer_SetBuffer( u32 address )
{
	FrameBuffer * pFrameBuffer = FrameBuffer_FindBuffer(address);
	if (pFrameBuffer == NULL)
		pFrameBuffer = frameBuffer.top;

	DepthBuffer *current = depthBuffer.top;

	// Search through saved depth buffers
	while (current != NULL)
	{
		if (current->address == address)
		{
			if (pFrameBuffer != NULL && current->width != pFrameBuffer->width) {
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
		current->width = pFrameBuffer != NULL ? pFrameBuffer->width : VI.width;
		current->depth_texture = NULL;
		if (config.frameBufferEmulation) {
			glGenRenderbuffers(1, &current->renderbuf);
			glBindRenderbuffer(GL_RENDERBUFFER, current->renderbuf);
			if (pFrameBuffer != NULL)
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, pFrameBuffer->texture->realWidth, pFrameBuffer->texture->realHeight);
			else
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, (u32)pow2(OGL.width), (u32)pow2(OGL.height));
		}
	}

	if (config.frameBufferEmulation) {
		FrameBuffer_AttachDepthBuffer();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "DepthBuffer_SetBuffer( 0x%08X ); color buffer is 0x%08X\n",
			address, ( frameBuffer.top != NULL &&  frameBuffer.top->fbo > 0) ?  frameBuffer.top->startAddress : 0
		);
#endif

	}
	depthBuffer.current = current;
}

DepthBuffer *DepthBuffer_FindBuffer( u32 address )
{
	DepthBuffer *current = depthBuffer.top;

	while (current)
	{
		if (current->address == address)
			return current;
		current = current->lower;
	}

	return NULL;
}

void DepthBuffer_ClearBuffer() {
	if (!OGL.bImageTexture)
		return;
	DepthBuffer *current = depthBuffer.top;
	if (current == NULL || current->fbo == 0)
		return;
	float color[4] = {1.0f, 1.0f, 0.0f, 0.0f};
	glBindImageTexture(depthImageUnit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, current->fbo);
	OGL_DrawRect(0,0,VI.width, VI.height, color);
	glBindImageTexture(depthImageUnit, current->depth_texture->glName, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer.top->fbo);
}
