#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Types.h"
#include "Textures.h"
struct gDPTile;

struct FrameBuffer
{
	FrameBuffer *higher, *lower;

	CachedTexture *texture;
	GLuint fbo;

	u32 startAddress, endAddress;
	u32 size, width, height, fillcolor;
	bool cleared;
	gDPTile *loadTile;
	float scaleX, scaleY;
};

struct FrameBufferInfo
{
	FrameBuffer *top, *bottom, *current;
	int numBuffers;
	GLenum drawBuffer;
};

extern FrameBufferInfo frameBuffer;
extern bool g_bCopyToRDRAM;

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_SaveBuffer( u32 address, u16 format, u16 size, u16 width, u16 height );
void FrameBuffer_RenderBuffer( u32 address );
void FrameBuffer_RemoveBuffer( u32 address );
void FrameBuffer_CopyToRDRAM( u32 address );
FrameBuffer *FrameBuffer_FindBuffer( u32 address );
void FrameBuffer_ActivateBufferTexture( s16 t, FrameBuffer *buffer );
void FrameBuffer_ActivateBufferTextureBG( s16 t, FrameBuffer *buffer );

#endif
