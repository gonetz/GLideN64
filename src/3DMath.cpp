#include <math.h>
#include "3DMath.h"

#ifdef WIN32_ASM
#undef WIN32_ASM
#endif

Mtx MultMatrix(Mtx m0, const Mtx& m1)
{
	int i;
	Mtx mdest;
	for (i = 0; i < 4; i++)
	{
		mdest.v[0][i] = m0.v[0][i]*m1.v[0][0] + m0.v[1][i]*m1.v[0][1] + m0.v[2][i]*m1.v[0][2] + m0.v[3][i]*m1.v[0][3];
		mdest.v[1][i] = m0.v[0][i]*m1.v[1][0] + m0.v[1][i]*m1.v[1][1] + m0.v[2][i]*m1.v[1][2] + m0.v[3][i]*m1.v[1][3];
		mdest.v[2][i] = m0.v[0][i]*m1.v[2][0] + m0.v[1][i]*m1.v[2][1] + m0.v[2][i]*m1.v[2][2] + m0.v[3][i]*m1.v[2][3];
		mdest.v[3][i] = m0.v[3][i]*m1.v[3][3] + m0.v[2][i]*m1.v[3][2] + m0.v[1][i]*m1.v[3][1] + m0.v[0][i]*m1.v[3][0];
	}

	return mdest;
}

void MultMatrix2(Mtx& m0, Mtx m1)
{
	m0 = MultMatrix(m0, m1);
}

void TransformVectorNormalize(float vec[3], Mtx mtx)
{
	float vres[3];
	vres[0] = mtx.v[0][0] * vec[0] + mtx.v[1][0] * vec[1] + mtx.v[2][0] * vec[2];
	vres[1] = mtx.v[0][1] * vec[0] + mtx.v[1][1] * vec[1] + mtx.v[2][1] * vec[2];
	vres[2] = mtx.v[0][2] * vec[0] + mtx.v[1][2] * vec[1] + mtx.v[2][2] * vec[2];
	vec[0] = vres[0];
	vec[1] = vres[1];
	vec[2] = vres[2];

	Normalize(vec);
}

void InverseTransformVectorNormalize(float src[3], float dst[3], Mtx mtx)
{
	dst[0] = mtx.v[0][0] * src[0] + mtx.v[0][1] * src[1] + mtx.v[0][2] * src[2];
	dst[1] = mtx.v[1][0] * src[0] + mtx.v[1][1] * src[1] + mtx.v[1][2] * src[2];
	dst[2] = mtx.v[2][0] * src[0] + mtx.v[2][1] * src[1] + mtx.v[2][2] * src[2];

	Normalize(dst);
}

void Normalize(float v[3])
{
#ifdef WIN32_ASM
	__asm {
		mov		esi, dword ptr [v]
										//	ST(6)			ST(5)			ST(4)			ST(3)			ST(2)			ST(1)			ST
		fld		dword ptr [esi+08h]		//																									v2
		fld		dword ptr [esi+04h]		//																					v2				v1
		fld		dword ptr [esi]			//																	v2				v1				v0
		fld1							//													v2				v1				v0				1.0
		fld		ST(3)					//									v2				v1				v0				1.0				v2
		fmul	ST, ST					//									v2				v1				v0				1.0				v2*v2
		fld		ST(3)					//					v2				v1				v0				1.0				v2*v2			v1
		fmul	ST, ST					//					v2				v1				v0				1.0				v2*v2			v1*v1
		fld		ST(3)					//	v2				v1				v0				1.0				v2*v2			v1*v1			v0
		fmul	ST, ST					//	v2				v1				v0				1.0				v2*v2			v1*v1			v0*v0
		fadd							//					v2				v1				v0				1.0				v2*v2			v1*v1+v0*v0
		fadd							//									v2				v1				v0				1.0				v2*v2+v1*v1+v0*v0
		ftst							// Compare ST to 0
		fstsw	ax						// Store FPU status word in ax
		sahf							// Transfer ax to flags register
		jz		End						// Skip if length is zero
		fsqrt							//									v2				v1				v0				1.0				len
		fdiv							//													v2				v1				v0				1.0/len
		fmul	ST(3), ST				//													v2*(1.0/len)	v1				v0				1.0/len
		fmul	ST(2), ST				//													v2*(1.0/len)	v1*(1.0/len)	v0				1.0/len
		fmul							//																	v2*(1.0/len)	v1*(1.0/len)	v0*(1.0/len)
		fstp	dword ptr [esi]			//																					v2*(1.0/len)	v1*(1.0/len)
		fstp	dword ptr [esi+04h]		//																									v2*(1.0/len)
		fstp	dword ptr [esi+08h]		//
End:
		finit
	}
#else // WIN32_ASM
	float len;

	len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	if (len != 0.0)	{
		len = sqrtf( len );
		v[0] /= len;
		v[1] /= len;
		v[2] /= len;
	}
#endif // WIN32_ASM
}

void InverseTransformVectorNormalizeN(float src[][3], float dst[][3], Mtx mtx, u32 count)
{
	for (u32 i = 0; i < count; i++)
	{
		InverseTransformVectorNormalize((float(*))src[i], (float(*))dst[i], mtx);
	}
}

void CopyMatrix( float m0[4][4], float m1[4][4] )
{
#ifdef WIN32_ASM
	__asm {
		mov		esi, [m1]
		mov		edi, [m0]

		mov		eax, dword ptr [esi+00h]
		mov		dword ptr [edi+00h], eax
		mov		eax, dword ptr [esi+04h]
		mov		dword ptr [edi+04h], eax
		mov		eax, dword ptr [esi+08h]
		mov		dword ptr [edi+08h], eax
		mov		eax, dword ptr [esi+0Ch]
		mov		dword ptr [edi+0Ch], eax

		mov		eax, dword ptr [esi+10h]
		mov		dword ptr [edi+10h], eax
		mov		eax, dword ptr [esi+14h]
		mov		dword ptr [edi+14h], eax
		mov		eax, dword ptr [esi+18h]
		mov		dword ptr [edi+18h], eax
		mov		eax, dword ptr [esi+1Ch]
		mov		dword ptr [edi+1Ch], eax

		mov		eax, dword ptr [esi+20h]
		mov		dword ptr [edi+20h], eax
		mov		eax, dword ptr [esi+24h]
		mov		dword ptr [edi+24h], eax
		mov		eax, dword ptr [esi+28h]
		mov		dword ptr [edi+28h], eax
		mov		eax, dword ptr [esi+2Ch]
		mov		dword ptr [edi+2Ch], eax

		mov		eax, dword ptr [esi+30h]
		mov		dword ptr [edi+30h], eax
		mov		eax, dword ptr [esi+34h]
		mov		dword ptr [edi+34h], eax
		mov		eax, dword ptr [esi+38h]
		mov		dword ptr [edi+38h], eax
		mov		eax, dword ptr [esi+3Ch]
		mov		dword ptr [edi+3Ch], eax
	}
#else
	memcpy( m0, m1, 16 * sizeof( float ) );
#endif // WIN32_ASM
}
