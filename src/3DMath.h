#ifndef _3DMATH_H
#define _3DMATH_H
#include <utility>
#include <memory.h>
#include <string.h>
#include <Types.h>
#include "GBI.h"

typedef f32 Vec __attribute__((vector_size(16)));
struct Mtx
{
	Vec v[4];

	Vec& operator[](int i) { return v[i]; }
	const Vec& operator[](int i) const { return v[i]; }
};

Mtx MultMatrix(Mtx m0, const Mtx& m1);
void MultMatrix2(Mtx& m0, Mtx m1);
void TransformVectorNormalize(float vec[3], Mtx mtx);
void InverseTransformVectorNormalize(float src[3], float dst[3], Mtx mtx);
void InverseTransformVectorNormalizeN(float src[][3], float dst[][3], Mtx mtx, u32 count);
void Normalize(float v[3]);
float DotProduct(const f32 v0[3], const f32 v1[3]);

#ifdef WIN32_ASM
#undef WIN32_ASM
#endif

inline float DotProduct(const f32 v0[3], const f32 v1[3])
{
	float	dot;
#ifdef WIN32_ASM
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
#else // WIN32_ASM
	dot = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
#endif // WIN32_ASM
	return dot;
}

inline float GetFloatMatrixElement(s16 _int, u16 _fract)
{
	const s32 element = (_int << 16) | _fract;
	return _FIXED2FLOAT(element, 16);
}

inline std::pair<s16, u16> GetIntMatrixElement(f32 _elem)
{
	const s32 value = static_cast<s32>(_elem * 65536.0f);
	return std::pair<s16, u16>(static_cast<s16>(value >> 16), static_cast<u16>(value & 0xFFFF));
}

#endif
