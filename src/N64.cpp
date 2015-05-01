#include "N64.h"

u8 *HEADER;
u8 *DMEM;
u8 *IMEM;
u64 TMEM[TMEM_SIZE];
u8 *RDRAM;

u32 RDRAMSize;

N64Regs REG;

bool ConfigOpen = false;
