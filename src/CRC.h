#include "Types.h"

void CRC_Init();

// Strict checksum calculation. usually CRC32
u32 CRC_Calculate_Strict( u32 crc, const void *buffer, u32 count );
u32 CRC_Calculate( u32 crc, const void *buffer, u32 count );
u32 CRC_CalculatePalette( u32 crc, const void *buffer, u32 count );
