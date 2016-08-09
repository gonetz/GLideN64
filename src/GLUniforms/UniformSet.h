#ifndef UNIFORM_SET_H
#define UNIFORM_SET_H

#include "../UniformCollection.h"

class UniformSet : public UniformCollection
{
public:

	UniformSet() {}
	~UniformSet() {}

	virtual void bindWithShaderCombiner(ShaderCombiner * _pCombiner);
	virtual void setColorData(ColorUniforms _index, u32 _dataSize, const void * _data) {}
	virtual void updateTextureParameters() {}
	virtual void updateLightParameters() {}
	virtual void updateUniforms(ShaderCombiner * _pCombiner, OGLRender::RENDER_STATE _renderState);

private:
	struct fv3Uniform {
		GLint loc = -1;
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
		GLint loc = -1;
		float val[4];
		void set(float * _pVal, bool _force) {
			const size_t szData = sizeof(float)* 4;
			if (loc >= 0 && (_force || memcmp(val, _pVal, szData) != 0)) {
				memcpy(val, _pVal, szData);
				glUniform4fv(loc, 1, _pVal);
			}
		}
	};

	struct UniformSetLocation
	{
		UniformSetLocation(GLuint _program) : m_program(_program) {}

		GLuint m_program;

		// Texture parameters
		ShaderCombiner::fv2Uniform uTexScale, uTexOffset[2], uCacheScale[2], uCacheOffset[2], uCacheShiftScale[2], uTextureSize[2];
		ShaderCombiner::iv2Uniform uCacheFrameBuffer;

		// Colors
		fv4Uniform uFogColor, uCenterColor, uScaleColor, uBlendColor, uEnvColor, uPrimColor;
		ShaderCombiner::fUniform uPrimLod, uK4, uK5;

		// Lights
		fv3Uniform uLightDirection[8], uLightColor[8];
	};

	void _updateColorUniforms(UniformSetLocation & _location, bool _bForce);
	void _updateTextureUniforms(UniformSetLocation & _location, bool _bUsesT0, bool _bUsesT1, bool _bForce);
	void _updateTextureSize(UniformSetLocation & _location, bool _bUsesT0, bool _bUsesT1, bool _bForce);
	void _updateLightUniforms(UniformSetLocation & _location, bool _bForce);

	typedef std::map<u64, UniformSetLocation> Uniforms;
	Uniforms m_uniforms;
};

#endif // UNIFORM_SET_H
