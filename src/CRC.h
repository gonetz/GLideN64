#include "Types.h"

void CRC_BuildTable();

u32 CRC_Calculate(void *buffer, u32 count);
u32 Hash_CalculatePalette(void *buffer, u32 count);
u32 Hash_Calculate(u32 hash, void *buffer, u32 count);

