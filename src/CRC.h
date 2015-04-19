#include "Types.h"

void CRC_BuildTable();

// CRC32
u32 CRC_Calculate( u32 crc, const void *buffer, u32 count );
u32 CRC_CalculatePalette( u32 crc, const void *buffer, u32 count );
// Fast checksum calculation from Glide64
u32 textureCRC(u8 * addr, u32 height, u32 stride);
