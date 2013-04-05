#include <stdio.h>
#include "glN64.h"
#include "GBI.h"
#include "RDP.h"
#include "RSP.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "L3D.h"
#include "L3DEX.h"
#include "L3DEX2.h"
#include "S2DEX.h"
#include "S2DEX2.h"
#include "F3DDKR.h"
#include "F3DWRUS.h"
#include "F3DPD.h"
#include "Types.h"
#ifndef __LINUX__
# include "Resource.h"
#else // !__LINUX__
# include <glib.h>
# include <gtk/gtk.h>
#endif // __LINUX__
#include "CRC.h"
#include "Debug.h"

u32 uc_crc, uc_dcrc;
char uc_str[256];

SpecialMicrocodeInfo specialMicrocodes[] =
{
	{ F3DWRUS,	FALSE,	0xd17906e2, "RSP SW Version: 2.0D, 04-01-96" },
	{ F3DWRUS,	FALSE,	0x94c4c833, "RSP SW Version: 2.0D, 04-01-96" },

	{ S2DEX,	FALSE,	0x9df31081, "RSP Gfx ucode S2DEX  1.06 Yoshitaka Yasumoto Nintendo." },

	{ F3DDKR,	FALSE,	0x8d91244f, "Diddy Kong Racing" },
	{ F3DDKR,	FALSE,	0x6e6fc893, "Diddy Kong Racing" },
	{ F3DDKR,	FALSE,	0xbde9d1fb, "Jet Force Gemini" },
	{ F3DPD,	FALSE,	0x1c4f7869, "Perfect Dark" }
};

u32 G_RDPHALF_1, G_RDPHALF_2, G_RDPHALF_CONT;
u32 G_SPNOOP;
u32 G_SETOTHERMODE_H, G_SETOTHERMODE_L;
u32 G_DL, G_ENDDL, G_CULLDL, G_BRANCH_Z;
u32 G_LOAD_UCODE;
u32 G_MOVEMEM, G_MOVEWORD;
u32 G_MTX, G_POPMTX;
u32 G_GEOMETRYMODE, G_SETGEOMETRYMODE, G_CLEARGEOMETRYMODE;
u32 G_TEXTURE;
u32 G_DMA_IO, G_DMA_DL, G_DMA_TRI, G_DMA_MTX, G_DMA_VTX, G_DMA_OFFSETS;
u32 G_SPECIAL_1, G_SPECIAL_2, G_SPECIAL_3;
u32 G_VTX, G_MODIFYVTX, G_VTXCOLORBASE;
u32 G_TRI1, G_TRI2, G_TRI4;
u32 G_QUAD, G_LINE3D;
u32 G_RESERVED0, G_RESERVED1, G_RESERVED2, G_RESERVED3;
u32 G_SPRITE2D_BASE;
u32 G_BG_1CYC, G_BG_COPY;
u32 G_OBJ_RECTANGLE, G_OBJ_SPRITE, G_OBJ_MOVEMEM;
u32 G_SELECT_DL, G_OBJ_RENDERMODE, G_OBJ_RECTANGLE_R;
u32 G_OBJ_LOADTXTR, G_OBJ_LDTX_SPRITE, G_OBJ_LDTX_RECT, G_OBJ_LDTX_RECT_R;
u32 G_RDPHALF_0;

u32 G_MTX_STACKSIZE;
u32 G_MTX_MODELVIEW;
u32 G_MTX_PROJECTION;
u32 G_MTX_MUL;
u32 G_MTX_LOAD;
u32 G_MTX_NOPUSH;
u32 G_MTX_PUSH;

u32 G_TEXTURE_ENABLE;
u32 G_SHADING_SMOOTH;
u32 G_CULL_FRONT;
u32 G_CULL_BACK;
u32 G_CULL_BOTH;
u32 G_CLIPPING;

u32 G_MV_VIEWPORT;

u32 G_MWO_aLIGHT_1, G_MWO_bLIGHT_1;
u32 G_MWO_aLIGHT_2, G_MWO_bLIGHT_2;
u32 G_MWO_aLIGHT_3, G_MWO_bLIGHT_3;
u32 G_MWO_aLIGHT_4, G_MWO_bLIGHT_4;
u32 G_MWO_aLIGHT_5, G_MWO_bLIGHT_5;
u32 G_MWO_aLIGHT_6, G_MWO_bLIGHT_6;
u32 G_MWO_aLIGHT_7, G_MWO_bLIGHT_7;
u32 G_MWO_aLIGHT_8, G_MWO_bLIGHT_8;

//GBIFunc GBICmd[256];
GBIInfo GBI;

void GBI_Unknown( u32 w0, u32 w1 )
{
#ifdef DEBUG
	if (Debug.level == DEBUG_LOW)
		DebugMsg( DEBUG_LOW | DEBUG_UNKNOWN, "UNKNOWN GBI COMMAND 0x%02X", _SHIFTR( w0, 24, 8 ) );
	if (Debug.level == DEBUG_MEDIUM)
		DebugMsg( DEBUG_MEDIUM | DEBUG_UNKNOWN, "Unknown GBI Command 0x%02X", _SHIFTR( w0, 24, 8 ) );
	else if (Debug.level == DEBUG_HIGH)
		DebugMsg( DEBUG_HIGH | DEBUG_UNKNOWN, "// Unknown GBI Command 0x%02X", _SHIFTR( w0, 24, 8 ) );
#endif
}

#ifndef __LINUX__
INT_PTR CALLBACK MicrocodeDlgProc( HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			for (int i = 0; i < numMicrocodeTypes; i++)
			{
				SendDlgItemMessage( hWndDlg, IDC_MICROCODE, CB_ADDSTRING, 0, (LPARAM)MicrocodeTypes[i] );
			}
			SendDlgItemMessage( hWndDlg, IDC_MICROCODE, CB_SETCURSEL, 0, 0 );

			char text[1024];
			sprintf( text, "Microcode CRC:\t\t0x%08x\r\nMicrocode Data CRC:\t0x%08x\r\nMicrocode Text:\t\t%s", uc_crc, uc_dcrc, uc_str );
			SendDlgItemMessage( hWndDlg, IDC_TEXTBOX, WM_SETTEXT, NULL, (LPARAM)text );
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog( hWndDlg, SendDlgItemMessage( hWndDlg, IDC_MICROCODE, CB_GETCURSEL, 0, 0 ) );
					return TRUE;

				case IDCANCEL:
					EndDialog( hWndDlg, NONE );
					return TRUE;
			}
			break;
	}

	return FALSE;
}
#else // !__LINUX__
static int selectedMicrocode = -1;
static GtkWidget *microcodeWindow = 0;
static GtkWidget *microcodeList = 0;

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

static int MicrocodeDialog()
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

	snprintf( buf, 1024, "0x%8.8X", uc_crc );
	gtk_label_set_text( GTK_LABEL(crcLabel), buf );
	snprintf( buf, 1024, "0x%8.8X", uc_dcrc );
	gtk_label_set_text( GTK_LABEL(crcDataLabel), buf );
	gtk_label_set_text( GTK_LABEL(textLabel), uc_str );

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
#endif // __LINUX__

MicrocodeInfo *GBI_AddMicrocode()
{
	MicrocodeInfo *newtop = (MicrocodeInfo*)malloc( sizeof( MicrocodeInfo ) );

	newtop->lower = GBI.top;
	newtop->higher = NULL;

	if (GBI.top)
		GBI.top->higher = newtop;

	if (!GBI.bottom)
		GBI.bottom = newtop;

    GBI.top = newtop;

	GBI.numMicrocodes++;

	return newtop;
}

void GBI_Init()
{
	GBI.top = NULL;
	GBI.bottom = NULL;
	GBI.current = NULL;
	GBI.numMicrocodes = 0;

	for (u32 i = 0; i <= 0xFF; i++)
		GBI.cmd[i] = GBI_Unknown;
}

void GBI_Destroy()
{
	while (GBI.bottom)
	{
		MicrocodeInfo *newBottom = GBI.bottom->higher;

		if (GBI.bottom == GBI.top)
			GBI.top = NULL;

		free( GBI.bottom );

		GBI.bottom = newBottom;

		if (GBI.bottom)
			GBI.bottom->lower = NULL;

		GBI.numMicrocodes--;
	}
}

MicrocodeInfo *GBI_DetectMicrocode( u32 uc_start, u32 uc_dstart, u16 uc_dsize )
{
	MicrocodeInfo *current;

	for (int i = 0; i < GBI.numMicrocodes; i++)
	{
		current = GBI.top;

		while (current)
		{
			if ((current->address == uc_start) && (current->dataAddress == uc_dstart) && (current->dataSize == uc_dsize))
				return current;

			current = current->lower;
		}
	}

	current = GBI_AddMicrocode();

	current->address = uc_start;
	current->dataAddress = uc_dstart;
	current->dataSize = uc_dsize;
	current->NoN = FALSE;
	current->type = NONE;

	// See if we can identify it by CRC
	uc_crc = CRC_Calculate( 0xFFFFFFFF, &RDRAM[uc_start & 0x1FFFFFFF], 4096 );
	for (u32 i = 0; i < sizeof( specialMicrocodes ) / sizeof( SpecialMicrocodeInfo ); i++)
	{
		if (uc_crc == specialMicrocodes[i].crc)
		{
			current->type = specialMicrocodes[i].type;
			return current;
		}
	}

	// See if we can identify it by text
	char uc_data[2048];
	UnswapCopy( &RDRAM[uc_dstart & 0x1FFFFFFF], uc_data, 2048 );
	strcpy( uc_str, "Not Found" );

	for (u32 i = 0; i < 2048; i++)
	{
		if ((uc_data[i] == 'R') && (uc_data[i+1] == 'S') && (uc_data[i+2] == 'P'))
		{
			u32 j = 0;
			while (uc_data[i+j] > 0x0A)
			{
				uc_str[j] = uc_data[i+j];
				j++;
			}

			uc_str[j] = 0x00;

			int type = NONE;

			if (strncmp( &uc_str[4], "SW", 2 ) == 0)
			{
				type = F3D;
			}
			else if (strncmp( &uc_str[4], "Gfx", 3 ) == 0)
			{
				current->NoN = (strncmp( &uc_str[20], ".NoN", 4 ) == 0);

				if (strncmp( &uc_str[14], "F3D", 3 ) == 0)
				{
					if (uc_str[28] == '1')
						type = F3DEX;
					else if (uc_str[31] == '2')
						type = F3DEX2;
				}
				else if (strncmp( &uc_str[14], "L3D", 3 ) == 0)
				{
					if (uc_str[28] == '1')
						type = L3DEX;
					else if (uc_str[31] == '2')
						type = L3DEX2;
				}
				else if (strncmp( &uc_str[14], "S2D", 3 ) == 0)
				{
					if (uc_str[28] == '1')
						type = S2DEX;
					else if (uc_str[31] == '2')
						type = S2DEX2;
				}
			}

			if (type != NONE)
			{
				current->type = type;
				return current;
			}

			break;
		}
	}

	for (u32 i = 0; i < sizeof( specialMicrocodes ) / sizeof( SpecialMicrocodeInfo ); i++)
	{
		if (strcmp( uc_str, specialMicrocodes[i].text ) == 0)
		{
			current->type = specialMicrocodes[i].type;
			return current;
		}
	}

	// Let the user choose the microcode
#ifndef __LINUX__
	current->type = DialogBox( hInstance, MAKEINTRESOURCE( IDD_MICROCODEDLG ), hWnd, MicrocodeDlgProc );
#else // !__LINUX__
	printf( "glN64: Warning - unknown ucode!!!\n" );
	current->type = MicrocodeDialog();
#endif // __LINUX__
	return current;
}

void GBI_MakeCurrent( MicrocodeInfo *current )
{
	if (current != GBI.top)
	{
		if (current == GBI.bottom)
		{
			GBI.bottom = current->higher;
			GBI.bottom->lower = NULL;
		}
		else
		{
			current->higher->lower = current->lower;
			current->lower->higher = current->higher;
		}

		current->higher = NULL;
		current->lower = GBI.top;
		GBI.top->higher = current;
		GBI.top = current;
	}

	if (!GBI.current || (GBI.current->type != current->type))
	{
		for (int i = 0; i <= 0xFF; i++)
			GBI.cmd[i] = GBI_Unknown;

		RDP_Init();

		switch (current->type)
		{
			case F3D:		F3D_Init();		break;
			case F3DEX:		F3DEX_Init();	break;
			case F3DEX2:	F3DEX2_Init();	break;
			case L3D:		L3D_Init();		break;
			case L3DEX:		L3DEX_Init();	break;
			case L3DEX2:	L3DEX2_Init();	break;
			case S2DEX:		S2DEX_Init();	break;
			case S2DEX2:	S2DEX2_Init();	break;
			case F3DDKR:	F3DDKR_Init();	break;
			case F3DWRUS:	F3DWRUS_Init();	break;
			case F3DPD:		F3DPD_Init();	break;
		}
	}

	GBI.current = current;
}
