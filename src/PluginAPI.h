#ifndef COMMONPLUGINAPI_H
#define COMMONPLUGINAPI_H

#ifdef MUPENPLUSAPI
#include "m64p_plugin.h"
#else
#include "windows/GLideN64_windows.h"
#include "ZilmarGFX_1_3.h"
#include "FrameBufferInfoAPI.h"
#define RSPTHREAD
#endif

#ifdef RSPTHREAD
#include "QueueExecutor.h"
#endif

class PluginAPI
{
public:
	// Common
	void MoveScreen(int /*_xpos*/, int /*_ypos*/) {}
	void ViStatusChanged() {}
	void ViWidthChanged() {}

	void ProcessDList();
	void ProcessRDPList();
	void RomClosed();
	void RomOpen();
	void ShowCFB();
	void UpdateScreen();
	int InitiateGFX(const GFX_INFO & _gfxInfo);
	void ChangeWindow();

	void FindPluginPath(wchar_t * _strPath);
	void GetUserDataPath(wchar_t * _strPath);
	void GetUserCachePath(wchar_t * _strPath);
	bool isRomOpen() const { return m_bRomOpen; }

	void Restart();

#ifndef MUPENPLUSAPI
	// Zilmar
	void DllTest(HWND /*_hParent*/) {}
	void DrawScreen() {}
	void CloseDLL(void) {}

	void CaptureScreen(char * _Directory);
	void DllConfig(HWND _hParent);
	void GetDllInfo (PLUGIN_INFO * PluginInfo);
	void ReadScreen(void **_dest, long *_width, long *_height);

	void DllAbout(/*HWND _hParent*/);

	// FrameBufferInfo extension
	void FBWrite(unsigned int addr, unsigned int size);
	void FBWList(FrameBufferModifyEntry *plist, unsigned int size);
	void FBRead(unsigned int addr);
	void FBGetFrameBufferInfo(void *pinfo);
#else
	// MupenPlus
	void ResizeVideoOutput(int _Width, int _Height);
	void ReadScreen2(void * _dest, int * _width, int * _height, int _front);

	m64p_error PluginStartup(m64p_dynlib_handle _CoreLibHandle);
	m64p_error PluginShutdown();
	m64p_error PluginGetVersion(
		m64p_plugin_type * _PluginType,
		int * _PluginVersion,
		int * _APIVersion,
		const char ** _PluginNamePtr,
		int * _Capabilities
	);
	void SetRenderingCallback(void (*callback)(int));

	// FrameBufferInfo extension
	void FBWrite(unsigned int addr, unsigned int size);
	void FBRead(unsigned int addr);
	void FBGetFrameBufferInfo(void *pinfo);
#endif

	static PluginAPI & get();

private:
	PluginAPI()
		: m_bRomOpen(false)
	{}
	PluginAPI(const PluginAPI &) = delete;

	void _initiateGFX(const GFX_INFO & _gfxInfo) const;

	bool m_bRomOpen;
#ifdef RSPTHREAD
	std::mutex m_initMutex;
	QueueExecutor m_executor;
#endif
};

inline PluginAPI & api()
{
	return PluginAPI::get();
}

#endif // COMMONPLUGINAPI_H
