#include "../winlnxdefs.h"
#include "../PluginAPI.h"
#include "../OpenGL.h"
#include "../Config.h"

int PluginAPI::InitiateGFX(const GFX_INFO & _gfxInfo)
{
	_initiateGFX(_gfxInfo);

	Config_LoadConfig();
	OGL.hScreen = NULL;

	return TRUE;
}
