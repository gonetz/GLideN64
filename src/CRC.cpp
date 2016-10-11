#include "CRC.h"
#include "CRC32.h"

void CRC_Init()
{
	CRC32_BuildTable();
}

u32 CRC_Calculate_Strict( u32 crc, const void * buffer, u32 count )
{
	return CRC32_Calculate(crc, buffer, count);
}

u32 CRC_Calculate( u32 crc, const void * buffer, u32 count )
{
	return CRC32_Calculate(crc, buffer, count);
}

u32 CRC_CalculatePalette(u32 crc, const void * buffer, u32 count )
{
	return CRC32_CalculatePalette(crc, buffer, count);
}
