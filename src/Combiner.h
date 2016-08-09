#ifndef COMBINER_H
#define COMBINER_H

#include <map>

#include "GLideN64.h"
#include "OpenGL.h"
#include "gDP.h"
#include "Types.h"

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
#define LIGHT			21
#define HW_LIGHT		22

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

class ShaderCombiner;
class UniformCollection;
class CombinerInfo
{
public:
	void init();
	void destroy();
	void update();
	void setCombine(u64 _mux);

	ShaderCombiner * getCurrent() const {return m_pCurrent;}
	bool isChanged() const {return m_bChanged;}
	bool isShaderCacheSupported() const { return m_bShaderCacheSupported; }
	size_t getCombinersNumber() const { return m_combiners.size();  }

	static CombinerInfo & get();

	void updatePrimColor();
	void updateEnvColor();
	void updateFogColor();
	void updateBlendColor();
	void updateKeyColor();
	void updateConvertColor();

	void updateTextureParameters();
	void updateLightParameters();
	// Update uniforms for GL without UniformBlock support
	void updateParameters(OGLRender::RENDER_STATE _renderState);

private:
	CombinerInfo()
		: m_bChanged(false)
		, m_bShaderCacheSupported(false)
		, m_shadersLoaded(0)
		, m_configOptionsBitSet(0)
		, m_pCurrent(nullptr) {}
	CombinerInfo(const CombinerInfo &);

	void _saveShadersStorage() const;
	bool _loadShadersStorage();
	u32 _getConfigOptionsBitSet() const;
	ShaderCombiner * _compile(u64 mux) const;

	bool m_bChanged;
	bool m_bShaderCacheSupported;
	u32 m_shadersLoaded;
	u32 m_configOptionsBitSet;

	ShaderCombiner * m_pCurrent;
	typedef std::map<u64, ShaderCombiner *> Combiners;
	Combiners m_combiners;
	UniformCollection * m_pUniformCollection;
};

inline
ShaderCombiner * currentCombiner() {
	return CombinerInfo::get().getCurrent();
}

void Combiner_Init();
void Combiner_Destroy();

inline
u64 getCombinerKey(u64 _mux)
{
	gDPCombine cmb;
	cmb.mux = _mux;
	cmb.muxs0 |= (gDP.otherMode.cycleType<<24);
	return cmb.mux;
}

#endif

