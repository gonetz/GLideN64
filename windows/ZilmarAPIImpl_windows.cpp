#include "GLideN64_Windows.h"
#include "../PluginAPI.h"
#include "../GLideN64.h"
#include "../GLideNUI/GLideNUI.h"

void PluginAPI::DllAbout(HWND _hParent)
{
//	MessageBox(_hParent, "GLideN64. Based on Orkin's glN64 v0.4.1", pluginName, MB_OK | MB_ICONINFORMATION );
	RunAbout();
}
