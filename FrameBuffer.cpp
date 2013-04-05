#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // __LINUX__
#include "OpenGL.h"
#include "FrameBuffer.h"
#include "RSP.h"
#include "RDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "Types.h"

FrameBufferInfo frameBuffer;

void FrameBuffer_Init()
{
	frameBuffer.current = NULL;
	frameBuffer.top = NULL;
	frameBuffer.bottom = NULL;
	frameBuffer.numBuffers = 0;
}

void FrameBuffer_RemoveBottom()
{
	FrameBuffer *newBottom = frameBuffer.bottom->higher;

	TextureCache_Remove( frameBuffer.bottom->texture );

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

void FrameBuffer_SaveBuffer( u32 address, u16 size, u16 width, u16 height )
{
	FrameBuffer *current = frameBuffer.top;

	// Search through saved frame buffers
	while (current != NULL)
	{
		if ((current->startAddress == address) &&
			(current->width == width) &&
			(current->height == height) &&
			(current->size == size))
		{
			if ((current->scaleX != OGL.scaleX) ||
				(current->scaleY != OGL.scaleY))
			{
				FrameBuffer_Remove( current );
				break;
			}

			glBindTexture( GL_TEXTURE_2D, current->texture->glName );
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, OGL.height - current->texture->height + OGL.heightOffset, current->texture->width, current->texture->height );
			*(u32*)&RDRAM[current->startAddress] = current->startAddress;

			current->changed = TRUE;

			FrameBuffer_MoveToTop( current );

			gSP.changed |= CHANGED_TEXTURE;
			return;
		}
		current = current->lower;
	}

	// Wasn't found, create a new one
	current = FrameBuffer_AddTop();

	current->startAddress = address;
	current->endAddress = address + ((width * height << size >> 1) - 1);
	current->width = width;
	current->height = height;
	current->size = size;
	current->scaleX = OGL.scaleX;
	current->scaleY = OGL.scaleY;

	current->texture->width = current->width * OGL.scaleX;
	current->texture->height = current->height * OGL.scaleY;
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
	current->texture->realWidth = pow2( current->width * OGL.scaleX );
	current->texture->realHeight = pow2( current->height * OGL.scaleY );
	current->texture->textureBytes = current->texture->realWidth * current->texture->realHeight * 4;
	cache.cachedBytes += current->texture->textureBytes;

	glBindTexture( GL_TEXTURE_2D, current->texture->glName );
	glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 0, OGL.height - current->texture->height + OGL.heightOffset, current->texture->realWidth, current->texture->realHeight, 0 );
	*(u32*)&RDRAM[current->startAddress] = current->startAddress;

	current->changed = TRUE;

	gSP.changed |= CHANGED_TEXTURE;
}

void FrameBuffer_RenderBuffer( u32 address )
{
	FrameBuffer *current = frameBuffer.top;

	while (current != NULL)
	{
		if ((current->startAddress <= address) &&
			(current->endAddress >= address))
		{
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

			glDrawBuffer( GL_FRONT );
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
			glDrawBuffer( GL_BACK );

/*			glEnable( GL_TEXTURE_2D );
			glActiveTextureARB( GL_TEXTURE0_ARB );
			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );*/
			glPopAttrib();

			current->changed = FALSE;

			FrameBuffer_MoveToTop( current );

			gSP.changed |= CHANGED_TEXTURE | CHANGED_VIEWPORT;
			gDP.changed |= CHANGED_COMBINE;
			return;
		}
		current = current->lower;
	}
}

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

	if (gDP.loadType == LOADTYPE_TILE)
	{
		buffer->texture->offsetS = gDP.loadTile->uls;
		buffer->texture->offsetT = (float)buffer->height - (gDP.loadTile->ult + (gDP.textureImage.address - buffer->startAddress) / (buffer->width << buffer->size >> 1));
	}
	else
	{
		buffer->texture->offsetS = 0.0f;
		buffer->texture->offsetT = (float)buffer->height - (gDP.textureImage.address - buffer->startAddress) / (buffer->width << buffer->size >> 1);
	}

	FrameBuffer_MoveToTop( buffer );
	TextureCache_ActivateTexture( t, buffer->texture );
}
