#ifndef COMBINER_H
#define COMBINER_H

#include "glN64.h"
#include "OpenGL.h"
#include "gDP.h"
#include "Types.h"

#define TEXTURE_ENV					0
#define TEXTURE_ENV_COMBINE			1
#define NV_REGISTER_COMBINERS		2
#define NV_TEXTURE_ENV_COMBINE4		3

/*
 * G_SETCOMBINE: color combine modes
 */
/* Color combiner constants: */
#define G_CCMUX_COMBINED	0
#define G_CCMUX_TEXEL0		1
#define G_CCMUX_TEXEL1		2
#define G_CCMUX_PRIMITIVE	3
#define G_CCMUX_SHADE		4
#define G_CCMUX_ENVIRONMENT	5
#define G_CCMUX_CENTER		6
#define G_CCMUX_SCALE		6
#define G_CCMUX_COMBINED_ALPHA	7
#define G_CCMUX_TEXEL0_ALPHA	8
#define G_CCMUX_TEXEL1_ALPHA	9
#define G_CCMUX_PRIMITIVE_ALPHA	10
#define G_CCMUX_SHADE_ALPHA	11
#define G_CCMUX_ENV_ALPHA	12
#define G_CCMUX_LOD_FRACTION	13
#define G_CCMUX_PRIM_LOD_FRAC	14
#define G_CCMUX_NOISE		7
#define G_CCMUX_K4		7
#define G_CCMUX_K5		15
#define G_CCMUX_1		6
#define G_CCMUX_0		31

/* Alpha combiner constants: */
#define G_ACMUX_COMBINED	0
#define G_ACMUX_TEXEL0		1
#define G_ACMUX_TEXEL1		2
#define G_ACMUX_PRIMITIVE	3
#define G_ACMUX_SHADE		4
#define G_ACMUX_ENVIRONMENT	5
#define G_ACMUX_LOD_FRACTION	0
#define G_ACMUX_PRIM_LOD_FRAC	6
#define G_ACMUX_1		6
#define G_ACMUX_0		7

#define EncodeCombineMode( a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0,	\
		a1, b1, c1, d1,	Aa1, Ab1, Ac1, Ad1 ) \
		(u64)(((u64)(_SHIFTL( G_CCMUX_##a0, 20, 4 ) | _SHIFTL( G_CCMUX_##c0, 15, 5 ) | \
		_SHIFTL( G_ACMUX_##Aa0, 12, 3 ) | _SHIFTL( G_ACMUX_##Ac0, 9, 3 ) | \
		_SHIFTL( G_CCMUX_##a1, 5, 4 ) | _SHIFTL( G_CCMUX_##c1, 0, 5 )) << 32) | \
		(u64)(_SHIFTL( G_CCMUX_##b0, 28, 4 ) | _SHIFTL( G_CCMUX_##d0, 15, 3 ) | \
		_SHIFTL( G_ACMUX_##Ab0, 12, 3 ) | _SHIFTL( G_ACMUX_##Ad0, 9, 3 ) | \
		_SHIFTL( G_CCMUX_##b1, 24, 4 ) | _SHIFTL( G_ACMUX_##Aa1, 21, 3 ) | \
		_SHIFTL( G_ACMUX_##Ac1, 18, 3 ) | _SHIFTL( G_CCMUX_##d1, 6, 3 ) | \
		_SHIFTL( G_ACMUX_##Ab1, 3, 3 ) | _SHIFTL( G_ACMUX_##Ad1, 0, 3 )))

#define	G_CC_PRIMITIVE				0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE
#define	G_CC_SHADE					0, 0, 0, SHADE, 0, 0, 0, SHADE
#define	G_CC_MODULATEI				TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE
#define	G_CC_MODULATEIA				TEXEL0, 0, SHADE, 0, TEXEL0, 0, SHADE, 0
#define	G_CC_MODULATEIDECALA		TEXEL0, 0, SHADE, 0, 0, 0, 0, TEXEL0
#define	G_CC_MODULATERGB			G_CC_MODULATEI
#define	G_CC_MODULATERGBA			G_CC_MODULATEIA
#define	G_CC_MODULATERGBDECALA		G_CC_MODULATEIDECALA
#define	G_CC_MODULATEI_PRIM			TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define	G_CC_MODULATEIA_PRIM		TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0
#define	G_CC_MODULATEIDECALA_PRIM	TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0
#define	G_CC_MODULATERGB_PRIM		G_CC_MODULATEI_PRIM
#define	G_CC_MODULATERGBA_PRIM		G_CC_MODULATEIA_PRIM
#define	G_CC_MODULATERGBDECALA_PRIM	G_CC_MODULATEIDECALA_PRIM
#define	G_CC_DECALRGB				0, 0, 0, TEXEL0, 0, 0, 0, SHADE
#define	G_CC_DECALRGBA				0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0
#define	G_CC_BLENDI					ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDIA				ENVIRONMENT, SHADE, TEXEL0, SHADE, TEXEL0, 0, SHADE, 0
#define	G_CC_BLENDIDECALA			ENVIRONMENT, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define	G_CC_BLENDRGBA				TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDRGBDECALA			TEXEL0, SHADE, TEXEL0_ALPHA, SHADE, 0, 0, 0, TEXEL0
#define G_CC_ADDRGB					1, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_ADDRGBDECALA			1, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_REFLECTRGB				ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_REFLECTRGBDECALA		ENVIRONMENT, 0, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_HILITERGB				PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define G_CC_HILITERGBA				PRIMITIVE, SHADE, TEXEL0, SHADE, PRIMITIVE, SHADE, TEXEL0, SHADE
#define G_CC_HILITERGBDECALA		PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, TEXEL0
#define G_CC_SHADEDECALA			0, 0, 0, SHADE, 0, 0, 0, TEXEL0
#define	G_CC_BLENDPE				PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0
#define	G_CC_BLENDPEDECALA			PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, TEXEL0
#define	_G_CC_BLENDPE				ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, TEXEL0, 0, SHADE, 0
#define	_G_CC_BLENDPEDECALA			ENVIRONMENT, PRIMITIVE, TEXEL0, PRIMITIVE, 0, 0, 0, TEXEL0
#define	_G_CC_TWOCOLORTEX			PRIMITIVE, SHADE, TEXEL0, SHADE, 0, 0, 0, SHADE
#define	_G_CC_SPARSEST				PRIMITIVE, TEXEL0, LOD_FRACTION, TEXEL0, PRIMITIVE, TEXEL0, LOD_FRACTION, TEXEL0
#define G_CC_TEMPLERP				TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0
#define	G_CC_TRILERP				TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0, TEXEL1, TEXEL0, LOD_FRACTION, TEXEL0
#define	G_CC_INTERFERENCE			TEXEL0, 0, TEXEL1, 0, TEXEL0, 0, TEXEL1, 0
#define	G_CC_1CYUV2RGB				TEXEL0, K4, K5, TEXEL0, 0, 0, 0, SHADE
#define	G_CC_YUV2RGB				TEXEL1, K4, K5, TEXEL1, 0, 0, 0, 0
#define	G_CC_PASS2					0, 0, 0, COMBINED, 0, 0, 0, COMBINED
#define	G_CC_MODULATEI2				COMBINED, 0, SHADE, 0, 0, 0, 0, SHADE
#define	G_CC_MODULATEIA2			COMBINED, 0, SHADE, 0, COMBINED, 0, SHADE, 0
#define	G_CC_MODULATERGB2			G_CC_MODULATEI2
#define	G_CC_MODULATERGBA2			G_CC_MODULATEIA2
#define	G_CC_MODULATEI_PRIM2		COMBINED, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define	G_CC_MODULATEIA_PRIM2		COMBINED, 0, PRIMITIVE, 0, COMBINED, 0, PRIMITIVE, 0
#define	G_CC_MODULATERGB_PRIM2		G_CC_MODULATEI_PRIM2
#define	G_CC_MODULATERGBA_PRIM2		G_CC_MODULATEIA_PRIM2
#define	G_CC_DECALRGB2				0, 0, 0, COMBINED, 0, 0, 0, SHADE
#define	G_CC_BLENDI2				ENVIRONMENT, SHADE, COMBINED, SHADE, 0, 0, 0, SHADE
#define	G_CC_BLENDIA2				ENVIRONMENT, SHADE, COMBINED, SHADE, COMBINED, 0, SHADE, 0
#define	G_CC_CHROMA_KEY2			TEXEL0, CENTER, SCALE, 0, 0, 0, 0, 0
#define G_CC_HILITERGB2				ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, SHADE
#define G_CC_HILITERGBA2			ENVIRONMENT, COMBINED, TEXEL0, COMBINED, ENVIRONMENT, COMBINED, TEXEL0, COMBINED
#define G_CC_HILITERGBDECALA2		ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, TEXEL0
#define G_CC_HILITERGBPASSA2		ENVIRONMENT, COMBINED, TEXEL0, COMBINED, 0, 0, 0, COMBINED

// Internal combiner commands
#define LOAD		0
#define SUB			1
#define MUL			2
#define ADD			3
#define INTER		4

// Internal generalized combiner inputs
#define COMBINED		0
#define TEXEL0			1
#define TEXEL1			2
#define PRIMITIVE		3
#define SHADE			4
#define ENVIRONMENT		5
#define CENTER			6
#define SCALE			7
#define COMBINED_ALPHA	8
#define TEXEL0_ALPHA	9
#define TEXEL1_ALPHA	10
#define PRIMITIVE_ALPHA	11
#define SHADE_ALPHA		12
#define ENV_ALPHA		13
#define LOD_FRACTION	14
#define PRIM_LOD_FRAC	15
#define NOISE			16
#define K4				17
#define K5				18
#define ONE				19
#define ZERO			20

struct CombinerOp
{
	int op;
	int param1;
	int param2;
	int param3;
};

struct CombinerStage
{
	int numOps;
	CombinerOp op[6];
};

struct Combiner
{
	int numStages;
	CombinerStage stage[2];
};

struct CombineCycle
{
	int sa, sb, m, a;
};

static int saRGBExpanded[] = 
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,		
	SHADE,				ENVIRONMENT,		ONE,				NOISE,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO
};

static int sbRGBExpanded[] = 
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,		
	SHADE,				ENVIRONMENT,		CENTER,				K4,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO
};

static int mRGBExpanded[] = 
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,		
	SHADE,				ENVIRONMENT,		SCALE,				COMBINED_ALPHA,
	TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,	SHADE_ALPHA,
	ENV_ALPHA,			LOD_FRACTION,		PRIM_LOD_FRAC,		K5,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO
};

static int aRGBExpanded[] = 
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,		
	SHADE,				ENVIRONMENT,		ONE,				ZERO
};

static int saAExpanded[] = 
{
	COMBINED,			TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,		
	SHADE_ALPHA,		ENV_ALPHA,			ONE,				ZERO
};

static int sbAExpanded[] = 
{
	COMBINED,			TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,		
	SHADE_ALPHA,		ENV_ALPHA,			ONE,				ZERO
};

static int mAExpanded[] = 
{
	LOD_FRACTION,		TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,		
	SHADE_ALPHA,		ENV_ALPHA,			PRIM_LOD_FRAC,		ZERO,
};

static int aAExpanded[] = 
{
	COMBINED,			TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,		
	SHADE_ALPHA,		ENV_ALPHA,			ONE,				ZERO
};

static int CCEncodeA[] =
{
	0, 1, 2, 3, 4, 5, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7, 15, 15, 6, 15
};

static int CCEncodeB[] =
{
	0, 1, 2, 3, 4, 5, 6, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7, 15, 15, 15
};

static int CCEncodeC[] =
{
	0, 1, 2, 3, 4, 5, 31, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31, 31, 15, 31, 31
};

static int CCEncodeD[] =
{
	0, 1, 2, 3, 4, 5, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 6, 15
};

static DWORD64 ACEncodeA[] =
{
	7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, 6, 7
};

static DWORD64 ACEncodeB[] =
{
	7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, 6, 7
};

static DWORD64 ACEncodeC[] =
{
	7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 6, 7, 7, 7, 7, 7
};

static DWORD64 ACEncodeD[] =
{
	7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, 6, 7
};

struct CachedCombiner
{
	gDPCombine combine;

	void *compiled;
	CachedCombiner *left, *right;
};

extern struct CombinerInfo
{
	struct
	{
		WORD color, secondaryColor, alpha;
	} vertex;

	CachedCombiner *root, *current;

	int compiler;

	BOOL usesT0, usesT1, usesNoise;
} combiner;

#define SetConstant( constant, color, alpha ) \
	switch (color) \
	{ \
		case PRIMITIVE: \
			constant.r = gDP.primColor.r; \
			constant.g = gDP.primColor.g; \
			constant.b = gDP.primColor.b; \
			break; \
		case ENVIRONMENT: \
			constant.r = gDP.envColor.r; \
			constant.g = gDP.envColor.g; \
			constant.b = gDP.envColor.b; \
			break; \
		case PRIMITIVE_ALPHA: \
			constant.r = gDP.primColor.a; \
			constant.g = gDP.primColor.a; \
			constant.b = gDP.primColor.a; \
			break; \
		case ENV_ALPHA: \
			constant.r = gDP.envColor.a; \
			constant.g = gDP.envColor.a; \
			constant.b = gDP.envColor.a; \
			break; \
		case PRIM_LOD_FRAC: \
			constant.r = gDP.primColor.l; \
			constant.g = gDP.primColor.l; \
			constant.b = gDP.primColor.l; \
			break; \
		case ONE: \
			constant.r = 1.0f; \
			constant.g = 1.0f; \
			constant.b = 1.0f; \
			break; \
		case ZERO: \
			constant.r = 0.0f; \
			constant.g = 0.0f; \
			constant.b = 0.0f; \
			break; \
	} \
\
	switch (alpha) \
	{ \
		case PRIMITIVE_ALPHA: \
			constant.a = gDP.primColor.a; \
			break; \
		case ENV_ALPHA: \
			constant.a = gDP.envColor.a; \
			break; \
		case PRIM_LOD_FRAC: \
			constant.a = gDP.primColor.l; \
			break; \
		case ONE: \
			constant.a = 1.0f; \
			break; \
		case ZERO: \
			constant.a = 0.0f; \
			break; \
	} 

void Combiner_Init();
void Combiner_UpdateCombineColors();
void Combiner_UpdateCombineMode();
void Combiner_SetCombine( u64 mux );
void Combiner_SelectCombine( u64 mux );
void Combiner_SetCombineStates();
void Combiner_Destroy();
void Combiner_BeginTextureUpdate();
void Combiner_EndTextureUpdate();
#endif

