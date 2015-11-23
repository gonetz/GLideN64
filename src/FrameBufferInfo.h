#ifndef _FRAME_BUFFER_INFO_H_
#define _FRAME_BUFFER_INFO_H_

#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS

#include "Types.h"
#include "PluginAPI.h"

struct FrameBufferInfo
{
	unsigned int addr;
	unsigned int size;
	unsigned int width;
	unsigned int height;
};

void FrameBufferWrite(u32 addr, u32 size);

void FrameBufferWriteList(FrameBufferModifyEntry *plist, u32 size);

void FrameBufferRead(u32 addr);

void FrameBufferGetInfo(void *pinfo);

#endif // _FRAME_BUFFER_INFO_H_
