#include "OpenGL.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "Debug.h"
#include "gDP.h"

CombinerInfo combiner;

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

void Combiner_Init()
{
	InitGLSLCombiner();
	combiner.root = NULL;
	combiner.current = NULL;
}

void Combiner_UpdateCombineColors()
{
	combiner.current->compiled->UpdateColors();
	gDP.changed &= ~CHANGED_COMBINE_COLORS;
}

void Combiner_UpdateCombineFBInfo()
{
	combiner.current->compiled->UpdateFBInfo();
}

void Combiner_UpdateCombineDepthInfo()
{
	if (combiner.current != NULL)
		combiner.current->compiled->UpdateDepthInfo();
}

void Combiner_UpdateAlphaTestInfo()
{
	if (combiner.current != NULL)
		combiner.current->compiled->UpdateAlphaTestInfo();
}

void Combiner_UpdateTextureInfo()
{
	if (combiner.current != NULL)
		combiner.current->compiled->UpdateTextureInfo();
}

void Combiner_UpdateRenderState() {
	if (combiner.current != NULL)
		combiner.current->compiled->UpdateRenderState();
}

void Combiner_SimplifyCycle( CombineCycle *cc, CombinerStage *stage )
{
	// Load the first operand
	stage->op[0].op = LOAD;
	stage->op[0].param1 = cc->sa;
	stage->numOps = 1;

	// If we're just subtracting zero, skip it
	if (cc->sb != ZERO)
	{
		// Subtracting a number from itself is zero
		if (cc->sb == stage->op[0].param1)
			stage->op[0].param1 = ZERO;
		else
		{
			stage->op[1].op = SUB;
			stage->op[1].param1 = cc->sb;
			stage->numOps++;
		}
	}

	// If we either subtracted, or didn't load a zero
	if ((stage->numOps > 1) || (stage->op[0].param1 != ZERO))
	{
		// Multiplying by zero is zero
		if (cc->m == ZERO)
		{
			stage->numOps = 1;
			stage->op[0].op = LOAD;
			stage->op[0].param1 = ZERO;
		}
		else
		{
			// Multiplying by one, so just do a load
			if ((stage->numOps == 1) && (stage->op[0].param1 == ONE))
				stage->op[0].param1 = cc->m;
			else
			{
				stage->op[stage->numOps].op = MUL;
				stage->op[stage->numOps].param1 = cc->m;
				stage->numOps++;
			}
		}
	}

	// Don't bother adding zero
	if (cc->a != ZERO)
	{
		// If all we have so far is zero, then load this instead
		if ((stage->numOps == 1) && (stage->op[0].param1 == ZERO))
			stage->op[0].param1 = cc->a;
		else
		{
			stage->op[stage->numOps].op = ADD;
			stage->op[stage->numOps].param1 = cc->a;
			stage->numOps++;
		}
	}

	// Handle interpolation
	if ((stage->numOps == 4) && (stage->op[1].param1 == stage->op[3].param1))
	{
		stage->numOps = 1;
		stage->op[0].op = INTER;
		stage->op[0].param2 = stage->op[1].param1;
		stage->op[0].param3 = stage->op[2].param1;
	}
}

CachedCombiner *Combiner_Compile( u64 mux )
{
	gDPCombine combine;

	combine.mux = mux;

	int numCycles;

	Combiner color, alpha;

	if (gDP.otherMode.cycleType == G_CYC_2CYCLE)
	{
		numCycles = 2;
		color.numStages = 2;
		alpha.numStages = 2;
	}
	else
	{
		numCycles = 1;
		color.numStages = 1;
		alpha.numStages = 1;
	}

	CombineCycle cc[2];
	CombineCycle ac[2];

	// Decode and expand the combine mode into a more general form
	cc[0].sa = saRGBExpanded[combine.saRGB0];
	cc[0].sb = sbRGBExpanded[combine.sbRGB0];
	cc[0].m  = mRGBExpanded[combine.mRGB0];
	cc[0].a  = aRGBExpanded[combine.aRGB0];
	ac[0].sa = saAExpanded[combine.saA0];
	ac[0].sb = sbAExpanded[combine.sbA0];
	ac[0].m  = mAExpanded[combine.mA0];
	ac[0].a  = aAExpanded[combine.aA0];

	cc[1].sa = saRGBExpanded[combine.saRGB1];
	cc[1].sb = sbRGBExpanded[combine.sbRGB1];
	cc[1].m  = mRGBExpanded[combine.mRGB1];
	cc[1].a  = aRGBExpanded[combine.aRGB1];
	ac[1].sa = saAExpanded[combine.saA1];
	ac[1].sb = sbAExpanded[combine.sbA1];
	ac[1].m  = mAExpanded[combine.mA1];
	ac[1].a  = aAExpanded[combine.aA1];

	for (int i = 0; i < numCycles; i++)
	{
		// Simplify each RDP combiner cycle into a combiner stage
		Combiner_SimplifyCycle( &cc[i], &color.stage[i] );
		Combiner_SimplifyCycle( &ac[i], &alpha.stage[i] );
	}

	CachedCombiner *cached = (CachedCombiner*)malloc( sizeof( CachedCombiner ) );

	cached->combine.mux = combine.mux;
	cached->left = NULL;
	cached->right = NULL;

	cached->compiled = new GLSLCombiner( &color, &alpha );

	return cached;
}

void Combiner_DeleteCombiner( CachedCombiner *combiner )
{
	if (combiner->left) Combiner_DeleteCombiner( combiner->left );
	if (combiner->right) Combiner_DeleteCombiner( combiner->right );

	delete combiner->compiled;
	free( combiner );
}

void Combiner_Destroy()
{
	DestroyGLSLCombiner();

	if (combiner.root)
	{
		Combiner_DeleteCombiner( combiner.root );
		combiner.root = NULL;
	}
}

DWORD64 Combiner_EncodeCombineMode( WORD saRGB0, WORD sbRGB0, WORD mRGB0, WORD aRGB0,
								   WORD saA0,   WORD sbA0,   WORD mA0,   WORD aA0,
								   WORD saRGB1, WORD sbRGB1, WORD mRGB1, WORD aRGB1,
								   WORD saA1,   WORD sbA1,   WORD mA1,   WORD aA1 )
{
	return (((DWORD64)CCEncodeA[saRGB0] << 52) | ((DWORD64)CCEncodeB[sbRGB0] << 28) | ((DWORD64)CCEncodeC[mRGB0] << 47) | ((DWORD64)CCEncodeD[aRGB0] << 15) |
		((DWORD64)ACEncodeA[saA0] << 44) | ((DWORD64)ACEncodeB[sbA0] << 12) | ((DWORD64)ACEncodeC[mA0] << 41) | ((DWORD64)ACEncodeD[aA0] << 9) |
		((DWORD64)CCEncodeA[saRGB1] << 37) | ((DWORD64)CCEncodeB[sbRGB1] << 24) | ((DWORD64)CCEncodeC[mRGB1]      ) | ((DWORD64)CCEncodeD[aRGB1] <<  6) |
		((DWORD64)ACEncodeA[saA1] << 18) | ((DWORD64)ACEncodeB[sbA1] <<  3) | ((DWORD64)ACEncodeC[mA1] << 18) | ((DWORD64)ACEncodeD[aA1]     ));
}

void Combiner_SelectCombine( u64 mux )
{
	if (combiner.current != NULL && combiner.current->combine.mux == mux) {
		combiner.changed = false;
		return;
	}
	CachedCombiner *current = combiner.root;
	CachedCombiner *parent = current;

	while (current)
	{
		parent = current;

		if (mux == current->combine.mux)
			break;
		else if (mux < current->combine.mux)
			current = current->left;
		else
			current = current->right;
	}

	if (current == NULL)
	{
		current = Combiner_Compile( mux );

		if (parent == NULL)
			combiner.root = current;
		else if (parent->combine.mux > current->combine.mux)
			parent->left = current;
		else
			parent->right = current;
	}

	combiner.current = current;
	combiner.changed = true;

	gDP.changed |= CHANGED_COMBINE_COLORS;
}

void Combiner_SetCombineStates()
{
	combiner.current->compiled->Set();
}

void Combiner_SetCombine( u64 mux )
{
	Combiner_SelectCombine( mux );
	Combiner_SetCombineStates();
}
