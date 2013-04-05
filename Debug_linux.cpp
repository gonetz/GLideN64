#include "Debug.h"

#ifdef DEBUG

#include <stdarg.h>
#include <stdio.h>


DebugInfo Debug;
static bool dumpMessages = FALSE;
static char dumpFilename[1024];

void
OpenDebugDlg()
{
}


void
CloseDebugDlg()
{
}


void
DebugRSPState( DWORD pci, DWORD pc, DWORD cmd, DWORD w0, DWORD w1 )
{
}


void
DebugMsg( WORD type, LPCSTR format, ... )
{
	char text[1024];

	va_list va;
	va_start( va, format );
	vsprintf( text, format, va );
	va_end( va );

	if (dumpMessages)
	{
		FILE *f = fopen( dumpFilename, "a" );
		if (f != 0)
		{
			fprintf( f, text );
			fclose( f );
		}
	}
	fprintf( stderr, "glN64 Debug: %s", text );
}


void
StartDump( char *filename )
{
	dumpMessages = TRUE;
	strcpy( dumpFilename, filename );
	FILE *f = fopen( dumpFilename, "w" );
	if (f != 0)
		fclose( f );
}


void EndDump()
{
	dumpMessages = FALSE;
}

#endif // DEBUG
