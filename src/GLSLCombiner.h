#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

#include <vector>
#include <iostream>
#include "gDP.h"
#include "Combiner.h"

class ShaderCombiner {
public:
	ShaderCombiner();
	ShaderCombiner(Combiner & _color, Combiner & _alpha, const gDPCombine & _combine);
	~ShaderCombiner();

	void update(bool _bForce);
	void updateFogMode(bool _bForce = false);
	void updateDitherMode(bool _bForce = false);
	void updateLOD(bool _bForce = false);
	void updateFrameBufferInfo(bool _bForce = false);
	void updateDepthInfo(bool _bForce = false);
	void updateAlphaTestInfo(bool _bForce = false);
	void updateTextureInfo(bool _bForce = false);
	void updateRenderState(bool _bForce = false);
	void updateRenderTarget(bool _bForce = false);
	void updateScreenCoordsScale(bool _bForce = false);
	void updateBlendMode(bool _bForce = false);
	void disableBlending();

	u64 getKey() const {return m_key;}

	bool usesTile(u32 _t) const {
		if (_t == 0)
			return (m_nInputs & ((1<<TEXEL0)|(1<<TEXEL0_ALPHA))) != 0;
		return (m_nInputs & ((1 << TEXEL1) | (1 << TEXEL1_ALPHA))) != 0;
	}
	bool usesTexture() const { return (m_nInputs & ((1 << TEXEL1)|(1 << TEXEL1_ALPHA)|(1 << TEXEL0)|(1 << TEXEL0_ALPHA))) != 0; }
	bool usesLOD() const { return (m_nInputs & (1 << LOD_FRACTION)) != 0; }
	bool usesShade() const { return (m_nInputs & ((1 << SHADE) | (1 << SHADE_ALPHA))) != 0; }
	bool usesShadeColor() const { return (m_nInputs & (1 << SHADE)) != 0; }
	bool usesHwLighting() const { return (m_nInputs & (1 << HW_LIGHT)) != 0; }

	friend std::ostream & operator<< (std::ostream & _os, const ShaderCombiner & _combiner);
	friend std::istream & operator>> (std::istream & _os, ShaderCombiner & _combiner);

	static void getShaderCombinerOptionsSet(std::vector<u32> & _vecOptions);

private:
	friend class UniformBlock;
	friend class UniformSet;

	struct iUniform	{
		GLint loc = -1;
		int val;
		void set(int _val, bool _force) {
			if (loc >= 0 && (_force || val != _val)) {
				val = _val;
				glUniform1i(loc, _val);
			}
		}
	};

	struct fUniform {
		GLint loc = -1;
		float val;
		void set(float _val, bool _force) {
			if (loc >= 0 && (_force || val != _val)) {
				val = _val;
				glUniform1f(loc, _val);
			}
		}
	};

	struct fv2Uniform {
		GLint loc = -1;
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
		GLint loc = -1;
		int val[2];
		void set(int _val1, int _val2, bool _force) {
			if (loc >= 0 && (_force || val[0] != _val1 || val[1] != _val2)) {
				val[0] = _val1;
				val[1] = _val2;
				glUniform2i(loc, _val1, _val2);
			}
		}
	};

	struct i4Uniform {
		GLint loc = -1;
		int val0, val1, val2, val3;
		void set(int _val0, int _val1, int _val2, int _val3, bool _force) {
			if (loc < 0)
				return;
			if (_force || _val0 != val0 || _val1 != val1 || _val2 != val2 || _val3 != val3) {
				val0 = _val0;
				val1 = _val1;
				val2 = _val2;
				val3 = _val3;
				glUniform4i(loc, val0, val1, val2, val3);
			}
		}
	};

	struct UniformLocation
	{
		iUniform uTex0, uTex1, uMSTex0, uMSTex1, uDepthTex,
			uTexNoise, uTlutImage, uZlutImage, uDepthImage,
			uFogUsage, uEnableLod, uEnableAlphaTest,
			uEnableDepth, uEnableDepthCompare, uEnableDepthUpdate,
			uDepthMode, uDepthSource, uRenderState,
			uMaxTile, uTextureDetail, uTexturePersp, uTextureFilterMode, uMSAASamples,
			uAlphaCompareMode, uAlphaDitherMode, uColorDitherMode,
			uCvgXAlpha, uAlphaCvgSel, uRenderTarget,
			uForceBlendCycle1, uForceBlendCycle2;

		fUniform uMinLod, uDeltaZ, uAlphaTestValue, uMSAAScale;

		fv2Uniform uScreenScale, uDepthScale, uFogScale, uScreenCoordsScale;

		iv2Uniform uMSTexEnabled, uFbMonochrome, uFbFixedAlpha;

		i4Uniform uBlendMux1, uBlendMux2;
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

	u64 m_key;
	UniformLocation m_uniforms;
	GLuint m_program;
	int m_nInputs;
	bool m_bNeedUpdate;
};

void InitShaderCombiner();
void DestroyShaderCombiner();

#ifdef GL_IMAGE_TEXTURES_SUPPORT
void SetDepthFogCombiner();
void SetMonochromeCombiner();
#endif // GL_IMAGE_TEXTURES_SUPPORT
bool SetDepthTextureCombiner();

//#define USE_TOONIFY

#endif //GLSL_COMBINER_H
