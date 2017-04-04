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

void InverseTransformVectorNormalize2x(float src[2][3], float dst[2][3], float mtx[4][4] )
{
    asm volatile (
    // Load src
    "vld3.32        {d0,d1,d2}, [%1]              \n\t"    // load vector 0-1
    
    // Load mtx
    "vld1.32        {d6-d9}, [%0]!                \n\t"    // load row 0-1
    "vld1.32        {d10-d11}, [%0]               \n\t"    // load row 2
    
    // Multiply and add
    "vmul.f32       d3, d0, d6[0]                 \n\t"    //dst[0] = v[0]*mtx[0][0]
    "vmul.f32       d4, d0, d8[0]                 \n\t"    //dst[1] = v[0]*mtx[1][0]
    "vmul.f32       d5, d0, d10[0]                \n\t"    //dst[2] = v[0]*mtx[2][0]
    "vmla.f32       d3, d1, d6[1]                 \n\t"    //dst[0] = v[1]*mtx[0][1]
    "vmla.f32       d4, d1, d8[1]                 \n\t"    //dst[1] += v[1]*mtx[1][1]
    "vmla.f32       d5, d1, d10[1]                \n\t"    //dst[2] += v[1]*mtx[2][1]
    "vmla.f32       d3, d2, d7[0]                 \n\t"    //dst[0] += v[2]*mtx[0][2]
    "vmla.f32       d4, d2, d9[0]                 \n\t"    //dst[1] += v[2]*mtx[1][2]
    "vmla.f32       d5, d2, d11[0]                \n\t"    //dst[2] += v[2]*mtx[2][2]

    // Normalize 2x
    "vmul.f32       d0, d3, d3                    \n\t"    //len = dst[0]*dst[0]
    "vmla.f32       d0, d4, d4                    \n\t"    //len += dst[1]*dst[1]
    "vmla.f32       d0, d5, d5                    \n\t"    //len += dst[2]*dst[2]

    "vmov.f32       d1, d0                        \n\t"    //d1 = d0
    "vrsqrte.f32    d0, d0                        \n\t"    //d0 = ~ 1.0 / sqrt(d0)
    "vmul.f32       d2, d0, d1                    \n\t"    //d2 = d0 * d1
    "vrsqrts.f32    d6, d2, d0                    \n\t"    //d3 = (3 - d0 * d2) / 2
    "vmul.f32       d0, d0, d6                    \n\t"    //d0 = d0 * d3
    "vmul.f32       d2, d0, d1                    \n\t"    //d2 = d0 * d1
    "vrsqrts.f32    d6, d2, d0                    \n\t"    //d3 = (3 - d0 * d3) / 2
    "vmul.f32       d0, d0, d6                    \n\t"    //d0 = d0 * d3

    "vmul.f32       d3, d3, d0                    \n\t"    //dst[0] = dst[0]*(1/len)
    "vmul.f32       d4, d4, d0                    \n\t"    //dst[1] = dst[1]*(1/len)
    "vmul.f32       d5, d5, d0                    \n\t"    //dst[2] = dst[2]*(1/len)

    // Store two dst vectors
    "vst3.32        {d3, d4, d5}, [%2]            \n\t"    // store vector 0-1
    : "+r"(mtx), "+r"(src), "+r"(dst)  :
    : "d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d11", "memory"
    );
}

void InverseTransformVectorNormalize4x(float src[4][3], float dst[4][3], float mtx[4][4] )
{
    asm volatile (
    // Load four src vectors
    "vld3.32        {d0,d2,d4}, [%1]!            \n\t"    // load vector 0-1
    "vld3.32        {d1,d3,d5}, [%1]             \n\t"    // load vector 2-3
    
    // Load mtx
    "vld1.32        {d6-d9}, [%0]!               \n\t"    // load row 0-1
    "vld1.32        {d10-d11}, [%0]              \n\t"    // load row 2
    
    // Multiply and add
    "vmul.f32       q8, q0, d6[0]                \n\t"    //dst[0] = v[0]*mtx[0][0]
    "vmul.f32       q9, q0, d8[0]                \n\t"    //dst[1] = v[0]*mtx[1][0]
    "vmul.f32       q10, q0, d10[0]              \n\t"    //dst[2] = v[0]*mtx[2][0]
    "vmla.f32       q8, q1, d6[1]                \n\t"    //dst[0] += v[1]*mtx[0][1]
    "vmla.f32       q9, q1, d8[1]                \n\t"    //dst[1] += v[1]*mtx[1][1]
    "vmla.f32       q10, q1, d10[1]              \n\t"    //dst[2] += v[1]*mtx[2][1]
    "vmla.f32       q8, q2, d7[0]                \n\t"    //dst[0] += v[0]*mtx[2][1]
    "vmla.f32       q9, q2, d9[0]                \n\t"    //dst[1] += v[2]*mtx[2][1]
    "vmla.f32       q10, q2, d11[0]              \n\t"    //dst[2] += v[2]*mtx[2][2]

    // Normalize 4x
    "vmul.f32       q0, q8, q8                   \n\t"    //len = dst[0]*dst[0]
    "vmla.f32       q0, q9, q9                   \n\t"    //len += dst[1]*dst[1]
    "vmla.f32       q0, q10, q10                 \n\t"    //len += dst[2]*dst[2]

    "vmov.f32       q1, q0                       \n\t"    //d1 = len
    "vrsqrte.f32    q0, q0                       \n\t"    //q0 = ~ 1.0 / sqrt(q0)
    "vmul.f32       q7, q0, q1                   \n\t"    //q7 = q0 * q1
    "vrsqrts.f32    q6, q7, q0                   \n\t"    //q6 = (3 - q0 * q7) / 2
    "vmul.f32       q0, q0, q6                   \n\t"    //d0 = d0 * d15
    "vmul.f32       q7, q0, q1                   \n\t"    //q7 = q0 * q1
    "vrsqrts.f32    q6, q7, q0                   \n\t"    //q6 = (3 - q0 * q7) / 2
    "vmul.f32       q0, q0, q6                   \n\t"    //q0 = q0 * q6

    "vmul.f32       q8, q8, q0                   \n\t"    //dst[0] = dst[0]*(1/len)
    "vmul.f32       q9, q9, q0                   \n\t"    //dst[1] = dst[1]*(1/len)
    "vmul.f32       q10, q10, q0                 \n\t"    //dst[2] = dst[2]*(1/len)

    // Store four dst vectors
    "vst3.32        {d16, d18, d20}, [%2]!       \n\t"    // store vector 0-1
    "vst3.32        {d17, d19, d21}, [%2]        \n\t"    // store vector 2-3

    : "+r"(mtx), "+r"(src), "+r"(dst) :
    : "d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d11","d12","d13","d14","d15","d16","d17","d18","d19",
    "d20","d21", "memory"
    );
}
