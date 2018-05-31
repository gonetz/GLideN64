#include "RSP.h"
#include "GBI.h"

void RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
    struct _N64Matrix
    {
        s16 integer[4][4];
        u16 fraction[4][4];
    } *n64Mat = (struct _N64Matrix *)&RDRAM[address];

    int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            mtx[i][j] = (f32)(n64Mat->integer[i][j^1]) + _FIXED2FLOAT(n64Mat->fraction[i][j^1],16);
}
