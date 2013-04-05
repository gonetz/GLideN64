#if !defined( DEBUG_H ) && defined( _DEBUG )
#define DEBUG_H

#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif
#include <stdio.h>

#define		DEBUG_LOW		0x1000
#define		DEBUG_MEDIUM	0x2000
#define		DEBUG_HIGH		0x4000
#define		DEBUG_DETAIL	0x8000

#define		DEBUG_HANDLED	0x0001
#define		DEBUG_UNHANDLED 0x0002
#define		DEBUG_IGNORED	0x0004
#define		DEBUG_UNKNOWN	0x0008
#define		DEBUG_ERROR		0x0010
#define		DEBUG_COMBINE	0x0020
#define		DEBUG_TEXTURE	0x0040
#define		DEBUG_VERTEX	0x0080
#define		DEBUG_TRIANGLE	0x0100
#define		DEBUG_MATRIX	0x0200

struct DebugInfo
{
	WORD show, level;
	BOOL detail, paused, step;
	struct
	{
		DWORD pci, pc, cmd, w0, w1;
	} rsp;
};

extern DebugInfo Debug;

void OpenDebugDlg();
void CloseDebugDlg();
void DebugRSPState( DWORD pci, DWORD pc, DWORD cmd, DWORD w0, DWORD w1 );
void DebugMsg( WORD type, LPCSTR format, ... );
void StartDump( char *filename );
void EndDump();

#endif // DEBUG_H
