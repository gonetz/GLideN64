#ifndef RSP_H
#define RSP_H

#ifndef __LINUX__
#include <windows.h>
#else
#include "winlnxdefs.h"
#include <SDL/SDL.h>
#include  <SDL/SDL_thread.h>
#endif
#include "N64.h"
#include "GBI.h"
#include "gSP.h"
#include "Types.h"

typedef struct
{
	u32 PC[18], PCi, busy, halt, close, DList, uc_start, uc_dstart, cmd, nextCmd, count;
} RSPInfo;

extern RSPInfo RSP;

#define RSP_SegmentToPhysical( segaddr ) ((gSP.segment[(segaddr >> 24) & 0x0F] + (segaddr & 0x00FFFFFF)) & 0x00FFFFFF)

void RSP_Init();
void RSP_ProcessDList();
void RSP_LoadMatrix( f32 mtx[4][4], u32 address );

#endif
