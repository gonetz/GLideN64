#include "GLideN64_MupenPlus.h"
#include <errno.h>
#include <string.h>

#include "../Config.h"
#include "../GLideN64.h"
#include "../RSP.h"
#include "../Textures.h"
#include "../OpenGL.h"
#include "../Log.h"

Config config;

const u32 uMegabyte = 1024U*1024U;

struct Option
{
	const char* name;
	u32* data;
	const int initial;
};

Option configOptions[] =
{
	{"#GLideN64 Graphics Plugin for N64", NULL, 0},
	{"config version", &config.version, 0},
	{"", NULL, 0},

	{"#Window Settings:", NULL, 0},
	{"window width", &config.video.windowedWidth, 640},
	{"window height", &config.video.windowedHeight, 480},
	{"#Texture Settings:", NULL, 0},
	{"force bilinear", &config.texture.forceBilinear, 0},
	{"cache size", &config.texture.maxBytes, 64*uMegabyte},
	{"texture bit depth", &config.texture.textureBitDepth, 1},
	{"#Emulation Settings:", NULL, 0},
	{"enable fog", &config.enableFog, 1},
	{"enable noise", &config.enableNoise, 1},
	{"enable LOD", &config.enableLOD, 1},
	{"enable HW lighting", &config.enableHWLighting, 0},
	{"#Frame Buffer Settings:", NULL, 0},
	{"enable hardware FB", &config.frameBufferEmulation.enable, 0},
	{"enable copy Color Buffer to RDRAM", &config.frameBufferEmulation.copyToRDRAM, 0},
	{"enable copy Depth Buffer to RDRAM", &config.frameBufferEmulation.copyDepthToRDRAM, 0},
	{"enable copy Color Buffer from RDRAM", &config.frameBufferEmulation.copyFromRDRAM, 0},
	{"enable ignore CFB", &config.frameBufferEmulation.ignoreCFB, 0},
	{"enable N64 depth compare", &config.frameBufferEmulation.N64DepthCompare, 0}
};

const int configOptionsSize = sizeof(configOptions) / sizeof(Option);

void Config_WriteConfig(const char *filename)
{
	//config.version = CONFIG_VERSION;
	FILE* f = fopen(filename, "w");
	if (!f) {
		fprintf( stderr, "[GLideN64]: Could Not Open %s for writing\n", filename);
		return;
	}

	for(int i=0; i<configOptionsSize; i++) {
		Option *o = &configOptions[i];
		fprintf(f, "%s", o->name);
		if (o->data)
			fprintf(f,"=%i", *(o->data));
		fprintf(f, "\n");
	}

	fclose(f);
}

void Config_SetDefault()
{
	for(int i=0; i < configOptionsSize; i++) {
		Option *o = &configOptions[i];
		if (o->data) *(o->data) = o->initial;
	}
	config.video.fullscreenWidth = 640;
	config.video.fullscreenHeight = 480;
}

void Config_SetOption(char* line, char* val)
{
	for(int i=0; i< configOptionsSize; i++) {
		Option *o = &configOptions[i];
#ifndef _WINDOWS
		if (strcasecmp(line, o->name) == 0)	{
#else
		if (_stricmp(line, o->name) == 0)	{
#endif
			if (o->data) {
				int v = atoi(val);
				*(o->data) = v;
			}
			break;
		}
	}
}

void Config_LoadConfig()
{
	static bool loaded = false;
	FILE *f;
	char line[4096];

	if (loaded)
		return;

	loaded = true;

	Config_SetDefault();

	// read configuration
#ifndef GLES2
	const char *filename = ConfigGetSharedDataFilepath("GLideN64.cfg");
	const char * pConfigName = "GLideN64.cfg";
	const char * pConfigPath = ConfigGetUserConfigPath();
	const size_t nPathLen = strlen(pConfigPath);
	const size_t configNameLen = nPathLen + strlen(pConfigName) + 2;
	char * pConfigFullName = new char[configNameLen];
	strcpy(pConfigFullName, pConfigPath);
	if (pConfigPath[nPathLen-1] != '/')
		strcat(pConfigFullName, "/");
	strcat(pConfigFullName, pConfigName);
	f = fopen(pConfigFullName, "r");
	if (!f) {
		fprintf( stderr, "[GLideN64]: (WW) Couldn't open config file '%s' for reading: %s\n", pConfigFullName, strerror( errno ) );
		fprintf( stderr, "[GLideN64]: Attempting to write new Config \n");
		Config_WriteConfig(pConfigFullName);
		delete[] pConfigFullName;
		return;
	}
	delete[] pConfigFullName;
#else
	const char *filename = ConfigGetSharedDataFilepath("gles2gliden64.conf");
	f = fopen(filename, "r");
	if (!f) {
		LOG(LOG_MINIMAL, "[gles2GlideN64]: Couldn't open config file '%s' for reading: %s\n", filename, strerror( errno ) );
		LOG(LOG_MINIMAL, "[gles2GlideN64]: Attempting to write new Config \n");
		Config_WriteConfig(filename);
		return;
	}
	LOG(LOG_MINIMAL, "[gles2GlideN64]: Loading Config from %s \n", filename);
#endif
	while (!feof( f )) {
		char *val;
		fgets( line, 4096, f );

		if (line[0] == '#' || line[0] == '\n')
			continue;

		val = strchr( line, '=' );
		if (!val) continue;

		*val++ = '\0';

		Config_SetOption(line,val);
	}
/*
	if (config.version < CONFIG_VERSION)
	{
		LOG(LOG_WARNING, "[gles2N64]: Wrong config version, rewriting config with defaults\n");
		Config_SetDefault();
		Config_WriteConfig(filename);
	}
*/
	fclose(f);
}
