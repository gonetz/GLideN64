#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

#include <vector>
#include "gDP.h"
#include "Combiner.h"

class ShaderCombiner {
public:
	ShaderCombiner(Combiner & _color, Combiner & _alpha, const gDPCombine & _combine);
	~ShaderCombiner();

	void update(bool _bForce);
	void updateFogMode(bool _bForce = false);
	void updateDitherMode(bool _bForce = false);
	void updateLOD(bool _bForce = false);
	void updateFBInfo(bool _bForce = false);
	void updateDepthInfo(bool _bForce = false);
	void updateAlphaTestInfo(bool _bForce = false);
	void updateTextureInfo(bool _bForce = false);
	void updateRenderState(bool _bForce = false);
	void updateGammaCorrection(bool _bForce = false);

	u64 getMux() const {return m_combine.mux;}

	bool usesTile(u32 _t) const {
		if (_t == 0)
			return (m_nInputs & ((1<<TEXEL0)|(1<<TEXEL0_ALPHA))) != 0;
		return (m_nInputs & ((1 << TEXEL1) | (1 << TEXEL1_ALPHA))) != 0;
	}
	bool usesTexture() const { return (m_nInputs & ((1 << TEXEL1)|(1 << TEXEL1_ALPHA)|(1 << TEXEL0)|(1 << TEXEL0_ALPHA))) != 0; }
	bool usesLOD() const { return (m_nInputs & (1 << LOD_FRACTION)) != 0; }
	bool usesShade() const { return (m_nInputs & ((1 << SHADE) | (1 << SHADE_ALPHA))) != 0; }
	bool usesShadeColor() const { return (m_nInputs & (1 << SHADE)) != 0; }

private:
	friend class UniformBlock;
	friend class UniformSet;

	struct iUniform	{
		GLint loc;
		int val;
		void set(int _val, bool _force) {
			if (loc >= 0 && (_force || val != _val)) {
				val = _val;
				glUniform1i(loc, _val);
			}
		}
	};

	struct fUniform {
		GLint loc;
		float val;
		void set(float _val, bool _force) {
			if (loc >= 0 && (_force || val != _val)) {
				val = _val;
				glUniform1f(loc, _val);
			}
		}
	};

	struct fv2Uniform {
		GLint loc;
		float val[2];
		void set(float _val1, float _val2, bool _force) {
			if (loc >= 0 && (_force || val[0] != _val1 || val[1] != _val2)) {
				val[0] = _val1;
				val[1] = _val2;
				glUniform2f(loc, _val1, _val2);
			}
		}
	};

	struct iv2Uniform {
		GLint loc;
		int val[2];
		void set(int _val1, int _val2, bool _force) {
			if (loc >= 0 && (_force || val[0] != _val1 || val[1] != _val2)) {
				val[0] = _val1;
				val[1] = _val2;
				glUniform2i(loc, _val1, _val2);
			}
		}
	};

	struct UniformLocation
	{
		iUniform uTex0, uTex1, uMSTex0, uMSTex1, uTexNoise, uTlutImage, uZlutImage, uDepthImage,
			uFogMode, uFogUsage, uEnableLod, uEnableAlphaTest,
			uEnableDepth, uEnableDepthCompare, uEnableDepthUpdate,
			uDepthMode, uDepthSource, uRenderState, uSpecialBlendMode,
			uMaxTile, uTextureDetail, uTexturePersp, uTextureFilterMode, uMSAASamples,
			uAlphaCompareMode, uAlphaDitherMode, uColorDitherMode, uGammaCorrectionEnabled;

		fUniform uFogAlpha, uPrimitiveLod, uMinLod, uDeltaZ, uAlphaTestValue, uMSAAScale;

		fv2Uniform uScreenScale, uDepthScale, uFogScale;

		iv2Uniform uMSTexEnabled, uFb8Bit, uFbFixedAlpha;
	};

#ifdef OS_MAC_OS_X
#define glUniform1i glUniform1iARB
#define glUniform1f glUniform1fARB
#define glUniform2f glUniform2fARB
#define glUniform2i glUniform2iARB
#define glUniform3fv glUniform3fvARB
#define glUniform4fv glUniform4fvARB
#endif

	void _locate_attributes() const;
	void _locateUniforms();

	gDPCombine m_combine;
	UniformLocation m_uniforms;
	GLuint m_program;
	int m_nInputs;
};

void InitShaderCombiner();
void DestroyShaderCombiner();

#ifdef GL_IMAGE_TEXTURES_SUPPORT
void SetDepthFogCombiner();
void SetMonochromeCombiner();
#endif // GL_IMAGE_TEXTURES_SUPPORT

//#define USE_TOONIFY

#endif //GLSL_COMBINER_H
