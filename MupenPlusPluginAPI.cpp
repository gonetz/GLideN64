#include "PluginAPI.h"
#include "Types.h"

extern "C" {

EXPORT m64p_error CALL PluginGetVersion(
	m64p_plugin_type * _PluginType,
	int * _PluginVersion,
	int * _APIVersion,
	const char ** _PluginNamePtr,
	int * _Capabilities
)
{
	return api().PluginGetVersion(_PluginType, _PluginVersion, _APIVersion, _PluginNamePtr, _Capabilities);
}

EXPORT m64p_error CALL PluginStartup(
	m64p_dynlib_handle CoreLibHandle,
	void *Context,
	void (*DebugCallback)(void *, int, const char *)
)
{
	return api().PluginStartup(CoreLibHandle);
}

EXPORT m64p_error CALL PluginShutdown(void)
{
	return api().PluginShutdown();
}

EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
	api().ReadScreen2(dest, width, height, front);
}

EXPORT void CALL SetRenderingCallback(void (*callback)(int))
{
	api().SetRenderingCallback(callback);
}

EXPORT void CALL FBRead(u32 addr)
{
	api().FBRead(addr);
}

EXPORT void CALL FBWrite(u32 addr, u32 size)
{
	api().FBWrite(addr, size);
}

EXPORT void CALL FBGetFrameBufferInfo(void *p)
{
	api().FBGetFrameBufferInfo(p);
}

EXPORT void CALL ResizeVideoOutput(int Width, int Height)
{
	api().ResizeVideoOutput(Width, Height);
}

EXPORT void CALL SetFrameSkipping(bool autoSkip, int maxSkips)
{
	api().SetFrameSkipping(autoSkip, maxSkips);
}

EXPORT void CALL SetStretchVideo(bool stretch)
{
	api().SetStretchVideo(stretch);
}

EXPORT void CALL StartGL()
{
	api().StartGL();
}

EXPORT void CALL StopGL()
{
	api().StopGL();
}

EXPORT void CALL ResizeGL(int width, int height)
{
	api().ResizeGL(width, height);
}

} // extern "C"
