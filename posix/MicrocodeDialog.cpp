#include <glib.h>
#include <gtk/gtk.h>
#include "../winlnxdefs.h"
#include "../GLideN64.h"
#include "../GBI.h"

static int selectedMicrocode = -1;
static GtkWidget *microcodeWindow = 0;
static GtkWidget *microcodeList = 0;

static const char *MicrocodeTypes[] =
{
	"Fast3D",
	"F3DEX",
	"F3DEX2",
	"Line3D",
	"L3DEX",
	"L3DEX2",
	"S2DEX",
	"S2DEX2",
	"Perfect Dark",
	"DKR/JFG",
	"Waverace US",
	"None"
};

static const int numMicrocodeTypes = 11;
static unsigned int uc_crc;
static const char * uc_str;

static void okButton_clicked( GtkWidget *widget, void *data )
{
	gtk_widget_hide( microcodeWindow );
	if (GTK_LIST(microcodeList)->selection != 0)
	{
		char *text = 0;
		GtkListItem *item = GTK_LIST_ITEM(GTK_LIST(microcodeList)->selection->data);
		GtkLabel *label = GTK_LABEL(GTK_BIN(item)->child);
		gtk_label_get( label, &text );
		if (text != 0)
			for (int i = 0; i < numMicrocodeTypes; i++)
				if (!strcmp( text, MicrocodeTypes[i] ))
				{
					selectedMicrocode = i;
					return;
				}
	}

	selectedMicrocode = NONE;
}

static void stopButton_clicked( GtkWidget *widget, void *data )
{
	gtk_widget_hide( microcodeWindow );
	selectedMicrocode = NONE;
}

static gint
delete_question_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return TRUE; // undeleteable
}

int MicrocodeDialog(u32 _crc, const char * _str)
{
	GtkWidget *infoLabel;
	GtkWidget *infoFrame, *infoTable;
	GtkWidget *crcInfoLabel, *crcDataInfoLabel, *textInfoLabel;
	GtkWidget *crcLabel, *crcDataLabel, *textLabel;
	GtkWidget *selectUcodeLabel;
	GtkWidget *microcodeLabel;
	GtkWidget *okButton, *stopButton;
	GList *ucodeList = 0;
	char buf[1024];

	if (!g_thread_supported())
		g_thread_init( NULL );
	gdk_threads_enter();

	// create dialog
	if (microcodeWindow == 0)
	{
		microcodeWindow = gtk_dialog_new();
		gtk_signal_connect( GTK_OBJECT(microcodeWindow), "delete_event",
							GTK_SIGNAL_FUNC(delete_question_event), (gpointer)NULL );
		sprintf( buf, "%s - unknown microcode", pluginName );
		gtk_window_set_title( GTK_WINDOW(microcodeWindow), buf );
		gtk_container_set_border_width( GTK_CONTAINER(GTK_DIALOG(microcodeWindow)->vbox), 11 );

		// ok button
		okButton = gtk_button_new_with_label( "Ok" );
		gtk_signal_connect_object( GTK_OBJECT(okButton), "clicked",
							   GTK_SIGNAL_FUNC(okButton_clicked), NULL );
		gtk_container_add( GTK_CONTAINER(GTK_DIALOG(microcodeWindow)->action_area), okButton );

		// stop button
		stopButton = gtk_button_new_with_label( "Stop" );
		gtk_signal_connect_object( GTK_OBJECT(stopButton), "clicked",
							   GTK_SIGNAL_FUNC(stopButton_clicked), NULL );
		gtk_container_add( GTK_CONTAINER(GTK_DIALOG(microcodeWindow)->action_area), stopButton );

		// info label
		infoLabel = gtk_label_new( "Unknown microcode. Please notify Orkin, including the following information:" );
		gtk_box_pack_start_defaults( GTK_BOX(GTK_DIALOG(microcodeWindow)->vbox), infoLabel );

		// info frame
		infoFrame = gtk_frame_new( "Microcode info" );
		gtk_container_set_border_width( GTK_CONTAINER(infoFrame), 7 );
		gtk_box_pack_start_defaults( GTK_BOX(GTK_DIALOG(microcodeWindow)->vbox), infoFrame );

		infoTable = gtk_table_new( 3, 2, FALSE );
		gtk_container_set_border_width( GTK_CONTAINER(infoTable), 7 );
		gtk_table_set_col_spacings( GTK_TABLE(infoTable), 3 );
		gtk_table_set_row_spacings( GTK_TABLE(infoTable), 3 );
		gtk_container_add( GTK_CONTAINER(infoFrame), infoTable );

		crcInfoLabel = gtk_label_new( "Microcode CRC:" );
		crcDataInfoLabel = gtk_label_new( "Microcode Data CRC:" );
		textInfoLabel = gtk_label_new( "Microcode Text:" );

		crcLabel = gtk_label_new( "" );
		crcDataLabel = gtk_label_new( "" );
		textLabel = gtk_label_new( "" );

		gtk_table_attach_defaults( GTK_TABLE(infoTable), crcInfoLabel, 0, 1, 0, 1 );
		gtk_table_attach_defaults( GTK_TABLE(infoTable), crcLabel, 1, 2, 0, 1 );
		gtk_table_attach_defaults( GTK_TABLE(infoTable), crcDataInfoLabel, 0, 1, 1, 2 );
		gtk_table_attach_defaults( GTK_TABLE(infoTable), crcDataLabel, 1, 2, 1, 2 );
		gtk_table_attach_defaults( GTK_TABLE(infoTable), textInfoLabel, 0, 1, 2, 3 );
		gtk_table_attach_defaults( GTK_TABLE(infoTable), textLabel, 1, 2, 2, 3 );

		selectUcodeLabel = gtk_label_new( "You can manually select the closest matching microcode." );
		for (int i = 0; i < numMicrocodeTypes; i++)
			ucodeList = g_list_append( ucodeList, gtk_list_item_new_with_label( MicrocodeTypes[i] ) );
		microcodeList = gtk_list_new();
		gtk_list_set_selection_mode( GTK_LIST(microcodeList), GTK_SELECTION_SINGLE );
		gtk_list_append_items( GTK_LIST(microcodeList), ucodeList );

		gtk_box_pack_start_defaults( GTK_BOX(GTK_DIALOG(microcodeWindow)->vbox), selectUcodeLabel );
		gtk_box_pack_start_defaults( GTK_BOX(GTK_DIALOG(microcodeWindow)->vbox), microcodeList );
	}

	snprintf( buf, 1024, "0x%8.8X", _crc );
	gtk_label_set_text( GTK_LABEL(crcLabel), buf );
	snprintf( buf, 1024, "0x%8.8X", 0 );
	gtk_label_set_text( GTK_LABEL(crcDataLabel), buf );
	gtk_label_set_text( GTK_LABEL(textLabel), _str );

	selectedMicrocode = -1;
	gtk_widget_show_all( microcodeWindow );

	while (selectedMicrocode == -1)
	{
//		if( gtk_main_iteration() )
//			break;
		usleep( 10000 );
	}
	gdk_threads_leave();
	return selectedMicrocode;
}
