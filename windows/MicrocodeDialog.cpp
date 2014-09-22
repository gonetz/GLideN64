#include "GLideN64_Windows.h"
#include <commctrl.h>
#include <stdio.h>

#include "../GBI.h"
#include "../Resource.h"

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

INT_PTR CALLBACK MicrocodeDlgProc( HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			for (int i = 0; i < numMicrocodeTypes; i++) {
				SendDlgItemMessage( hWndDlg, IDC_MICROCODE, CB_ADDSTRING, 0, (LPARAM)MicrocodeTypes[i] );
			}
			SendDlgItemMessage( hWndDlg, IDC_MICROCODE, CB_SETCURSEL, 0, 0 );

			char text[1024];
			sprintf( text, "Microcode CRC:\t\t0x%08x\r\nMicrocode Text:\t\t%s", uc_crc, uc_str );
			SendDlgItemMessage( hWndDlg, IDC_TEXTBOX, WM_SETTEXT, NULL, (LPARAM)text );
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
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

int MicrocodeDialog(unsigned int _crc, const char * _str)
{
	uc_crc = _crc;
	uc_str = _str;
	return DialogBox( hInstance, MAKEINTRESOURCE( IDD_MICROCODEDLG ), hWnd, MicrocodeDlgProc );
}
