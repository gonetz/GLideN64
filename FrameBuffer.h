#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Types.h"
#include "Textures.h"

struct FrameBuffer
{
	FrameBuffer *higher, *lower;

	CachedTexture *texture;

	u32 startAddress, endAddress;
	u32 size, width, height, changed;
	float scaleX, scaleY;
};

struct FrameBufferInfo
{
	FrameBuffer *top, *bottom, *current;
	int numBuffers;
};

extern FrameBufferInfo frameBuffer;

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_SaveBuffer( u32 address, u16 size, u16 width, u16 height );
void FrameBuffer_RenderBuffer( u32 address );
void FrameBuffer_RestoreBuffer( u32 address, u16 size, u16 width );
void FrameBuffer_RemoveBuffer( u32 address );
FrameBuffer *FrameBuffer_FindBuffer( u32 address );
void FrameBuffer_ActivateBufferTexture( s16 t, FrameBuffer *buffer );

#endif
