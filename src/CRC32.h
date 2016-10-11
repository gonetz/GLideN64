#include "Types.h"

void CRC32_BuildTable();

u32 CRC32_Calculate( u32 crc, const void *buffer, u32 count );
u32 CRC32_CalculatePalette( u32 crc, const void *buffer, u32 count );
