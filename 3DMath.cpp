#include <math.h>
#include "3DMath.h"

void MultMatrix(float m0[4][4], float m1[4][4], float dest[4][4])
{
    int i;
    for (i = 0; i < 4; i++)
    {
        dest[0][i] = m0[0][i]*m1[0][0] + m0[1][i]*m1[0][1] + m0[2][i]*m1[0][2] + m0[3][i]*m1[0][3];
        dest[1][i] = m0[0][i]*m1[1][0] + m0[1][i]*m1[1][1] + m0[2][i]*m1[1][2] + m0[3][i]*m1[1][3];
        dest[2][i] = m0[0][i]*m1[2][0] + m0[1][i]*m1[2][1] + m0[2][i]*m1[2][2] + m0[3][i]*m1[2][3];
        dest[3][i] = m0[3][i]*m1[3][3] + m0[2][i]*m1[3][2] + m0[1][i]*m1[3][1] + m0[0][i]*m1[3][0];
    }
}

void MultMatrix2(float m0[4][4], float m1[4][4])
{
    float dst[4][4];
    MultMatrix(m0, m1, dst);
    memcpy( m0, dst, sizeof(float) * 16 );
}

void TransformVectorNormalize(float vec[3], float mtx[4][4])
{
    float len;

	float vres[3];
	vres[0] = mtx[0][0] * vec[0]
		   + mtx[1][0] * vec[1]
		   + mtx[2][0] * vec[2];
	vres[1] = mtx[0][1] * vec[0]
		   + mtx[1][1] * vec[1]
		   + mtx[2][1] * vec[2];
	vres[2] = mtx[0][2] * vec[0]
		   + mtx[1][2] * vec[1]
		   + mtx[2][2] * vec[2];
	memcpy(vec, vres, sizeof(float)*3);
    len = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
    if (len != 0.0)
    {
        len = sqrtf(len);
        vec[0] /= len;
        vec[1] /= len;
        vec[2] /= len;
    }
}
