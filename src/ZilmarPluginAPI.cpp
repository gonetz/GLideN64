#ifdef OS_WINDOWS
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // OS_WINDOWS

#include "PluginAPI.h"
#include "N64.h"

extern "C" {

EXPORT void CALL RomOpen (void)
{
	RDRAMSize = 0;
	api().RomOpen();
}

EXPORT void CALL CaptureScreen(const char * Directory)
{
    api().CaptureScreen(const_cast<char *>(Directory));
}

EXPORT void CALL CloseDLL (void)
{
	api().CloseDLL();
}

EXPORT void CALL DllAbout(void * hParent)
{
    api().DllAbout(hParent);
}

EXPORT void CALL DllConfig(void * hParent)
{
    api().DllConfig(hParent);
}

EXPORT void CALL DllTest(void * hParent)
{
    api().DllTest(hParent);
}

EXPORT void CALL DrawScreen (void)
{
	api().DrawScreen();
}

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	api().GetDllInfo(PluginInfo);
}

EXPORT void CALL ReadScreen (void **dest, long *width, long *height)
{
	api().ReadScreen(dest, width, height);
}

EXPORT void CALL DllCrtFree(void* addr)
{
	free(addr);
}

void CALL mge_get_video_size(int32_t* width, int32_t* height)
{
	api().GetVideoSize(width, height);
}

EXPORT void CALL PluginLoaded(void)
{
    // No-op or initialization if needed
}

EXPORT void CALL DrawStatus(const char * lpString, int32_t RightAlign)
{
    api().DrawStatus(lpString, RightAlign);
}

}
