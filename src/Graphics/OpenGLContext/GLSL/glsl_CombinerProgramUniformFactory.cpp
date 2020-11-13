#include <Config.h>
#include "glsl_CombinerProgramUniformFactory.h"
#include <Graphics/Parameters.h>
#include <Graphics/Context.h>

#include <Textures.h>
#include <NoiseTexture.h>
#include <FrameBuffer.h>
#include <DisplayWindow.h>
#include <Debugger.h>
#include <GBI.h>
#include <RSP.h>
#include <gSP.h>
#include <gDP.h>
#include <VI.h>

namespace glsl {

/*---------------Uniform-------------*/

struct iUniform	{
	GLint loc = -1;
	int val = -999;
	void set(int _val, bool _force) {
		if (loc >= 0 && (_force || val != _val)) {
			val = _val;
			glUniform1i(loc, _val);
		}
	}
};

struct fUniform {
	GLint loc = -1;
	float val = -9999.9f;
	void set(float _val, bool _force) {
		if (loc >= 0 && (_force || val != _val)) {
			val = _val;
			glUniform1f(loc, _val);
		}
	}
};

struct fv2Uniform {
	GLint loc = -1;
	float val1 = -9999.9f, val2 = -9999.9f;
	void set(float _val1, float _val2, bool _force) {
		if (loc >= 0 && (_force || val1 != _val1 || val2 != _val2)) {
			val1 = _val1;
			val2 = _val2;
			glUniform2f(loc, _val1, _val2);
		}
	}
};

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

struct iv2Uniform {
	GLint loc = -1;
	int val1 = -999, val2 = -999;
	void set(int _val1, int _val2, bool _force) {
		if (loc >= 0 && (_force || val1 != _val1 || val2 != _val2)) {
			val1 = _val1;
			val2 = _val2;
			glUniform2i(loc, _val1, _val2);
		}
	}
};

struct i4Uniform {
	GLint loc = -1;
	int val0 = -999, val1 = -999, val2 = -999, val3 = -999;
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


/*---------------UniformGroup-------------*/

#define LocateUniform(A) \
	A.loc = glGetUniformLocation(_program, #A);

class UNoiseTex : public UniformGroup
{
public:
	UNoiseTex(GLuint _program) {
		LocateUniform(uTexNoise);
	}

	void update(bool _force) override
	{
		uTexNoise.set(int(graphics::textureIndices::NoiseTex), _force);
	}

private:
	iUniform uTexNoise;
};

class UDepthTex : public UniformGroup
{
public:
	UDepthTex(GLuint _program) {
		LocateUniform(uDepthTex);
	}

	void update(bool _force) override
	{
		uDepthTex.set(int(graphics::textureIndices::DepthTex), _force);
	}

private:
	iUniform uDepthTex;
};

class UZLutTexture : public UniformGroup
{
public:
	UZLutTexture(GLuint _program) {
		LocateUniform(uZlutImage);
	}

	void update(bool _force) override
	{
		uZlutImage.set(int(graphics::textureIndices::ZLUTTex), _force);
	}

private:
	iUniform uZlutImage;
};

class UTextures : public UniformGroup
{
public:
	UTextures(GLuint _program) {
		LocateUniform(uTex0);
		LocateUniform(uTex1);
	}

	void update(bool _force) override
	{
		uTex0.set(0, _force);
		uTex1.set(1, _force);
	}

private:
	iUniform uTex0;
	iUniform uTex1;
};

class UMSAATextures : public UniformGroup
{
public:
	UMSAATextures(GLuint _program) {
		LocateUniform(uMSTex0);
		LocateUniform(uMSTex1);
		LocateUniform(uMSAASamples);
	}

	void update(bool _force) override
	{
		uMSTex0.set(int(graphics::textureIndices::MSTex[0]), _force);
		uMSTex1.set(int(graphics::textureIndices::MSTex[1]), _force);
		uMSAASamples.set(config.video.multisampling, _force);
	}

private:
	iUniform uMSTex0;
	iUniform uMSTex1;
	iUniform uMSAASamples;
};

class UScreenSpaceTriangleInfo : public UniformGroup
{
public:
	UScreenSpaceTriangleInfo(GLuint _program) {
		LocateUniform(uScreenSpaceTriangle);
	}

	void update(bool _force) override
	{
		uScreenSpaceTriangle.set(
			(dwnd().getDrawer().getDrawingState() == DrawingState::ScreenSpaceTriangle) ? 1 : 0, _force);
	}

private:
	iUniform uScreenSpaceTriangle;
};

class UFrameBufferInfo : public UniformGroup
{
public:
	UFrameBufferInfo(GLuint _program) {
		LocateUniform(uFbMonochrome);
		LocateUniform(uFbFixedAlpha);
		LocateUniform(uMSTexEnabled);
	}

	void update(bool _force) override
	{
		int nFbMonochromeMode0 = 0, nFbMonochromeMode1 = 0;
		int nFbFixedAlpha0 = 0, nFbFixedAlpha1 = 0;
		int nMSTex0Enabled = 0, nMSTex1Enabled = 0;
		TextureCache & cache = textureCache();
		if (cache.current[0] != nullptr && cache.current[0]->frameBufferTexture != CachedTexture::fbNone) {
			if (cache.current[0]->size == G_IM_SIZ_8b) {
				nFbMonochromeMode0 = 1;
				if (gDP.otherMode.imageRead == 0)
					nFbFixedAlpha0 = 1;
			} else if (gSP.textureTile[0]->size == G_IM_SIZ_16b && gSP.textureTile[0]->format == G_IM_FMT_IA) {
				nFbMonochromeMode0 = 2;
			} else if ((config.generalEmulation.hacks & hack_ZeldaMonochrome) != 0 &&
					   cache.current[0]->size == G_IM_SIZ_16b &&
					   gSP.textureTile[0]->size == G_IM_SIZ_8b &&
					   gSP.textureTile[0]->format == G_IM_FMT_CI) {
				// Zelda monochrome effect
				nFbMonochromeMode0 = 3;
				nFbMonochromeMode1 = 3;
			}

			nMSTex0Enabled = cache.current[0]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
		}
		if (cache.current[1] != nullptr && cache.current[1]->frameBufferTexture != CachedTexture::fbNone) {
			if (cache.current[1]->size == G_IM_SIZ_8b) {
				nFbMonochromeMode1 = 1;
				if (gDP.otherMode.imageRead == 0)
					nFbFixedAlpha1 = 1;
			}
			else if (gSP.textureTile[1]->size == G_IM_SIZ_16b && gSP.textureTile[1]->format == G_IM_FMT_IA)
				nFbMonochromeMode1 = 2;
			nMSTex1Enabled = cache.current[1]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
		}
		uFbMonochrome.set(nFbMonochromeMode0, nFbMonochromeMode1, _force);
		uFbFixedAlpha.set(nFbFixedAlpha0, nFbFixedAlpha1, _force);
		uMSTexEnabled.set(nMSTex0Enabled, nMSTex1Enabled, _force);
		gDP.changed &= ~CHANGED_FB_TEXTURE;
	}

private:
	iv2Uniform uFbMonochrome;
	iv2Uniform uFbFixedAlpha;
	iv2Uniform uMSTexEnabled;
};


class UFog : public UniformGroup
{
public:
	UFog(GLuint _program) {
		LocateUniform(uFogUsage);
		LocateUniform(uFogScale);
	}

	void update(bool _force) override
	{
		if (RSP.LLE) {
			uFogUsage.set(0, _force);
			return;
		}

		int nFogUsage = ((gSP.geometryMode & G_FOG) != 0) ? 1 : 0;
		if (GBI.getMicrocodeType() == F3DAM) {
			const s16 fogMode = ((gSP.geometryMode >> 13) & 9) + 0xFFF8;
			if (fogMode == 0)
				nFogUsage = 1;
			else if (fogMode > 0)
				nFogUsage = 2;
		}
		uFogUsage.set(nFogUsage, _force);
		uFogScale.set(gSP.fog.multiplierf, gSP.fog.offsetf, _force);
	}

private:
	iUniform uFogUsage;
	fv2Uniform uFogScale;
};

class UBlendMode1Cycle : public UniformGroup
{
public:
	UBlendMode1Cycle(GLuint _program) {
		LocateUniform(uBlendMux1);
		LocateUniform(uForceBlendCycle1);
	}

	void update(bool _force) override
	{
		uBlendMux1.set(gDP.otherMode.c1_m1a,
			gDP.otherMode.c1_m1b,
			gDP.otherMode.c1_m2a,
			gDP.otherMode.c1_m2b,
			_force);

		const int forceBlend1 = (int)gDP.otherMode.forceBlender;
		uForceBlendCycle1.set(forceBlend1, _force);
	}

private:
	i4Uniform uBlendMux1;
	iUniform uForceBlendCycle1;
};

class UBlendMode2Cycle : public UniformGroup
{
public:
	UBlendMode2Cycle(GLuint _program) {
		LocateUniform(uBlendMux1);
		LocateUniform(uBlendMux2);
		LocateUniform(uForceBlendCycle1);
		LocateUniform(uForceBlendCycle2);
	}

	void update(bool _force) override
	{
		uBlendMux1.set(gDP.otherMode.c1_m1a,
			gDP.otherMode.c1_m1b,
			gDP.otherMode.c1_m2a,
			gDP.otherMode.c1_m2b,
			_force);

		uBlendMux2.set(gDP.otherMode.c2_m1a,
			gDP.otherMode.c2_m1b,
			gDP.otherMode.c2_m2a,
			gDP.otherMode.c2_m2b,
			_force);

		const int forceBlend1 = 1;
		uForceBlendCycle1.set(forceBlend1, _force);
		const int forceBlend2 = gDP.otherMode.forceBlender;
		uForceBlendCycle2.set(forceBlend2, _force);

		if (!graphics::Context::DualSourceBlending || dwnd().getDrawer().isTexrectDrawerMode()) {
			// Modes, which shader blender can't emulate
			const u32 mode = _SHIFTR(gDP.otherMode.l, 16, 16);
			switch (mode) {
			case 0x0040:
				// Mia Hamm Soccer
				// clr_in * a_in + clr_mem * (1-a)
				// clr_in * a_in + clr_in * (1-a)
			case 0x0050:
				// A Bug's Life
				// clr_in * a_in + clr_mem * (1-a)
				// clr_in * a_in + clr_mem * (1-a)
				uForceBlendCycle1.set(0, _force);
				uForceBlendCycle2.set(0, _force);
				break;
			case 0x0150:
				// Tony Hawk
				// clr_in * a_in + clr_mem * (1-a)
				// clr_in * a_fog + clr_mem * (1-a_fog)
				if ((config.generalEmulation.hacks & hack_TonyHawk) != 0) {
					uForceBlendCycle1.set(0, _force);
					uForceBlendCycle2.set(0, _force);
				}
				break;
			}
		}

	}

private:
	i4Uniform uBlendMux1;
	i4Uniform uBlendMux2;
	iUniform uForceBlendCycle1;
	iUniform uForceBlendCycle2;
};

class UBlendCvg : public UniformGroup
{
public:
	UBlendCvg(GLuint _program) {
		LocateUniform(uCvgDest);
		LocateUniform(uBlendAlphaMode);
	}

	void update(bool _force) override
	{
		uCvgDest.set(gDP.otherMode.cvgDest, _force);
		if (dwnd().getDrawer().isTexrectDrawerMode())
			uBlendAlphaMode.set(2, _force); // No alpha blend in texrect drawing mode
		else
			uBlendAlphaMode.set(gDP.otherMode.forceBlender, _force);
	}
private:
	iUniform uCvgDest;
	iUniform uBlendAlphaMode;
};

class UDitherMode : public UniformGroup
{
public:
	UDitherMode(GLuint _program, bool _usesNoise)
	: m_usesNoise(_usesNoise)
	{
		LocateUniform(uAlphaCompareMode);
		LocateUniform(uAlphaDitherMode);
		LocateUniform(uColorDitherMode);
	}

	void update(bool _force) override
	{
		if (gDP.otherMode.cycleType < G_CYC_COPY) {
			uAlphaCompareMode.set(gDP.otherMode.alphaCompare, _force);
			uAlphaDitherMode.set(gDP.otherMode.alphaDither, _force);
			uColorDitherMode.set(gDP.otherMode.colorDither, _force);
		}
		else {
			uAlphaCompareMode.set(0, _force);
			uAlphaDitherMode.set(0, _force);
			uColorDitherMode.set(0, _force);
		}

		bool updateNoiseTex = m_usesNoise;
		updateNoiseTex |= (gDP.otherMode.cycleType < G_CYC_COPY) && (gDP.otherMode.colorDither == G_CD_NOISE || gDP.otherMode.alphaDither == G_AD_NOISE || gDP.otherMode.alphaCompare == G_AC_DITHER);
		if (updateNoiseTex)
			g_noiseTexture.update();
	}

private:
	iUniform uAlphaCompareMode;
	iUniform uAlphaDitherMode;
	iUniform uColorDitherMode;
	bool m_usesNoise;
};

class UScreenScale : public UniformGroup
{
public:
	UScreenScale(GLuint _program) {
		LocateUniform(uScreenScale);
	}

	void update(bool _force) override
	{
		if (dwnd().getDrawer().isTexrectDrawerMode()) {
			uScreenScale.set(1.0f, 1.0f, _force);
			return;
		}

		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer == nullptr)
			uScreenScale.set(dwnd().getScaleX(), dwnd().getScaleY(), _force);
		else
			uScreenScale.set(pBuffer->m_scale, pBuffer->m_scale, _force);
	}

private:
	fv2Uniform uScreenScale;
};

class UMipmap1 : public UniformGroup
{
public:
	UMipmap1(GLuint _program) {
		LocateUniform(uMinLod);
		LocateUniform(uMaxTile);
	}

	void update(bool _force) override
	{
		uMinLod.set(gDP.primColor.m, _force);
		uMaxTile.set(gSP.texture.level, _force);
	}

private:
	fUniform uMinLod;
	iUniform uMaxTile;
};

class UMipmap2 : public UniformGroup
{
public:
	UMipmap2(GLuint _program) {
		LocateUniform(uEnableLod);
		LocateUniform(uTextureDetail);
	}

	void update(bool _force) override
	{
		const int uCalcLOD = (gDP.otherMode.textureLOD == G_TL_LOD) ? 1 : 0;
		uEnableLod.set(uCalcLOD, _force);
		uTextureDetail.set(gDP.otherMode.textureDetail, _force);
	}

private:
	iUniform uEnableLod;
	iUniform uTextureDetail;
};

class UTexturePersp : public UniformGroup
{
public:
	UTexturePersp(GLuint _program) {
		LocateUniform(uTexturePersp);
	}

	void update(bool _force) override
	{
		const u32 texturePersp = (RSP.LLE || GBI.isTexturePersp()) ? gDP.otherMode.texturePersp : 1U;
		uTexturePersp.set(texturePersp, _force);
	}

private:
	iUniform uTexturePersp;
};

class UTextureFetchMode : public UniformGroup
{
public:
	UTextureFetchMode(GLuint _program) {
		LocateUniform(uTextureFilterMode);
		LocateUniform(uTextureFormat);
		LocateUniform(uTextureConvert);
		LocateUniform(uConvertParams);
	}

	void update(bool _force) override
	{
		int textureFilter = gDP.otherMode.textureFilter;
		uTextureFilterMode.set(textureFilter, _force);
		uTextureFormat.set(gSP.textureTile[0]->format, gSP.textureTile[1]->format, _force);
		uTextureConvert.set(gDP.otherMode.convert_one, _force);
		if (gDP.otherMode.bi_lerp0 == 0 || gDP.otherMode.bi_lerp1 == 0)
			uConvertParams.set(gDP.convert.k0, gDP.convert.k1, gDP.convert.k2, gDP.convert.k3, _force);
	}

private:
	iUniform uTextureFilterMode;
	iv2Uniform uTextureFormat;
	iUniform uTextureConvert;
	i4Uniform uConvertParams;
};

class UAlphaTestInfo : public UniformGroup
{
public:
	UAlphaTestInfo(GLuint _program) {
		LocateUniform(uEnableAlphaTest);
		LocateUniform(uAlphaCvgSel);
		LocateUniform(uCvgXAlpha);
		LocateUniform(uAlphaTestValue);
	}

	void update(bool _force) override
	{
		if (gDP.otherMode.cycleType == G_CYC_FILL) {
			uEnableAlphaTest.set(0, _force);
		}
		else if (gDP.otherMode.cycleType == G_CYC_COPY) {
			if (gDP.otherMode.alphaCompare & G_AC_THRESHOLD) {
				uEnableAlphaTest.set(1, _force);
				uAlphaCvgSel.set(0, _force);
				uAlphaTestValue.set(0.5f, _force);
			}
			else {
				uEnableAlphaTest.set(0, _force);
			}
		}
		else if ((gDP.otherMode.alphaCompare & G_AC_THRESHOLD) != 0) {
			uEnableAlphaTest.set(1, _force);
			uAlphaTestValue.set(gDP.blendColor.a, _force);
			uAlphaCvgSel.set(gDP.otherMode.alphaCvgSel, _force);
		}
		else {
			uEnableAlphaTest.set(0, _force);
		}

		uCvgXAlpha.set(gDP.otherMode.cvgXAlpha, _force);
	}

private:
	iUniform uEnableAlphaTest;
	iUniform uAlphaCvgSel;
	iUniform uCvgXAlpha;
	fUniform uAlphaTestValue;
};

class UDepthScale : public UniformGroup
{
public:
	UDepthScale(GLuint _program) {
		LocateUniform(uDepthScale);
	}

	void update(bool _force) override
	{
		if (RSP.LLE)
			uDepthScale.set(0.5f, 0.5f, _force);
		else
			uDepthScale.set(gSP.viewport.vscale[2], gSP.viewport.vtrans[2], _force);
	}

private:
	fv2Uniform uDepthScale;
};

class UDepthInfo : public UniformGroup
{
public:
	UDepthInfo(GLuint _program) {
		LocateUniform(uEnableDepth);
		LocateUniform(uEnableDepthCompare);
		LocateUniform(uEnableDepthUpdate);
		LocateUniform(uDepthMode);
		LocateUniform(uDepthSource);
		LocateUniform(uPrimDepth);
		LocateUniform(uDeltaZ);
	}

	void update(bool _force) override
	{
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer == nullptr || pBuffer->m_pDepthBuffer == nullptr)
			return;

		const bool nDepthEnabled = ((gSP.geometryMode & G_ZBUFFER) || gDP.otherMode.depthSource == G_ZS_PRIM) &&
									gDP.otherMode.cycleType <= G_CYC_2CYCLE;
		uEnableDepth.set(nDepthEnabled ? 1 : 0, _force);
		if (nDepthEnabled) {
			uEnableDepthCompare.set(gDP.otherMode.depthCompare, _force);
			uEnableDepthUpdate.set(gDP.otherMode.depthUpdate, _force);
		} else {
			uEnableDepthCompare.set(0, _force);
			uEnableDepthUpdate.set(0, _force);
		}
		uDepthMode.set(gDP.otherMode.depthMode, _force);
		uDepthSource.set(gDP.otherMode.depthSource, _force);
		if (gDP.otherMode.depthSource == G_ZS_PRIM) {
			uDeltaZ.set(gDP.primDepth.deltaZ, _force);
			uPrimDepth.set(gDP.primDepth.z, _force);
		}
	}

private:
	iUniform uEnableDepth;
	iUniform uEnableDepthCompare;
	iUniform uEnableDepthUpdate;
	iUniform uDepthMode;
	iUniform uDepthSource;
	fUniform uPrimDepth;
	fUniform uDeltaZ;
};

class UDepthSource : public UniformGroup
{
public:
	UDepthSource(GLuint _program) {
		LocateUniform(uDepthSource);
		LocateUniform(uPrimDepth);
	}

	void update(bool _force) override
	{
		uDepthSource.set(gDP.otherMode.depthSource, _force);
		if (gDP.otherMode.depthSource == G_ZS_PRIM)
			uPrimDepth.set(gDP.primDepth.z, _force);
	}

private:
	iUniform uDepthSource;
	fUniform uPrimDepth;
};

class URenderTarget : public UniformGroup
{
public:
	URenderTarget(GLuint _program) {
		LocateUniform(uRenderTarget);
	}

	void update(bool _force) override
	{
		int renderTarget = 0;
		if (isCurrentColorImageDepthImage()) {
			renderTarget = isDepthCompareEnabled() ? 2 : 1;
		}
		uRenderTarget.set(renderTarget, _force);
	}

private:
	iUniform uRenderTarget;
};

class UClampMode : public UniformGroup
{
public:
	UClampMode(GLuint _program) {
		LocateUniform(uClampMode);
	}

	void update(bool _force) override
	{
		int clampMode = -1;
		switch (gfxContext.getClampMode())
		{
		case graphics::ClampMode::ClippingEnabled:
			clampMode = 0;
			break;
		case graphics::ClampMode::NoNearPlaneClipping:
			clampMode = 1;
			break;
		case graphics::ClampMode::NoClipping:
			clampMode = 2;
			break;
		}
		uClampMode.set(clampMode, _force);
	}

private:
	iUniform uClampMode;
};

class UClipRatio : public UniformGroup
{
public:
	UClipRatio(GLuint _program) {
		LocateUniform(uClipRatio);
	}

	void update(bool _force) override
	{
		uClipRatio.set(float(gSP.clipRatio), _force);
	}

private:
	fUniform uClipRatio;
};

class UPolygonOffset : public UniformGroup
{
public:
	UPolygonOffset(GLuint _program) {
		LocateUniform(uPolygonOffset);
	}

	void update(bool _force) override
	{
		f32 offset = gfxContext.isEnabled(graphics::enable::POLYGON_OFFSET_FILL) ? 0.003f : 0.0f;
		uPolygonOffset.set(offset, _force);
	}

private:
	fUniform uPolygonOffset;
};


class UScreenCoordsScale : public UniformGroup
{
public:
	UScreenCoordsScale(GLuint _program) {
		LocateUniform(uScreenCoordsScale);
	}

	void update(bool _force) override
	{
		f32 scaleX, scaleY;
		calcCoordsScales(frameBufferList().getCurrent(), scaleX, scaleY);
		uScreenCoordsScale.set(2.0f*scaleX, -2.0f*scaleY, _force);
	}

private:
	fv2Uniform uScreenCoordsScale;
};

class UColors : public UniformGroup
{
public:
	UColors(GLuint _program) {
		LocateUniform(uFogColor);
		LocateUniform(uCenterColor);
		LocateUniform(uScaleColor);
		LocateUniform(uBlendColor);
		LocateUniform(uEnvColor);
		LocateUniform(uPrimColor);
		LocateUniform(uPrimLod);
		LocateUniform(uK4);
		LocateUniform(uK5);
	}

	void update(bool _force) override
	{
		uFogColor.set(&gDP.fogColor.r, _force);
		uCenterColor.set(&gDP.key.center.r, _force);
		uScaleColor.set(&gDP.key.scale.r, _force);
		uBlendColor.set(&gDP.blendColor.r, _force);
		uEnvColor.set(&gDP.envColor.r, _force);
		uPrimColor.set(&gDP.primColor.r, _force);
		uPrimLod.set(gDP.primColor.l, _force);
		uK4.set(_FIXED2FLOATCOLOR(gDP.convert.k4, 8 ), _force);
		uK5.set(_FIXED2FLOATCOLOR(gDP.convert.k5, 8 ), _force);
	}

private:
	fv4Uniform uFogColor;
	fv4Uniform uCenterColor;
	fv4Uniform uScaleColor;
	fv4Uniform uBlendColor;
	fv4Uniform uEnvColor;
	fv4Uniform uPrimColor;
	fUniform uPrimLod;
	fUniform uK4;
	fUniform uK5;
};

class URectColor : public UniformGroup
{
public:
	URectColor(GLuint _program) {
		LocateUniform(uRectColor);
	}

	void update(bool _force) override
	{
		uRectColor.set(&gDP.rectColor.r, _force);
	}

private:
	fv4Uniform uRectColor;
};

class UTextureSize : public UniformGroup
{
public:
	UTextureSize(GLuint _program, bool _useT0, bool _useT1)
	: m_useT0(_useT0)
	, m_useT1(_useT1)
	{
		LocateUniform(uTextureSize[0]);
		LocateUniform(uTextureSize[1]);
	}

	void update(bool _force) override
	{
		TextureCache & cache = textureCache();
		if (m_useT0 && cache.current[0] != NULL)
			uTextureSize[0].set((float)cache.current[0]->width, (float)cache.current[0]->height, _force);
		if (m_useT1 && cache.current[1] != NULL)
			uTextureSize[1].set((float)cache.current[1]->width, (float)cache.current[1]->height, _force);
	}

private:
	fv2Uniform uTextureSize[2];
	bool m_useT0;
	bool m_useT1;
};

class UTextureParams : public UniformGroup
{
public:
	UTextureParams(GLuint _program, bool _useT0, bool _useT1)
	{
		m_useTile[0] = _useT0;
		m_useTile[1] = _useT1;
		LocateUniform(uTexOffset[0]);
		LocateUniform(uTexOffset[1]);
		LocateUniform(uCacheShiftScale[0]);
		LocateUniform(uCacheShiftScale[1]);
		LocateUniform(uCacheScale[0]);
		LocateUniform(uCacheScale[1]);
		LocateUniform(uCacheOffset[0]);
		LocateUniform(uCacheOffset[1]);
		LocateUniform(uTexScale);
		LocateUniform(uTexSize[0]);
		LocateUniform(uTexSize[1]);
		LocateUniform(uCacheFrameBuffer);
	}

	void update(bool _force) override
	{
		int nFB[2] = { 0, 0 };
		TextureCache & cache = textureCache();
		for (u32 t = 0; t < 2; ++t) {
			if (!m_useTile[t])
				continue;

			gDPTile * pTile = gSP.textureTile[t];
			if (pTile != nullptr) {
				if (pTile->textureMode == TEXTUREMODE_BGIMAGE || pTile->textureMode == TEXTUREMODE_FRAMEBUFFER_BG)
					uTexOffset[t].set(0.0f, 0.0f, _force);
				else {
					float fuls = pTile->fuls;
					float fult = pTile->fult;
					if (pTile->frameBufferAddress > 0) {
						FrameBuffer * pBuffer = frameBufferList().getBuffer(pTile->frameBufferAddress);
						if (pBuffer != nullptr) {
							if (pTile->masks > 0 && pTile->clamps == 0)
								fuls = float(pTile->uls % (1 << pTile->masks));
							if (pTile->maskt > 0 && pTile->clampt == 0)
								fult = float(pTile->ult % (1 << pTile->maskt));
						} else {
							pTile->frameBufferAddress = 0;
						}
					}
					uTexOffset[t].set(fuls, fult, _force);
				}
			}

			CachedTexture *_pTexture = cache.current[t];
			if (_pTexture != nullptr) {
				f32 shiftScaleS = 1.0f;
				f32 shiftScaleT = 1.0f;
				getTextureShiftScale(t, cache, shiftScaleS, shiftScaleT);
				uCacheShiftScale[t].set(shiftScaleS, shiftScaleT, _force);
				uCacheScale[t].set(_pTexture->hdRatioS, _pTexture->hdRatioT, _force);
				uCacheOffset[t].set(_pTexture->offsetS, _pTexture->offsetT, _force);
				nFB[t] = _pTexture->frameBufferTexture;
			}
		}

		uCacheFrameBuffer.set(nFB[0], nFB[1], _force);
		uTexScale.set(gSP.texture.scales, gSP.texture.scalet, _force);
	}

private:
	bool m_useTile[2];
	fv2Uniform uTexOffset[2];
	fv2Uniform uCacheShiftScale[2];
	fv2Uniform uCacheScale[2];
	fv2Uniform uCacheOffset[2];
	fv2Uniform uTexScale;
	fv2Uniform uTexSize[2];
	iv2Uniform uCacheFrameBuffer;
};

class UTextureEngine : public UniformGroup
{
public:
	UTextureEngine(GLuint _program, bool _useT0, bool _useT1)
	{
		m_useTile[0] = _useT0;
		m_useTile[1] = _useT1;
		LocateUniform(uTexWrap0);
		LocateUniform(uTexWrap1);
		LocateUniform(uTexClamp0);
		LocateUniform(uTexClamp1);
		LocateUniform(uTexWrapEn0);
		LocateUniform(uTexWrapEn1);
		LocateUniform(uTexClampEn0);
		LocateUniform(uTexClampEn1);
		LocateUniform(uTexMirrorEn0);
		LocateUniform(uTexMirrorEn1);
		LocateUniform(uTexSize0);
		LocateUniform(uTexSize1);
	}

	void update(bool _force) override
	{
		std::array<f32, 2> aTexWrap[2] = { { 1024.0f, 1024.0f }, { 1024.0f, 1024.0f } };
		std::array<f32, 2> aTexClamp[2] = { { 1024.0f, 1024.0f }, { 1024.0f, 1024.0f } };
		std::array<f32, 2> aTexWrapEn[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
		std::array<f32, 2> aTexClampEn[2] = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
		std::array<f32, 2> aTexMirrorEn[2] = { { 0.0f, 0.0f }, { 0.0f,0.0f }};
		std::array<f32, 2> aTexSize[2] = { { 1024.0f, 1024.0f }, { 1024.0f, 1024.0f } };

		TextureCache & cache = textureCache();
		const bool replaceTex1ByTex0 = needReplaceTex1ByTex0();
		for (u32 t = 0; t < 2; ++t) {
			if (!m_useTile[t])
				continue;

			const u32 tile = replaceTex1ByTex0 ? 0 : t;
			const gDPTile * pTile = gSP.textureTile[tile];
			CachedTexture * pTexture = cache.current[tile];
			if (pTile == nullptr || pTexture == nullptr)
				continue;

			aTexSize[t][0] = pTexture->width * pTexture->hdRatioS;
			aTexSize[t][1] = pTexture->height * pTexture->hdRatioT;

			/* Not sure if special treatment of framebuffer textures is correct */
			if (pTexture->frameBufferTexture != CachedTexture::fbNone)
			{
				aTexClamp[t][0] = f32(pTexture->width) * pTexture->hdRatioS - 1.0f;
				aTexClamp[t][1] = f32(pTexture->height) * pTexture->hdRatioT - 1.0f;
				aTexWrapEn[t][0] = 0.0;
				aTexWrapEn[t][1] = 0.0;
				aTexClampEn[t][0] = 1.0;
				aTexClampEn[t][1] = 1.0;
				aTexMirrorEn[t][0] = 0.0;
				aTexMirrorEn[t][1] = 0.0;
			} else if (pTile->textureMode != TEXTUREMODE_NORMAL || g_debugger.isDebugMode()) {
				aTexWrapEn[t][0] = 0.0;
				aTexWrapEn[t][1] = 0.0;
				aTexClampEn[t][0] = 0.0;
				aTexClampEn[t][1] = 0.0;
				aTexMirrorEn[t][0] = 0.0;
				aTexMirrorEn[t][1] = 0.0;
			} else {
				aTexWrap[t][0] = f32(1 << pTile->masks) * pTexture->hdRatioS;
				aTexWrap[t][1] = f32(1 << pTile->maskt) * pTexture->hdRatioT;
				aTexClamp[t][0] = (pTile->flrs - pTile->fuls + 1.0f) * pTexture->hdRatioS - 1.0f;
				aTexClamp[t][1] = (pTile->flrt - pTile->fult + 1.0f) * pTexture->hdRatioT - 1.0f;
				aTexWrapEn[t][0] = f32(pTile->masks == 0 ? 0 : 1);
				aTexWrapEn[t][1] = f32(pTile->maskt == 0 ? 0 : 1);
				aTexClampEn[t][0] = f32(gDP.otherMode.cycleType == G_CYC_COPY ? 0 : (pTile->masks == 0 ? 1 : pTile->clamps));
				aTexClampEn[t][1] = f32(gDP.otherMode.cycleType == G_CYC_COPY ? 0 : (pTile->maskt == 0 ? 1 : pTile->clampt));
				aTexMirrorEn[t][0] = f32(pTile->masks == 0 ? 0 : pTile->mirrors);
				aTexMirrorEn[t][1] = f32(pTile->maskt == 0 ? 0 : pTile->mirrort);
			}
		}

		uTexWrap0.set(aTexWrap[0][0], aTexWrap[0][1], _force);
		uTexWrap1.set(aTexWrap[1][0], aTexWrap[1][1], _force);
		uTexClamp0.set(aTexClamp[0][0], aTexClamp[0][1], _force);
		uTexClamp1.set(aTexClamp[1][0], aTexClamp[1][1], _force);
		uTexWrapEn0.set(aTexWrapEn[0][0], aTexWrapEn[0][1], _force);
		uTexWrapEn1.set(aTexWrapEn[1][0], aTexWrapEn[1][1], _force);
		uTexClampEn0.set(aTexClampEn[0][0], aTexClampEn[0][1], _force);
		uTexClampEn1.set(aTexClampEn[1][0], aTexClampEn[1][1], _force);
		uTexMirrorEn0.set(aTexMirrorEn[0][0], aTexMirrorEn[0][1], _force);
		uTexMirrorEn1.set(aTexMirrorEn[1][0], aTexMirrorEn[1][1], _force);
		uTexSize0.set(aTexSize[0][0], aTexSize[0][1], _force);
		uTexSize1.set(aTexSize[1][0], aTexSize[1][1], _force);

	}

private:
	bool m_useTile[2];
	fv2Uniform uTexWrap0;
	fv2Uniform uTexWrap1;
	fv2Uniform uTexClamp0;
	fv2Uniform uTexClamp1;
	fv2Uniform uTexWrapEn0;
	fv2Uniform uTexWrapEn1;
	fv2Uniform uTexClampEn0;
	fv2Uniform uTexClampEn1;
	fv2Uniform uTexMirrorEn0;
	fv2Uniform uTexMirrorEn1;
	fv2Uniform uTexSize0;
	fv2Uniform uTexSize1;
};

class ULights : public UniformGroup
{
public:
	ULights(GLuint _program)
	{
		char buf[32];
		for (s32 i = 0; i < 8; ++i) {
			sprintf(buf, "uLightDirection[%d]", i);
			uLightDirection[i].loc = glGetUniformLocation(_program, buf);
			sprintf(buf, "uLightColor[%d]", i);
			uLightColor[i].loc = glGetUniformLocation(_program, buf);
		}
	}

	void update(bool _force) override
	{
		for (u32 i = 0; i <= gSP.numLights; ++i) {
			uLightDirection[i].set(gSP.lights.xyz[i], _force);
			uLightColor[i].set(gSP.lights.rgb[i], _force);
		}
	}

private:
	fv3Uniform uLightDirection[8];
	fv3Uniform uLightColor[8];
};


/*---------------CombinerProgramUniformFactory-------------*/

void CombinerProgramUniformFactory::buildUniforms(GLuint _program,
												  const CombinerInputs & _inputs,
												  const CombinerKey & _key,
												  UniformGroups & _uniforms)
{
	_uniforms.emplace_back(new UNoiseTex(_program));
	_uniforms.emplace_back(new UScreenSpaceTriangleInfo(_program));

	if (!m_glInfo.isGLES2) {
		_uniforms.emplace_back(new UDepthTex(_program));
		_uniforms.emplace_back(new UDepthScale(_program));
	}

	if (_inputs.usesTexture()) {
		_uniforms.emplace_back(new UTextures(_program));

		if (config.video.multisampling != 0)
			_uniforms.emplace_back(new UMSAATextures(_program));

		_uniforms.emplace_back(new UFrameBufferInfo(_program));

		if (_inputs.usesLOD()) {
			_uniforms.emplace_back(new UMipmap1(_program));
			if (config.generalEmulation.enableLOD != 0)
				_uniforms.emplace_back(new UMipmap2(_program));
		} else if (_key.getCycleType() < G_CYC_COPY) {
			_uniforms.emplace_back(new UTextureFetchMode(_program));
		}

		_uniforms.emplace_back(new UTexturePersp(_program));

		if (m_glInfo.isGLES2)
			_uniforms.emplace_back(new UTextureSize(_program, _inputs.usesTile(0), _inputs.usesTile(1)));

		if (!_key.isRectKey())
			_uniforms.emplace_back(new UTextureParams(_program, _inputs.usesTile(0), _inputs.usesTile(1)));

		_uniforms.emplace_back(new UTextureEngine(_program, _inputs.usesTile(0), _inputs.usesTile(1)));
	}

	_uniforms.emplace_back(new UFog(_program));

	if (config.generalEmulation.enableLegacyBlending == 0) {
		switch (_key.getCycleType()) {
		case G_CYC_1CYCLE:
			_uniforms.emplace_back(new UBlendMode1Cycle(_program));
			break;
		case G_CYC_2CYCLE:
			_uniforms.emplace_back(new UBlendMode2Cycle(_program));
			break;
		}
	}

	_uniforms.emplace_back(new UBlendCvg(_program));

	_uniforms.emplace_back(new UDitherMode(_program, _inputs.usesNoise()));

	_uniforms.emplace_back(new UScreenScale(_program));

	_uniforms.emplace_back(new UAlphaTestInfo(_program));

	if ((config.generalEmulation.hacks & hack_RE2) != 0 && config.generalEmulation.enableFragmentDepthWrite != 0)
		_uniforms.emplace_back(new UZLutTexture(_program));

	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable)
		_uniforms.emplace_back(new UDepthInfo(_program));
	else
		_uniforms.emplace_back(new UDepthSource(_program));

	if (config.generalEmulation.enableFragmentDepthWrite != 0 ||
		config.frameBufferEmulation.N64DepthCompare != Config::dcDisable)
		_uniforms.emplace_back(new URenderTarget(_program));

	if (m_glInfo.isGLESX && m_glInfo.noPerspective) {
		_uniforms.emplace_back(new UClampMode(_program));
		_uniforms.emplace_back(new UPolygonOffset(_program));
	}

	_uniforms.emplace_back(new UClipRatio(_program));

	_uniforms.emplace_back(new UScreenCoordsScale(_program));

	_uniforms.emplace_back(new UColors(_program));

	if (_key.isRectKey())
		_uniforms.emplace_back(new URectColor(_program));

	if (_inputs.usesHwLighting())
		_uniforms.emplace_back(new ULights(_program));
}

CombinerProgramUniformFactory::CombinerProgramUniformFactory(const opengl::GLInfo & _glInfo)
: m_glInfo(_glInfo)
{
}

}
