#ifndef _3DMATH_H
#define _3DMATH_H
#include <memory.h>

inline void CopyMatrix( float m0[4][4], float m1[4][4] )
{
#ifdef WIN32
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
#endif // WIN32
}

inline void MultMatrix( float m0[4][4], float m1[4][4] )
{
#ifdef WIN32
 	__asm {
		mov		esi, dword ptr [m0]
		mov		edi, dword ptr [m1]
		mov		cx, 4

MultMatrix_Loop:
										//	ST(5)			ST(4)			ST(3)			ST(2)			ST(1)			ST
		fld		dword ptr [esi+30h]     //																					m030
		fld		dword ptr [esi+20h]		//																	m030			m020
		fld		dword ptr [esi+10h]		//													m030			m020			m010
		fld		dword ptr [esi]			//									m030			m020			m010			m000

		fld		dword ptr [edi]			//					m030			m020			m010			m000			m100
		fmul	ST, ST(1)				//					m030			m020			m010			m000			m000*m100
		fld		dword ptr [edi+04h]		//	m030			m020			m010			m000			m000*m100		m101
		fmul	ST, ST(3)				//	m030			m020			m010			m000			m000*m100		m010*m101
		fadd							//					m030			m020			m010			m000			m000*m100+m010*m101
		fld		dword ptr [edi+08h]		//	m030			m020			m010			m000			m000*m100+m010*m101 m102
		fmul	ST, ST(4)				//	m030			m020			m010			m000			m000*m100+m010*m101 m020*m102
		fadd							//					m030			m020			m010			m000			m000*m100+m010*m101+m020*m102
		fld		dword ptr [edi+0Ch]		//	m030			m020			m010			m000			m000*m100+m010*m101+m020*m102 m103
		fmul	ST, ST(5)				//	m030			m020			m010			m000			m000*m100+m010*m101+m020*m102 m030*m103
		fadd							//					m030			m020			m010			m000			m000*m100+m010*m101+m020*m102+m030*m103
		fstp	dword ptr [esi]			//									m030			m020			m010			m000

		fld		dword ptr [edi+10h]		//					m030			m020			m010			m000			m110
		fmul	ST, ST(1)				//					m030			m020			m010			m000			m000*m110
		fld		dword ptr [edi+14h]		//	m030			m020			m010			m000			m000*m110		m111
		fmul	ST, ST(3)				//	m030			m020			m010			m000			m000*m110		m010*m111
		fadd							//					m030			m020			m010			m000			m000*m110+m010*m111
		fld		dword ptr [edi+18h]		//	m030			m020			m010			m000			m000*m110+m010*m111 m112
		fmul	ST, ST(4)				//	m030			m020			m010			m000			m000*m110+m010*m111 m020*m112
		fadd							//					m030			m020			m010			m000			m000*m110+m010*m111+m020*m112
		fld		dword ptr [edi+1Ch]		//	m030			m020			m010			m000			m000*m110+m010*m111+m020*m112 m113
		fmul	ST, ST(5)				//	m030			m020			m010			m000			m000*m110+m010*m111+m020*m112 m030*m113
		fadd							//					m030			m020			m010			m000			m000*m110+m010*m111+m020*m112+m030*m113
		fstp	dword ptr [esi+10h]		//									m030			m020			m010			m000

		fld		dword ptr [edi+20h]		//					m030			m020			m010			m000			m120
		fmul	ST, ST(1)				//					m030			m020			m010			m000			m000*m120
		fld		dword ptr [edi+24h]		//	m030			m020			m010			m000			m000*m120		m121
		fmul	ST, ST(3)				//	m030			m020			m010			m000			m000*m120		m010*m121
		fadd							//					m030			m020			m010			m000			m000*m120+m010*m121
		fld		dword ptr [edi+28h]		//	m030			m020			m010			m000			m000*m120+m010*m121 m122
		fmul	ST, ST(4)				//	m030			m020			m010			m000			m000*m120+m010*m121 m020*m122
		fadd							//					m030			m020			m010			m000			m000*m120+m010*m121+m020*m122
		fld		dword ptr [edi+2Ch]		//	m030			m020			m010			m000			m000*m120+m010*m121+m020*m122 m123
		fmul	ST, ST(5)				//	m030			m020			m010			m000			m000*m120+m010*m121+m020*m122 m030*m123
		fadd							//					m030			m020			m010			m000			m000*m120+m010*m121+m020*m122+m030*m123
		fstp	dword ptr [esi+20h]		//									m030			m020			m010			m000

		fld		dword ptr [edi+30h]		//					m030			m020			m010			m000			m130
		fmulp	ST(1), ST				//									m030			m020			m010			m000*m130
		fld		dword ptr [edi+34h]		//					m030			m020			m010			m000*m130		m131
		fmulp	ST(2), ST				//									m030			m020			m010*m131		m000*m130
		fadd							//													m030			m020			m010*m131+m000*m130
		fld		dword ptr [edi+38h]		//									m030			m020			m010*m131+m000*m130 m132
		fmulp	ST(2), ST				//													m030			m020*m132		m010*m131+m000*m130
		fadd							//																	m030			m020*m132+m010*m131+m000*m130
		fld		dword ptr [edi+3Ch]		//													m030			m020*m132+m010*m131+m000*m130 m133
		fmulp	ST(2), ST				//																	m030*m133		m020*m132+m010*m131+m000*m130
		fadd							//																					m030*m133+m020*m132+m010*m131+m000*m130
		fstp	dword ptr [esi+30h]		//

		add		esi, 4
		dec		cx
		cmp		cx, 0
		ja		MultMatrix_Loop
	}
#else // WIN32
	int i;
	float dst[4][4];

	for (i = 0; i < 4; i++)
	{
		dst[0][i] = m0[0][i]*m1[0][0] + m0[1][i]*m1[0][1] + m0[2][i]*m1[0][2] + m0[3][i]*m1[0][3];
		dst[1][i] = m0[0][i]*m1[1][0] + m0[1][i]*m1[1][1] + m0[2][i]*m1[1][2] + m0[3][i]*m1[1][3];
		dst[2][i] = m0[0][i]*m1[2][0] + m0[1][i]*m1[2][1] + m0[2][i]*m1[2][2] + m0[3][i]*m1[2][3];
		dst[3][i] = m0[3][i]*m1[3][3] + m0[2][i]*m1[3][2] + m0[1][i]*m1[3][1] + m0[0][i]*m1[3][0];
	}
	memcpy( m0, dst, sizeof(float) * 16 );
#endif // WIN32
}

inline void Transpose3x3Matrix( float mtx[4][4] )
{
#ifdef WIN32
	__asm
	{
		mov		esi, [mtx]

		mov		eax, dword ptr [esi+04h]
		mov		ebx, dword ptr [esi+10h]
		mov		dword ptr [esi+04h], ebx
		mov		dword ptr [esi+10h], eax

		mov		eax, dword ptr [esi+08h]
		mov		ebx, dword ptr [esi+20h]
		mov		dword ptr [esi+08h], ebx
		mov		dword ptr [esi+20h], eax

		mov		eax, dword ptr [esi+18h]
		mov		ebx, dword ptr [esi+24h]
		mov		dword ptr [esi+18h], ebx
		mov		dword ptr [esi+24h], eax
	}
#else // WIN32
	float tmp;

	tmp = mtx[0][1];
	mtx[0][1] = mtx[1][0];
	mtx[1][0] = tmp;

	tmp = mtx[0][2];
	mtx[0][2] = mtx[2][0];
	mtx[2][0] = tmp;

	tmp = mtx[1][2];
	mtx[1][2] = mtx[2][1];
	mtx[2][1] = tmp;
#endif // WIN32
}

inline void TransformVertex( float vtx[4], float mtx[4][4] )//, float perspNorm )
{
#ifdef WIN32
	__asm {
		mov		esi, dword ptr [vtx]
		mov		ebx, dword ptr [mtx]
										//	ST(4)			ST(3)			ST(2)			ST(1)			ST
		fld		dword ptr [esi+8]		//																	vtx2
		fld		dword ptr [esi+4]		//													vtx2			vtx1
		fld		dword ptr [esi]			//									vtx2			vtx1			vtx0

		fld		dword ptr [ebx]			//					vtx2			vtx1			vtx0			mtx00
		fmul	ST, ST(1)				//					vtx2			vtx1			vtx0			vtx0*mtx00
		fld		dword ptr [ebx+10h]		//	vtx2			vtx1			vtx0			vtx0*mtx00		mtx10
		fmul	ST, ST(3)				//	vtx2			vtx1			vtx0			vtx0*mtx00		vtx1*mtx10
		fadd							//					vtx2			vtx1			vtx0			vtx0*mtx00+vtx1*mtx10
		fld		dword ptr [ebx+20h]		//	vtx2			vtx1			vtx0			vtx0*mtx00+vtx1*mtx10 mtx20
		fmul	ST, ST(4)				//	vtx2			vtx1			vtx0			vtx0*mtx00+vtx1*mtx10 vtx2*mtx20
		fadd							//					vtx2			vtx1			vtx0			vtx0*mtx00+vtx1*mtx10+vtx2*mtx20
		fadd	dword ptr [ebx+30h]		//					vtx2			vtx1			vtx0			vtx0*mtx00+vtx1*mtx10+vtx2*mtx20+mtx30
//		fmul	dword ptr [perspNorm]	//																	(vtx2*mtx23+vtx1*mtx13+vtx0*mtx03+mtx33)*perspNorm
		fstp	dword ptr [esi]			//									vtx2			vtx1			vtx[0]

		fld		dword ptr [ebx+04h]		//					vtx2			vtx1			vtx0			mtx01
		fmul	ST, ST(1)				//					vtx2			vtx1			vtx0			vtx0*mtx01
		fld		dword ptr [ebx+14h]		//	vtx2			vtx1			vtx0			vtx0*mtx01		mtx11
		fmul	ST, ST(3)				//	vtx2			vtx1			vtx0			vtx0*mtx01		vtx1*mtx11
		fadd							//					vtx2			vtx1			vtx0			vtx0*mtx01+vtx1*mtx11
		fld		dword ptr [ebx+24h]		//	vtx2			vtx1			vtx0			vtx0*mtx01+vtx1*mtx11 mtx21
		fmul	ST, ST(4)				//	vtx2			vtx1			vtx0			vtx0*mtx01+vtx1*mtx11 vtx2*mtx21
		fadd							//					vtx2			vtx1			vtx0			vtx0*mtx01+vtx1*mtx11+vtx2*mtx21
		fadd	dword ptr [ebx+34h]		//					vtx2			vtx1			vtx0			vtx0*mtx01+vtx1*mtx11+vtx2*mtx21+mtx31
//		fmul	dword ptr [perspNorm]	//																	(vtx2*mtx23+vtx1*mtx13+vtx0*mtx03+mtx33)*perspNorm
		fstp	dword ptr [esi+04h]		//									vtx2			vtx1			vtx[0]

		fld		dword ptr [ebx+08h]		//					vtx2			vtx1			vtx0			mtx02
		fmul	ST, ST(1)				//					vtx2			vtx1			vtx0			vtx0*mtx02
		fld		dword ptr [ebx+18h]		//	vtx2			vtx1			vtx0			vtx0*mtx02		mtx12
		fmul	ST, ST(3)				//	vtx2			vtx1			vtx0			vtx0*mtx02		vtx1*mtx12
		fadd							//					vtx2			vtx1			vtx0			vtx0*mtx02+vtx1*mtx12
		fld		dword ptr [ebx+28h]		//	vtx2			vtx1			vtx0			vtx0*mtx02+vtx1*mtx12 mtx22
		fmul	ST, ST(4)				//	vtx2			vtx1			vtx0			vtx0*mtx02+vtx1*mtx12 vtx2*mtx22
		fadd							//					vtx2			vtx1			vtx0			vtx0*mtx02+vtx1*mtx12+vtx2*mtx22
		fadd	dword ptr [ebx+38h]		//					vtx2			vtx1			vtx0			vtx0*mtx02+vtx1*mtx12+vtx2*mtx22+mtx32
//		fmul	dword ptr [perspNorm]	//																	(vtx2*mtx23+vtx1*mtx13+vtx0*mtx03+mtx33)*perspNorm
		fstp	dword ptr [esi+08h]		//									vtx2			vtx1			vtx0

		fld		dword ptr [ebx+0Ch]		//					vtx2			vtx1			vtx0			mtx03
		fmulp	ST(1), ST				//									vtx2			vtx1			vtx0*mtx03
		fld		dword ptr [ebx+1Ch]		//					vtx2			vtx1			vtx0*mtx03		mtx13
		fmulp	ST(2), ST				//									vtx2			vtx1*mtx13		vtx0*mtx03
		fadd							//													vtx2			vtx1*mtx13+vtx0*mtx03
		fld		dword ptr [ebx+2Ch]		//									vtx2			vtx1*mtx13+vtx0*mtx03 mtx23
		fmulp	ST(2), ST				//													vtx2*mtx23		vtx1*mtx13+vtx0*mtx03
		fadd							//																	vtx2*mtx23+vtx1*mtx13+vtx0*mtx03
		fadd	dword ptr [ebx+3Ch]		//																	vtx2*mtx23+vtx1*mtx13+vtx0*mtx03+mtx33
//		fmul	dword ptr [perspNorm]	//																	(vtx2*mtx23+vtx1*mtx13+vtx0*mtx03+mtx33)*perspNorm
		fstp	dword ptr [esi+0Ch]		//
	}
#else // WIN32
	float x, y, z, w;
	x = vtx[0];
	y = vtx[1];
	z = vtx[2];
	w = vtx[3];

	vtx[0] = x * mtx[0][0] +
	         y * mtx[1][0] +
	         z * mtx[2][0];

	vtx[1] = x * mtx[0][1] +
	         y * mtx[1][1] +
	         z * mtx[2][1];

	vtx[2] = x * mtx[0][2] +
	         y * mtx[1][2] +
	         z * mtx[2][2];

	vtx[3] = x * mtx[0][3] +
	         y * mtx[1][3] +
	         z * mtx[2][3];

	vtx[0] += mtx[3][0];
	vtx[1] += mtx[3][1];
	vtx[2] += mtx[3][2];
	vtx[3] += mtx[3][3];
#endif // WIN32
}

inline void TransformVector( float vec[3], float mtx[4][4] )
{
#ifdef WIN32
	__asm {
		mov		esi, dword ptr [vec]
		mov		ebx, dword ptr [mtx]
										//	ST(4)			ST(3)			ST(2)			ST(1)			ST
		fld		dword ptr [esi+8]		//																	vec2
		fld		dword ptr [esi+4]		//													vec2			vtx1
		fld		dword ptr [esi]			//									vec2			vec1			vec0

		fld		dword ptr [ebx]			//					vec2			vec1			vec0			mtx00
		fmul	ST, ST(1)				//					vec2			vec1			vec0			vec0*mtx00
		fld		dword ptr [ebx+10h]		//	vec2			vec1			vec0			vec0*mtx00		mtx10
		fmul	ST, ST(3)				//	vec2			vec1			vec0			vec0*mtx00		vec1*mtx10
		fadd							//					vec2			vec1			vec0			vec0*mtx00+vec1*mtx10
		fld		dword ptr [ebx+20h]		//	vec2			vec1			vec0			vec0*mtx00+vec1*mtx10 mtx20
		fmul	ST, ST(4)				//	vec2			vec1			vec0			vec0*mtx00+vec1*mtx10 vec2*mtx20
		fadd							//					vec2			vec1			vec0			vec0*mtx00+vec1*mtx10+vec2*mtx20
		fstp	dword ptr [esi]			//									vec2			vec1			vec[0]

		fld		dword ptr [ebx+04h]		//					vec2			vec1			vec0			mtx01
		fmul	ST, ST(1)				//					vec2			vec1			vec0			vec0*mtx01
		fld		dword ptr [ebx+14h]		//	vec2			vec1			vec0			vec0*mtx01		mtx11
		fmul	ST, ST(3)				//	vec2			vec1			vec0			vec0*mtx01		vec1*mtx11
		fadd							//					vec2			vec1			vec0			vec0*mtx01+vec1*mtx11
		fld		dword ptr [ebx+24h]		//	vec2			vec1			vec0			vec0*mtx01+vec1*mtx11 mtx21
		fmul	ST, ST(4)				//	vec2			vec1			vec0			vec0*mtx01+vec1*mtx11 vec2*mtx21
		fadd							//					vec2			vec1			vec0			vec0*mtx01+vec1*mtx11+vec2*mtx21
		fstp	dword ptr [esi+04h]		//									vec2			vec1			vec[0]

		fld		dword ptr [ebx+08h]		//					vec2			vec1			vec0			mtx02
		fmulp	ST(1), ST				//									vec2			vec1			vec0*mtx02
		fld		dword ptr [ebx+18h]		//					vec2			vec1			vec0*mtx02		mtx12
		fmulp	ST(2), ST				//									vec2			vec1*mtx12		vec0*mtx02
		fadd							//													vec2			vec1*mtx12+vec0*mtx02
		fld		dword ptr [ebx+28h]		//									vec2			vec1*mtx12+vec0*mtx03 mtx22
		fmulp	ST(2), ST				//													vec2*mtx22		vec1*mtx12+vec0*mtx02
		fadd							//																	vec2*mtx22+vec1*mtx12+vec0*mtx02
		fstp	dword ptr [esi+08h]		//
	}
#else // WIN32
	vec[0] = mtx[0][0] * vec[0]
		   + mtx[1][0] * vec[1]
		   + mtx[2][0] * vec[2];
	vec[1] = mtx[0][1] * vec[0]
		   + mtx[1][1] * vec[1]
		   + mtx[2][1] * vec[2];
	vec[2] = mtx[0][2] * vec[0]
		   + mtx[1][2] * vec[1]
		   + mtx[2][2] * vec[2];
#endif // WIN32
}

inline void Normalize( float v[3] )
{
#ifdef WIN32
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
#else // WIN32
	float len;

	len = (float)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len != 0.0)
	{
		len = (float)sqrt( len );
		v[0] /= (float)len;
		v[1] /= (float)len;
		v[2] /= (float)len;
	}
#endif // WIN32
}

inline float DotProduct( float v0[3], float v1[3] )
{
	float	dot;
#ifdef WIN32
	__asm {
		mov		esi, dword ptr [v0]
		mov		edi, dword ptr [v1]
		lea		ebx, [dot]

		fld		dword ptr [esi]
		fmul	dword ptr [edi]
		fld		dword ptr [esi+04h]
		fmul	dword ptr [edi+04h]
		fld		dword ptr [esi+08h]
		fmul	dword ptr [edi+08h]
		fadd
		fadd
		fstp	dword ptr [ebx]
	}
#else // WIN32
	dot = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
#endif // WIN32
	return dot;
}

#endif
