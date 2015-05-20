#include "OpenGL.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "UniformCollection.h"
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

void Combiner_Init() {
	CombinerInfo & cmbInfo = CombinerInfo::get();
	cmbInfo.init();
	InitShaderCombiner();
	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	cmbInfo.setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
	cmbInfo.setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1));
}

void Combiner_Destroy() {
	DestroyShaderCombiner();
	CombinerInfo::get().destroy();
}

CombinerInfo & CombinerInfo::get()
{
	static CombinerInfo info;
	return info;
}

void CombinerInfo::init()
{
	m_pCurrent = NULL;
	m_pUniformCollection = createUniformCollection();
}

void CombinerInfo::destroy()
{
	delete m_pUniformCollection;
	m_pUniformCollection = NULL;
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

	numCycles = gDP.otherMode.cycleType + 1;
	color.numStages = numCycles;
	alpha.numStages = numCycles;

	CombineCycle cc[2];
	CombineCycle ac[2];

	// Decode and expand the combine mode into a more general form
	cc[1].sa = saRGBExpanded[combine.saRGB1];
	cc[1].sb = sbRGBExpanded[combine.sbRGB1];
	cc[1].m  = mRGBExpanded[combine.mRGB1];
	cc[1].a  = aRGBExpanded[combine.aRGB1];
	ac[1].sa = saAExpanded[combine.saA1];
	ac[1].sb = sbAExpanded[combine.sbA1];
	ac[1].m  = mAExpanded[combine.mA1];
	ac[1].a  = aAExpanded[combine.aA1];

	// Simplify each RDP combiner cycle into a combiner stage
	if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
		SimplifyCycle(&cc[1], &color.stage[0]);
		SimplifyCycle(&ac[1], &alpha.stage[0]);
	}
	else {
		cc[0].sa = saRGBExpanded[combine.saRGB0];
		cc[0].sb = sbRGBExpanded[combine.sbRGB0];
		cc[0].m = mRGBExpanded[combine.mRGB0];
		cc[0].a = aRGBExpanded[combine.aRGB0];
		ac[0].sa = saAExpanded[combine.saA0];
		ac[0].sb = sbAExpanded[combine.sbA0];
		ac[0].m = mAExpanded[combine.mA0];
		ac[0].a = aAExpanded[combine.aA0];

		SimplifyCycle(&cc[0], &color.stage[0]);
		SimplifyCycle(&ac[0], &alpha.stage[0]);
		SimplifyCycle(&cc[1], &color.stage[1]);
		SimplifyCycle(&ac[1], &alpha.stage[1]);
	}

	return new ShaderCombiner( color, alpha, combine );
}

void CombinerInfo::update()
{
	// TODO: find, why gDP.changed & CHANGED_COMBINE not always works (e.g. Mario Tennis).
//	if (gDP.changed & CHANGED_COMBINE) {
		if (gDP.otherMode.cycleType == G_CYC_COPY)
			setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
		else if (gDP.otherMode.cycleType == G_CYC_FILL)
			setCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE));
		else
			setCombine(gDP.combine.mux);
		gDP.changed &= ~CHANGED_COMBINE;
//	}
}

void CombinerInfo::setCombine(u64 _mux )
{
	if (m_pCurrent != NULL && m_pCurrent->getMux() == _mux) {
		m_bChanged = false;
		m_pCurrent->update(false);
		return;
	}
	Combiners::const_iterator iter = m_combiners.find(_mux);
	if (iter != m_combiners.end()) {
		m_pCurrent = iter->second;
		m_pCurrent->update(false);
	} else {
		m_pCurrent = _compile(_mux);
		m_pCurrent->update(true);
		m_pUniformCollection->bindWithShaderCombiner(m_pCurrent);
		m_combiners[_mux] = m_pCurrent;
	}
	m_bChanged = true;
}

void CombinerInfo::updatePrimColor()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->setColorData(UniformCollection::cuPrimColor, sizeof(f32)* 5, &gDP.primColor.r);
}

void CombinerInfo::updateEnvColor()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->setColorData(UniformCollection::cuEnvColor, sizeof(f32)* 4, &gDP.envColor.r);
}

void CombinerInfo::updateFogColor()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->setColorData(UniformCollection::cuFogColor, sizeof(f32)* 4, &gDP.fogColor.r);
}

void CombinerInfo::updateBlendColor()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->setColorData(UniformCollection::cuBlendColor, sizeof(f32)* 4, &gDP.blendColor.r);
}

void CombinerInfo::updateKeyColor()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->setColorData(UniformCollection::cuCenterColor, sizeof(f32)* 8, &gDP.key.center.r);
}

void CombinerInfo::updateConvertColor()
{
	if (m_pUniformCollection == NULL)
		return;
	f32 convert[2] = { gDP.convert.k4*0.0039215689f, gDP.convert.k5*0.0039215689f };
	m_pUniformCollection->setColorData(UniformCollection::cuK4, sizeof(convert), convert);
}

void CombinerInfo::updateTextureParameters()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->updateTextureParameters();
}

void CombinerInfo::updateLightParameters()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->updateLightParameters();
	gSP.changed &= ~CHANGED_LIGHT;
}

void CombinerInfo::updateParameters()
{
	if (m_pUniformCollection != NULL)
		m_pUniformCollection->updateUniforms(m_pCurrent);
}
