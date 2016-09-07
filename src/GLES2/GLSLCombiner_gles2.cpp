#include <assert.h>
#include <stdio.h>
#include <string>
#include <memory>

#include <N64.h>
#include <OpenGL.h>
#include <Config.h>
#include <GLSLCombiner.h>
#include <ShaderUtils.h>
#include <FrameBuffer.h>
#include <DepthBuffer.h>
#include <RSP.h>
#include <VI.h>
#include <Log.h>
#include <FBOTextureFormats.h>

#include "Shaders_gles2.h"

using namespace std;

static GLuint  g_vertex_shader_object;
static GLuint  g_vertex_shader_object_notex;

GLuint g_monochrome_image_program = 0;

static bool g_weakGLSL = false;

#define GL_RED16 GL_R16UI

static std::string strFragmentShader;

class NoiseTexture
{
public:
	NoiseTexture() : m_pTexture(nullptr), m_pData(nullptr), m_DList(0) {}
	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture;
	std::unique_ptr<GLubyte[]> m_pData;
	u32 m_DList;
} noiseTex;

void NoiseTexture::init()
{
	if (config.generalEmulation.enableNoise == 0)
		return;
	m_pTexture = textureCache().addFrameBufferTexture();
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 640;
	m_pTexture->realHeight = 580;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_pData.reset(new GLubyte[640 * 580]);
}

void NoiseTexture::destroy()
{
	if (m_pTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = nullptr;
	}
}

void NoiseTexture::update()
{
	if (m_DList == video().getBuffersSwapCount() || config.generalEmulation.enableNoise == 0)
		return;

	if (VI.width*VI.height == 0)
		return;

	for (u32 y = 0; y < VI.height; ++y)	{
		for (u32 x = 0; x < VI.width; ++x)
			m_pData[x + y*VI.width] = rand() & 0xFF;
	}

	glActiveTexture(GL_TEXTURE0 + g_noiseTexIndex);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VI.width, VI.height, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_pData.get());
	m_DList = video().getBuffersSwapCount();
}

static
GLuint _createShader(GLenum _type, const char * _strShader)
{
	GLuint shader_object = glCreateShader(_type);
	glShaderSource(shader_object, 1, &_strShader, NULL);
	glCompileShader(shader_object);
	assert(checkShaderCompileStatus(shader_object));
	return shader_object;
}

void InitShaderCombiner()
{
	if (strstr((const char*)glGetString(GL_VERSION), "OpenGL ES 2") != NULL) {
		const char * strRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		if (strstr(strRenderer, "PowerVR") != NULL || strstr(strRenderer, "Adreno") != NULL) {
			g_weakGLSL = true;
			LOG(LOG_MINIMAL, "GPU with week GLSL detected: %s\n", strRenderer);
		}
	}

	g_vertex_shader_object = _createShader(GL_VERTEX_SHADER, vertex_shader);
	g_vertex_shader_object_notex = _createShader(GL_VERTEX_SHADER, vertex_shader_notex);

	strFragmentShader.reserve(1024*5);

	noiseTex.init();
	g_monochrome_image_program = createShaderProgram(default_vertex_shader, zelda_monochrome_fragment_shader);
}

void DestroyShaderCombiner() {
	strFragmentShader.clear();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteShader(g_vertex_shader_object);
	g_vertex_shader_object = 0;
	glDeleteShader(g_vertex_shader_object_notex);
	g_vertex_shader_object_notex = 0;

	glDeleteProgram(g_monochrome_image_program);
	g_monochrome_image_program = 0;
	noiseTex.destroy();
}

ShaderCombiner::ShaderCombiner(Combiner & _color, Combiner & _alpha, const gDPCombine & _combine) : m_key(getCombinerKey(_combine.mux))
{
	std::string strCombiner;
	m_nInputs = compileCombiner(_color, _alpha, strCombiner);

	if (usesTexture()) {
		strFragmentShader.assign(fragment_shader_header_common_variables);
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE && config.generalEmulation.enableLegacyBlending == 0)
			strFragmentShader.append(fragment_shader_header_common_variables_blend_mux_2cycle);
		strFragmentShader.append(fragment_shader_header_common_functions);
	} else {
		strFragmentShader.assign(fragment_shader_header_common_variables_notex);
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE && config.generalEmulation.enableLegacyBlending == 0)
			strFragmentShader.append(fragment_shader_header_common_variables_blend_mux_2cycle);
		strFragmentShader.append(fragment_shader_header_common_functions_notex);
	}

	strFragmentShader.append(fragment_shader_header_main);
	if (config.generalEmulation.enableLegacyBlending == 0)
		strFragmentShader.append(fragment_shader_blend_mux);

	const bool bUseLod = usesLOD();
	if (bUseLod) {
		strFragmentShader.append("  lowp vec4 readtex0, readtex1; \n");
		strFragmentShader.append("  lowp float lod_frac = mipmap(readtex0, readtex1);	\n");
	} else {
		if (usesTile(0)) {
			strFragmentShader.append("  nCurrentTile = 0; \n");
			strFragmentShader.append("  lowp vec4 readtex0 = readTex(uTex0, vTexCoord0, uFbMonochrome[0], uFbFixedAlpha[0]); \n");
		}
		if (usesTile(1)) {
			strFragmentShader.append("  nCurrentTile = 1; \n");
			strFragmentShader.append("  lowp vec4 readtex1 = readTex(uTex1, vTexCoord1, uFbMonochrome[1], uFbFixedAlpha[1]); \n");
		}
	}

	const bool bUseHWLight = config.generalEmulation.enableHWLighting != 0 && GBI.isHWLSupported() && usesShadeColor();
	if (bUseHWLight)
		strFragmentShader.append("  calc_light(vNumLights, vShadeColor.rgb, input_color); \n");
	else
		strFragmentShader.append("  input_color = vShadeColor.rgb;\n");

	strFragmentShader.append("  vec_color = vec4(input_color, vShadeColor.a); \n");
	strFragmentShader.append(strCombiner);
	strFragmentShader.append("  gl_FragColor = fragColor; \n");
	strFragmentShader.append(fragment_shader_end);

	if (config.generalEmulation.enableNoise == 0)
		strFragmentShader.append(fragment_shader_dummy_noise);

	if (bUseHWLight)
		strFragmentShader.append(fragment_shader_calc_light);
	if (bUseLod)
		strFragmentShader.append(fragment_shader_fake_mipmap);
	else if (usesTexture()) {
		if (config.texture.bilinearMode == BILINEAR_3POINT)
			strFragmentShader.append(fragment_shader_readtex_3point);
		else
			strFragmentShader.append(fragment_shader_readtex);
	}
	if (config.generalEmulation.enableNoise != 0)
		strFragmentShader.append(fragment_shader_noise);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar * strShaderData = strFragmentShader.data();
	glShaderSource(fragmentShader, 1, &strShaderData, NULL);
	glCompileShader(fragmentShader);
	if (!checkShaderCompileStatus(fragmentShader))
		LOG(LOG_ERROR, "Error in fragment shader:\n%s\n", strFragmentShader.data());

	m_program = glCreateProgram();
	_locate_attributes();
	if (usesTexture())
		glAttachShader(m_program, g_vertex_shader_object);
	else
		glAttachShader(m_program, g_vertex_shader_object_notex);
	glAttachShader(m_program, fragmentShader);
	glLinkProgram(m_program);
	assert(checkProgramLinkStatus(m_program));
	glDeleteShader(fragmentShader);
	_locateUniforms();
}

ShaderCombiner::~ShaderCombiner() {
	glDeleteProgram(m_program);
	m_program = 0;
}

#define LocateUniform(A) \
	m_uniforms.A.loc = glGetUniformLocation(m_program, #A);

void ShaderCombiner::_locateUniforms() {
	LocateUniform(uTex0);
	LocateUniform(uTex1);
	LocateUniform(uTexNoise);
	LocateUniform(uTlutImage);
	LocateUniform(uZlutImage);
	LocateUniform(uDepthImage);
	LocateUniform(uFogUsage);
	LocateUniform(uScreenCoordsScale);
	LocateUniform(uAlphaCompareMode);
	LocateUniform(uCvgXAlpha);
	LocateUniform(uAlphaCvgSel);
	LocateUniform(uEnableAlphaTest);
	LocateUniform(uEnableDepth);
	LocateUniform(uEnableDepthCompare)
	LocateUniform(uEnableDepthUpdate);
	LocateUniform(uDepthMode);
	LocateUniform(uDepthSource);
	LocateUniform(uFbMonochrome);
	LocateUniform(uFbFixedAlpha);
	LocateUniform(uMaxTile)
	LocateUniform(uTexturePersp);
	LocateUniform(uTextureFilterMode);
	LocateUniform(uForceBlendCycle1);
	LocateUniform(uForceBlendCycle2);

	LocateUniform(uMinLod);
	LocateUniform(uDeltaZ);
	LocateUniform(uAlphaTestValue);

	LocateUniform(uRenderState);

	LocateUniform(uScreenScale);
	LocateUniform(uFogScale);

	LocateUniform(uBlendMux1);
	LocateUniform(uBlendMux2);
}

void ShaderCombiner::_locate_attributes() const {
	glBindAttribLocation(m_program, SC_POSITION, "aPosition");
	glBindAttribLocation(m_program, SC_COLOR, "aColor");
	glBindAttribLocation(m_program, SC_TEXCOORD0, "aTexCoord0");
	glBindAttribLocation(m_program, SC_TEXCOORD1, "aTexCoord1");
	glBindAttribLocation(m_program, SC_NUMLIGHTS, "aNumLights");
	glBindAttribLocation(m_program, SC_MODIFY, "aModify");
}

void ShaderCombiner::update(bool _bForce) {
	glUseProgram(m_program);

	if (_bForce) {
		m_uniforms.uTexNoise.set(g_noiseTexIndex, true);
		if (usesTexture()) {
			m_uniforms.uTex0.set(0, true);
			m_uniforms.uTex1.set(1, true);
		}
		updateFrameBufferInfo(true);
		updateRenderState(true);
	}

	updateFogMode(_bForce);
	updateBlendMode(_bForce);
	updateDitherMode(_bForce);
	updateLOD(_bForce);
	updateTextureInfo(_bForce);
	updateAlphaTestInfo(_bForce);
	updateDepthInfo(_bForce);
	updateScreenCoordsScale(_bForce);
}

void ShaderCombiner::updateRenderState(bool _bForce)
{
	m_uniforms.uRenderState.set(video().getRender().getRenderState(), _bForce);
}

void ShaderCombiner::updateScreenCoordsScale(bool _bForce)
{
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	const float scaleX = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_width : VI.rwidth;
	const float scaleY = pCurrentBuffer != NULL ? 1.0f / pCurrentBuffer->m_height : VI.rheight;
	m_uniforms.uScreenCoordsScale.set(2.0f*scaleX, -2.0f*scaleY, _bForce);
}

void ShaderCombiner::updateFogMode(bool _bForce)
{
	if (RSP.bLLE) {
		m_uniforms.uFogUsage.set(0, _bForce);
		return;
	}

	int nFogUsage = ((gSP.geometryMode & G_FOG) != 0) ? 1 : 0;
	if (!GBI.isTextureGen())
		// F-Zero ucode seems to always use fog mode when fog is used in blender.
		nFogUsage |= (gDP.otherMode.c1_m1a == 3 || gDP.otherMode.c1_m2a == 3) ? 1 : 0;
	m_uniforms.uFogUsage.set(nFogUsage, _bForce);
	m_uniforms.uFogScale.set((float)gSP.fog.multiplier / 256.0f, (float)gSP.fog.offset / 256.0f, _bForce);
}

void ShaderCombiner::updateBlendMode(bool _bForce)
{
	if (config.generalEmulation.enableLegacyBlending != 0)
		return;

	if (gDP.otherMode.cycleType <= G_CYC_2CYCLE) {
		m_uniforms.uBlendMux1.set(gDP.otherMode.c1_m1a,
								  gDP.otherMode.c1_m1b,
								  gDP.otherMode.c1_m2a,
								  gDP.otherMode.c1_m2b,
								  _bForce);
	}
	int forceBlend1 = gDP.otherMode.cycleType == G_CYC_2CYCLE ? 1 : 0;
	int forceBlend2 = 0;

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType <= G_CYC_2CYCLE) {
		forceBlend1 = 1;
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
			forceBlend2 = 1;
			m_uniforms.uBlendMux2.set(gDP.otherMode.c2_m1a,
									  gDP.otherMode.c2_m1b,
									  gDP.otherMode.c2_m2a,
									  gDP.otherMode.c2_m2b,
									  _bForce);
		}
	}

	m_uniforms.uForceBlendCycle1.set(forceBlend1, _bForce);
	m_uniforms.uForceBlendCycle2.set(forceBlend2, _bForce);
}

void ShaderCombiner::disableBlending()
{
	if (config.generalEmulation.enableLegacyBlending != 0)
		return;

	m_uniforms.uForceBlendCycle1.set(0, false);
	m_uniforms.uForceBlendCycle2.set(0, false);
}

void ShaderCombiner::updateDitherMode(bool _bForce)
{
	if (gDP.otherMode.cycleType < G_CYC_COPY)
		m_uniforms.uAlphaCompareMode.set(gDP.otherMode.alphaCompare, _bForce);
	else
		m_uniforms.uAlphaCompareMode.set(0, _bForce);

	const int nDither = (gDP.otherMode.cycleType < G_CYC_COPY) && (gDP.otherMode.alphaCompare == G_AC_DITHER) ? 1 : 0;
	if ((m_nInputs & (1 << NOISE)) + nDither != 0) {
		if (config.frameBufferEmulation.nativeResFactor == 0)
			m_uniforms.uScreenScale.set(video().getScaleX(), video().getScaleY(), _bForce);
		else
			m_uniforms.uScreenScale.set(float(config.frameBufferEmulation.nativeResFactor), float(config.frameBufferEmulation.nativeResFactor), _bForce);
		noiseTex.update();
	}
}

void ShaderCombiner::updateLOD(bool _bForce)
{
	if (usesLOD()) {
		m_uniforms.uMinLod.set(gDP.primColor.m, _bForce);
		m_uniforms.uMaxTile.set(gSP.texture.level, _bForce);
	}
}

void ShaderCombiner::updateTextureInfo(bool _bForce) {
	m_uniforms.uTexturePersp.set(gDP.otherMode.texturePersp, _bForce);
	if (config.texture.bilinearMode == BILINEAR_3POINT)
		m_uniforms.uTextureFilterMode.set(gDP.otherMode.textureFilter | (gSP.objRendermode&G_OBJRM_BILERP), _bForce);
}

void ShaderCombiner::updateFrameBufferInfo(bool _bForce) {
	if (!usesTexture())
		return;

	int nFbMonochromeMode0 = 0, nFbMonochromeMode1 = 0;
	int nFbFixedAlpha0 = 0, nFbFixedAlpha1 = 0;
	int nMSTex0Enabled = 0, nMSTex1Enabled = 0;
	TextureCache & cache = textureCache();
	if (cache.current[0] != NULL && cache.current[0]->frameBufferTexture != CachedTexture::fbNone) {
		if (cache.current[0]->size == G_IM_SIZ_8b) {
			nFbMonochromeMode0 = 1;
			if (gDP.otherMode.imageRead == 0)
				nFbFixedAlpha0 = 1;
		} else if (gSP.textureTile[0]->size == G_IM_SIZ_16b && gSP.textureTile[0]->format == G_IM_FMT_IA)
			nFbMonochromeMode0 = 2;
	}
	if (cache.current[1] != NULL && cache.current[1]->frameBufferTexture != CachedTexture::fbNone) {
		if (cache.current[1]->size == G_IM_SIZ_8b) {
			nFbMonochromeMode1 = 1;
			if (gDP.otherMode.imageRead == 0)
				nFbFixedAlpha1 = 1;
		} else if (gSP.textureTile[1]->size == G_IM_SIZ_16b && gSP.textureTile[1]->format == G_IM_FMT_IA)
			nFbMonochromeMode1 = 2;
	}
	m_uniforms.uFbMonochrome.set(nFbMonochromeMode0, nFbMonochromeMode1, _bForce);
	m_uniforms.uFbFixedAlpha.set(nFbFixedAlpha0, nFbFixedAlpha1, _bForce);

	gDP.changed &= ~CHANGED_FB_TEXTURE;
}

void ShaderCombiner::updateDepthInfo(bool _bForce) {
	if (config.frameBufferEmulation.N64DepthCompare == 0 || !video().getRender().isImageTexturesSupported())
		return;

	FrameBuffer * pBuffer = frameBufferList().getCurrent();
	if (pBuffer == NULL || pBuffer->m_pDepthBuffer == NULL)
		return;

	const int nDepthEnabled = (gSP.geometryMode & G_ZBUFFER) > 0 ? 1 : 0;
	m_uniforms.uEnableDepth.set(nDepthEnabled, _bForce);
	if (nDepthEnabled == 0) {
		m_uniforms.uEnableDepthCompare.set(0, _bForce);
		m_uniforms.uEnableDepthUpdate.set(0, _bForce);
	} else {
		m_uniforms.uEnableDepthCompare.set(gDP.otherMode.depthCompare, _bForce);
		m_uniforms.uEnableDepthUpdate.set(gDP.otherMode.depthUpdate, _bForce);
	}
	m_uniforms.uDepthMode.set(gDP.otherMode.depthMode, _bForce);
	m_uniforms.uDepthSource.set(gDP.otherMode.depthSource, _bForce);
	if (gDP.otherMode.depthSource == G_ZS_PRIM)
		m_uniforms.uDeltaZ.set(gDP.primDepth.deltaZ, _bForce);
}

void ShaderCombiner::updateAlphaTestInfo(bool _bForce) {
	if (gDP.otherMode.cycleType == G_CYC_FILL) {
		m_uniforms.uEnableAlphaTest.set(0, _bForce);
	} else if (gDP.otherMode.cycleType == G_CYC_COPY) {
		if (gDP.otherMode.alphaCompare & G_AC_THRESHOLD) {
			m_uniforms.uEnableAlphaTest.set(1, _bForce);
			m_uniforms.uAlphaCvgSel.set(0, _bForce);
			m_uniforms.uAlphaTestValue.set(0.5f, _bForce);
		} else {
			m_uniforms.uEnableAlphaTest.set(0, _bForce);
		}
	} else if ((gDP.otherMode.alphaCompare & G_AC_THRESHOLD) != 0) {
		m_uniforms.uEnableAlphaTest.set(1, _bForce);
		m_uniforms.uAlphaTestValue.set(gDP.blendColor.a, _bForce);
		m_uniforms.uAlphaCvgSel.set(gDP.otherMode.alphaCvgSel, _bForce);
	} else {
		m_uniforms.uEnableAlphaTest.set(0, _bForce);
	}

	m_uniforms.uCvgXAlpha.set(gDP.otherMode.cvgXAlpha, _bForce);
}

void SetMonochromeCombiner() {
	glUseProgram(g_monochrome_image_program);
	static int texLoc = -1;
	if (texLoc < 0) {
		texLoc = glGetUniformLocation(g_monochrome_image_program, "uColorImage");
		glUniform1i(texLoc, 0);
	}
	static int sizeLoc = -1;
	if (sizeLoc < 0) {
		glGetUniformLocation(g_monochrome_image_program, "uScreenSize");
		glUniform2f(sizeLoc, (float)video().getWidth(), (float)video().getHeight());
	}
	gDP.changed |= CHANGED_COMBINE;
}

bool SetDepthTextureCombiner() {
	// All I can do for GLES2 is just to clear depth buffer.
	glDepthMask(TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);
	return false;
}
