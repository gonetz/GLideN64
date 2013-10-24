#include "winlnxdefs.h"
# include <SDL/SDL.h>
#include <errno.h>
#include <string.h>

#include "Config.h"
#include "GLideN64.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

struct Option
{
	const char* name;
	unsigned int* data;
	const int initial;
};

Option configOptions[] =
{
	{"#GLideN64 Graphics Plugin for N64", NULL, 0},
//	{"config version", &config.version, 0},
	{"", NULL, 0},

//	{"#Window Settings:", NULL, 0},
	{"window width", &OGL.windowedWidth, 640},
	{"window height", &OGL.windowedHeight, 480},
	{"force bilinear", &OGL.forceBilinear, 0},
	{"enable 2xSAI", &OGL.enable2xSaI, 0},
	{"enable fog", &OGL.fog, 1},
	{"cache size", &cache.maxBytes, 64*1048576},
	{"enable HardwareFB", &OGL.frameBufferTextures, 0},
	{"texture depth", &OGL.textureBitDepth, 1}
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
	OGL.fullscreenWidth = 640;
	OGL.fullscreenHeight = 480;
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
	const char *filename = ConfigGetSharedDataFilepath("glN64.conf");
	f = fopen(filename, "r");
	if (!f) {
		fprintf( stderr, "[GLideN64]: (WW) Couldn't open config file '%s' for reading: %s\n", filename, strerror( errno ) );
		fprintf( stderr, "[GLideN64]: Attempting to write new Config \n");
		Config_WriteConfig(filename);
		return;
	}

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
