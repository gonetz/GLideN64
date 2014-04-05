#include "winlnxdefs.h"
#include <SDL.h>
#include <errno.h>
#include <string.h>

#include "Config.h"
#include "GLideN64.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

Config config;

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
	{"enable 2xSAI", &config.texture.enable2xSaI, 0},
	{"cache size", &cache.maxBytes, 64*1048576},
	{"texture depth", &config.texture.textureBitDepth, 1},
	{"#Emulation Settings:", NULL, 0},
	{"enable fog", &config.enableFog, 1},
	{"enable HardwareFB", &config.frameBufferEmulation.enable, 0}
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
		if (strcasecmp(line, o->name) == 0)	{
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
