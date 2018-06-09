#include "RSP.h"
#include "GBI.h"

void RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
    s32u32 value;

    struct _N64Matrix
    {
        s16 integer[4][4];
        u16 fraction[4][4];
    } *n64Mat = (struct _N64Matrix *)&RDRAM[address];

    int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
        {
            value.s = (s32)n64Mat->integer[i][j^1] << 16;
            value.u += n64Mat->fraction[i][j^1];
            mtx[i][j] = _FIXED2FLOAT(value.s,16);
        }
}
