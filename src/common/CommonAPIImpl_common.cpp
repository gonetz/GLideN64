#ifdef OS_WINDOWS
# include <windows.h>
#else
# include <winlnxdefs.h>
#endif // OS_WINDOWS
#include <assert.h>

#include <PluginAPI.h>

#include <N64.h>
#include <GLideN64.h>
#include <RSP.h>
#include <RDP.h>
#include <VI.h>
#include <Config.h>
#include <FrameBufferInfo.h>
#include <TextureFilterHandler.h>
#include <Log.h>
#include "Graphics/Context.h"
#include <DisplayWindow.h>

PluginAPI & PluginAPI::get()
{
	static PluginAPI api;
	return api;
}

void PluginAPI::ProcessDList()
{
	LOG(LOG_APIFUNC, "ProcessDList\n");
#ifdef RSPTHREAD
	m_executor.sync(RSP_ProcessDList);
#else
	RSP_ProcessDList();
#endif
}

void PluginAPI::ProcessRDPList()
{
	LOG(LOG_APIFUNC, "ProcessRDPList\n");
#ifdef RSPTHREAD
	m_executor.sync(RDP_ProcessRDPList);
#else
	RDP_ProcessRDPList();
#endif
}

void PluginAPI::RomClosed()
{
	LOG(LOG_APIFUNC, "RomClosed\n");
	m_bRomOpen = false;
#ifdef RSPTHREAD
	std::lock_guard<std::mutex> lck(m_initMutex);
#if WIN32
	bool main = GetCurrentThreadId() == hWndThread;
	bool running = m_executor.stopAsync([&]()
	{
		TFH.dumpcache();
		dwnd().stop();
		GBI.destroy();
		if (main)
		{
			PostThreadMessage(hWndThread, WM_APP + 1, 0, 0);
		}
	});

	if (main && running)
	{
		MSG msg;
		while (GetMessage(&msg, 0, 0, 0))
		{
			if (msg.message == WM_APP + 1)
				break;

			DispatchMessage(&msg);
		}
	}

	if (running)
	{
		m_executor.stopWait();
	}
#else
	bool running = m_executor.disableTasksAndAsync([&]()
	 {
		TFH.dumpcache();
		dwnd().stop();
		GBI.destroy();
	 });
	 if (running)
	 {
		 m_executor.stop();
	 }
#endif
#else
	TFH.dumpcache();
	dwnd().stop();
	GBI.destroy();
#endif
}

void PluginAPI::RomOpen()
{
	LOG(LOG_APIFUNC, "RomOpen\n");
#ifdef RSPTHREAD
	std::lock_guard<std::mutex> lck(m_initMutex);
	m_executor.start(false /*allowSameThreadExec*/);
	m_executor.async([]()
	{
		RSP_Init();
		GBI.init();
		Config_LoadConfig();
		dwnd().start();
	});
#else
	RSP_Init();
	GBI.init();
	Config_LoadConfig();
	dwnd().start();
#endif
	m_bRomOpen = true;
}

void PluginAPI::Restart()
{
#ifdef RSPTHREAD
	std::lock_guard<std::mutex> lck(m_initMutex);
	if (m_bRomOpen)
	{
		m_executor.async([]() {
			// RomClosed
			TFH.dumpcache();
			dwnd().stop();
			GBI.destroy();

			// RomOpen
			RSP_Init();
			GBI.init();
			Config_LoadConfig();
			dwnd().start();
		});
	}
#else
	Config_LoadConfig();
	dwnd().restart();
#endif
}

void PluginAPI::ShowCFB()
{
	gDP.changed |= CHANGED_CPU_FB_WRITE;
}

void PluginAPI::UpdateScreen()
{
	LOG(LOG_APIFUNC, "UpdateScreen\n");
#ifdef RSPTHREAD
	m_executor.async(VI_UpdateScreen);
#else
	VI_UpdateScreen();
#endif
}

void PluginAPI::_initiateGFX(const GFX_INFO & _gfxInfo) const {
	HEADER = _gfxInfo.HEADER;
	DMEM = _gfxInfo.DMEM;
	IMEM = _gfxInfo.IMEM;
	RDRAM = _gfxInfo.RDRAM;

	REG.MI_INTR = _gfxInfo.MI_INTR_REG;
	REG.DPC_START = _gfxInfo.DPC_START_REG;
	REG.DPC_END = _gfxInfo.DPC_END_REG;
	REG.DPC_CURRENT = _gfxInfo.DPC_CURRENT_REG;
	REG.DPC_STATUS = _gfxInfo.DPC_STATUS_REG;
	REG.DPC_CLOCK = _gfxInfo.DPC_CLOCK_REG;
	REG.DPC_BUFBUSY = _gfxInfo.DPC_BUFBUSY_REG;
	REG.DPC_PIPEBUSY = _gfxInfo.DPC_PIPEBUSY_REG;
	REG.DPC_TMEM = _gfxInfo.DPC_TMEM_REG;

	REG.VI_STATUS = _gfxInfo.VI_STATUS_REG;
	REG.VI_ORIGIN = _gfxInfo.VI_ORIGIN_REG;
	REG.VI_WIDTH = _gfxInfo.VI_WIDTH_REG;
	REG.VI_INTR = _gfxInfo.VI_INTR_REG;
	REG.VI_V_CURRENT_LINE = _gfxInfo.VI_V_CURRENT_LINE_REG;
	REG.VI_TIMING = _gfxInfo.VI_TIMING_REG;
	REG.VI_V_SYNC = _gfxInfo.VI_V_SYNC_REG;
	REG.VI_H_SYNC = _gfxInfo.VI_H_SYNC_REG;
	REG.VI_LEAP = _gfxInfo.VI_LEAP_REG;
	REG.VI_H_START = _gfxInfo.VI_H_START_REG;
	REG.VI_V_START = _gfxInfo.VI_V_START_REG;
	REG.VI_V_BURST = _gfxInfo.VI_V_BURST_REG;
	REG.VI_X_SCALE = _gfxInfo.VI_X_SCALE_REG;
	REG.VI_Y_SCALE = _gfxInfo.VI_Y_SCALE_REG;

	CheckInterrupts = _gfxInfo.CheckInterrupts;

	REG.SP_STATUS = nullptr;
}

void PluginAPI::ChangeWindow()
{
	LOG(LOG_APIFUNC, "ChangeWindow\n");
	dwnd().setToggleFullscreen();
	if (!m_bRomOpen)
		dwnd().closeWindow();
}

void PluginAPI::FBWrite(unsigned int _addr, unsigned int _size)
{
#ifdef RSPTHREAD
	m_executor.sync([=]()
	{
		FBInfo::fbInfo.Write(_addr, _size);
	});
#else
	FBInfo::fbInfo.Write(_addr, _size);
#endif
}

void PluginAPI::FBRead(unsigned int _addr)
{
#ifdef RSPTHREAD
	m_executor.sync([=]()
	{
		FBInfo::fbInfo.Read(_addr);
	});
#else
	FBInfo::fbInfo.Read(_addr);
#endif
}

void PluginAPI::FBGetFrameBufferInfo(void * _pinfo)
{
#ifdef RSPTHREAD
	m_executor.sync([=]()
	{
		FBInfo::fbInfo.GetInfo(_pinfo);
	});
#else
	FBInfo::fbInfo.GetInfo(_pinfo);
#endif
}

#ifndef MUPENPLUSAPI
void PluginAPI::FBWList(FrameBufferModifyEntry * _plist, unsigned int _size)
{
	FBInfo::fbInfo.WriteList(reinterpret_cast<FBInfo::FrameBufferModifyEntry*>(_plist), _size);
}

void PluginAPI::ReadScreen(void **_dest, long *_width, long *_height)
{
#ifdef RSPTHREAD
	m_executor.sync([=]()
	{
		dwnd().readScreen(_dest, _width, _height);
	});
#else
	dwnd().readScreen(_dest, _width, _height);
#endif
}
#endif
