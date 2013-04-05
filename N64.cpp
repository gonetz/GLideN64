#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // __LINUX__
#include "N64.h"
#include "Types.h"

u8 *DMEM;
u8 *IMEM;
u64 TMEM[512];
u8 *RDRAM;
u32 RDRAMSize;

N64Regs REG;
