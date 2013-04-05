#include <windows.h>
#include <stdio.h>
#include <process.h>
#include "glN64.h"
#include "Debug.h"
#include "resource.h"
#include "RDP.h"
#include "RSP.h"
#include "RichEdit.h"

DebugInfo Debug;

CHARFORMAT handledFormat = 
{ 
	sizeof( CHARFORMAT ),
	CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE,
	NULL,
	200,
	0,
	RGB( 0, 0, 0 ),
	NULL,
	NULL,
	(TCHAR*)"Courier New"
};

CHARFORMAT unknownFormat = 
{ 
	sizeof( CHARFORMAT ),
	CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE,
	NULL,
	200,
	0,
	RGB( 128, 128, 0 ),
	NULL,
	NULL,
	(TCHAR*)"Courier New"
};

CHARFORMAT errorFormat = 
{ 
	sizeof( CHARFORMAT ),
	CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE,
	NULL,
	200,
	0,
	RGB( 128, 0, 0 ),
	NULL,
	NULL,
	(TCHAR*)"Courier New"
};

CHARFORMAT detailFormat = 
{ 
	sizeof( CHARFORMAT ),
	CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE,
	NULL,
	200,
	0,
	RGB( 0, 128, 0 ),
	NULL,
	NULL,
	(TCHAR*)"Courier New"
};

HWND hDebugDlg;
BOOL DumpMessages;
FILE *dumpFile;
char dumpFilename[256];

BOOL CALLBACK DebugDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg) 
    { 
		case WM_INITDIALOG:
			SendDlgItemMessage( hDlg, IDC_DEBUGEDIT, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&handledFormat );
			break;

        case WM_COMMAND: 
            switch (LOWORD(wParam)) 
            { 
/*                case IDC_DUMP: 
					RSP.dumpNextDL = TRUE;
                    return TRUE; 
				case IDC_VERIFYCACHE:
					if (!TextureCache_Verify())
						MessageBox( NULL, "Texture cache chain is currupted!", "glNintendo64()", MB_OK );
					else
						MessageBox( NULL, "Texture cache chain verified", "glNintendo64()", MB_OK );
					return TRUE;*/
				case IDC_DEBUGDETAIL:
					Debug.detail = (SendDlgItemMessage( hDlg, IDC_DEBUGDETAIL, BM_GETCHECK, NULL, NULL ) == BST_CHECKED);
					break;
				case IDC_DEBUGHIGH:
					if (SendDlgItemMessage( hDlg, IDC_DEBUGHIGH, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.level = DEBUG_HIGH;
					break;
				case IDC_DEBUGMEDIUM:
					if (SendDlgItemMessage( hDlg, IDC_DEBUGMEDIUM, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.level = DEBUG_MEDIUM;
					break;
				case IDC_DEBUGLOW:
					if (SendDlgItemMessage( hDlg, IDC_DEBUGLOW, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.level = DEBUG_LOW;
					break;
				case IDC_SHOWHANDLED:
					if (SendDlgItemMessage( hDlg, IDC_SHOWHANDLED, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_HANDLED;
					else
						Debug.show &= ~DEBUG_HANDLED;
					break;
				case IDC_SHOWUNHANDLED:
					if (SendDlgItemMessage( hDlg, IDC_SHOWUNHANDLED, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_UNHANDLED;
					else
						Debug.show &= ~DEBUG_UNHANDLED;
					break;
				case IDC_SHOWUNKNOWN:
					if (SendDlgItemMessage( hDlg, IDC_SHOWUNKNOWN, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_UNKNOWN;
					else
						Debug.show &= ~DEBUG_UNKNOWN;
					break;
				case IDC_SHOWIGNORED:
					if (SendDlgItemMessage( hDlg, IDC_SHOWIGNORED, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_IGNORED;
					else
						Debug.show &= ~DEBUG_IGNORED;
					break;
				case IDC_SHOWERRORS:
					if (SendDlgItemMessage( hDlg, IDC_SHOWERRORS, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_ERROR;
					else
						Debug.show &= ~DEBUG_ERROR;
					break;
				case IDC_SHOWCOMBINE:
					if (SendDlgItemMessage( hDlg, IDC_SHOWCOMBINE, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_COMBINE;
					else
						Debug.show &= ~DEBUG_COMBINE;
					break;
				case IDC_SHOWTEXTURE:
					if (SendDlgItemMessage( hDlg, IDC_SHOWTEXTURE, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_TEXTURE;
					else
						Debug.show &= ~DEBUG_TEXTURE;
					break;
				case IDC_SHOWVERTEX:
					if (SendDlgItemMessage( hDlg, IDC_SHOWVERTEX, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_VERTEX;
					else
						Debug.show &= ~DEBUG_VERTEX;
					break;
				case IDC_SHOWTRIANGLE:
					if (SendDlgItemMessage( hDlg, IDC_SHOWTRIANGLE, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_TRIANGLE;
					else
						Debug.show &= ~DEBUG_TRIANGLE;
					break;
				case IDC_SHOWMATRIX:
					if (SendDlgItemMessage( hDlg, IDC_SHOWMATRIX, BM_GETCHECK, NULL, NULL ) == BST_CHECKED)
						Debug.show |= DEBUG_MATRIX;
					else
						Debug.show &= ~DEBUG_MATRIX;
					break;
				case IDC_PAUSE:
					Debug.paused = TRUE;
					break;
				case IDC_RUN:
					Debug.paused = FALSE;
					break;
				case IDC_STEP:
					Debug.step = TRUE;
					break;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
    } 

	return FALSE;
}

void DebugDlgThreadFunc( void* )
{
	LoadLibrary( "RichEd20.dll" );
	hDebugDlg = CreateDialog( hInstance, MAKEINTRESOURCE(IDD_DEBUGDLG), hWnd, DebugDlgProc );

	MSG msg;

	while (GetMessage( &msg, hDebugDlg, 0, 0 ) != 0)
	{ 
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void OpenDebugDlg()
{
	DumpMessages = FALSE;
/*	hDebugDlg = CreateDialog( hInstance, MAKEINTRESOURCE(IDD_DEBUGDLG), NULL, DebugDlgProc );

	MSG msg;

	while (GetMessage( &msg, NULL, 0, 0)
	{
		if (IsWindow(hwndGoto)
			IsDialogMessage( hwndGoto, &msg );
	}
	_endthread();*/
	_beginthread( DebugDlgThreadFunc, 255, NULL );
}

void CloseDebugDlg()
{
	DestroyWindow( hDebugDlg );
}

void StartDump( char *filename )
{
	DumpMessages = TRUE;
	strcpy( dumpFilename, filename );
	dumpFile = fopen( filename, "w" );
	fclose( dumpFile );
}

void EndDump()
{
	DumpMessages = FALSE;
	fclose( dumpFile );
}

void DebugRSPState( DWORD pci, DWORD pc, DWORD cmd, DWORD w0, DWORD w1 )
{
	Debug.rsp.pci = pci;
	Debug.rsp.pc = pc;
	Debug.rsp.cmd = cmd;
	Debug.rsp.w0 = w0;
	Debug.rsp.w1 = w1;
}

void DebugMsg( WORD type, LPCSTR format, ... )
{
	char text[256];

	va_list va;
 	va_start( va, format );

	if (DumpMessages)
	{
		dumpFile = fopen( dumpFilename, "a+" ); 
		vfprintf( dumpFile, format, va );
		fclose( dumpFile );
	}

	if ((((type & DEBUG_DETAIL) && Debug.detail) || ((type & 0xF000) == Debug.level)) && (type & 0x0FFF & Debug.show))
	{
		for (int i = 0; i < 4 * RSP.PCi; i++)
			text[i] = ' ';

		vsprintf( text + 4 * RSP.PCi, format, va );

		INT length = SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, WM_GETTEXTLENGTH, 0, 0 );
		INT lengthLimit = SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_GETLIMITTEXT, 0, 0 );

		if ((length + strlen( text )) > lengthLimit)
		{
			INT lineLength = SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_LINELENGTH, 0, 0 );
			SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_SETSEL, 0, lineLength + 1 );
			SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)"" );
		}

		SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_SETSEL, length, length );

		if (type & DEBUG_DETAIL)
			SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&detailFormat );
		else if (type & DEBUG_ERROR)
			SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&errorFormat );
		else if (type & DEBUG_UNKNOWN)
			SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&unknownFormat );
		else
			SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&handledFormat );

		SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)text );
		SendDlgItemMessage( hDebugDlg, IDC_DEBUGEDIT, EM_LINESCROLL, 0, 1 );

		sprintf( text, "%02X", Debug.rsp.pci );
		SendDlgItemMessage( hDebugDlg, IDC_PCI, WM_SETTEXT, NULL, (LPARAM)text );

		sprintf( text, "%08X", Debug.rsp.pc );
		SendDlgItemMessage( hDebugDlg, IDC_PC, WM_SETTEXT, NULL, (LPARAM)text );

		sprintf( text, "%02X", Debug.rsp.cmd );
		SendDlgItemMessage( hDebugDlg, IDC_CMD, WM_SETTEXT, NULL, (LPARAM)text );

		sprintf( text, "%08X", Debug.rsp.w0 );
		SendDlgItemMessage( hDebugDlg, IDC_W0, WM_SETTEXT, NULL, (LPARAM)text );

		sprintf( text, "%08X", Debug.rsp.w1 );
		SendDlgItemMessage( hDebugDlg, IDC_W1, WM_SETTEXT, NULL, (LPARAM)text );
	}
	va_end( va );
}
