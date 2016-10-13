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
	unsigned int i;
	u32 *data = (u32 *) buffer;

	count /= 4;
	for(i = 0; i < count; ++i) {
		crc += data[i];
		crc += (crc << 10);
		crc ^= (crc >> 6);
	}

	crc += (crc << 3);
	crc ^= (crc >> 11);
	crc += (crc << 15);
	return crc;
}

u32 CRC_CalculatePalette( u32 crc, const void * buffer, u32 count )
{
	unsigned int i;
	u16 *data = (u16 *) buffer;

	count /= 4;
	for(i = 0; i < count; ++i) {
		crc += data[i << 2];
		crc += (crc << 10);
		crc ^= (crc >> 6);
	}

	crc += (crc << 3);
	crc ^= (crc >> 11);
	crc += (crc << 15);
	return crc;
}
