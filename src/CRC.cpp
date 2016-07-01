#include "Types.h"

#define CRC32_POLYNOMIAL     0x04C11DB7

unsigned int CRCTable[ 256 ];

u32 Reflect( u32 ref, char ch )
{
	u32 value = 0;

	// Swap bit 0 for bit 7
	// bit 1 for bit 6, etc.
	for (int i = 1; i < (ch + 1); i++)
	{
		if(ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

void CRC_BuildTable()
{
	u32 crc;

	for (int i = 0; i < 256; i++)
	{
		crc = Reflect( i, 8 ) << 24;
		for (int j = 0; j < 8; j++)
			crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);

		CRCTable[i] = Reflect( crc, 32 );
	}
}

u32 CRC_Calculate(void *buffer, u32 count)
{
	u8 *p;
	u32 crc = 0xffffffff;

	p = (u8*) buffer;

	while (count--)
		crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];

	return ~crc;
}

u32 Hash_CalculatePalette(void *buffer, u32 count)
{
	unsigned int i;
	u16 *data = (u16 *) buffer;
	u32 hash = 0xffffffff;

	count /= 4;
	for(i = 0; i < count; ++i) {
		hash += data[i << 2];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

u32 Hash_Calculate(u32 hash, void *buffer, u32 count)
{
	unsigned int i;
	u32 *data = (u32 *) buffer;

	count /= 4;
	for(i = 0; i < count; ++i) {
		hash += data[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}
