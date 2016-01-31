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

struct FrameBuffer;
class FBInfo {
public:
	void Write(u32 addr, u32 size);

	void WriteList(FrameBufferModifyEntry *plist, u32 size);

	void Read(u32 addr);

	void GetInfo(void *pinfo);

private:
	const FrameBuffer * m_pWriteBuffer;
};

extern FBInfo fbInfo;

#endif // _FRAME_BUFFER_INFO_H_
