#ifndef _3DMATH_H
#define _3DMATH_H

#undef X86_ASM

inline void CopyMatrix( float m0[4][4], float m1[4][4] )
{
#ifndef __LINUX__
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
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"				"\n\t"
	"	mov		eax, dword ptr [esi+0x00]"	"\n\t"
	"	mov		dword ptr [edi+0x00], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x04]"	"\n\t"
	"	mov		dword ptr [edi+0x04], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x08]"	"\n\t"
	"	mov		dword ptr [edi+0x08], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x0C]"	"\n\t"
	"	mov		dword ptr [edi+0x0C], eax"	"\n\t"

	"	mov		eax, dword ptr [esi+0x10]"	"\n\t"
	"	mov		dword ptr [edi+0x10], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x14]"	"\n\t"
	"	mov		dword ptr [edi+0x14], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x18]"	"\n\t"
	"	mov		dword ptr [edi+0x18], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x1C]"	"\n\t"
	"	mov		dword ptr [edi+0x1C], eax"	"\n\t"

	"	mov		eax, dword ptr [esi+0x20]"	"\n\t"
	"	mov		dword ptr [edi+0x20], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x24]"	"\n\t"
	"	mov		dword ptr [edi+0x24], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x28]"	"\n\t"
	"	mov		dword ptr [edi+0x28], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x2C]"	"\n\t"
	"	mov		dword ptr [edi+0x2C], eax"	"\n\t"

	"	mov		eax, dword ptr [esi+0x30]"	"\n\t"
	"	mov		dword ptr [edi+0x30], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x34]"	"\n\t"
	"	mov		dword ptr [edi+0x34], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x38]"	"\n\t"
	"	mov		dword ptr [edi+0x38], eax"	"\n\t"
	"	mov		eax, dword ptr [esi+0x3C]"	"\n\t"
	"	mov		dword ptr [edi+0x3C], eax"	"\n\t"
	".att_syntax prefix"					"\n\t"
	: /* no output */
	: "S"(m1), "D"(m0)
	: "%eax", "memory" );
# else // X86_ASM
	memcpy( m0, m1, 16 * sizeof( float ) );
# endif // !X86_ASM
#endif
}

inline void MultMatrix( float m0[4][4], float m1[4][4] )
{
#ifndef __LINUX__
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
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"MultMatrix_Loop:"					"\n\t"
	"	fld		dword ptr [esi+0x30]"	"\n\t"
	"	fld		dword ptr [esi+0x20]"	"\n\t"
	"	fld		dword ptr [esi+0x10]"	"\n\t"
	"	fld		dword ptr [esi]"		"\n\t"

	"	fld		dword ptr [edi]"		"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [edi+0x04]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x08]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x0C]"	"\n\t"
	"	fmul	ST, ST(5)"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi]"		"\n\t"

	"	fld		dword ptr [edi+0x10]"	"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [edi+0x14]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x18]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x1C]"	"\n\t"
	"	fmul	ST, ST(5)"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi+0x10]"	"\n\t"

	"	fld		dword ptr [edi+0x20]"	"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [edi+0x24]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x28]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x2C]"	"\n\t"
	"	fmul	ST, ST(5)"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi+0x20]"	"\n\t"

	"	fld		dword ptr [edi+0x30]"	"\n\t"
	"	fmulp	ST(1), ST"				"\n\t"
	"	fld		dword ptr [edi+0x34]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x38]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [edi+0x3C]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi+0x30]"	"\n\t"

	"	add		esi, 4"					"\n\t"
	"	dec		cx"						"\n\t"
	"	cmp		cx, 0"					"\n\t"
	"	ja		MultMatrix_Loop"		"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(m0), "D"(m1), "c"(4)
	: "memory" );
# else // X86_ASM
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
# endif // !X86_ASM
#endif // __LINUX__
}

inline void Transpose3x3Matrix( float mtx[4][4] )
{
#ifndef __LINUX__
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
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"				"\n\t"
	"	mov		eax, dword ptr [esi+0x04]"	"\n\t"
	"	mov		ebx, dword ptr [esi+0x10]"	"\n\t"
	"	mov		dword ptr [esi+0x04], ebx"	"\n\t"
	"	mov		dword ptr [esi+0x10], eax"	"\n\t"

	"	mov		eax, dword ptr [esi+0x08]"	"\n\t"
	"	mov		ebx, dword ptr [esi+0x20]"	"\n\t"
	"	mov		dword ptr [esi+0x08], ebx"	"\n\t"
	"	mov		dword ptr [esi+0x20], eax"	"\n\t"

	"	mov		eax, dword ptr [esi+0x18]"	"\n\t"
	"	mov		ebx, dword ptr [esi+0x24]"	"\n\t"
	"	mov		dword ptr [esi+0x18], ebx"	"\n\t"
	"	mov		dword ptr [esi+0x24], eax"	"\n\t"
	".att_syntax prefix"					"\n\t"
	: /* no output */
	: "S"(mtx)
	: "%eax", "%ebx", "memory" );
# else // X86_ASM
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
# endif // !X86_ASM
#endif // __LINUX__
}

inline void TransformVertex( float vtx[4], float mtx[4][4] )//, float perspNorm )
{
#ifndef __LINUX__
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
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"	fld		dword ptr [esi+8]"		"\n\t"
	"	fld		dword ptr [esi+4]"		"\n\t"
	"	fld		dword ptr [esi]"		"\n\t"

	"	fld		dword ptr [ebx]"		"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [ebx+0x10]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x20]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fadd	dword ptr [ebx+0x30]"	"\n\t"
//	"	fmul	dword ptr [perspNorm]"	"\n\t"
	"	fstp	dword ptr [esi]"		"\n\t"

	"	fld		dword ptr [ebx+0x04]"	"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [ebx+0x14]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x24]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fadd	dword ptr [ebx+0x34]"	"\n\t"
//	"	fmul	dword ptr [perspNorm]"	"\n\t"
	"	fstp	dword ptr [esi+0x04]"	"\n\t"

	"	fld		dword ptr [ebx+0x08]"	"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [ebx+0x18]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x28]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fadd	dword ptr [ebx+0x38]"	"\n\t"
//	"	fmul	dword ptr [perspNorm]"	"\n\t"
	"	fstp	dword ptr [esi+0x08]"	"\n\t"

	"	fld		dword ptr [ebx+0x0C]"	"\n\t"
	"	fmulp	ST(1), ST"				"\n\t"
	"	fld		dword ptr [ebx+0x1C]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x2C]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fadd	dword ptr [ebx+0x3C]"	"\n\t"
//	"	fmul	dword ptr [perspNorm]"	"\n\t"
	"	fstp	dword ptr [esi+0x0C]"	"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(vtx), "b"(mtx)
	: "memory" );
# else // X86_ASM
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
# endif // X86_ASM
#endif // __LINUX__
}

inline void TransformVector( float vec[3], float mtx[4][4] )
{
#ifndef __LINUX__
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
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"	fld		dword ptr [esi+8]"		"\n\t"
	"	fld		dword ptr [esi+4]"		"\n\t"
	"	fld		dword ptr [esi]"		"\n\t"

	"	fld		dword ptr [ebx]"		"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [ebx+0x10]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x20]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi]"		"\n\t"

	"	fld		dword ptr [ebx+0x04]"	"\n\t"
	"	fmul	ST, ST(1)"				"\n\t"
	"	fld		dword ptr [ebx+0x14]"	"\n\t"
	"	fmul	ST, ST(3)"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x24]"	"\n\t"
	"	fmul	ST, ST(4)"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi+0x04]"	"\n\t"

	"	fld		dword ptr [ebx+0x08]"	"\n\t"
	"	fmulp	ST(1), ST"				"\n\t"
	"	fld		dword ptr [ebx+0x18]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fld		dword ptr [ebx+0x28]"	"\n\t"
	"	fmulp	ST(2), ST"				"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [esi+0x08]"	"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(vec), "b"(mtx)
	: "memory" );
# else // X86_ASM
	vec[0] = mtx[0][0] * vec[0]
		   + mtx[1][0] * vec[1]
		   + mtx[2][0] * vec[2];
	vec[1] = mtx[0][1] * vec[0]
		   + mtx[1][1] * vec[1]
		   + mtx[2][1] * vec[2];
	vec[2] = mtx[0][2] * vec[0]
		   + mtx[1][2] * vec[1]
		   + mtx[2][2] * vec[2];
# endif // !X86_ASM
#endif // __LINUX__
}

inline void Normalize( float v[3] )
{
#ifndef __LINUX__
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
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"	fld		dword ptr [esi+0x08]"	"\n\t"
	"	fld		dword ptr [esi+0x04]"	"\n\t"
	"	fld		dword ptr [esi]"		"\n\t"
	"	fld1"							"\n\t"
	"	fld		ST(3)"					"\n\t"
	"	fmul	ST, ST"					"\n\t"
	"	fld		ST(3)"					"\n\t"
	"	fmul	ST, ST"					"\n\t"
	"	fld		ST(3)"					"\n\t"
	"	fmul	ST, ST"					"\n\t"
	"	fadd"							"\n\t"
	"	fadd"							"\n\t"
	"	ftst"							"\n\t"
	"	fstsw	ax"						"\n\t"
	"	sahf"							"\n\t"
	"	jz		End"					"\n\t"
	"	fsqrt"							"\n\t"
	"	fdiv"							"\n\t"
	"	fmul	ST(3), ST"				"\n\t"
	"	fmul	ST(2), ST"				"\n\t"
	"	fmul"							"\n\t"
	"	fstp	dword ptr [esi]"		"\n\t"
	"	fstp	dword ptr [esi+0x04]"	"\n\t"
	"	fstp	dword ptr [esi+0x08]"	"\n\t"
	"End:"								"\n\t"
	"	finit"							"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(v)
	: "memory", "cc" );
# else // X86_ASM
	float len;

	len = (float)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if (len != 0.0)
	{
		len = (float)sqrt( len );
		v[0] /= (float)len;
		v[1] /= (float)len;
		v[2] /= (float)len;
	}
# endif // X86_ASM
#endif // __LINUX__
}

inline void Normalize2D( float v[2] )
{
#ifndef __LINUX__
	__asm {
		mov		esi, dword ptr [v]

										//	ST(6)			ST(5)			ST(4)			ST(3)			ST(2)			ST(1)			ST
		fld		dword ptr [esi+04h]		//																									v1
		fld		dword ptr [esi]			//																					v1				v0
		fld1							//																	v1				v0				1.0
		fld		ST(2)					//													v1				v0				1.0				v1
		fmul	ST, ST					//													v1				v0				1.0				v1*v1
		fld		ST(2)					//									v1				v0				1.0				v1*v1			v0
		fmul	ST, ST					//									v1				v0				1.0				v1*v1			v0*v0
		fadd							//													v1				v0				1.0				v1*v1+v0*v0
		fsqrt							//													v1				v0				1.0				len
		fdiv							//																	v1				v0				1.0/len
		fmul	ST(2), ST				//																	v1*(1.0/len)	v0				1.0/len
		fmul							//																					v1*(1.0/len)	v0*(1.0/len)
		fstp	dword ptr [esi]			//																									v1*(1.0/len)
		fstp	dword ptr [esi+04h]		//
	}
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"	fld		dword ptr [esi+0x04]"	"\n\t"
	"	fld		dword ptr [esi]"		"\n\t"
	"	fld1"							"\n\t"
	"	fld		ST(2)"					"\n\t"
	"	fmul	ST, ST"					"\n\t"
	"	fld		ST(2)"					"\n\t"
	"	fmul	ST, ST"					"\n\t"
	"	fadd"							"\n\t"
	"	fsqrt"							"\n\t"
	"	fdiv"							"\n\t"
	"	fmul	ST(2), ST"				"\n\t"
	"	fmul"							"\n\t"
	"	fstp	dword ptr [esi]"		"\n\t"
	"	fstp	dword ptr [esi+0x04]"	"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(v)
	: "memory" );
# else // X86_ASM
	float len;

	len = (float)sqrt( v[0]*v[0] + v[1]*v[1] );
/*	if (len != 0.0)
	{*/
		v[0] /= len;
		v[1] /= len;
/*	}
	else
	{
		v[0] = 0.0;
		v[1] = 0.0;
	}*/
# endif // !X86_ASM
#endif // __LINUX__
}


/*inline float Determinate4x4( float m[4][4] )
{
	float m_2233_3223;
	float m_2133_3123;
	float m_2132_3122;
	float m_2033_3023;
	float m_2032_3022;
	float m_2031_3021;
	float det, ret;

	__asm {
		mov		esi, dword ptr [m]
	
										//	ST(7)			ST(6)			ST(5)			ST(4)			ST(3)			ST(2)			ST(1)			ST
		fld		dword ptr [esi+20h]		//																													m20
		fld		dword ptr [esi+30h]		//																									m20				m30

		fld		dword ptr [esi+34h]		//																					m20				m30				m31
		fmul	ST, ST(2)				//																					m20				m30				m20*m31
		fld		dword ptr [esi+24h]		//																	m20				m30				m20*m31			m21
		fmul	ST, ST(2)				//																	m20				m30				m20*m31			m30*m21
		fsub							//																					m20				m30				m20*m31-m30*m21
		fstp	dword ptr [m_2031_3021] //																									m20				m30

		fld		dword ptr [esi+38h]		//																					m20				m30				m32
		fmul	ST, ST(2)				//																					m20				m30				m20*m32
		fld		dword ptr [esi+28h]		//																	m20				m30				m20*m32			m22
		fmul	ST, ST(2)				//																	m20				m30				m20*m32			m30*m22
		fsub							//																					m20				m30				m20*m32-m30*m22
		fstp	dword ptr [m_2032_3022]	//																									m20				m30

		fld		dword ptr [esi+3Ch]		//																					m20				m30				m33
		fmulp	ST(2), ST				//																									m20*m33			m30
		fld		dword ptr [esi+2Ch]		//																					m20*m33			m30				m23
		fmul							//																									m20*m33			m30*m23
		fsub							//																													m20*m33-m30*m23
		fstp	dword ptr [m_2033_3023]	//

		fld		dword ptr [esi+24h]		//																													m21
		fld		dword ptr [esi+34h]		//																									m21				m31
		fld		dword ptr [esi+38h]		//																					m21				m31				m32
		fld		dword ptr [esi+28h]		//																	m21				m31				m32				m22

		fld		ST(1)					//													m21				m31				m32				m22				m32
		fmul	ST, ST(4)				//													m21				m31				m32				m22				m21*m32
		fld		ST(1)					//									m21				m31				m32				m22				m21*m32			m22
		fmul	ST, ST(4)				//									m21				m31				m32				m22				m21*m32			m31*m22
		fsub							//													m21				m31				m32				m22				m21*m32-m31*m22
		fstp	dword ptr [m_2132_3122] //																	m21				m31				m32				m22
		
		fld		dword ptr [esi+3Ch]		//													m21				m31				m32				m22				m33
		fxch	ST(1), ST				//													m21				m31				m32				m33				m22
		fmul	ST, ST(1)				//													m21				m31				m32				m33				m22*m33
		fld		dword ptr [esi+2Ch]		//									m21				m31				m32				m33				m22*m33			m23
		fxch	ST(3), ST				//									m21				m31				m23				m33				m22*m33			m32
		fmul	ST, ST(3)				//									m21				m31				m23				m33				m22*m33			m32*m23
		fsub							//													m21				m31				m23				m33				m22*m33-m32*m23
		fstp	dword ptr [m_2233_3223] //																	m21				m31				m23				m33
		
		fmulp	ST(3), ST				//																					m21*m33			m31				m23
		fmul							//																									m21*m33			m31*m23
		fsub							//																													m21*m33-m31*m23
		tstp	dword ptr [m_2133_3123] //

		fld0							//																													0.0
		fld		dword ptr [esi+1Ch]		//																									0.0				m13
		fld		dword ptr [esi+18h]		//																					0.0				m13				m12
		fld		dword ptr [m_2233_3223] //																	0.0				m13				m12				m_2233_3223

		fld		dword ptr [esi+14h]		//													0.0				m13				m12				m_2233_3223		m11
		fmul	ST, ST(1)				//													0.0				m13				m12				m_2233_3223		m11*m_2233_3223
		fld		dword ptr [m_2133_3123]	//									0.0				m13				m12				m_2233_3223		m11*m_2233_3223 m_2133_3123
		fmul	ST, ST(3)				//									0.0				m13				m12				m_2233_3223		m11*m_2233_3223 m12*m_2133_3123
		fsub							//													0.0				m13				m12				m_2233_3223		m11*m_2233_3223-m12*m_2133_3123
		fld		dword ptr [m_2132_3122]	//									0.0				m13				m12				m_2233_3223		m11*m_2233_3223-m12*m_2133_3123 m_2132_3122
		fmul	ST, ST(4)				//									0.0				m13				m12				m_2233_3223		m11*m_2233_3223-m12*m_2133_3123 m13*m_2132_3122
		fadd							//													0.0				m13				m12				m_2233_3223		m11*m_2233_3223-m12*m_2133_3123+m13*m_2132_3122=det1
		fmul	dword ptr [esi]			//													0.0				m13				m12				m_2233_3223		det1*m00=res
		faddp	ST(4), ST				//																	res				m13				m12				m_2233_3223
// needs work from here on
		fmul	dword ptr [esi+10h]		//																	res				m13				m12				m10*m_2233_3223
		fld		dword ptr [m_2033_3023]	//													res				m13				m12				m10*m_2233_3223	m_2033_3023
		fxch	ST(2), ST				//													res				m13				m_2033_3023		m10*m_2233_3223	m12
		fmul	ST, ST(2)				//													res				m13				m_2033_3023		m10*m_2233_3223	m12*m_2033_3023
		fsub							//																	res				m13				m_2033_3023		m10*m_2233_3223-m12*m_2033_3023
		fld		dword ptr [m_2032_3022]	//													res				m13				m_2033_3023		m10*m_2233_3223-m12*m_2033_3023	m_2032_3022
		fmul	ST, ST(3)				//													res				m13				m_2033_3023		m10*m_2233_3223-m12*m_2033_3023	m13*m_2032_3022
		fadd							//																	res				m13				m_2033_3023		m10*m_2233_3223-m12*m_2033_3023+m13*m_2032_3022=det
		fmul	dword ptr [esi+04h]		//																	res				m13				m_2033_3023		det*m01
		fsubp	ST(3), ST				//																					res				m13				m_2033_3023

		fld		dword ptr [esi+10h]

		det = mr._21 * mr_3244_4234 - mr._22 * mr_3144_4134 + mr._24 * mr_3142_4132;
		res += mr._13 * det;
	}
}*/

inline float DotProduct( float v0[3], float v1[3] )
{
	float	dot;
#ifndef __LINUX__
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
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"	fld		dword ptr [esi]"		"\n\t"
	"	fmul	dword ptr [edi]"		"\n\t"
	"	fld		dword ptr [esi+0x04]"	"\n\t"
	"	fmul	dword ptr [edi+0x04]"	"\n\t"
	"	fld		dword ptr [esi+0x08]"	"\n\t"
	"	fmul	dword ptr [edi+0x08]"	"\n\t"
	"	fadd"							"\n\t"
	"	fadd"							"\n\t"
	"	fstp	dword ptr [ebx]"		"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(v0), "D"(v1), "b"(&dot)
	: "memory" );
# else // X86_ASM
	dot = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
# endif // !X86_ASM
#endif // __LINUX__
	return dot;
}

#endif
