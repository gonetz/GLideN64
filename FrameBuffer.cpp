#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // __LINUX__
#include <assert.h>
#include "OpenGL.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "RSP.h"
#include "RDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "Types.h"
#include "Debug.h"

FrameBufferInfo frameBuffer;

void FrameBuffer_Init()
{
	frameBuffer.current = NULL;
	frameBuffer.top = NULL;
	frameBuffer.bottom = NULL;
	frameBuffer.numBuffers = 0;
	frameBuffer.drawBuffer = GL_BACK;
}

void FrameBuffer_RemoveBottom()
{
	FrameBuffer *newBottom = frameBuffer.bottom->higher;

	TextureCache_Remove( frameBuffer.bottom->texture );
	if (frameBuffer.bottom->fbo != 0)
		ogl_glDeleteFramebuffers(1, &frameBuffer.bottom->fbo);

	if (frameBuffer.bottom == frameBuffer.top)
		frameBuffer.top = NULL;

	free( frameBuffer.bottom );

    frameBuffer.bottom = newBottom;
	
	if (frameBuffer.bottom != NULL)
		frameBuffer.bottom->lower = NULL;

	frameBuffer.numBuffers--;
}

void FrameBuffer_Remove( FrameBuffer *buffer )
{
	if ((buffer == frameBuffer.bottom) &&
		(buffer == frameBuffer.top))
	{
		frameBuffer.top = NULL;
		frameBuffer.bottom = NULL;
	}
	else if (buffer == frameBuffer.bottom)
	{
		frameBuffer.bottom = buffer->higher;

		if (frameBuffer.bottom)
			frameBuffer.bottom->lower = NULL;
	}
	else if (buffer == frameBuffer.top)
	{
		frameBuffer.top = buffer->lower;

		if (frameBuffer.top)
			frameBuffer.top->higher = NULL;
	}
	else
	{
		buffer->higher->lower = buffer->lower;
		buffer->lower->higher = buffer->higher;
	}

	if (buffer->texture)
		TextureCache_Remove( buffer->texture );
	if (buffer->fbo != 0)
		ogl_glDeleteFramebuffers(1, &buffer->fbo);

	free( buffer );

	frameBuffer.numBuffers--;
}

void FrameBuffer_RemoveBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.bottom;

	while (current != NULL)
	{
		if (current->startAddress == address)
		{
			current->texture = NULL;
			FrameBuffer_Remove( current );
			return;
		}
		current = current->higher;
	}
}

FrameBuffer *FrameBuffer_AddTop()
{
	FrameBuffer *newtop = (FrameBuffer*)malloc( sizeof( FrameBuffer ) );

	newtop->texture = TextureCache_AddTop();
	newtop->fbo = 0;

	newtop->lower = frameBuffer.top;
	newtop->higher = NULL;

	if (frameBuffer.top)
		frameBuffer.top->higher = newtop;

	if (!frameBuffer.bottom)
		frameBuffer.bottom = newtop;

    frameBuffer.top = newtop;

	frameBuffer.numBuffers++;

	return newtop;
}

void FrameBuffer_MoveToTop( FrameBuffer *newtop )
{
	if (newtop == frameBuffer.top)
		return;

	if (newtop == frameBuffer.bottom)
	{
		frameBuffer.bottom = newtop->higher;
		frameBuffer.bottom->lower = NULL;
	}
	else
	{
		newtop->higher->lower = newtop->lower;
		newtop->lower->higher = newtop->higher;
	}

	newtop->higher = NULL;
	newtop->lower = frameBuffer.top;
	frameBuffer.top->higher = newtop;
	frameBuffer.top = newtop;

	TextureCache_MoveToTop( newtop->texture );
}

void FrameBuffer_Destroy()
{
	while (frameBuffer.bottom)
		FrameBuffer_RemoveBottom();
}

void FrameBuffer_SaveBuffer( u32 address, u16 format, u16 size, u16 width, u16 height )
{
	frameBuffer.drawBuffer = GL_DRAW_FRAMEBUFFER;

	FrameBuffer *current = frameBuffer.top;
	// Search through saved frame buffers
	while (current != NULL)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
		{
			if ((current->startAddress != address) ||
				(current->width != width) ||
				//(current->height != height) ||
				(current->size != size) ||  // TODO FIX ME
				(current->scaleX != OGL.scaleX) ||
				(current->scaleY != OGL.scaleY))
			{
				FrameBuffer_Remove( current );
				current = NULL;
				break;
			}

			FrameBuffer_MoveToTop( current );
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, current->fbo);
			break;
		}
		current = current->lower;
	}

	if (current == NULL) {
		// Wasn't found, create a new one
		current = FrameBuffer_AddTop();

		current->startAddress = address;
		current->endAddress = address + ((width * height << size >> 1) - 1);
		current->width = width;
		current->height = height;
		current->size = size;
		current->scaleX = OGL.scaleX;
		current->scaleY = OGL.scaleY;

		current->texture->width = (u32)(current->width * OGL.scaleX);
		current->texture->height = (u32)(current->height * OGL.scaleY);
		current->texture->format = format;
		current->texture->size = size;
		current->texture->clampS = 1;
		current->texture->clampT = 1;
		current->texture->address = current->startAddress;
		current->texture->clampWidth = current->width;
		current->texture->clampHeight = current->height;
		current->texture->frameBufferTexture = TRUE;
		current->texture->maskS = 0;
		current->texture->maskT = 0;
		current->texture->mirrorS = 0;
		current->texture->mirrorT = 0;
		current->texture->realWidth = (u32)pow2( current->texture->width );
		current->texture->realHeight = (u32)pow2( current->texture->height );
		current->texture->textureBytes = current->texture->realWidth * current->texture->realHeight * 4;
		cache.cachedBytes += current->texture->textureBytes;

		glBindTexture( GL_TEXTURE_2D, current->texture->glName );
		if (size > G_IM_SIZ_8b)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, current->texture->realWidth, current->texture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, current->texture->realWidth, current->texture->realHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		ogl_glGenFramebuffers(1, &current->fbo);
		ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, current->fbo);
		ogl_glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, current->texture->glName, 0);
	}

	if (depthBuffer.top != NULL && depthBuffer.top->renderbuf > 0) {
		ogl_glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer.top->renderbuf);
		ogl_glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer.top->renderbuf);
	}

	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
	ogl_glDrawBuffers(2,  attachments, current->texture->glName);
	assert(checkFBO());

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "FrameBuffer_SaveBuffer( 0x%08X ); depth buffer is 0x%08X\n",
		address, (depthBuffer.top != NULL && depthBuffer.top->renderbuf > 0) ? depthBuffer.top->address : 0
	);
#endif
	*(u32*)&RDRAM[current->startAddress] = current->startAddress;

	current->changed = TRUE;

	gSP.changed |= CHANGED_TEXTURE;
}


#if 1
void FrameBuffer_RenderBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.top;

	while (current != NULL)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
		{
			ogl_glBindFramebuffer(GL_READ_FRAMEBUFFER, current->fbo);
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glDrawBuffer( GL_FRONT );
			ogl_glBlitFramebuffer(
//				0, 0, current->texture->realWidth, current->texture->realHeight,
				0, 0, OGL.width, OGL.height,
				0, OGL.heightOffset, OGL.width, OGL.height+OGL.heightOffset,
				GL_COLOR_BUFFER_BIT, GL_LINEAR
			);
			glDrawBuffer( GL_BACK );
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer.top->fbo);
			/*
			current = current->lower;
			while (current != NULL)
			{
				if ((current->startAddress <= address) &&
					(current->endAddress >= address))
				assert(false);
				current = current->lower;
			}
			*/
			return;
		}
		current = current->lower;
	}
}
#else

void FrameBuffer_RenderBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.top;

	while (current != NULL)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
		{
			/*
			float fill_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, current->fbo);
			ogl_glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer.top->renderbuf);
			ogl_glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer.top->renderbuf);
			GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
			ogl_glDrawBuffers(2,  attachments, current->texture->glName);
			assert(checkFBO());
			OGL_ClearDepthBuffer();
			OGL_ClearColorBuffer(fill_color);
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer.top->fbo);
			*/

			glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

			Combiner_BeginTextureUpdate();
			TextureCache_ActivateTexture( 0, current->texture );
			Combiner_SetCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1 ) );
/*			if (OGL.ARB_multitexture)
			{
				for (int i = 0; i < OGL.maxTextureUnits; i++)
				{
					glActiveTextureARB( GL_TEXTURE0_ARB + i );
					glDisable( GL_TEXTURE_2D );
				}

				glActiveTextureARB( GL_TEXTURE0_ARB );
			}

			TextureCache_ActivateTexture( 0, current->texture );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
			glEnable( GL_TEXTURE_2D );*/

			glDisable( GL_BLEND );
			glDisable( GL_ALPHA_TEST );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_CULL_FACE );
			glDisable( GL_POLYGON_OFFSET_FILL );
//			glDisable( GL_REGISTER_COMBINERS_NV );
			glDisable( GL_FOG );

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
 			glOrtho( 0, OGL.width, 0, OGL.height, -1.0f, 1.0f );
			glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );
			glDisable( GL_SCISSOR_TEST );

			float u1, v1;

			u1 = (float)current->texture->width / (float)current->texture->realWidth;
			v1 = (float)current->texture->height / (float)current->texture->realHeight;

			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glDrawBuffer( GL_FRONT );
			glBegin(GL_QUADS);
 				glTexCoord2f( 0.0f, 0.0f );
				glVertex2f( 0.0f, (GLfloat)(OGL.height - current->texture->height) );

				glTexCoord2f( 0.0f, v1 );
				glVertex2f( 0.0f, (GLfloat)OGL.height );

 				glTexCoord2f( u1,  v1 );
				glVertex2f( current->texture->width, (GLfloat)OGL.height );

 				glTexCoord2f( u1, 0.0f );
				glVertex2f( current->texture->width, (GLfloat)(OGL.height - current->texture->height) );
			glEnd();
			glDrawBuffer( GL_BACK );
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer.top->fbo);
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "FrameBuffer_RenderBuffer( 0x%08X ); \n", address);
#endif

/*			glEnable( GL_TEXTURE_2D );
			glActiveTextureARB( GL_TEXTURE0_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );*/
			glLoadIdentity();
			glPopAttrib();

			current->changed = FALSE;

			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
			gDP.changed |= CHANGED_COMBINE;

			return;
		}
		current = current->lower;
	}
}
#endif

void FrameBuffer_RestoreBuffer( u32 address, u16 size, u16 width )
{
	FrameBuffer *current = frameBuffer.top;

	while (current != NULL)
	{
		if ((current->startAddress == address) &&
			(current->width == width) &&
			(current->size == size))
		{
			glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

/*			if (OGL.ARB_multitexture)
			{
				for (int i = 0; i < OGL.maxTextureUnits; i++)
				{
					glActiveTextureARB( GL_TEXTURE0_ARB + i );
					glDisable( GL_TEXTURE_2D );
				}

				glActiveTextureARB( GL_TEXTURE0_ARB );
			}

			TextureCache_ActivateTexture( 0, current->texture );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
			glEnable( GL_TEXTURE_2D );*/
			Combiner_BeginTextureUpdate();
			TextureCache_ActivateTexture( 0, current->texture );
			Combiner_SetCombine( EncodeCombineMode( 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1 ) );
			
			glDisable( GL_BLEND );
			glDisable( GL_ALPHA_TEST );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_SCISSOR_TEST );
			glDisable( GL_CULL_FACE );
			glDisable( GL_POLYGON_OFFSET_FILL );
			//glDisable( GL_REGISTER_COMBINERS_NV );
			glDisable( GL_FOG );
//			glDepthMask( FALSE );

			glMatrixMode( GL_PROJECTION );
			glLoadIdentity();
 			glOrtho( 0, OGL.width, 0, OGL.height, -1.0f, 1.0f );
//			glOrtho( 0, RDP.width, RDP.height, 0, -1.0f, 1.0f );
			glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );

			float u1, v1;

			u1 = (float)current->texture->width / (float)current->texture->realWidth;
			v1 = (float)current->texture->height / (float)current->texture->realHeight;

			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBegin(GL_QUADS); 
 				glTexCoord2f( 0.0f, 0.0f );
				glVertex2f( 0.0f, OGL.height - current->texture->height );

				glTexCoord2f( 0.0f, v1 );
				glVertex2f( 0.0f, OGL.height );

 				glTexCoord2f( u1,  v1 );
				glVertex2f( current->texture->width, OGL.height );

 				glTexCoord2f( u1, 0.0f );
				glVertex2f( current->texture->width, OGL.height - current->texture->height );
			glEnd();
			ogl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer.top->fbo);
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "FrameBuffer_RestoreBuffer( 0x%08X ); \n", address);
#endif

			glLoadIdentity();
			glPopAttrib();

			FrameBuffer_MoveToTop( current );

			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
			gDP.changed |= CHANGED_COMBINE;
			return;
		}
		current = current->lower;
	}
}

FrameBuffer *FrameBuffer_FindBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.top;

	while (current)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
			return current;
		current = current->lower;
	}

	return NULL;
}

void FrameBuffer_ActivateBufferTexture( s16 t, FrameBuffer *buffer )
{
    buffer->texture->scaleS = OGL.scaleX / (float)buffer->texture->realWidth;
    buffer->texture->scaleT = OGL.scaleY / (float)buffer->texture->realHeight;

	if (gSP.textureTile[t]->shifts > 10)
		buffer->texture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[t]->shifts));
	else if (gSP.textureTile[t]->shifts > 0)
		buffer->texture->shiftScaleS = 1.0f / (float)(1 << gSP.textureTile[t]->shifts);
	else
		buffer->texture->shiftScaleS = 1.0f;

	if (gSP.textureTile[t]->shiftt > 10)
		buffer->texture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[t]->shiftt));
	else if (gSP.textureTile[t]->shiftt > 0)
		buffer->texture->shiftScaleT = 1.0f / (float)(1 << gSP.textureTile[t]->shiftt);
	else
		buffer->texture->shiftScaleT = 1.0f;

	const u32 shift = gSP.textureTile[t]->imageAddress - buffer->startAddress;
	const u32 factor = buffer->width << buffer->size >> 1;
	if (gSP.textureTile[t]->loadType == LOADTYPE_TILE)
	{
		buffer->texture->offsetS = gDP.loadTile->uls;
		buffer->texture->offsetT = (float)(buffer->height - (gDP.loadTile->ult + shift/factor));
	}
	else
	{
		buffer->texture->offsetS = (float)(shift % factor);
		buffer->texture->offsetT = (float)(buffer->height - shift/factor);
	}

//	FrameBuffer_RenderBuffer(buffer->startAddress);
	TextureCache_ActivateTexture( t, buffer->texture );
	gDP.changed |= CHANGED_FB_TEXTURE;
}

void FrameBuffer_ActivateBufferTextureBG( s16 t, FrameBuffer *buffer )
{
	buffer->texture->scaleS = OGL.scaleX / (float)buffer->texture->realWidth;
	buffer->texture->scaleT = OGL.scaleY / (float)buffer->texture->realHeight;

	buffer->texture->shiftScaleS = 1.0f;
	buffer->texture->shiftScaleT = 1.0f;

	buffer->texture->offsetS = gSP.bgImage.imageX;
	buffer->texture->offsetT = (float)buffer->height - gSP.bgImage.imageY;

	//	FrameBuffer_RenderBuffer(buffer->startAddress);
	TextureCache_ActivateTexture( t, buffer->texture );
	gDP.changed |= CHANGED_FB_TEXTURE;
}
