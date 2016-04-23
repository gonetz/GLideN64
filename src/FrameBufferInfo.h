#ifndef _FRAME_BUFFER_INFO_H_
#define _FRAME_BUFFER_INFO_H_

#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS

#include "Types.h"
#include "PluginAPI.h"

struct FrameBuffer;

namespace FBInfo {

	struct FrameBufferInfo
	{
		unsigned int addr;
		unsigned int size;
		unsigned int width;
		unsigned int height;
	};

	struct FrameBufferModifyEntry
	{
		unsigned int addr;
		unsigned int val;
		unsigned int size;
	};

	class FBInfo {
	public:
		FBInfo();

		void Write(u32 addr, u32 size);

		void WriteList(FrameBufferModifyEntry *plist, u32 size);

		void Read(u32 addr);

		void GetInfo(void *pinfo);

		bool isSupported() const { return m_supported; }

		void reset();

	private:
		const FrameBuffer * m_pWriteBuffer;
		const FrameBuffer * m_pReadBuffer;
		bool m_supported;
	};

	extern FBInfo fbInfo;
}

#endif // _FRAME_BUFFER_INFO_H_
