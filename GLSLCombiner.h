#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

#include "gDP.h"
#include "Combiner.h"

class ShaderCombiner {
public:
	ShaderCombiner(Combiner & _color, Combiner & _alpha, const gDPCombine & _combine);
	~ShaderCombiner();

	void update();
	void updateColors(bool _bForce = false);
	void updateFBInfo(bool _bForce = false);
	void updateDepthInfo(bool _bForce = false);
	void updateAlphaTestInfo(bool _bForce = false);
	void updateTextureInfo(bool _bForce = false);
	void updateRenderState(bool _bForce = false);
	void updateLight(bool _bForce = false);

	u64 getMux() const {return m_combine.mux;}

	bool usesT0() const {return (m_nInputs & ((1<<TEXEL0)|(1<<TEXEL0_ALPHA))) != 0;}
	bool usesT1() const {return (m_nInputs & ((1<<TEXEL1)|(1<<TEXEL1_ALPHA))) != 0;}
	bool usesLOD() const {return (m_nInputs & (1<<LOD_FRACTION)) != 0;}
	bool usesShadeColor() const {return (m_nInputs & ((1<<SHADE)|(1<<SHADE_ALPHA))) != 0;}

private:
	struct iUniform {GLint loc; int val;};
	struct fUniform {GLint loc; float val;};
	struct fv2Uniform {GLint loc; float val[2];};
	struct iv2Uniform {GLint loc; int val[2];};
	struct fv3Uniform {GLint loc; float val[3];};
	struct fv4Uniform {GLint loc; float val[4];};

	struct UniformLocation
	{
		iUniform uTex0, uTex1, uTlutImage, uZlutImage, uDepthImage,
			uFogMode, uFogUsage, uEnableLod, uEnableAlphaTest,
			uEnableDepth, uEnableDepthCompare, uEnableDepthUpdate,
			uDepthMode, uFb8Bit, uFbFixedAlpha, uRenderState,
			uMaxTile, uTextureDetail, uTexturePersp,
			uAlphaCompareMode, uAlphaDitherMode, uColorDitherMode, uGammaCorrectionEnabled;

		fUniform uFogMultiplier, uFogOffset, uK4, uK5, uPrimLod, uNoiseTime, uScreenWidth, uScreenHeight,
			uLodXScale, uLodYScale, uMinLod, uDepthTrans, uDepthScale, uAlphaTestValue;

		fv4Uniform uEnvColor, uPrimColor, uFogColor, uCenterColor, uScaleColor;

		fv2Uniform uTexScale, uTexOffset[2], uTexMask[2],
			uCacheShiftScale[2], uCacheScale[2], uCacheOffset[2];

		fv3Uniform uLightDirection[8], uLightColor[8];

		iv2Uniform uCacheFrameBuffer;
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
	void _setIUniform(iUniform & _u, int _val, bool _force) {
		if (_force|| _u.val != _val) {
			_u.val = _val;
			glUniform1i(_u.loc, _val);
		}
	}
	void _setFUniform(fUniform & _u, float _val, bool _force) {
		if (_force|| _u.val != _val) {
			_u.val = _val;
			glUniform1f(_u.loc, _val);
		}
	}
	void _setFV2Uniform(fv2Uniform & _u, float _val1, float _val2, bool _force) {
		if (_force|| _u.val[0] != _val1 || _u.val[1] != _val2) {
			_u.val[0] = _val1;
			_u.val[1] = _val2;
			glUniform2f(_u.loc, _val1, _val2);
		}
	}
	void _setIV2Uniform(iv2Uniform & _u, int _val1, int _val2, bool _force) {
		if (_force|| _u.val[0] != _val1 || _u.val[1] != _val2) {
			_u.val[0] = _val1;
			_u.val[1] = _val2;
			glUniform2i(_u.loc, _val1, _val2);
		}
	}
	void _setV3Uniform(fv3Uniform & _u, float * _pVal, bool _force) {
		const size_t szData = sizeof(float)*3;
		if (_force|| memcmp(_u.val, _pVal, szData) > 0) {
			memcpy(_u.val, _pVal, szData);
			glUniform3fv(_u.loc, 1, _pVal);
		}
	}
	void _setV4Uniform(fv4Uniform & _u, float * _pVal, bool _force) {
		const size_t szData = sizeof(float)*4;
		if (_force|| memcmp(_u.val, _pVal, szData) > 0) {
			memcpy(_u.val, _pVal, szData);
			glUniform4fv(_u.loc, 1, _pVal);
		}
	}

	gDPCombine m_combine;
	UniformLocation m_uniforms;
	GLuint m_program;
	int m_nInputs;
};

void InitShaderCombiner();
void DestroyShaderCombiner();

#ifdef GL_IMAGE_TEXTURES_SUPPORT
extern GLuint g_draw_shadow_map_program;
extern GLuint g_monochrome_image_program;
void SetMonochromeCombiner(GLuint _program);
#endif // GL_IMAGE_TEXTURES_SUPPORT

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment);

//#define USE_TOONIFY

#endif //GLSL_COMBINER_H
