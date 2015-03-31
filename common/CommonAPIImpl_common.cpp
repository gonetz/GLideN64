#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "../winlnxdefs.h"
#endif // OS_WINDOWS
#include <assert.h>

#include "../PluginAPI.h"

#include "../N64.h"
#include "../GLideN64.h"
#include "../OpenGL.h"
#include "../RSP.h"
#include "../RDP.h"
#include "../VI.h"
#include "../Config.h"
#include "../Debug.h"
#include "../Log.h"

PluginAPI & PluginAPI::get()
{
	static PluginAPI api;
	return api;
}

#ifdef RSPTHREAD
void RSP_ThreadProc(std::mutex * _pRspThreadMtx, std::mutex * _pPluginThreadMtx, std::condition_variable_any * _pRspThreadCv, std::condition_variable_any * _pPluginThreadCv, API_COMMAND * _pCommand)
{
	_pRspThreadMtx->lock();
	RSP_Init();
	GBI.init();
	Config_LoadConfig();
	video().start();
	TFH.init();
	assert(!isGLError());

	while (true) {
		_pPluginThreadMtx->lock();
		_pPluginThreadCv->notify_one();
		_pPluginThreadMtx->unlock();
		_pRspThreadCv->wait(*_pRspThreadMtx);
		switch (*_pCommand) {
		case acProcessDList:
			RSP_ProcessDList();
			break;
		case acProcessRDPList:
			RDP_ProcessRDPList();
			break;
		case acUpdateScreen:
			VI_UpdateScreen();
			break;
		case acRomClosed:
			TFH.shutdown();
			video().stop();
			GBI.destroy();
			*_pCommand = acNone;
			_pRspThreadMtx->unlock();
			_pPluginThreadMtx->lock();
			_pPluginThreadCv->notify_one();
			_pPluginThreadMtx->unlock();
			return;
		}
		assert(!isGLError());
		*_pCommand = acNone;
	}
}

void PluginAPI::_callAPICommand(API_COMMAND _command)
{
	m_command = _command;
	m_pluginThreadMtx.lock();
	m_rspThreadMtx.lock();
	m_rspThreadCv.notify_one();
	m_rspThreadMtx.unlock();
	m_pluginThreadCv.wait(m_pluginThreadMtx);
	m_pluginThreadMtx.unlock();
}
#endif

void PluginAPI::ProcessDList()
{
	LOG(LOG_APIFUNC, "ProcessDList\n");
#ifdef RSPTHREAD
	_callAPICommand(acProcessDList);
#else
	RSP_ProcessDList();
#endif
}

void PluginAPI::ProcessRDPList()
{
	LOG(LOG_APIFUNC, "ProcessRDPList\n");
#ifdef RSPTHREAD
	_callAPICommand(acProcessRDPList);
#else
	RDP_ProcessRDPList();
#endif
}

void PluginAPI::RomClosed()
{
	LOG(LOG_APIFUNC, "RomClosed\n");
#ifdef RSPTHREAD
	_callAPICommand(acRomClosed);
	delete m_pRspThread;
	m_pRspThread = NULL;
#else
	TFH.shutdown();
	video().stop();
	GBI.destroy();
#endif

#ifdef DEBUG
	CloseDebugDlg();
#endif
}

void PluginAPI::RomOpen()
{
	LOG(LOG_APIFUNC, "RomOpen\n");
#ifdef RSPTHREAD
	m_pluginThreadMtx.lock();
	m_pRspThread = new std::thread(RSP_ThreadProc, &m_rspThreadMtx, &m_pluginThreadMtx, &m_rspThreadCv, &m_pluginThreadCv, &m_command);
	m_pRspThread->detach();
	m_pluginThreadCv.wait(m_pluginThreadMtx);
	m_pluginThreadMtx.unlock();
#else
	RSP_Init();
	GBI.init();
	Config_LoadConfig();
	video().start();
#endif

#ifdef DEBUG
	OpenDebugDlg();
#endif
}

void PluginAPI::ShowCFB()
{
	gSP.changed |= CHANGED_CPU_FB_WRITE;
}

void PluginAPI::UpdateScreen()
{
	LOG(LOG_APIFUNC, "UpdateScreen\n");
#ifdef RSPTHREAD
	_callAPICommand(acUpdateScreen);
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
}

void PluginAPI::ChangeWindow()
{
	video().setToggleFullscreen();
}
