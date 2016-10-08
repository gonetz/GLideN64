#include "CRC.h"

#define CRC32_POLYNOMIAL     0x04C11DB7

unsigned int CRCTable[ 256 ];

u32 Reflect( u32 ref, char ch )
{
	 u32 value = 0;

	 // Swap bit 0 for bit 7
	 // bit 1 for bit 6, etc.
	 for (int i = 1; i < (ch + 1); ++i) {
		  if(ref & 1)
			value |= 1 << (ch - i);
		  ref >>= 1;
	 }
	 return value;
}

void CRC_BuildTable()
{
	u32 crc;

	for (int i = 0; i < 256; ++i) {
		crc = Reflect( i, 8 ) << 24;
		for (int j = 0; j < 8; ++j)
			crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);

		CRCTable[i] = Reflect( crc, 32 );
	}
}

u32 CRC_Calculate_Strict( u32 crc, const void * buffer, u32 count )
{
	u8 *p;
	u32 orig = crc;

	p = (u8*) buffer;
	while (count--)
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

	return crc ^ orig;
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
