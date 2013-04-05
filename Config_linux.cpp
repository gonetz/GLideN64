#include "winlnxdefs.h"
#include "SDL.h"
#include <errno.h>
#include <gtk/gtk.h>
#include <string.h>

#include "Config.h"
#include "glN64.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

static GtkWidget *configWindow = NULL;
//static GtkWidget *bitdepthCombo[2], *resolutionCombo[2];
static GtkWidget *resolutionCombo;
static GtkWidget *enable2xSAICheck, *forceBilinearCheck, *enableFogCheck;
static GtkWidget *enableHardwareFBCheck, *enablePolygonStippleCheck;
static GtkWidget *textureDepthCombo;
static GtkWidget *textureCacheEntry;
static const char *pluginDir = 0;

static const char *textureBitDepth[] =
{
	"16-bit only (faster)",
	"16-bit and 32-bit (normal)",
	"32-bit only (best for 2xSaI)",
	0
};

static void okButton_clicked( GtkWidget *widget, void *data )
{
	const char *text;
	int i, i1, i2;
	FILE *f;

	gtk_widget_hide( configWindow );

	// apply configuration
/*	text = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(resolutionCombo[0])->entry) );
	if (sscanf( text, "%d x %d", &i1, &i2 ) != 2)
	{
		i1 = 640;
		i2 = 480;
	}
	OGL.fullscreenWidth = i1;
	OGL.fullscreenHeight = i2;

	text = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(bitdepthCombo[0])->entry) );
	if (sscanf( text, "%d Bit", &i1 ) != 1)
		i1 = 0;
	OGL.fullscreenBits = i1;

	text = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(resolutionCombo[1])->entry) );
	if (sscanf( text, "%d x %d", &i1, &i2 ) != 2)
	{
		i1 = 640;
		i2 = 480;
	}
	OGL.windowedWidth = i1;
	OGL.windowedHeight = i2;

	text = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(bitdepthCombo[1])->entry) );
	if (sscanf( text, "%d Bit", &i1 ) != 1)
		i1 = 0;
	OGL.windowedBits = i1;*/

	text = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(resolutionCombo)->entry) );
	if (sscanf( text, "%d x %d", &i1, &i2 ) != 2)
	{
		i1 = 640;
		i2 = 480;
	}
	OGL.fullscreenWidth = OGL.windowedWidth = i1;
	OGL.fullscreenHeight = OGL.windowedHeight = i2;

	OGL.forceBilinear = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(forceBilinearCheck) );
	OGL.enable2xSaI = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(enable2xSAICheck) );
	OGL.fog = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(enableFogCheck) );
	OGL.frameBufferTextures = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(enableHardwareFBCheck) );
	OGL.usePolygonStipple = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(enablePolygonStippleCheck) );
	const char *depth = gtk_entry_get_text( GTK_ENTRY(GTK_COMBO(textureDepthCombo)->entry) );
	OGL.textureBitDepth = 1;
	for (i = 0; textureBitDepth[i] != 0; i++)
	{
		if (!strcmp( depth, textureBitDepth[i] ))
			OGL.textureBitDepth = i;
	}
	cache.maxBytes = atoi( gtk_entry_get_text( GTK_ENTRY(textureCacheEntry) ) ) * 1048576;

	// write configuration
	if (pluginDir == 0)
		pluginDir = GetPluginDir();

	char filename[PATH_MAX];
	snprintf( filename, PATH_MAX, "%s/glN64.conf", pluginDir );
	f = fopen( filename, "w" );
	if (!f)
	{
		fprintf( stderr, "[glN64]: (EE) Couldn't save config file '%s': %s\n", filename, strerror( errno ) );
		return;
	}

/*	fprintf( f, "fullscreen width=%d\n",      OGL.fullscreenWidth );
	fprintf( f, "fullscreen height=%d\n",     OGL.fullscreenHeight );
	fprintf( f, "fullscreen depth=%d\n",      OGL.fullscreenBits );*/
	fprintf( f, "width=%d\n",        OGL.windowedWidth );
	fprintf( f, "height=%d\n",       OGL.windowedHeight );
//	fprintf( f, "windowed depth=%d\n",        OGL.windowedBits );*/
/*	fprintf( f, "width=%d\n",                 OGL.width );
	fprintf( f, "height=%d\n",                OGL.height );*/
	fprintf( f, "force bilinear=%d\n",        OGL.forceBilinear );
	fprintf( f, "enable 2xSAI=%d\n",          OGL.enable2xSaI );
	fprintf( f, "enable fog=%d\n",            OGL.fog );
	fprintf( f, "enable HardwareFB=%d\n",     OGL.frameBufferTextures );
	fprintf( f, "enable dithered alpha=%d\n", OGL.usePolygonStipple );
	fprintf( f, "texture depth=%d\n",         OGL.textureBitDepth );
	fprintf( f, "cache size=%d\n",            cache.maxBytes / 1048576 );

	fclose( f );

}

static void cancelButton_clicked( GtkWidget *widget, void *data )
{
	gtk_widget_hide( configWindow );
}

static void configWindow_show( GtkWidget *widget, void *data )
{
	static char text[300];

	// display
/*	gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(bitdepthCombo[0])->entry),
	                     (OGL.fullscreenBits == 32) ? "32 Bit" :
	                    ((OGL.fullscreenBits == 16) ? "16 Bit" :
	                                                  "Desktop") );
	sprintf( text, "%d x %d", OGL.fullscreenWidth, OGL.fullscreenHeight );
	gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(resolutionCombo[0])->entry), text );

	gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(bitdepthCombo[1])->entry),
	                     (OGL.windowedBits == 32) ? "32 Bit" :
	                    ((OGL.windowedBits == 16) ? "16 Bit" :
	                                                "Desktop") );
	sprintf( text, "%d x %d", OGL.windowedWidth, OGL.windowedHeight );
	gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(resolutionCombo[1])->entry), text );*/

	sprintf( text, "%d x %d", OGL.windowedWidth, OGL.windowedHeight );
	gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(resolutionCombo)->entry), text );

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(enable2xSAICheck),          (OGL.enable2xSaI) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(forceBilinearCheck),        (OGL.forceBilinear) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(enableFogCheck),            (OGL.fog) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(enablePolygonStippleCheck), (OGL.usePolygonStipple) );

	// textures
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(enableHardwareFBCheck), (OGL.frameBufferTextures) );
	gtk_entry_set_text( GTK_ENTRY(GTK_COMBO(textureDepthCombo)->entry), textureBitDepth[OGL.textureBitDepth] );
	sprintf( text, "%d", cache.maxBytes / 1048576 );
	gtk_entry_set_text( GTK_ENTRY(textureCacheEntry), text );
}

static int Config_CreateWindow()
{
	GtkWidget *okButton, *cancelButton;
	GtkWidget *displayFrame, *texturesFrame;
	GtkWidget *displayTable, *texturesTable;
//	GtkWidget *fullscreenModeLabel, *windowedModeLabel;
	GtkWidget *videoModeLabel;
//	GtkWidget *bitdepthLabel, *resolutionLabel;
	GtkWidget *resolutionLabel;
	GtkWidget *textureDepthLabel;
	GtkWidget *textureCacheLabel;
	GList *resolutionList = NULL, *textureDepthList = NULL;
	SDL_Rect **modes;
	static char modeBuf[20][20];

	int i;

	// create dialog
	configWindow = gtk_dialog_new();
	gtk_signal_connect_object( GTK_OBJECT(configWindow), "delete-event",
                               GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete), GTK_OBJECT(configWindow) );
	gtk_signal_connect_object( GTK_OBJECT(configWindow), "show",
                               GTK_SIGNAL_FUNC(configWindow_show), NULL );
	gtk_window_set_title( GTK_WINDOW(configWindow), pluginName );

	// ok button
	okButton = gtk_button_new_with_label( "Ok" );
	gtk_signal_connect_object( GTK_OBJECT(okButton), "clicked",
                               GTK_SIGNAL_FUNC(okButton_clicked), NULL );
	gtk_container_add( GTK_CONTAINER(GTK_DIALOG(configWindow)->action_area), okButton );

	// cancel button
	cancelButton = gtk_button_new_with_label( "Cancel" );
	gtk_signal_connect_object( GTK_OBJECT(cancelButton), "clicked",
                               GTK_SIGNAL_FUNC(cancelButton_clicked), NULL );
	gtk_container_add( GTK_CONTAINER(GTK_DIALOG(configWindow)->action_area), cancelButton );

	// display frame
	displayFrame = gtk_frame_new( "Display" );
	gtk_container_set_border_width( GTK_CONTAINER(displayFrame), 7 );
	gtk_container_add( GTK_CONTAINER(GTK_DIALOG(configWindow)->vbox), displayFrame );

	displayTable = gtk_table_new( 5, 3, FALSE );
	gtk_container_set_border_width( GTK_CONTAINER(displayTable), 7 );
	gtk_table_set_col_spacings( GTK_TABLE(displayTable), 3 );
	gtk_table_set_row_spacings( GTK_TABLE(displayTable), 3 );
	gtk_container_add( GTK_CONTAINER(displayFrame), displayTable );

/*	fullscreenModeLabel = gtk_label_new( "Fullscreen mode" );
	windowedModeLabel = gtk_label_new( "Windowed mode" );
	bitdepthLabel = gtk_label_new( "Bit depth" );*/
	videoModeLabel = gtk_label_new( "Video mode" );
	resolutionLabel = gtk_label_new( "Resolution" );

/*	for (i = 0; i < 2; i++)
	{
		static GList *bitdepthList = NULL;
		if (!bitdepthList)
		{
			bitdepthList = g_list_append( bitdepthList, "Desktop" );
			bitdepthList = g_list_append( bitdepthList, "16 bit" );
			bitdepthList = g_list_append( bitdepthList, "32 bit" );
		}

		bitdepthCombo[i] = gtk_combo_new();
		gtk_combo_set_value_in_list( GTK_COMBO(bitdepthCombo[i]), TRUE, FALSE );
		gtk_combo_set_popdown_strings( GTK_COMBO(bitdepthCombo[i]), bitdepthList );

		resolutionCombo[i] = gtk_combo_new();
		gtk_combo_set_value_in_list( GTK_COMBO(resolutionCombo[i]), TRUE, FALSE );
	}*/

	// get video mode list
/*	modes = SDL_ListModes( NULL, SDL_HWSURFACE );//SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE | SDL_HWSURFACE | SDL_HWACCEL );

	if (modes == (SDL_Rect **)0)
	{
		printf( "glNintendo64: No modes available!\n" );
	}
	else if (modes == (SDL_Rect **)-1)
	{
		printf( "glNintendo64: All resolutions available.\n" );
	}
	else
	{
		char buf[200];
		for (i = 0; modes[i]; i++)// ++i)
		{
			sprintf( modeBuf[i], "%d x %d", modes[i]->w, modes[i]->h );
			resolutionList = g_list_append( resolutionList, modeBuf[i] );
		}
	}*/
	resolutionList = g_list_append( resolutionList, (void *)"320 x 240" );
	resolutionList = g_list_append( resolutionList, (void *)"400 x 300" );
	resolutionList = g_list_append( resolutionList, (void *)"480 x 360" );
	resolutionList = g_list_append( resolutionList, (void *)"640 x 480" );
	resolutionList = g_list_append( resolutionList, (void *)"800 x 600" );
	resolutionList = g_list_append( resolutionList, (void *)"960 x 720" );
	resolutionList = g_list_append( resolutionList, (void *)"1024 x 768" );
	resolutionList = g_list_append( resolutionList, (void *)"1152 x 864" );
	resolutionList = g_list_append( resolutionList, (void *)"1280 x 960" );
	resolutionList = g_list_append( resolutionList, (void *)"1280 x 1024" );
	resolutionList = g_list_append( resolutionList, (void *)"1440 x 1080" );
	resolutionList = g_list_append( resolutionList, (void *)"1600 x 1200" );

	resolutionCombo = gtk_combo_new();
	gtk_combo_set_value_in_list( GTK_COMBO(resolutionCombo), TRUE, FALSE );
	gtk_combo_set_popdown_strings( GTK_COMBO(resolutionCombo), resolutionList );

	enable2xSAICheck = gtk_check_button_new_with_label( "Enable 2xSAI texture scaling" );
	forceBilinearCheck = gtk_check_button_new_with_label( "Force bilinear filtering" );
	enableFogCheck = gtk_check_button_new_with_label( "Enable fog" );
	enablePolygonStippleCheck = gtk_check_button_new_with_label( "Enable dithered alpha testing" );

/*	// row 0
	gtk_table_attach_defaults( GTK_TABLE(displayTable), bitdepthLabel, 1, 2, 0, 1 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), resolutionLabel, 2, 3, 0, 1 );

	// row 1
	gtk_table_attach_defaults( GTK_TABLE(displayTable), fullscreenModeLabel, 0, 1, 1, 2 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), bitdepthCombo[0], 1, 2, 1, 2 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), resolutionCombo[0], 2, 3, 1, 2 );

	// row 2
	gtk_table_attach_defaults( GTK_TABLE(displayTable), windowedModeLabel, 0, 1, 2, 3 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), bitdepthCombo[1], 1, 2, 2, 3 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), resolutionCombo[1], 2, 3, 2, 3 );
*/
	// row 0
	gtk_table_attach_defaults( GTK_TABLE(displayTable), resolutionLabel, 2, 3, 0, 1 );
	
	// row 1
	gtk_table_attach_defaults( GTK_TABLE(displayTable), videoModeLabel, 0, 1, 1, 2 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), resolutionCombo, 2, 3, 1, 2 );
	
	// row 3
	gtk_table_attach_defaults( GTK_TABLE(displayTable), enableFogCheck, 0, 1, 3, 4 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), forceBilinearCheck, 1, 2, 3, 4 );

	// row 4
	gtk_table_attach_defaults( GTK_TABLE(displayTable), enable2xSAICheck, 0, 1, 4, 5 );
	gtk_table_attach_defaults( GTK_TABLE(displayTable), enablePolygonStippleCheck, 1, 2, 4, 5 );

	// textures frame
	texturesFrame = gtk_frame_new( "Textures" );
	gtk_container_set_border_width( GTK_CONTAINER(texturesFrame), 7 );
	gtk_container_add( GTK_CONTAINER(GTK_DIALOG(configWindow)->vbox), texturesFrame );

	texturesTable = gtk_table_new( 3, 2, FALSE );
	gtk_container_set_border_width( GTK_CONTAINER(texturesTable), 7 );
	gtk_table_set_col_spacings( GTK_TABLE(texturesTable), 3 );
	gtk_table_set_row_spacings( GTK_TABLE(texturesTable), 3 );
	gtk_container_add( GTK_CONTAINER(texturesFrame), texturesTable );

	textureDepthLabel = gtk_label_new( "Texture bit depth" );
	textureDepthCombo = gtk_combo_new();
	if (!textureDepthList)
	{
		i = 0;
		while (textureBitDepth[i] != 0)
		{
			textureDepthList = g_list_append( textureDepthList, (void *)textureBitDepth[i] );
			i++;
		}
	}
	gtk_combo_set_popdown_strings( GTK_COMBO(textureDepthCombo), textureDepthList );
	gtk_combo_set_value_in_list( GTK_COMBO(textureDepthCombo), TRUE, FALSE );

	textureCacheLabel = gtk_label_new( "Texture cache size (MB)" );
	textureCacheEntry = gtk_entry_new();
	gtk_entry_set_text( GTK_ENTRY(textureCacheEntry), "0" );

	enableHardwareFBCheck = gtk_check_button_new_with_label( "HW framebuffer textures (experimental)" );

	// row 0
	gtk_table_attach_defaults( GTK_TABLE(texturesTable), textureDepthLabel, 0, 1, 0, 1 );
	gtk_table_attach_defaults( GTK_TABLE(texturesTable), textureDepthCombo, 1, 2, 0, 1 );

	// row 1
	gtk_table_attach_defaults( GTK_TABLE(texturesTable), textureCacheLabel, 0, 1, 1, 2 );
	gtk_table_attach_defaults( GTK_TABLE(texturesTable), textureCacheEntry, 1, 2, 1, 2 );

	// row 2
	gtk_table_attach_defaults( GTK_TABLE(texturesTable), enableHardwareFBCheck, 0, 2, 2, 3 );

	return 0;
}

void Config_LoadConfig()
{
	static int loaded = 0;
	FILE *f;
	char line[2000];

	if (loaded)
		return;

	loaded = 1;

	if (pluginDir == 0)
		pluginDir = GetPluginDir();

	// default configuration
	OGL.fullscreenWidth = 640;
	OGL.fullscreenHeight = 480;
//	OGL.fullscreenBits = 0;
	OGL.windowedWidth = 640;
	OGL.windowedHeight = 480;
//	OGL.windowedBits = 0;
	OGL.forceBilinear = 0;
	OGL.enable2xSaI = 0;
	OGL.fog = 1;
	OGL.textureBitDepth = 1; // normal (16 & 32 bits)
	OGL.frameBufferTextures = 0;
	OGL.usePolygonStipple = 0;
	cache.maxBytes = 32 * 1048576;

	// read configuration
	char filename[PATH_MAX];
	snprintf( filename, PATH_MAX, "%s/glN64.conf", pluginDir );
	f = fopen( filename, "r" );
	if (!f)
	{
		fprintf( stderr, "[glN64]: (WW) Couldn't open config file '%s' for reading: %s\n", filename, strerror( errno ) );
		return;
	}

	while (!feof( f ))
	{
		char *val;
		fgets( line, 2000, f );

		val = strchr( line, '=' );
		if (!val)
			continue;
		*val++ = '\0';

/*		if (!strcasecmp( line, "fullscreen width" ))
		{
			OGL.fullscreenWidth = atoi( val );
		}
		else if (!strcasecmp( line, "fullscreen height" ))
		{
			OGL.fullscreenHeight = atoi( val );
		}
		else if (!strcasecmp( line, "fullscreen depth" ))
		{
			OGL.fullscreenBits = atoi( val );
		}
		else if (!strcasecmp( line, "windowed width" ))
		{
			OGL.windowedWidth = atoi( val );
		}
		else if (!strcasecmp( line, "windowed height" ))
		{
			OGL.windowedHeight = atoi( val );
		}
		else if (!strcasecmp( line, "windowed depth" ))
		{
			OGL.windowedBits = atoi( val );
		}*/
		if (!strcasecmp( line, "width" ))
		{
			int w = atoi( val );
			OGL.fullscreenWidth = OGL.windowedWidth = (w == 0) ? (640) : (w);
		}
		else if (!strcasecmp( line, "height" ))
		{
			int h = atoi( val );
			OGL.fullscreenHeight = OGL.windowedHeight = (h == 0) ? (480) : (h);
		}
		else if (!strcasecmp( line, "force bilinear" ))
		{
			OGL.forceBilinear = atoi( val );
		}
		else if (!strcasecmp( line, "enable 2xSAI" ))
		{
			OGL.enable2xSaI = atoi( val );
		}
		else if (!strcasecmp( line, "enable fog" ))
		{
			OGL.fog = atoi( val );
		}
		else if (!strcasecmp( line, "cache size" ))
		{
			cache.maxBytes = atoi( val ) * 1048576;
		}
		else if (!strcasecmp( line, "enable HardwareFB" ))
		{
			OGL.frameBufferTextures = atoi( val );
		}
		else if (!strcasecmp( line, "enable dithered alpha" ))
		{
			OGL.usePolygonStipple = atoi( val );
		}
		else if (!strcasecmp( line, "texture depth" ))
		{
			OGL.textureBitDepth = atoi( val );
		}
		else
		{
			printf( "Unknown config option: %s\n", line );
		}
	}

	fclose( f );
}

void Config_DoConfig()
{
	Config_LoadConfig();

	if (!configWindow)
		Config_CreateWindow();

	gtk_widget_show_all( configWindow );
}
