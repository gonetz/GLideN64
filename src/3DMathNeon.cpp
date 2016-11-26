#include "3DMath.h"

void MultMatrix( float m0[4][4], float m1[4][4], float dest[4][4])
{
	asm volatile (
	"vld1.32 		{d0, d1}, [%1]!			\n\t"	//q0 = m1
	"vld1.32 		{d2, d3}, [%1]!	    	\n\t"	//q1 = m1+4
	"vld1.32 		{d4, d5}, [%1]!	    	\n\t"	//q2 = m1+8
	"vld1.32 		{d6, d7}, [%1]	    	\n\t"	//q3 = m1+12
	"vld1.32 		{d16, d17}, [%0]!		\n\t"	//q8 = m0
	"vld1.32 		{d18, d19}, [%0]!   	\n\t"	//q9 = m0+4
	"vld1.32 		{d20, d21}, [%0]!   	\n\t"	//q10 = m0+8
	"vld1.32 		{d22, d23}, [%0]    	\n\t"	//q11 = m0+12

	"vmul.f32 		q12, q8, d0[0] 			\n\t"	//q12 = q8 * d0[0]
	"vmul.f32 		q13, q8, d2[0] 		    \n\t"	//q13 = q8 * d2[0]
	"vmul.f32 		q14, q8, d4[0] 		    \n\t"	//q14 = q8 * d4[0]
	"vmul.f32 		q15, q8, d6[0]	 		\n\t"	//q15 = q8 * d6[0]
	"vmla.f32 		q12, q9, d0[1] 			\n\t"	//q12 = q9 * d0[1]
	"vmla.f32 		q13, q9, d2[1] 		    \n\t"	//q13 = q9 * d2[1]
	"vmla.f32 		q14, q9, d4[1] 		    \n\t"	//q14 = q9 * d4[1]
	"vmla.f32 		q15, q9, d6[1] 		    \n\t"	//q15 = q9 * d6[1]
	"vmla.f32 		q12, q10, d1[0] 		\n\t"	//q12 = q10 * d0[0]
	"vmla.f32 		q13, q10, d3[0] 		\n\t"	//q13 = q10 * d2[0]
	"vmla.f32 		q14, q10, d5[0] 		\n\t"	//q14 = q10 * d4[0]
	"vmla.f32 		q15, q10, d7[0] 		\n\t"	//q15 = q10 * d6[0]
	"vmla.f32 		q12, q11, d1[1] 		\n\t"	//q12 = q11 * d0[1]
	"vmla.f32 		q13, q11, d3[1] 		\n\t"	//q13 = q11 * d2[1]
	"vmla.f32 		q14, q11, d5[1] 		\n\t"	//q14 = q11 * d4[1]
	"vmla.f32 		q15, q11, d7[1]	 	    \n\t"	//q15 = q11 * d6[1]

	"vst1.32 		{d24, d25}, [%2]! 		\n\t"	//d = q12
	"vst1.32 		{d26, d27}, [%2]! 	    \n\t"	//d+4 = q13
	"vst1.32 		{d28, d29}, [%2]! 	    \n\t"	//d+8 = q14
	"vst1.32 		{d30, d31}, [%2] 	    \n\t"	//d+12 = q15

	:"+r"(m0), "+r"(m1), "+r"(dest):
	: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
	"d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
	"d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
	"memory"
	);
}

void TransformVectorNormalize(float vec[3], float mtx[4][4])
{
	asm volatile (
	"vld1.32 		{d0}, [%1]  			\n\t"	//Q0 = v
	"flds    		s2, [%1, #8]  			\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]	    \n\t"	//Q3 = m+8

	"vmul.f32 		q2, q9, d0[0]			\n\t"	//q2 = q9*Q0[0]
	"vmla.f32 		q2, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q2, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]

	"vmul.f32 		d0, d4, d4				\n\t"	//d0 = d0*d0
	"vpadd.f32 		d0, d0, d0				\n\t"	//d0 = d[0] + d[1]
	"vmla.f32 		d0, d5, d5				\n\t"	//d0 = d0 + d1*d1

	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d3) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4

	"vmul.f32 		q2, q2, d0[0]			\n\t"	//d0= d2*d4

	"vst1.32 		{d4}, [%1] 	    	    \n\t"	//Q4 = m+12
	"fsts   		s10, [%1, #8] 	    	\n\t"	//Q4 = m+12
	: "+r"(mtx): "r"(vec)
	: "d0","d1","d2","d3","d18","d19","d20","d21","d22", "d23", "memory"
	);
}

void InverseTransformVectorNormalize(float src[3], float dst[3], float mtx[4][4])
{
	asm volatile (
	"vld1.32 		{d0}, [%1]  			\n\t"	//Q0 = v
	"flds    		s2, [%1, #8]  			\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//D18 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//D20 = m+4
	"vld1.32 		{d22, d23}, [%0]	    \n\t"	//D22 = m+8

	"vmul.f32 		q2, q0, q9				\n\t"
	"vmul.f32 		q3, q0, q10				\n\t"
	"vmul.f32 		q4, q0, q11				\n\t"
	"vpadd.f32 		d4, d4, d5				\n\t"
	"vpadd.f32 		d4, d4, d4				\n\t"  //d4[0] = sum of q2
	"vpadd.f32 		d8, d8, d9				\n\t"
	"vpadd.f32 		d5, d8, d8				\n\t"  //d5[0] = sum of q4
	"vpadd.f32 		d6, d6, d7				\n\t"
	"vpadd.f32 		d10, d6, d6				\n\t"
	"vmov.f32 		s9, s20					\n\t"  //d4[1] = sum of q3

	"vmul.f32 		d0, d4, d4				\n\t"	//d0 = d0*d0
	"vpadd.f32 		d0, d0, d0				\n\t"	//d0 = d[0] + d[1]
	"vmla.f32 		d0, d5, d5				\n\t"	//d0 = d0 + d1*d1

	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d3) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4

	"vmul.f32 		q2, q2, d0[0]			\n\t"	//d0= d2*d4

	"vst1.32 		{d4}, [%2] 	    	    \n\t"	//Q4 = m+12
	"fsts   		s10, [%2, #8] 	    	\n\t"	//Q4 = m+12
	: "+r"(mtx): "r"(src), "r"(dst)
	: "d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d18","d19",
	"d20","d21","d22", "d23", "memory"
	);
}

void Normalize(float v[3])
{
	asm volatile (
	"vld1.32 		{d4}, [%0]!	    		\n\t"	//d4={x,y}
	"flds    		s10, [%0]   	    	\n\t"	//d5[0] = z
	"sub    		%0, %0, #8   	    	\n\t"	//d5[0] = z
	"vmul.f32 		d0, d4, d4				\n\t"	//d0= d4*d4
	"vpadd.f32 		d0, d0, d0				\n\t"	//d0 = d[0] + d[1]
	"vmla.f32 		d0, d5, d5				\n\t"	//d0 = d0 + d5*d5

	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d3) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4

	"vmul.f32 		q2, q2, d0[0]			\n\t"	//d0= d2*d4
	"vst1.32 		{d4}, [%0]!  			\n\t"	//d2={x0,y0}, d3={z0, w0}
	"fsts    		s10, [%0]     			\n\t"	//d2={x0,y0}, d3={z0, w0}

	:"+r"(v) :
	: "d0", "d1", "d2", "d3", "d4", "d5", "memory"
	);
}
float DotProduct(const float v0[3], const float v1[3])
{
	float dot;
	asm volatile (
	"vld1.32 		{d8}, [%1]!			\n\t"	//d8={x0,y0}
	"vld1.32 		{d10}, [%2]!		\n\t"	//d10={x1,y1}
	"flds 			s18, [%1, #0]	    \n\t"	//d9[0]={z0}
	"flds 			s22, [%2, #0]	    \n\t"	//d11[0]={z1}
	"vmul.f32 		d12, d8, d10		\n\t"	//d0= d2*d4
	"vpadd.f32 		d12, d12, d12		\n\t"	//d0 = d[0] + d[1]
	"vmla.f32 		d12, d9, d11		\n\t"	//d0 = d0 + d3*d5
	"fmrs	        %0, s24	    		\n\t"	//r0 = s0
	: "=r"(dot), "+r"(v0), "+r"(v1):
	: "d8", "d9", "d10", "d11", "d12"

	);
	return dot;
}
