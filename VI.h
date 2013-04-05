#ifndef VI_H
#define VI_H
#include "Types.h"

struct VIInfo
{
	u32 width, height;
	u32 lastOrigin;
};

extern VIInfo VI;

void VI_UpdateSize();
void VI_UpdateScreen();

#endif

