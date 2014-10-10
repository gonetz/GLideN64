#if !defined( DEBUG_H ) && defined( USE_DEBUG )
#define DEBUG_H

#include "Types.h"
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
	u16 show, level;
	u8 detail, paused, step;
	struct
	{
		u32 pci, pc, cmd, w0, w1;
	} rsp;
};

extern DebugInfo Debug;

void OpenDebugDlg();
void CloseDebugDlg();
void DebugRSPState( u32 pci, u32 pc, u32 cmd, u32 w0, u32 w1 );
void DebugMsg( u16 type, const char * format, ... );
void StartDump( char *filename );
void EndDump();
#else
#define DebugMsg(A, ...)
#define DebugRSPState(A, ...)
#endif // DEBUG_H
