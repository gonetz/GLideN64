#include "UniformSet.h"
#include "../Config.h"
#include "../Textures.h"

#define LocateUniform2(A) \
	location.A.loc = glGetUniformLocation(program, #A);

void UniformSet::bindWithShaderCombiner(ShaderCombiner * _pCombiner)
{
	const u64 mux = _pCombiner->getMux();
	const GLuint program = _pCombiner->m_program;
	m_uniforms.emplace(mux, program);
	UniformSetLocation & location = m_uniforms.at(mux);

	// Texture parameters
	if (_pCombiner->usesTexture()) {
		LocateUniform2(uTexScale);
		LocateUniform2(uTexMask[0]);
		LocateUniform2(uTexMask[1]);
		LocateUniform2(uTexOffset[0]);
		LocateUniform2(uTexOffset[1]);
		LocateUniform2(uCacheScale[0]);
		LocateUniform2(uCacheScale[1]);
		LocateUniform2(uCacheOffset[0]);
		LocateUniform2(uCacheOffset[1]);
		LocateUniform2(uCacheShiftScale[0]);
		LocateUniform2(uCacheShiftScale[1]);
		LocateUniform2(uCacheFrameBuffer);
		LocateUniform2(uTextureSize[0]);
		LocateUniform2(uTextureSize[1]);
		_updateTextureUniforms(location, _pCombiner->usesTile(0), _pCombiner->usesTile(1), true);
	}

	// Colors
	LocateUniform2(uFogColor);
	LocateUniform2(uCenterColor);
	LocateUniform2(uScaleColor);
	LocateUniform2(uBlendColor);
	LocateUniform2(uEnvColor);
	LocateUniform2(uPrimColor);
	LocateUniform2(uPrimLod);
	LocateUniform2(uK4);
	LocateUniform2(uK5);
	_updateColorUniforms(location, true);

	// Lights
	if (config.generalEmulation.enableHWLighting != 0 && GBI.isHWLSupported() && _pCombiner->usesShadeColor()) {
		// locate lights uniforms
		char buf[32];
		for (s32 i = 0; i < 8; ++i) {
			sprintf(buf, "uLightDirection[%d]", i);
			location.uLightDirection[i].loc = glGetUniformLocation(program, buf);
			sprintf(buf, "uLightColor[%d]", i);
			location.uLightColor[i].loc = glGetUniformLocation(program, buf);
		}
		_updateLightUniforms(location, true);
	}
}

void UniformSet::_updateColorUniforms(UniformSetLocation & _location, bool _bForce)
{
	_location.uFogColor.set(&gDP.fogColor.r, _bForce);
	_location.uCenterColor.set(&gDP.key.center.r, _bForce);
	_location.uScaleColor.set(&gDP.key.scale.r, _bForce);
	_location.uBlendColor.set(&gDP.blendColor.r, _bForce);
	_location.uEnvColor.set(&gDP.envColor.r, _bForce);
	_location.uPrimColor.set(&gDP.primColor.r, _bForce);
	_location.uPrimLod.set(gDP.primColor.l, _bForce);
	_location.uK4.set(gDP.convert.k4*0.0039215689f, _bForce);
	_location.uK5.set(gDP.convert.k5*0.0039215689f, _bForce);
}


void UniformSet::_updateTextureUniforms(UniformSetLocation & _location, bool _bUsesT0, bool _bUsesT1, bool _bForce)
{
	int nFB[2] = { 0, 0 };
	const bool bUsesTile[2] = { _bUsesT0, _bUsesT1 };
	TextureCache & cache = textureCache();
	for (u32 t = 0; t < 2; ++t) {
		if (!bUsesTile[t])
			continue;

		if (gSP.textureTile[t] != NULL) {
			if (gSP.textureTile[t]->textureMode == TEXTUREMODE_BGIMAGE || gSP.textureTile[t]->textureMode == TEXTUREMODE_FRAMEBUFFER_BG) {
				_location.uTexOffset[t].set(0.0f, 0.0f, _bForce);
				_location.uTexMask[t].set(0.0f, 0.0f, _bForce);
			}
			else {
				_location.uTexOffset[t].set(gSP.textureTile[t]->fuls, gSP.textureTile[t]->fult, _bForce);
				_location.uTexMask[t].set(
					gSP.textureTile[t]->masks > 0 ? (float)(1 << gSP.textureTile[t]->masks) : 0.0f,
					gSP.textureTile[t]->maskt > 0 ? (float)(1 << gSP.textureTile[t]->maskt) : 0.0f,
					_bForce);
			}
		}

		if (cache.current[t] != NULL) {
			f32 shiftScaleS = 1.0f;
			f32 shiftScaleT = 1.0f;
			getTextureShiftScale(t, cache, shiftScaleS, shiftScaleT);
			_location.uCacheShiftScale[t].set(shiftScaleS, shiftScaleT, _bForce);
			_location.uCacheScale[t].set(cache.current[t]->scaleS, cache.current[t]->scaleT, _bForce);
			_location.uCacheOffset[t].set(cache.current[t]->offsetS, cache.current[t]->offsetT, _bForce);
			_location.uTextureSize[t].set(cache.current[t]->realWidth, cache.current[t]->realHeight, _bForce);
			nFB[t] = cache.current[t]->frameBufferTexture;
		}
	}

	_location.uCacheFrameBuffer.set(nFB[0], nFB[1], _bForce);
	_location.uTexScale.set(gSP.texture.scales, gSP.texture.scalet, _bForce);
}

void UniformSet::_updateLightUniforms(UniformSetLocation & _location, bool _bForce)
{
	for (s32 i = 0; i <= gSP.numLights; ++i) {
		_location.uLightDirection[i].set(&gSP.lights[i].x, _bForce);
		_location.uLightColor[i].set(&gSP.lights[i].r, _bForce);
	}
}

void UniformSet::updateUniforms(ShaderCombiner * _pCombiner)
{
	UniformSetLocation & location = m_uniforms.at(_pCombiner->getMux());

	_updateColorUniforms(location, false);

	OGLRender::RENDER_STATE rs = video().getRender().getRenderState();
	if ((rs == OGLRender::rsTriangle || rs == OGLRender::rsLine) && _pCombiner->usesTexture())
		_updateTextureUniforms(location, _pCombiner->usesTile(0), _pCombiner->usesTile(1), false);

	if (config.generalEmulation.enableHWLighting != 0 && GBI.isHWLSupported() && _pCombiner->usesShadeColor())
		_updateLightUniforms(location, false);
}

UniformCollection * createUniformCollection()
{
	return new UniformSet();
}
