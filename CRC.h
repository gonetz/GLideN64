#include "Types.h"

void CRC_BuildTable();

u32 CRC_Calculate( u32 crc, void *buffer, u32 count );
u32 CRC_CalculatePalette( u32 crc, void *buffer, u32 count );
