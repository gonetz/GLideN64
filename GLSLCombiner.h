#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

class GLSLCombiner : public OGLCombiner {
public:
	GLSLCombiner(Combiner *_color, Combiner *_alpha);
	virtual ~GLSLCombiner();
	virtual void Set();
	virtual void UpdateColors(bool _bForce = false);
	virtual void UpdateFBInfo(bool _bForce = false);
	virtual void UpdateDepthInfo(bool _bForce = false);
	virtual void UpdateAlphaTestInfo(bool _bForce = false);
	virtual void UpdateTextureInfo(bool _bForce = false);
	virtual void UpdateRenderState(bool _bForce = false);
	virtual void UpdateLight(bool _bForce = false);

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
			uEnableFog, uEnableLod, uEnableAlphaTest,
			uEnableDepth, uEnableDepthCompare, uEnableDepthUpdate,
			uDepthMode, uFb8Bit, uFbFixedAlpha, uRenderState,
			uMaxTile, uTextureDetail, uTexturePersp,
			uAlphaCompareMode, uAlphaDitherMode, uColorDitherMode;

		fUniform uFogMultiplier, uFogOffset, uK4, uK5, uPrimLod, uNoiseTime, uScreenWidth, uScreenHeight,
			uLodXScale, uLodYScale, uMinLod, uDepthTrans, uDepthScale, uAlphaTestValue;

		fv4Uniform uEnvColor, uPrimColor, uFogColor, uCenterColor, uScaleColor;

		fv2Uniform uTexScale, uTexOffset[2], uCacheShiftScale[2],
			uCacheScale[2], uCacheOffset[2];

		fv3Uniform uLightDirection[8], uLightColor[8];

		iv2Uniform uCacheFrameBuffer;
	};

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
			_u.val[2] = _val2;
			glUniform2f(_u.loc, _val1, _val2);
		}
	}
	void _setIV2Uniform(iv2Uniform & _u, int _val1, int _val2, bool _force) {
		if (_force|| _u.val[0] != _val1 || _u.val[1] != _val2) {
			_u.val[0] = _val1;
			_u.val[2] = _val2;
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

	UniformLocation m_uniforms;
	GLuint m_aShaders[8];
	GLuint m_program;
	int m_nInputs;
};

void InitGLSLCombiner();
void DestroyGLSLCombiner();
void GLSL_CalcLOD();
void GLSL_PostCalcLOD();

#endif //GLSL_COMBINER_H
