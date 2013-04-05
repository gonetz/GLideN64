#ifndef _2XSAI_H
#define _2XSAI_H
#include "Types.h"

void _2xSaI8888( u32 *srcPtr, u32 *destPtr, u16 width, u16 height, s32 clampS, s32 clampT );
void _2xSaI4444( u16 *srcPtr, u16 *destPtr, u16 width, u16 height, s32 clampS, s32 clampT );
void _2xSaI5551( u16 *srcPtr, u16 *destPtr, u16 width, u16 height, s32 clampS, s32 clampT );
#endif

