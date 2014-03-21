#ifndef VI_H
#define VI_H
#include "Types.h"

struct VIInfo
{
	u32 width, height, real_height;
	f32 rwidth, rheight;
	u32 vStart, vEnd, vHeight;
	u32 lastOrigin;

	VIInfo() : width(0), height(0), real_height(0), vStart(0), vEnd(0), vHeight(0), lastOrigin(0) {}
};

extern VIInfo VI;

void VI_UpdateSize();
void VI_UpdateScreen();

#endif

