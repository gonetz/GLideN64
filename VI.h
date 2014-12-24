#ifndef VI_H
#define VI_H
#include "Types.h"

struct VIInfo
{
	u32 width;
	u32 height, real_height;
	f32 rwidth, rheight;
	u32 lastOrigin;
	bool interlaced;

	VIInfo() :
		width(0), height(0), real_height(0),
		lastOrigin(-1), interlaced(false)
	{}
};

extern VIInfo VI;

void VI_UpdateSize();
void VI_UpdateScreen();

#endif

