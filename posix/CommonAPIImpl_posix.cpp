#include "../CommonPluginAPI.h"
#include "../OpenGL.h"
#include "../Config.h"

int CommonPluginAPI::InitiateGFX(const GFX_INFO & _gfxInfo)
{
	_initiateGFX(_gfxInfo);

	Config_LoadConfig();
	OGL.hScreen = NULL;

	return TRUE;
}
