#include "GLideN64_Windows.h"
#include <stdio.h>
#include <commctrl.h>

#include "../Config.h"
#include "../GLideN64.h"
#include "../Resource.h"
#include "../RSP.h"
#include "../Textures.h"
#include "../OpenGL.h"

Config config;
HWND hConfigDlg;

const u32 uMegabyte = 1024U*1024U;

struct
{
	struct
	{
		DWORD width, height, bitDepth, refreshRate;
	} selected;

	DWORD bitDepth[4];

	struct
	{
		DWORD	width, height;
	} resolution[32];

	DWORD refreshRate[32];

	DWORD	numBitDepths;
	DWORD	numResolutions;
	DWORD	numRefreshRates;
} fullscreen;

#define numWindowedModes 12

struct
{
	WORD width, height;
	const char *description;
} windowedModes[12] = {
	{ 320, 240, "320 x 240" },
	{ 400, 300, "400 x 300" },
	{ 480, 360, "480 x 360" },
	{ 640, 480, "640 x 480" },
	{ 800, 600, "800 x 600" },
	{ 960, 720, "960 x 720" },
	{ 1024, 768, "1024 x 768" },
	{ 1152, 864, "1152 x 864" },
	{ 1280, 960, "1280 x 960" },
	{ 1280, 1024, "1280 x 1024" },
	{ 1440, 1080, "1440 x 1080" },
	{ 1600, 1200, "1600 x 1200" }
};

void Config_LoadConfig()
{
	DWORD value, size;

	HKEY hKey;

	RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\N64 Emulation\\DLL\\glN64", 0, KEY_READ, &hKey );

	if (hKey)
	{
		RegQueryValueEx( hKey, "Fullscreen Bit Depth", 0, NULL, (BYTE*)&config.video.fullscreenBits, &size );
		RegQueryValueEx( hKey, "Fullscreen Width", 0, NULL, (BYTE*)&config.video.fullscreenWidth, &size );
		RegQueryValueEx( hKey, "Fullscreen Height", 0, NULL, (BYTE*)&config.video.fullscreenHeight, &size );
		RegQueryValueEx( hKey, "Fullscreen Refresh", 0, NULL, (BYTE*)&config.video.fullscreenRefresh, &size );
		RegQueryValueEx( hKey, "Windowed Width", 0, NULL, (BYTE*)&config.video.windowedWidth, &size );
		RegQueryValueEx( hKey, "Windowed Height", 0, NULL, (BYTE*)&config.video.windowedHeight, &size );
		RegQueryValueEx( hKey, "Windowed Width", 0, NULL, (BYTE*)&config.video.windowedWidth, &size );
		RegQueryValueEx( hKey, "Force Bilinear", 0, NULL, (BYTE*)&value, &size );
		config.texture.forceBilinear = value ? TRUE : FALSE;

		RegQueryValueEx( hKey, "Write depth", 0, NULL, (BYTE*)&value, &size );
		config.frameBufferEmulation.copyDepthToRDRAM = value ? TRUE : FALSE;

		RegQueryValueEx( hKey, "Enable Fog", 0, NULL, (BYTE*)&value, &size );
		config.enableFog = value ? TRUE : FALSE;

		RegQueryValueEx( hKey, "Texture Cache Size", 0, NULL, (BYTE*)&value, &size );
		config.texture.maxBytes = value * uMegabyte;

		RegQueryValueEx( hKey, "Hardware Frame Buffer Textures", 0, NULL, (BYTE*)&value, &size );
		config.frameBufferEmulation.enable = value ? TRUE : FALSE;

		RegQueryValueEx( hKey, "Hardware lighting", 0, NULL, (BYTE*)&value, &size );
		config.enableHWLighting = value ? TRUE : FALSE;

		RegQueryValueEx( hKey, "Texture Bit Depth", 0, NULL, (BYTE*)&value, &size );
		config.texture.textureBitDepth = value;

		RegCloseKey( hKey );
	}
	else
	{
		config.enableFog = TRUE;
		config.video.windowedWidth = 640;
		config.video.windowedHeight = 480;
		config.video.fullscreenWidth = 640;
		config.video.fullscreenHeight = 480;
		config.video.fullscreenBits = 16;
		config.video.fullscreenRefresh = 60;
		config.texture.forceBilinear = FALSE;
		config.texture.maxBytes = 32 * uMegabyte;
		config.frameBufferEmulation.enable = FALSE;
		config.frameBufferEmulation.copyDepthToRDRAM = FALSE;
		config.texture.textureBitDepth = 1;
		config.enableHWLighting = FALSE;
	}

	// manually set frame bufer emulation options
	config.frameBufferEmulation.copyToRDRAM = FALSE;
	config.frameBufferEmulation.copyFromRDRAM = FALSE;
	config.frameBufferEmulation.ignoreCFB = TRUE;
	config.frameBufferEmulation.N64DepthCompare = FALSE;
	config.enableLOD = TRUE;
}

void Config_SaveConfig()
{
	DWORD value;
	HKEY hKey;

	RegCreateKeyEx( HKEY_CURRENT_USER, "Software\\N64 Emulation\\DLL\\glN64", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL );

	RegSetValueEx( hKey, "Fullscreen Bit Depth", 0, REG_DWORD, (BYTE*)&config.video.fullscreenBits, 4 );
	RegSetValueEx( hKey, "Fullscreen Width", 0, REG_DWORD, (BYTE*)&config.video.fullscreenWidth, 4 );
	RegSetValueEx( hKey, "Fullscreen Height", 0, REG_DWORD, (BYTE*)&config.video.fullscreenHeight, 4 );
	RegSetValueEx( hKey, "Fullscreen Refresh", 0, REG_DWORD, (BYTE*)&config.video.fullscreenRefresh, 4 );
	RegSetValueEx( hKey, "Windowed Width", 0, REG_DWORD, (BYTE*)&config.video.windowedWidth, 4 );
	RegSetValueEx( hKey, "Windowed Height", 0, REG_DWORD, (BYTE*)&config.video.windowedHeight, 4 );

	value = config.texture.forceBilinear ? 1 : 0;
	RegSetValueEx( hKey, "Force Bilinear", 0, REG_DWORD, (BYTE*)&value, 4 );

	value = config.frameBufferEmulation.copyDepthToRDRAM ? 1 : 0;
	RegSetValueEx( hKey, "Write depth", 0, REG_DWORD, (BYTE*)&value, 4 );

	value = config.enableFog ? 1 : 0;
	RegSetValueEx( hKey, "Enable Fog", 0, REG_DWORD, (BYTE*)&value, 4 );

	value = config.texture.maxBytes / uMegabyte;
	RegSetValueEx( hKey, "Texture Cache Size", 0, REG_DWORD, (BYTE*)&value, 4 );

	value = config.frameBufferEmulation.enable ? 1 : 0;
	RegSetValueEx( hKey, "Hardware Frame Buffer Textures", 0, REG_DWORD, (BYTE*)&value, 4 );

	value = config.enableHWLighting ? 1 : 0;
	RegSetValueEx( hKey, "Hardware lighting", 0, REG_DWORD, (BYTE*)&value, 4 );

	value = config.texture.textureBitDepth;
	RegSetValueEx( hKey, "Texture Bit Depth", 0, REG_DWORD, (BYTE*)&value, 4 );

	RegCloseKey( hKey );
}

void Config_ApplyDlgConfig( HWND hWndDlg )
{
	char text[256];
	LRESULT i;

	SendDlgItemMessage( hWndDlg, IDC_CACHEMEGS, WM_GETTEXT, 4, (LPARAM)text );
	config.texture.maxBytes = atol( text ) * uMegabyte;

	config.texture.forceBilinear = (SendDlgItemMessage( hWndDlg, IDC_FORCEBILINEAR, BM_GETCHECK, NULL, NULL ) == BST_CHECKED);
	config.frameBufferEmulation.copyDepthToRDRAM = (SendDlgItemMessage( hWndDlg, IDC_ENABLEDEPTHWRITE, BM_GETCHECK, NULL, NULL ) == BST_CHECKED);
	config.enableFog = (SendDlgItemMessage( hWndDlg, IDC_FOG, BM_GETCHECK, NULL, NULL ) == BST_CHECKED);

	config.video.fullscreenBits = fullscreen.bitDepth[SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_GETCURSEL, 0, 0 )];
	i = SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_GETCURSEL, 0, 0 );
	config.video.fullscreenWidth = fullscreen.resolution[i].width;
	config.video.fullscreenHeight = fullscreen.resolution[i].height;
	config.video.fullscreenRefresh = fullscreen.refreshRate[SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_GETCURSEL, 0, 0 )];

	i = SendDlgItemMessage( hWndDlg, IDC_TEXTUREBPP, CB_GETCURSEL, 0, 0 );
	config.texture.textureBitDepth = (int)i;

	i = SendDlgItemMessage( hWndDlg, IDC_WINDOWEDRES, CB_GETCURSEL, 0, 0 );
	config.video.windowedWidth = windowedModes[i].width;
	config.video.windowedHeight = windowedModes[i].height;

	config.frameBufferEmulation.enable = (SendDlgItemMessage( hWndDlg, IDC_FRAMEBUFFER, BM_GETCHECK, NULL, NULL ) == BST_CHECKED);
	config.enableHWLighting = (SendDlgItemMessage( hWndDlg, IDC_HWLIGHT, BM_GETCHECK, NULL, NULL ) == BST_CHECKED);

	if (!video().isFullscreen())
		video().setWindowSize(config.video.windowedWidth, config.video.windowedHeight);

	Config_SaveConfig();
}

void UpdateFullscreenConfig( HWND hWndDlg )
{
	DEVMODE deviceMode;
	int i;
	char text[256];

	memset( &fullscreen.bitDepth, 0, sizeof( fullscreen.bitDepth ) );
	memset( &fullscreen.resolution, 0, sizeof( fullscreen.resolution ) );
	memset( &fullscreen.refreshRate, 0, sizeof( fullscreen.refreshRate ) );
	fullscreen.numBitDepths = 0;
	fullscreen.numResolutions = 0;
	fullscreen.numRefreshRates = 0;

	i = 0;
	SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_RESETCONTENT, 0, 0 );
	while (EnumDisplaySettings( NULL, i, &deviceMode ) != 0)
	{
		DWORD j = 0;
		for (; j < fullscreen.numBitDepths; j++)
		{
			if (deviceMode.dmBitsPerPel == fullscreen.bitDepth[j])
				break;
		}

		if ((deviceMode.dmBitsPerPel != fullscreen.bitDepth[j]) && (deviceMode.dmBitsPerPel > 8))
		{
			fullscreen.bitDepth[fullscreen.numBitDepths] = deviceMode.dmBitsPerPel;
			sprintf( text, "%i bit", deviceMode.dmBitsPerPel );
			SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_ADDSTRING, 0, (LPARAM)text );

			if (fullscreen.selected.bitDepth == deviceMode.dmBitsPerPel)
				SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_SETCURSEL, fullscreen.numBitDepths, 0 );
			fullscreen.numBitDepths++;
		}

		i++;
	}

	if (SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_GETCURSEL, 0, 0 ) == CB_ERR)
		SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_SETCURSEL, SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_GETCOUNT, 0, 0 ) - 1, 0 );


	i = 0;
	SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_RESETCONTENT, 0, 0 );
	while (EnumDisplaySettings( NULL, i, &deviceMode ) != 0)
	{
		DWORD j = 0;
		for (; j < fullscreen.numResolutions; j++)
		{
			if ((deviceMode.dmPelsWidth == fullscreen.resolution[j].width) &&
				(deviceMode.dmPelsHeight == fullscreen.resolution[j].height))
			{
				break;
			}
		}
		if (((deviceMode.dmPelsWidth != fullscreen.resolution[j].width) ||
			(deviceMode.dmPelsHeight != fullscreen.resolution[j].height)) &&
			(deviceMode.dmBitsPerPel != fullscreen.selected.bitDepth))
		{
			fullscreen.resolution[fullscreen.numResolutions].width = deviceMode.dmPelsWidth;
			fullscreen.resolution[fullscreen.numResolutions].height = deviceMode.dmPelsHeight;
			sprintf( text, "%i x %i", deviceMode.dmPelsWidth, deviceMode.dmPelsHeight );
			SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_ADDSTRING, 0, (LPARAM)text );

			if ((fullscreen.selected.width == deviceMode.dmPelsWidth) &&
				(fullscreen.selected.height == deviceMode.dmPelsHeight))
				SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_SETCURSEL, fullscreen.numResolutions, 0 );

			fullscreen.numResolutions++;
		}
		i++;
	}

	if (SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_GETCURSEL, 0, 0 ) == CB_ERR)
		SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_SETCURSEL, SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_GETCOUNT, 0, 0 ) - 1, 0 );

	i = 0;
	SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_RESETCONTENT, 0, 0 );
	while (EnumDisplaySettings( NULL, i, &deviceMode ) != 0)
	{
		DWORD j = 0;
		for (; j < fullscreen.numRefreshRates; j++)
		{
			if ((deviceMode.dmDisplayFrequency == fullscreen.refreshRate[j]))
				break;
		}
		if ((deviceMode.dmDisplayFrequency != fullscreen.refreshRate[j]) &&
				(deviceMode.dmPelsWidth == fullscreen.selected.width) &&
				(deviceMode.dmPelsHeight == fullscreen.selected.height) &&
				(deviceMode.dmBitsPerPel == fullscreen.selected.bitDepth))
		{
			fullscreen.refreshRate[j] = deviceMode.dmDisplayFrequency;
			sprintf( text, "%i Hz", deviceMode.dmDisplayFrequency );
			SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_ADDSTRING, 0, (LPARAM)text );

			if (fullscreen.selected.refreshRate == deviceMode.dmDisplayFrequency)
				SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_SETCURSEL, fullscreen.numRefreshRates, 0 );

			fullscreen.numRefreshRates++;
		}
		i++;
	}
	if (SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_GETCURSEL, 0, 0 ) == CB_ERR)
		SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_SETCURSEL, SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_GETCOUNT, 0, 0 ) - 1, 0 );
}

BOOL CALLBACK ConfigDlgProc( HWND hWndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	char text[256];
	LRESULT	 i;
	DEVMODE deviceMode;
	switch (message)
	{
		case WM_INITDIALOG:
			hConfigDlg = hWndDlg;

			fullscreen.selected.width = config.video.fullscreenWidth;
			fullscreen.selected.height = config.video.fullscreenHeight;
			fullscreen.selected.bitDepth = config.video.fullscreenBits;
			fullscreen.selected.refreshRate = config.video.fullscreenRefresh;
			UpdateFullscreenConfig( hWndDlg );

			EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &deviceMode );

			// Fill windowed mode resolution
			for (i = 0; i < numWindowedModes; i++)
			{
				if ((deviceMode.dmPelsWidth > windowedModes[i].width) &&
					(deviceMode.dmPelsHeight > windowedModes[i].height))
				{
					SendDlgItemMessage( hWndDlg, IDC_WINDOWEDRES, CB_ADDSTRING, 0, (LPARAM)windowedModes[i].description );
					if ((config.video.windowedWidth == windowedModes[i].width) &&
						(config.video.windowedHeight == windowedModes[i].height))
						SendDlgItemMessage( hWndDlg, IDC_WINDOWEDRES, CB_SETCURSEL, i, 0 );
				}
			}
			SendDlgItemMessage( hWndDlg, IDC_ENABLEDEPTHWRITE, BM_SETCHECK, config.frameBufferEmulation.copyDepthToRDRAM ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL );
			// Set forced bilinear check box
			SendDlgItemMessage( hWndDlg, IDC_FORCEBILINEAR, BM_SETCHECK, config.texture.forceBilinear ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL );
			SendDlgItemMessage( hWndDlg, IDC_TEXTUREBPP, CB_ADDSTRING, 0, (LPARAM)"16-bit only (faster)" );
			SendDlgItemMessage( hWndDlg, IDC_TEXTUREBPP, CB_ADDSTRING, 0, (LPARAM)"16-bit and 32-bit (normal)" );
			SendDlgItemMessage( hWndDlg, IDC_TEXTUREBPP, CB_ADDSTRING, 0, (LPARAM)"32-bit only (best for 2xSaI)" );
			SendDlgItemMessage( hWndDlg, IDC_TEXTUREBPP, CB_SETCURSEL, config.texture.textureBitDepth, 0 );
			// Enable/disable fog
			SendDlgItemMessage( hWndDlg, IDC_FOG, BM_SETCHECK, config.enableFog ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL );
			SendDlgItemMessage( hWndDlg, IDC_FRAMEBUFFER, BM_SETCHECK, config.frameBufferEmulation.enable ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL );

			SendDlgItemMessage( hWndDlg, IDC_HWLIGHT, BM_SETCHECK, config.enableHWLighting ? (LPARAM)BST_CHECKED : (LPARAM)BST_UNCHECKED, NULL );

			_ltoa( config.texture.maxBytes / uMegabyte, text, 10 );
			SendDlgItemMessage( hWndDlg, IDC_CACHEMEGS, WM_SETTEXT, NULL, (LPARAM)text );

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					Config_ApplyDlgConfig( hWndDlg );
					EndDialog( hWndDlg, wParam );
					hConfigDlg = NULL;
					return TRUE;

				case IDCANCEL:
					EndDialog( hWndDlg, wParam );
					hConfigDlg = NULL;
					return TRUE;

				case IDC_FULLSCREENBITDEPTH:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						fullscreen.selected.bitDepth = fullscreen.bitDepth[SendDlgItemMessage( hWndDlg, IDC_FULLSCREENBITDEPTH, CB_GETCURSEL, 0, 0 )];

						UpdateFullscreenConfig( hWndDlg );
					}
					break;
				case IDC_FULLSCREENRES:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						i = SendDlgItemMessage( hWndDlg, IDC_FULLSCREENRES, CB_GETCURSEL, 0, 0 );
						fullscreen.selected.width = fullscreen.resolution[i].width;
						fullscreen.selected.height = fullscreen.resolution[i].height;

						UpdateFullscreenConfig( hWndDlg );
					}
					break;
				case IDC_FULLSCREENREFRESH:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						fullscreen.selected.refreshRate = fullscreen.refreshRate[SendDlgItemMessage( hWndDlg, IDC_FULLSCREENREFRESH, CB_GETCURSEL, 0, 0 )];

						UpdateFullscreenConfig( hWndDlg );
					}
					break;
			}
	}
	return FALSE;
}

void Config_DoConfig(HWND hParent)
{
	if (!hConfigDlg)
		DialogBox( hInstance, MAKEINTRESOURCE(IDD_CONFIGDLG), hParent, (DLGPROC)ConfigDlgProc );
}
