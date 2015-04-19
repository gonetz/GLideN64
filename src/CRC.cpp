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

u32 textureCRC(u8 * addr, u32 height, u32 stride)
{
	const u32 width = stride / 8;
	const u32 line = stride % 8;
	u64 crc = 0;
	u64 twopixel_crc;

	u32 *  pixelpos = (u32*)addr;
	for (; height; height--) {
		int col = 0;
		for (u32 i = width; i; --i) {
			twopixel_crc = i * ((u64)(pixelpos[1] & 0xFFFEFFFE) + (u64)(pixelpos[0] & 0xFFFEFFFE) + crc);
			crc = (twopixel_crc >> 32) + twopixel_crc;
			pixelpos += 2;
		}
		crc = (height * crc >> 32) + height * crc;
		pixelpos = (u32*)((u8*)pixelpos + line);
	}

	return crc&0xFFFFFFFF;
}
