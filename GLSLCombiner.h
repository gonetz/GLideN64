#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

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
	void updateLight(bool _bForce = false);

	u64 getMux() const {return m_combine.mux;}

	bool usesT0() const {return (m_nInputs & ((1<<TEXEL0)|(1<<TEXEL0_ALPHA))) != 0;}
	bool usesT1() const {return (m_nInputs & ((1<<TEXEL1)|(1<<TEXEL1_ALPHA))) != 0;}
	bool usesLOD() const {return (m_nInputs & (1<<LOD_FRACTION)) != 0;}
	bool usesShadeColor() const {return (m_nInputs & ((1<<SHADE)|(1<<SHADE_ALPHA))) != 0;}

private:
	friend class UniformBlock;

	struct iUniform {GLint loc; int val;};
	struct fUniform {GLint loc; float val;};
	struct fv2Uniform {GLint loc; float val[2];};
	struct iv2Uniform {GLint loc; int val[2];};
	struct fv3Uniform {GLint loc; float val[3];};
	struct fv4Uniform {GLint loc; float val[4];};

	struct UniformLocation
	{
		iUniform uTex0, uTex1, uTexNoise, uTlutImage, uZlutImage, uDepthImage,
			uFogMode, uFogUsage, uEnableLod, uEnableAlphaTest,
			uEnableDepth, uEnableDepthCompare, uEnableDepthUpdate,
			uDepthMode, uDepthSource, uFb8Bit, uFbFixedAlpha, uRenderState, uSpecialBlendMode,
			uMaxTile, uTextureDetail, uTexturePersp, uTextureFilterMode,
			uAlphaCompareMode, uAlphaDitherMode, uColorDitherMode, uGammaCorrectionEnabled;

		fUniform uFogAlpha, uFogMultiplier, uFogOffset, uScreenWidth, uScreenHeight,
			uPrimitiveLod, uMinLod, uDeltaZ, uAlphaTestValue;

		fv2Uniform uTexScale, uScreenScale, uDepthScale, uTexOffset[2], uTexMask[2],
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
		if (_u.loc >= 0 && (_force || _u.val != _val)) {
			_u.val = _val;
			glUniform1i(_u.loc, _val);
		}
	}
	void _setFUniform(fUniform & _u, float _val, bool _force) {
		if (_u.loc >= 0 && (_force || _u.val != _val)) {
			_u.val = _val;
			glUniform1f(_u.loc, _val);
		}
	}
	void _setFV2Uniform(fv2Uniform & _u, float _val1, float _val2, bool _force) {
		if (_u.loc >= 0 && (_force || _u.val[0] != _val1 || _u.val[1] != _val2)) {
			_u.val[0] = _val1;
			_u.val[1] = _val2;
			glUniform2f(_u.loc, _val1, _val2);
		}
	}
	void _setIV2Uniform(iv2Uniform & _u, int _val1, int _val2, bool _force) {
		if (_u.loc >= 0 && (_force || _u.val[0] != _val1 || _u.val[1] != _val2)) {
			_u.val[0] = _val1;
			_u.val[1] = _val2;
			glUniform2i(_u.loc, _val1, _val2);
		}
	}
	void _setV3Uniform(fv3Uniform & _u, float * _pVal, bool _force) {
		const size_t szData = sizeof(float)*3;
		if (_u.loc >= 0 && (_force || memcmp(_u.val, _pVal, szData) != 0)) {
			memcpy(_u.val, _pVal, szData);
			glUniform3fv(_u.loc, 1, _pVal);
		}
	}
	void _setV4Uniform(fv4Uniform & _u, float * _pVal, bool _force) {
		const size_t szData = sizeof(float)*4;
		if (_u.loc >= 0 && (_force || memcmp(_u.val, _pVal, szData) != 0)) {
			memcpy(_u.val, _pVal, szData);
			glUniform4fv(_u.loc, 1, _pVal);
		}
	}

	gDPCombine m_combine;
	UniformLocation m_uniforms;
	GLuint m_program;
	int m_nInputs;
};

class UniformBlock
{
public:
	enum ColorUniforms {
		cuFogColor,
		cuCenterColor,
		cuScaleColor,
		cuBlendColor,
		cuEnvColor,
		cuPrimColor,
		cuPrimLod,
		cuK4,
		cuK5,
		cuTotal
	};

	UniformBlock();
	~UniformBlock();

	void attachShaderCombiner(ShaderCombiner * _pCombiner);
	void setColor(ColorUniforms _index, u32 _dataSize, const f32 * _data);

private:
	void _initColorsBuffer(GLuint _program);

	template <u32 _numUniforms, u32 _bindingPoint>
	struct UniformBlockData
	{
		UniformBlockData() : m_buffer(0), m_blockIndex(0), m_blockBindingPoint(_bindingPoint)
		{
			memset(m_indices, 0, sizeof(m_indices));
			memset(m_offsets, 0, sizeof(m_offsets));
		}
		~UniformBlockData()
		{
			if (m_buffer != 0) {
				glDeleteBuffers(1, &m_buffer);
				m_buffer = 0;
			}
		}

		GLint initBuffer(GLuint _program, const char * _strBlockName, const char ** _strUniformNames)
		{
			m_blockIndex = glGetUniformBlockIndex(_program, _strBlockName);

			GLint blockSize, numUniforms;
			glGetActiveUniformBlockiv(_program, m_blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
			glGetActiveUniformBlockiv(_program, m_blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms);
			assert(numUniforms == _numUniforms);

			glGetUniformIndices(_program, numUniforms, _strUniformNames, m_indices);
			glGetActiveUniformsiv(_program, numUniforms, m_indices, GL_UNIFORM_OFFSET, m_offsets);

			glUniformBlockBinding(_program, m_blockIndex, m_blockBindingPoint);
			glGenBuffers(1, &m_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
			return blockSize;
		}

		GLuint m_buffer;
		GLuint m_blockIndex;
		GLuint m_blockBindingPoint;
		GLuint m_indices[_numUniforms];
		GLint m_offsets[_numUniforms];
	};

	UniformBlockData<cuTotal, 1> m_colorsBlock;
};

void InitShaderCombiner();
void DestroyShaderCombiner();

#ifdef GL_IMAGE_TEXTURES_SUPPORT
void SetDepthFogCombiner();
void SetMonochromeCombiner();
#endif // GL_IMAGE_TEXTURES_SUPPORT

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment);
bool checkShaderCompileStatus(GLuint obj);
bool checkProgramLinkStatus(GLuint obj);

//#define USE_TOONIFY

#endif //GLSL_COMBINER_H
