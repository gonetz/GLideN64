#include "../PluginAPI.h"
#include "../OpenGL.h"
#include "../Config.h"

int PluginAPI::InitiateGFX(const GFX_INFO & _gfxInfo)
{
	Lock lock(this);
	_initiateGFX(_gfxInfo);

	Config_LoadConfig();

	return TRUE;
}
