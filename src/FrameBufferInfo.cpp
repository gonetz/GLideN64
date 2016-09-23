#include "FrameBufferInfoAPI.h"
#include "FrameBufferInfo.h"
#include "Config.h"
#include "OpenGL.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "RSP.h"
#include "VI.h"
#include "Log.h"

namespace FBInfo {

	FBInfo fbInfo;

	FBInfo::FBInfo()
		: m_pWriteBuffer(nullptr)
		, m_pReadBuffer(nullptr)
		, m_supported(false)
	{}

	void FBInfo::reset() {
		m_supported = false;
		m_pWriteBuffer = m_pReadBuffer = nullptr;
	}

	void FBInfo::Write(u32 addr, u32 size)
	{
		// TODO: remove debug print
		//debugPrint("FBWrite addr=%08lx size=%u\n", addr, size);

		const u32 address = RSP_SegmentToPhysical(addr);
		if (m_pWriteBuffer == nullptr)
			m_pWriteBuffer = frameBufferList().findBuffer(address);
		FrameBuffer_AddAddress(address, size);
	}

	void FBInfo::WriteList(FrameBufferModifyEntry *plist, u32 size)
	{
		debugPrint("FBWList size=%u\n", size);
		for (u32 i = 0; i < size; ++i)
			debugPrint(" plist[%u] addr=%08lx val=%08lx size=%u\n", i, plist[i].addr, plist[i].val, plist[i].size);
		const u32 address = RSP_SegmentToPhysical(plist[0].addr);
		m_pWriteBuffer = frameBufferList().findBuffer(address);
	}

	void FBInfo::Read(u32 addr)
	{
		// TODO: remove debug print
		//debugPrint("FBRead addr=%08lx \n", addr);

		const u32 address = RSP_SegmentToPhysical(addr);
		FrameBuffer * pBuffer = frameBufferList().findBuffer(address);
		if (pBuffer == nullptr || pBuffer == m_pWriteBuffer)
			return;

		if (pBuffer->m_isDepthBuffer) {
			if (config.frameBufferEmulation.fbInfoReadDepthChunk != 0)
				FrameBuffer_CopyDepthBufferChunk(address);
			else if (pBuffer != m_pReadBuffer)
				FrameBuffer_CopyDepthBuffer(address);
		} else {
			if (config.frameBufferEmulation.fbInfoReadColorChunk != 0)
				FrameBuffer_CopyChunkToRDRAM(address);
			else if (pBuffer != m_pReadBuffer)
				FrameBuffer_CopyToRDRAM(address, true);
		}

		m_pReadBuffer = pBuffer;
	}

	void FBInfo::GetInfo(void *pinfo)
	{
		//	debugPrint("FBGetInfo\n");
		FrameBufferInfo * pFBInfo = (FrameBufferInfo*)pinfo;
		memset(pFBInfo, 0, sizeof(FrameBufferInfo)* 6);

		if (config.frameBufferEmulation.fbInfoDisabled != 0)
			return;

		u32 idx = 0;
		DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
		if (pDepthBuffer != nullptr) {
			pFBInfo[idx].addr = pDepthBuffer->m_address;
			pFBInfo[idx].width = pDepthBuffer->m_width;
			pFBInfo[idx].height = VI.real_height;
			pFBInfo[idx++].size = 2;
		}
		frameBufferList().fillBufferInfo(&pFBInfo[idx], 6 - idx);

		m_pWriteBuffer = m_pReadBuffer = nullptr;
		m_supported = true;
	}
}
