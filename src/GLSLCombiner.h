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

#ifdef GL_UNIFORMBLOCK_SUPPORT
class UniformBlock
{
public:
	enum TextureUniforms {
		tuTexScale,
		tuTexMask,
		tuTexOffset,
		tuCacheScale,
		tuCacheOffset,
		tuCacheShiftScale,
		tuCacheFrameBuffer,
		tuTotal
	};

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

	enum LightUniforms {
		luLightDirection,
		luLightColor,
		luTotal
	};

	UniformBlock();
	~UniformBlock();

	void bindWithShaderCombiner(ShaderCombiner * _pCombiner);
	void setColorData(ColorUniforms _index, u32 _dataSize, const void * _data);
	void updateTextureParameters();
	void updateLightParameters();
	void updateUniforms(ShaderCombiner * _pCombiner) {}

private:
	void _initTextureBuffer(GLuint _program);
	void _initColorsBuffer(GLuint _program);
	void _initLightBuffer(GLuint _program);

	bool _isDataChanged(void * _pBuffer, const void * _pData, u32 _dataSize);

	template <u32 _numUniforms, u32 _bindingPoint>
	struct UniformBlockData
	{
		UniformBlockData() : m_buffer(0), m_blockBindingPoint(_bindingPoint)
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
			GLuint blockIndex = glGetUniformBlockIndex(_program, _strBlockName);
			if (blockIndex == GL_INVALID_INDEX)
				return 0;

			GLint blockSize, numUniforms;
			glGetActiveUniformBlockiv(_program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
			glGetActiveUniformBlockiv(_program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms);

			glGetUniformIndices(_program, numUniforms, _strUniformNames, m_indices);
			glGetActiveUniformsiv(_program, numUniforms, m_indices, GL_UNIFORM_OFFSET, m_offsets);

			glUniformBlockBinding(_program, blockIndex, m_blockBindingPoint);
			glGenBuffers(1, &m_buffer);
			return blockSize;
		}

		GLuint m_buffer;
		GLuint m_blockBindingPoint;
		GLuint m_indices[_numUniforms];
		GLint m_offsets[_numUniforms];
	};

	GLuint m_currentBuffer;

	UniformBlockData<tuTotal, 1> m_textureBlock;
	UniformBlockData<cuTotal, 2> m_colorsBlock;
	UniformBlockData<luTotal, 3> m_lightBlock;

	std::vector<GLbyte> m_textureBlockData;
	std::vector<GLbyte> m_colorsBlockData;
	std::vector<GLbyte> m_lightBlockData;
};
#else
class UniformBlock
{
public:

	UniformBlock() {}
	~UniformBlock() {}

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

	void bindWithShaderCombiner(ShaderCombiner * _pCombiner);
	void setColorData(ColorUniforms _index, u32 _dataSize, const void * _data) {}
	void updateTextureParameters() {}
	void updateLightParameters() {}
	void updateUniforms(ShaderCombiner * _pCombiner);

private:
	struct fv3Uniform {
		GLint loc;
		float val[3];
		void set(float * _pVal, bool _force) {
			const size_t szData = sizeof(float)* 3;
			if (loc >= 0 && (_force || memcmp(val, _pVal, szData) != 0)) {
				memcpy(val, _pVal, szData);
				glUniform3fv(loc, 1, _pVal);
			}
		}
	};

	struct fv4Uniform {
		GLint loc;
		float val[4];
		void set(float * _pVal, bool _force) {
			const size_t szData = sizeof(float)* 4;
			if (loc >= 0 && (_force || memcmp(val, _pVal, szData) != 0)) {
				memcpy(val, _pVal, szData);
				glUniform4fv(loc, 1, _pVal);
			}
		}
	};

	struct UniformBlockLocation
	{
		UniformBlockLocation(GLuint _program) : m_program(_program) {}

		GLuint m_program;

		// Texture parameters
		ShaderCombiner::fv2Uniform uTexScale, uTexMask[2], uTexOffset[2], uCacheScale[2], uCacheOffset[2], uCacheShiftScale[2];
		ShaderCombiner::iv2Uniform uCacheFrameBuffer, uTextureSize[2];

		// Colors
		fv4Uniform uFogColor, uCenterColor, uScaleColor, uBlendColor, uEnvColor, uPrimColor;
		ShaderCombiner::fUniform uPrimLod, uK4, uK5;

		// Lights
		fv3Uniform uLightDirection[8], uLightColor[8];
	};

	void _updateColorUniforms(UniformBlockLocation & _location, bool _bForce);
	void _updateTextureUniforms(UniformBlockLocation & _location, bool _bUsesT0, bool _bUsesT1, bool _bForce);
	void _updateLightUniforms(UniformBlockLocation & _location, bool _bForce);

	typedef std::map<u64, UniformBlockLocation> Uniforms;
	Uniforms m_uniforms;
};
#endif // GL_UNIFORMBLOCK_SUPPORT

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
