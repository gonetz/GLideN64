#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // __LINUX__

void CRC_BuildTable();

DWORD CRC_Calculate( DWORD crc, void *buffer, DWORD count );
DWORD CRC_CalculatePalette( DWORD crc, void *buffer, DWORD count );
