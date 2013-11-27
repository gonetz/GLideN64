#ifndef DEPTHBUFFER_H
#define DEPTHBUFFER_H

#include "Types.h"
#include "Textures.h"

struct DepthBuffer
{
	DepthBuffer *higher, *lower;

	u32 address, width;
	GLuint renderbuf;
	CachedTexture *depth_texture;
	GLuint fbo;
};

struct DepthBufferInfo
{
	DepthBuffer *top, *bottom, *current;
	int numBuffers;
};

extern DepthBufferInfo depthBuffer;

void DepthBuffer_Init();
void DepthBuffer_Destroy();
void DepthBuffer_SetBuffer( u32 address );
void DepthBuffer_RemoveBuffer( u32 address );
//DepthBuffer *DepthBuffer_FindBuffer( u32 address );

#endif
