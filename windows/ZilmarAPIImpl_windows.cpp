#include "GLideN64_Windows.h"
#include "../PluginAPI.h"
#include "../GLideN64.h"
#include "../OpenGL.h"

LONG		windowedStyle;
LONG		windowedExStyle;
RECT		windowedRect;
HMENU		windowedMenu;

void PluginAPI::ChangeWindow()
{
	if (!OGL.fullscreen) {
		DEVMODE fullscreenMode;
		memset( &fullscreenMode, 0, sizeof(DEVMODE) );
		fullscreenMode.dmSize = sizeof(DEVMODE);
		fullscreenMode.dmPelsWidth			= OGL.fullscreenWidth;
		fullscreenMode.dmPelsHeight			= OGL.fullscreenHeight;
		fullscreenMode.dmBitsPerPel			= OGL.fullscreenBits;
		fullscreenMode.dmDisplayFrequency	= OGL.fullscreenRefresh;
		fullscreenMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		if (ChangeDisplaySettings( &fullscreenMode, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL) {
			MessageBox( NULL, "Failed to change display mode", pluginName, MB_ICONERROR | MB_OK );
			return;
		}

		ShowCursor( FALSE );

		windowedMenu = GetMenu( hWnd );

		if (windowedMenu)
			SetMenu( hWnd, NULL );

		if (hStatusBar)
			ShowWindow( hStatusBar, SW_HIDE );

		windowedExStyle = GetWindowLong( hWnd, GWL_EXSTYLE );
		windowedStyle = GetWindowLong( hWnd, GWL_STYLE );

		SetWindowLong( hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST );
		SetWindowLong( hWnd, GWL_STYLE, WS_POPUP );

		GetWindowRect( hWnd, &windowedRect );

		OGL.fullscreen = TRUE;
		OGL_ResizeWindow();
	} else {
		ChangeDisplaySettings( NULL, 0 );

		ShowCursor( TRUE );

		if (windowedMenu)
			SetMenu( hWnd, windowedMenu );

		if (hStatusBar)
			ShowWindow( hStatusBar, SW_SHOW );

		SetWindowLong( hWnd, GWL_STYLE, windowedStyle );
		SetWindowLong( hWnd, GWL_EXSTYLE, windowedExStyle );
		SetWindowPos( hWnd, NULL, windowedRect.left, windowedRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

		OGL.fullscreen = FALSE;
		OGL_ResizeWindow();
	}
}

void PluginAPI::DllAbout(HWND _hParent)
{
	MessageBox(_hParent, "GLideN64 alpha. Based on Orkin's glN64 v0.4", pluginName, MB_OK | MB_ICONINFORMATION );
}
