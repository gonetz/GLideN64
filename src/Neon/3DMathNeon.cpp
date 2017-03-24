#include "3DMath.h"
#include <cmath>
#include "Log.h"

void MultMatrix( float m0[4][4], float m1[4][4], float dest[4][4])
{
    asm volatile (
    "vld1.32         {d0-d3}, [%1]!          \n\t"    //q0 & q1 = m1
    "vld1.32         {d16-d19}, [%0]!        \n\t"    //q8 & q9 = m0

    "vmul.f32        q12, q8, d0[0]          \n\t"    //q12 = q8 * d0[0]
    "vld1.32         {d4-d7}, [%1]           \n\t"    //q2 & q3= m1+8
    "vmul.f32        q13, q8, d2[0]          \n\t"    //q13 = q8 * d2[0]
    "vmul.f32        q14, q8, d4[0]          \n\t"    //q14 = q8 * d4[0]
    "vmul.f32        q15, q8, d6[0]          \n\t"    //q15 = q8 * d6[0]
    "vmla.f32        q12, q9, d0[1]          \n\t"    //q12 = q9 * d0[1]
    "vld1.32         {d20-d23}, [%0]         \n\t"    //q10 & q11 = m0+8
    "vmla.f32        q13, q9, d2[1]          \n\t"    //q13 = q9 * d2[1]
    "vmla.f32        q14, q9, d4[1]          \n\t"    //q14 = q9 * d4[1]
    "vmla.f32        q15, q9, d6[1]          \n\t"    //q15 = q9 * d6[1]
    "vmla.f32        q12, q10, d1[0]         \n\t"    //q12 = q10 * d0[0]
    "vmla.f32        q13, q10, d3[0]         \n\t"    //q13 = q10 * d2[0]
    "vmla.f32        q14, q10, d5[0]         \n\t"    //q14 = q10 * d4[0]
    "vmla.f32        q15, q10, d7[0]         \n\t"    //q15 = q10 * d6[0]
    "vmla.f32        q12, q11, d1[1]         \n\t"    //q12 = q11 * d0[1]
    "vmla.f32        q13, q11, d3[1]         \n\t"    //q13 = q11 * d2[1]
    "vmla.f32        q14, q11, d5[1]         \n\t"    //q14 = q11 * d4[1]
    "vmla.f32        q15, q11, d7[1]         \n\t"    //q15 = q11 * d6[1]

    "vst1.32         {d24-d27}, [%2]!        \n\t"    //d = q12 & q13
    "vst1.32         {d28-d31}, [%2]         \n\t"    //d+8 = q14 & q15 

    :"+r"(m0), "+r"(m1), "+r"(dest):
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
    "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
    "memory"
    );
}

void MultMatrix2(float m0[4][4], float m1[4][4])
{
    MultMatrix(m0, m1, m0);
}

void TransformVectorNormalize(float vec[3], float mtx[4][4])
{
    asm volatile (
    "vld1.32         {d0}, [%1]              \n\t"    //q0 =  v
    "flds            s2, [%1, #8]            \n\t"    //q0 =  v
    
    "vld1.32         {d18-d21}, [%0]!        \n\t"    //q9 =  m
    "vmul.f32        q2, q9, d0[0]           \n\t"    //q2 =  q9*d0[0]
    "vld1.32         {d22, d23}, [%0]        \n\t"    //q11 = m+8
    "vmla.f32        q2, q10, d0[1]          \n\t"    //q2 += q11*d0[1]
    "vmla.f32        q2, q11, d1[0]          \n\t"    //q2 += q11*d1[0]

    "vmul.f32        d0, d4, d4              \n\t"    //d0 =  d4*d4
    "vpadd.f32       d0, d0, d0              \n\t"    //d0 =  d[0] + d[1]
    "vmla.f32        d0, d5, d5              \n\t"    //d0 += d5*d5

    "vmov.f32        d1, d0                  \n\t"    //d1 = d0
    "vrsqrte.f32     d0, d0                  \n\t"    //d0 = ~ 1.0 / sqrt(d0)
    "vmul.f32        d2, d0, d1              \n\t"    //d2 = d0 * d1
    "vrsqrts.f32     d3, d2, d0              \n\t"    //d3 = (3 - d0 * d2) / 2
    "vmul.f32        d0, d0, d3              \n\t"    //d0 = d0 * d3
    "vmul.f32        d2, d0, d1              \n\t"    //d2 = d0 * d1
    "vrsqrts.f32     d3, d2, d0              \n\t"    //d3 = (3 - d0 * d3) / 2
    "vmul.f32        d0, d0, d3              \n\t"    //d0 = d0 * d3

    "vmul.f32        q2, q2, d0[0]           \n\t"    //q2= q2*d0[0]

    "vst1.32         {d4}, [%1]              \n\t"    //vec = d4
    "fsts            s10, [%1, #8]           \n\t"    //vec += d5[0]
    : "+r"(mtx), "+r"(vec) :
    : "d0","d1","d2","d3","d18","d19","d20","d21","d22", "d23", "memory"
    );
}

void InverseTransformVectorNormalize(float src[3], float dst[3], float mtx[4][4])
{
    asm volatile (
    "vld1.32        {d0}, [%1]!                   \n\t"    //d0 = src[0] & src[1]
    "vld1.32        {d1[0]}, [%1]                 \n\t"    //d1[0] = src[2]
    
    "vld4.32        {d18, d20, d22, d24}, [%0]!   \n\t"
    "vld4.32        {d19, d21, d23, d25}, [%0]    \n\t"
    "vmul.f32       q2, q9, d0[0]                 \n\t"    //q2 =  q9*d0[0]
    "vmla.f32       q2, q10, d0[1]                \n\t"    //q2 += q1*d0[1]
    "vmla.f32       q2, q11, d1[0]                \n\t"    //q2 += q2*d1[0]*/

    "vmul.f32       d0, d4, d4                    \n\t"    //d0 =  d4*d4
    "vpadd.f32      d0, d0, d0                    \n\t"    //d0 =  d[0] + d[1]
    "vmla.f32       d0, d5, d5                    \n\t"    //d0 += d5*d5

    "vmov.f32       d1, d0                        \n\t"    //d1 = d0
    "vrsqrte.f32    d0, d0                        \n\t"    //d0 = ~ 1.0 / sqrt(d0)
    "vmul.f32       d2, d0, d1                    \n\t"    //d2 = d0 * d1
    "vrsqrts.f32    d3, d2, d0                    \n\t"    //d3 = (3 - d0 * d2) / 2
    "vmul.f32       d0, d0, d3                    \n\t"    //d0 = d0 * d3
    "vmul.f32       d2, d0, d1                    \n\t"    //d2 = d0 * d1
    "vrsqrts.f32    d3, d2, d0                    \n\t"    //d3 = (3 - d0 * d3) / 2
    "vmul.f32       d0, d0, d3                    \n\t"    //d0 = d0 * d3

    "vmul.f32       q2, q2, d0[0]                 \n\t"    //q2 = q2*d0[0]

    "vst1.32        {d4}, [%2]!                   \n\t"    //dst = d4
    "vst1.32        {d5[0]}, [%2]                 \n\t"    //dst += d5[0]
    : "+r"(mtx), "+r"(src), "+r"(dst) :
    : "d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d18","d19",
    "d20","d21","d22","d23","d24","d25", "memory"
    );
}

void Normalize(float v[3])
{
    asm volatile (
    "vld1.32         {d4}, [%0]!             \n\t"    //d4={x,y}
    "flds            s10, [%0]               \n\t"    //d5[0] = z
    "sub             %0, %0, #8              \n\t"    //d5[0] = z
    "vmul.f32        d0, d4, d4              \n\t"    //d0= d4*d4
    "vpadd.f32       d0, d0, d0              \n\t"    //d0 = d[0] + d[1]
    "vmla.f32        d0, d5, d5              \n\t"    //d0 = d0 + d5*d5

    "vmov.f32        d1, d0                  \n\t"    //d1 = d0
    "vrsqrte.f32     d0, d0                  \n\t"    //d0 = ~ 1.0 / sqrt(d0)
    "vmul.f32        d2, d0, d1              \n\t"    //d2 = d0 * d1
    "vrsqrts.f32     d3, d2, d0              \n\t"    //d3 = (3 - d0 * d2) / 2
    "vmul.f32        d0, d0, d3              \n\t"    //d0 = d0 * d3
    "vmul.f32        d2, d0, d1              \n\t"    //d2 = d0 * d1
    "vrsqrts.f32     d3, d2, d0              \n\t"    //d3 = (3 - d0 * d3) / 2
    "vmul.f32        d0, d0, d3              \n\t"    //d0 = d0 * d3

    "vmul.f32        q2, q2, d0[0]           \n\t"    //q2 = q2*d0[0]
    "vst1.32         {d4}, [%0]!             \n\t"    //d4
    "fsts            s10, [%0]               \n\t"    //d2={x0,y0}, d3={z0, w0}

    :"+r"(v) :
    : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
    );
}
