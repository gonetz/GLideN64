#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <assert.h>
#include "N64.h"
#include "GLideN64.h"
#include "Debug.h"
#include "Types.h"
#include "RSP.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "3DMath.h"
#include "OpenGL.h"
#include "CRC.h"
#include <string.h>
#include "convert.h"
#include "S2DEX.h"
#include "VI.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "Config.h"
#include "Log.h"

void gSPTransformVertex4NEON(u32 v, float mtx[4][4])
{
	OGLRender & render = video().getRender();
	SPVertex & vtx = render.getVertex(v);
	void *ptr = &vtx.x;

	asm volatile (
	"vld1.32 		{d0, d1}, [%1]		  	\n\t"	//q0 = {x,y,z,w}
	"add 		    %1, %1, %2   		  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d2, d3}, [%1]	    	\n\t"	//q1 = {x,y,z,w}
	"add 		    %1, %1, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d4, d5}, [%1]	        \n\t"	//q2 = {x,y,z,w}
	"add 		    %1, %1, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d6, d7}, [%1]	        \n\t"	//q3 = {x,y,z,w}
	"sub 		    %1, %1, %3   		  	\n\t"	//q0 = {x,y,z,w}

	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//q9 = m
	"vld1.32 		{d20, d21}, [%0]!       \n\t"	//q10 = m
	"vld1.32 		{d22, d23}, [%0]!       \n\t"	//q11 = m
	"vld1.32 		{d24, d25}, [%0]        \n\t"	//q12 = m

	"vmov.f32 		q13, q12    			\n\t"	//q13 = q12
	"vmov.f32 		q14, q12    			\n\t"	//q14 = q12
	"vmov.f32 		q15, q12    			\n\t"	//q15 = q12

	"vmla.f32 		q12, q9, d0[0]			\n\t"	//q12 = q9*d0[0]
	"vmla.f32 		q13, q9, d2[0]			\n\t"	//q13 = q9*d0[0]
	"vmla.f32 		q14, q9, d4[0]			\n\t"	//q14 = q9*d0[0]
	"vmla.f32 		q15, q9, d6[0]			\n\t"	//q15 = q9*d0[0]
	"vmla.f32 		q12, q10, d0[1]			\n\t"	//q12 = q10*d0[1]
	"vmla.f32 		q13, q10, d2[1]			\n\t"	//q13 = q10*d0[1]
	"vmla.f32 		q14, q10, d4[1]			\n\t"	//q14 = q10*d0[1]
	"vmla.f32 		q15, q10, d6[1]			\n\t"	//q15 = q10*d0[1]
	"vmla.f32 		q12, q11, d1[0]			\n\t"	//q12 = q11*d1[0]
	"vmla.f32 		q13, q11, d3[0]			\n\t"	//q13 = q11*d1[0]
	"vmla.f32 		q14, q11, d5[0]			\n\t"	//q14 = q11*d1[0]
	"vmla.f32 		q15, q11, d7[0]			\n\t"	//q15 = q11*d1[0]

	"vst1.32 		{d24, d25}, [%1] 		\n\t"	//q12
	"add 		    %1, %1, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d26, d27}, [%1] 	    \n\t"	//q13
	"add 		    %1, %1, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d28, d29}, [%1] 	    \n\t"	//q14
	"add 		    %1, %1, %2   		  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d30, d31}, [%1]     	\n\t"	//q15

	: "+&r"(mtx), "+&r"(ptr)
	: "I"(sizeof(SPVertex)), "I"(3 * sizeof(SPVertex))
	: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
	  "d18","d19", "d20", "d21", "d22", "d23", "d24",
	  "d25", "d26", "d27", "d28", "d29", "d30", "d31", "memory"
	);
}

void gSPBillboardVertex4NEON(u32 v)
{
	int i = 0;

	OGLRender & render = video().getRender();
	SPVertex & vtx0 = render.getVertex(v);
	SPVertex & vtx1 = render.getVertex(i);

	void *ptr0 = (void*)&vtx0.x;
	void *ptr1 = (void*)&vtx1.x;
	asm volatile (

	"vld1.32 		{d0, d1}, [%0]		  	\n\t"	//q0 = {x,y,z,w}
	"add 		    %0, %0, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d2, d3}, [%0]	    	\n\t"	//q1 = {x,y,z,w}
	"add 		    %0, %0, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d4, d5}, [%0]	        \n\t"	//q2 = {x,y,z,w}
	"add 		    %0, %0, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d6, d7}, [%0]	        \n\t"	//q3 = {x,y,z,w}
	"sub 		    %0, %0, %3   		  	\n\t"	//q0 = {x,y,z,w}

	"vld1.32 		{d16, d17}, [%1]		\n\t"	//q2={x1,y1,z1,w1}
	"vadd.f32 		q0, q0, q8 			    \n\t"	//q1=q1+q1
	"vadd.f32 		q1, q1, q8 			    \n\t"	//q1=q1+q1
	"vadd.f32 		q2, q2, q8 			    \n\t"	//q1=q1+q1
	"vadd.f32 		q3, q3, q8 			    \n\t"	//q1=q1+q1
	"vst1.32 		{d0, d1}, [%0] 		    \n\t"	//
	"add 		    %0, %0, %2  		  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d2, d3}, [%0]          \n\t"	//
	"add 		    %0, %0, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d4, d5}, [%0]          \n\t"	//
	"add 		    %0, %0, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d6, d7}, [%0]          \n\t"	//
	: "+&r"(ptr0), "+&r"(ptr1)
	: "I"(sizeof(SPVertex)), "I"(3 * sizeof(SPVertex))
	: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
	  "d16", "d17", "memory"
	);
}
