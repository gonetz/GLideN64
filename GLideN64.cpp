#ifdef _WINDOWS
# include <windows.h>
# include <commctrl.h>
#else
# include "winlnxdefs.h"
#include <dlfcn.h>
#endif // _WINDOWS
#include "GLideN64.h"
#include "Debug.h"
#include "OpenGL.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "VI.h"
#include "Config.h"
#include "Textures.h"
#include "Combiner.h"

char pluginName[] = "GLideN64 alpha";

#ifndef MUPENPLUSAPI
#include "ZilmarGFX_1_3.h"

#ifdef _WINDOWS
HWND		hWnd;
HWND		hStatusBar;
//HWND		hFullscreen;
HWND		hToolBar;
HINSTANCE	hInstance;
#endif // _WINDOWS

char		*screenDirectory;

void (*CheckInterrupts)( void );

#ifdef _WINDOWS
LONG		windowedStyle;
LONG		windowedExStyle;
RECT		windowedRect;
HMENU		windowedMenu;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hinstDLL;

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Config_LoadConfig();
		OGL.hRC = NULL;
		OGL.hDC = NULL;
//		hFullscreen = NULL;
	}
	return TRUE;
}
#else
void
_init( void )
{
	Config_LoadConfig();
	OGL.hScreen = NULL;
}
#endif // _WINDOWS

EXPORT void CALL CaptureScreen ( char * Directory )
{
	screenDirectory = Directory;
	OGL.captureScreen = true;
}

EXPORT void CALL CloseDLL (void)
{
}

EXPORT void CALL DllAbout ( HWND hParent )
{
#ifdef _WINDOWS
	MessageBox( hParent, "GLideN64 alpha. Based on Orkin's glN64 v0.4", pluginName, MB_OK | MB_ICONINFORMATION );
#else
	puts( "GLideN64 alpha. Based on Orkin's glN64 v0.4" );
#endif
}

EXPORT void CALL DllConfig ( HWND hParent )
{
	Config_DoConfig();
}

EXPORT void CALL DllTest ( HWND hParent )
{
}

EXPORT void CALL DrawScreen (void)
{
}

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	PluginInfo->Version = 0x103;
	PluginInfo->Type = PLUGIN_TYPE_GFX;
	strcpy( PluginInfo->Name, pluginName );
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
}

#ifdef _WINDOWS
BOOL CALLBACK FindToolBarProc( HWND hWnd, LPARAM lParam )
{
	if (GetWindowLong( hWnd, GWL_STYLE ) & RBS_VARHEIGHT)
	{
		hToolBar = hWnd;
		return FALSE;
	}
	return TRUE;
}
#endif // _WINDOWS


EXPORT void CALL ReadScreen (void **dest, long *width, long *height)
{
	OGL_ReadScreen( dest, width, height );
}

#else // MUPENPLUSAPI
#include "m64p_plugin.h"

ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath = NULL;
ptr_ConfigGetUserConfigPath ConfigGetUserConfigPath = NULL;

void (*CheckInterrupts)( void );
void (*renderCallback)() = NULL;

extern "C" {

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context, void (*DebugCallback)(void *, int, const char *))
{
	ConfigGetSharedDataFilepath = (ptr_ConfigGetSharedDataFilepath)	dlsym(CoreLibHandle, "ConfigGetSharedDataFilepath");
	ConfigGetUserConfigPath = (ptr_ConfigGetUserConfigPath)	dlsym(CoreLibHandle, "ConfigGetUserConfigPath");
	return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
	OGL_Stop();
	return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType,
	int *PluginVersion, int *APIVersion, const char **PluginNamePtr,
	int *Capabilities)
{
	/* set version info */
	if (PluginType != NULL)
		*PluginType = M64PLUGIN_GFX;

	if (PluginVersion != NULL)
		*PluginVersion = PLUGIN_VERSION;

	if (APIVersion != NULL)
		*APIVersion = VIDEO_PLUGIN_API_VERSION;

	if (PluginNamePtr != NULL)
		*PluginNamePtr = pluginName;

	if (Capabilities != NULL)
	{
		*Capabilities = 0;
	}

	return M64ERR_SUCCESS;
}

EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
	//OGL_ReadScreen( dest, width, height );
}

EXPORT void CALL SetRenderingCallback(void (*callback)(int))
{
	static void (*renderCallback)(int) = NULL;
	renderCallback = callback;
}

EXPORT void CALL FBRead(u32 addr)
{
}

EXPORT void CALL FBWrite(u32 addr, u32 size) {
}

EXPORT void CALL FBGetFrameBufferInfo(void *p)
{
}

EXPORT void CALL ResizeVideoOutput(int Width, int Height)
{
}

EXPORT void CALL SetFrameSkipping(bool autoSkip, int maxSkips)
{
}

EXPORT void CALL SetStretchVideo(bool stretch)
{
}

EXPORT void CALL StartGL()
{
	OGL_Start();
}

EXPORT void CALL StopGL()
{
	OGL_Stop();
}

EXPORT void CALL ResizeGL(int width, int height)
{
/*
	int videoWidth = width;
	int videoHeight = height;

	if (!config.stretchVideo) {
		const float ratio = (config.romPAL ? 9.0f/11.0f : 0.75f);
		videoWidth = (int) (height / ratio);
		if (videoWidth > width) {
			videoWidth = width;
			videoHeight = (int) (width * ratio);
		}
	}

	int x = (width - videoWidth) / 2;
	int y = (height - videoHeight) / 2;

	OGL_ResizeWindow(x, y, videoWidth, videoHeight);
*/
}

} // extern "C"
#endif // MUPENPLUSAPI

//----------Common-------------------//

extern "C" {

EXPORT void CALL ChangeWindow (void)
{
# ifdef __LINUX__
	SDL_WM_ToggleFullScreen( OGL.hScreen );
# endif // __LINUX__
}

EXPORT void CALL MoveScreen (int xpos, int ypos)
{
}

EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info)
{
	DMEM = Gfx_Info.DMEM;
	IMEM = Gfx_Info.IMEM;
	RDRAM = Gfx_Info.RDRAM;

	REG.MI_INTR = Gfx_Info.MI_INTR_REG;
	REG.DPC_START = Gfx_Info.DPC_START_REG;
	REG.DPC_END = Gfx_Info.DPC_END_REG;
	REG.DPC_CURRENT = Gfx_Info.DPC_CURRENT_REG;
	REG.DPC_STATUS = Gfx_Info.DPC_STATUS_REG;
	REG.DPC_CLOCK = Gfx_Info.DPC_CLOCK_REG;
	REG.DPC_BUFBUSY = Gfx_Info.DPC_BUFBUSY_REG;
	REG.DPC_PIPEBUSY = Gfx_Info.DPC_PIPEBUSY_REG;
	REG.DPC_TMEM = Gfx_Info.DPC_TMEM_REG;

	REG.VI_STATUS = Gfx_Info.VI_STATUS_REG;
	REG.VI_ORIGIN = Gfx_Info.VI_ORIGIN_REG;
	REG.VI_WIDTH = Gfx_Info.VI_WIDTH_REG;
	REG.VI_INTR = Gfx_Info.VI_INTR_REG;
	REG.VI_V_CURRENT_LINE = Gfx_Info.VI_V_CURRENT_LINE_REG;
	REG.VI_TIMING = Gfx_Info.VI_TIMING_REG;
	REG.VI_V_SYNC = Gfx_Info.VI_V_SYNC_REG;
	REG.VI_H_SYNC = Gfx_Info.VI_H_SYNC_REG;
	REG.VI_LEAP = Gfx_Info.VI_LEAP_REG;
	REG.VI_H_START = Gfx_Info.VI_H_START_REG;
	REG.VI_V_START = Gfx_Info.VI_V_START_REG;
	REG.VI_V_BURST = Gfx_Info.VI_V_BURST_REG;
	REG.VI_X_SCALE = Gfx_Info.VI_X_SCALE_REG;
	REG.VI_Y_SCALE = Gfx_Info.VI_Y_SCALE_REG;

	CheckInterrupts = Gfx_Info.CheckInterrupts;

#ifndef MUPENPLUSAPI
#ifdef _WINDOWS
	hWnd = Gfx_Info.hWnd;
	hStatusBar = Gfx_Info.hStatusBar;
	hToolBar = NULL;

	EnumChildWindows( hWnd, FindToolBarProc,0 );
#else // _WINDOWS
	Config_LoadConfig();
	OGL.hScreen = NULL;
#endif // _WINDOWS
#else // MUPENPLUSAPI
	Config_LoadConfig();
//	Config_LoadRomConfig(Gfx_Info.HEADER);
#endif // MUPENPLUSAPI

	//OGL_Start();
	return TRUE;
}

EXPORT void CALL ProcessDList(void)
{
	RSP_ProcessDList();
}

EXPORT void CALL ProcessRDPList(void)
{
}

EXPORT void CALL RomClosed (void)
{
	OGL_Stop();

#ifdef DEBUG
	CloseDebugDlg();
#endif
}

EXPORT void CALL RomOpen (void)
{
	RSP_Init();

	OGL_ResizeWindow();

#ifdef DEBUG
	OpenDebugDlg();
#endif
}

EXPORT void CALL RomResumed(void)
{
}

EXPORT void CALL ShowCFB (void)
{
	gSP.changed |= CHANGED_CPU_FB_WRITE;
}

EXPORT void CALL UpdateScreen (void)
{
	VI_UpdateScreen();
}

EXPORT void CALL ViStatusChanged (void)
{
}

EXPORT void CALL ViWidthChanged (void)
{
}

}
