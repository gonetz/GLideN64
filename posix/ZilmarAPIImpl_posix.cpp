#include "../PluginAPI.h"
#include "../GLideN64.h"
#include "../OpenGL.h"

void PluginAPI::ChangeWindow()
{
#if defined(USE_SDL)
	SDL_WM_ToggleFullScreen( OGL.hScreen );
#endif
}

void PluginAPI::DllAbout(HWND _hParent)
{
	puts( "GLideN64 alpha. Based on Orkin's glN64 v0.4" );
}
