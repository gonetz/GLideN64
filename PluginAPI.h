#ifndef COMMONPLUGINAPI_H
#define COMMONPLUGINAPI_H

#ifdef MUPENPLUSAPI
#include "m64p_plugin.h"
#else
#include "ZilmarGFX_1_3.h"
#endif

#include "GLCriticalSection.h"

class PluginAPI : public GLCriticalSection
{
public:
	// Common
	void MoveScreen(int /*_xpos*/, int /*_ypos*/) {}
	void ProcessRDPList() {}
	void ViStatusChanged() {}
	void ViWidthChanged() {}

	void ProcessDList();
	void RomClosed();
	void RomOpen();
	void ShowCFB();
	void UpdateScreen();
	int InitiateGFX(const GFX_INFO & _gfxInfo);
	void ChangeWindow();

#ifndef MUPENPLUSAPI
	// Zilmar
	void DllTest(HWND /*_hParent*/) {}
	void DrawScreen() {}
	void CloseDLL(void) {}

	void CaptureScreen(char * _Directory);
	void DllConfig(HWND _hParent);
	void GetDllInfo (PLUGIN_INFO * PluginInfo);
	void ReadScreen(void **_dest, long *_width, long *_height);

	void DllAbout(HWND _hParent);
#else
	// MupenPlus
	void ReadScreen2(void * _dest, int * _width, int * _height, int _front) {}
	void FBRead(unsigned int _addr) {}
	void FBWrite(unsigned int addr, unsigned int size) {}
	void FBGetFrameBufferInfo(void * _p) {}
	void ResizeVideoOutput(int _Width, int _Height) {}
	void SetFrameSkipping(bool _autoSkip, int _maxSkips) {}
	void SetStretchVideo(bool _stretch) {}

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
	void StartGL();
	void StopGL();
	void ResizeGL(int _width, int _height);
#endif

	static PluginAPI & get()
	{
		static PluginAPI api;
		return api;
	}

private:
	PluginAPI() {}
	PluginAPI(const PluginAPI &);

	void _initiateGFX(const GFX_INFO & _gfxInfo);
};

inline PluginAPI & api()
{
	return PluginAPI::get();
}

#endif // COMMONPLUGINAPI_H
