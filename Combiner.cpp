#include "OpenGL.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "Debug.h"
#include "gDP.h"

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

void CombinerInfo::init()
{
	InitShaderCombiner();
	m_pCurrent = NULL;
}

void CombinerInfo::destroy()
{
	DestroyShaderCombiner();
	m_pCurrent = NULL;
	for (Combiners::iterator cur = m_combiners.begin(); cur != m_combiners.end(); ++cur)
		delete cur->second;
	m_combiners.clear();
}

static
void SimplifyCycle( CombineCycle *cc, CombinerStage *stage )
{
	// Load the first operand
	stage->op[0].op = LOAD;
	stage->op[0].param1 = cc->sa;
	stage->numOps = 1;

	// If we're just subtracting zero, skip it
	if (cc->sb != ZERO) {
		// Subtracting a number from itself is zero
		if (cc->sb == stage->op[0].param1)
			stage->op[0].param1 = ZERO;
		else {
			stage->op[1].op = SUB;
			stage->op[1].param1 = cc->sb;
			stage->numOps++;
		}
	}

	// If we either subtracted, or didn't load a zero
	if ((stage->numOps > 1) || (stage->op[0].param1 != ZERO)) {
		// Multiplying by zero is zero
		if (cc->m == ZERO) {
			stage->numOps = 1;
			stage->op[0].op = LOAD;
			stage->op[0].param1 = ZERO;
		} else {
			// Multiplying by one, so just do a load
			if ((stage->numOps == 1) && (stage->op[0].param1 == ONE))
				stage->op[0].param1 = cc->m;
			else {
				stage->op[stage->numOps].op = MUL;
				stage->op[stage->numOps].param1 = cc->m;
				stage->numOps++;
			}
		}
	}

	// Don't bother adding zero
	if (cc->a != ZERO) {
		// If all we have so far is zero, then load this instead
		if ((stage->numOps == 1) && (stage->op[0].param1 == ZERO))
			stage->op[0].param1 = cc->a;
		else {
			stage->op[stage->numOps].op = ADD;
			stage->op[stage->numOps].param1 = cc->a;
			stage->numOps++;
		}
	}

	// Handle interpolation
	if ((stage->numOps == 4) && (stage->op[1].param1 == stage->op[3].param1)) {
		stage->numOps = 1;
		stage->op[0].op = INTER;
		stage->op[0].param2 = stage->op[1].param1;
		stage->op[0].param3 = stage->op[2].param1;
	}
}

ShaderCombiner * CombinerInfo::_compile(u64 mux) const
{
	gDPCombine combine;

	combine.mux = mux;

	int numCycles;

	Combiner color, alpha;

	if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
		numCycles = 2;
		color.numStages = 2;
		alpha.numStages = 2;
	} else {
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

	for (int i = 0; i < numCycles; i++) {
		// Simplify each RDP combiner cycle into a combiner stage
		SimplifyCycle( &cc[i], &color.stage[i] );
		SimplifyCycle( &ac[i], &alpha.stage[i] );
	}

	return new ShaderCombiner( color, alpha, combine );
}

void CombinerInfo::setCombine(u64 _mux )
{
	if (m_pCurrent != NULL && m_pCurrent->getMux() == _mux) {
		m_bChanged = false;
		m_pCurrent->update();
		return;
	}
	Combiners::const_iterator iter = m_combiners.find(_mux);
	if (iter != m_combiners.end())
		m_pCurrent = iter->second;
	else {
		m_pCurrent = _compile(_mux);
		m_combiners[_mux] = m_pCurrent;
	}
	m_pCurrent->update();
	m_bChanged = true;
	gDP.changed |= CHANGED_COMBINE_COLORS;
}
