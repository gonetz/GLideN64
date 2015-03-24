#include "Types.h"

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

u32 CRC_Calculate( u32 crc, const void * buffer, u32 count )
{
	u8 *p;
	u32 orig = crc;

	p = (u8*) buffer;
	while (count--)
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

	return crc ^ orig;
}

u32 CRC_CalculatePalette(u32 crc, const void * buffer, u32 count )
{
	u8 *p;
	u32 orig = crc;

	p = (u8*) buffer;
	while (count--) {
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

		p += 6;
	}

	return crc ^ orig;
}

u32 Adler32(u32 crc, const void *buffer, u32 count)
{
	register u32 s1 = crc & 0xFFFF;
	register u32 s2 = (crc >> 16) & 0xFFFF;
	int k;
	const u8 *Buffer = (const u8*)buffer;

	if (Buffer == NULL)
		return 0;

	while (count > 0) {
		/* 5552 is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
		k = (count < 5552 ? count : 5552);
		count -= k;
		while (k--) {
			s1 += *Buffer++;
			s2 += s1;
		}
		/* 65521 is the largest prime smaller than 65536 */
		s1 %= 65521;
		s2 %= 65521;
	}

	return (s2 << 16) | s1;
}
