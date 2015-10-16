#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "../N64.h"
#include "../OpenGL.h"
#include "../Config.h"
#include "../GLSLCombiner.h"
#include "../ShaderUtils.h"
#include "../FrameBuffer.h"
#include "../DepthBuffer.h"
#include "../RSP.h"
#include "../VI.h"
#include "../Log.h"

#include "Shaders_ogl3x.h"

using namespace std;

static GLuint  g_vertex_shader_object;
static GLuint  g_vertex_shader_object_notex;
static GLuint  g_calc_light_shader_object;
static GLuint  g_calc_mipmap_shader_object;
static GLuint  g_calc_noise_shader_object;
static GLuint  g_calc_depth_shader_object;
static GLuint  g_readtex_shader_object;
static GLuint  g_readtex_ms_shader_object;
static GLuint  g_dither_shader_object;
static GLuint  g_monochrome_image_program = 0;

#ifdef GL_IMAGE_TEXTURES_SUPPORT
GLuint g_draw_shadow_map_program = 0;
static GLuint g_zlut_tex = 0;
GLuint g_tlut_tex = 0;
static u32 g_paletteCRC256 = 0;
#endif // GL_IMAGE_TEXTURES_SUPPORT

static std::string strFragmentShader;

class NoiseTexture
{
public:
	NoiseTexture() : m_pTexture(NULL), m_PBO(0), m_DList(0) {}
	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture;
	GLuint m_PBO;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Generate Pixel Buffer Object. Initialize it with max buffer size.
	glGenBuffers(1, &m_PBO);
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, 640*580, NULL, GL_DYNAMIC_DRAW);
}

void NoiseTexture::destroy()
{
	if (m_pTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = NULL;
	}
	glDeleteBuffers(1, &m_PBO);
	m_PBO = 0;
}

void NoiseTexture::update()
{
	if (m_DList == video().getBuffersSwapCount() || config.generalEmulation.enableNoise == 0)
		return;
	const u32 dataSize = VI.width*VI.height;
	if (dataSize == 0)
		return;
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
	if (ptr == NULL)
		return;
	for (u32 y = 0; y < VI.height; ++y)	{
		for (u32 x = 0; x < VI.width; ++x)
			ptr[x + y*VI.width] = rand()&0xFF;
	}
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer

	glActiveTexture(GL_TEXTURE0 + g_noiseTexIndex);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VI.width, VI.height, GL_RED, GL_UNSIGNED_BYTE, 0);
	m_DList = video().getBuffersSwapCount();
}


#ifdef GL_IMAGE_TEXTURES_SUPPORT
static
void InitZlutTexture()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

#ifdef GLESX
	std::vector<u32> vecZLUT(0x40000);
	const u16 * const zLUT16 = depthBufferList().getZLUT();
	for (u32 i = 0; i < 0x40000; ++i)
		vecZLUT[i] = zLUT16[i];
	const u32 * zLUT = vecZLUT.data();
#else
	const u16 * const zLUT = depthBufferList().getZLUT();
#endif
	glGenTextures(1, &g_zlut_tex);
	glBindTexture(GL_TEXTURE_2D, g_zlut_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.lutInternalFormat,
		512, 512, 0, fboFormats.lutFormat, fboFormats.lutType,
		zLUT);
	glBindImageTexture(ZlutImageUnit, g_zlut_tex, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);
}

static
void DestroyZlutTexture()
{
	if (!video().getRender().isImageTexturesSupported())
		return;
	glBindImageTexture(ZlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);
	if (g_zlut_tex > 0) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &g_zlut_tex);
		g_zlut_tex = 0;
	}
}

static
void InitShadowMapShader()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

	g_paletteCRC256 = 0;
	glGenTextures(1, &g_tlut_tex);
	glBindTexture(GL_TEXTURE_2D, g_tlut_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.lutInternalFormat, 256, 1, 0, fboFormats.lutFormat, fboFormats.lutType, NULL);

	g_draw_shadow_map_program = createShaderProgram(default_vertex_shader, shadow_map_fragment_shader_float);
}

static
void DestroyShadowMapShader()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

	glBindImageTexture(TlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);

	if (g_tlut_tex > 0) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &g_tlut_tex);
		g_tlut_tex = 0;
	}
	glDeleteProgram(g_draw_shadow_map_program);
	g_draw_shadow_map_program = 0;
}
#endif // GL_IMAGE_TEXTURES_SUPPORT

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
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	g_vertex_shader_object = _createShader(GL_VERTEX_SHADER, vertex_shader);
	g_vertex_shader_object_notex = _createShader(GL_VERTEX_SHADER, vertex_shader_notex);

	strFragmentShader.reserve(1024*5);

#ifndef GLESX
	g_calc_light_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_calc_light);
	g_calc_mipmap_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_mipmap);
	g_calc_noise_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_noise);
	g_readtex_shader_object = _createShader(GL_FRAGMENT_SHADER, config.texture.bilinearMode == BILINEAR_3POINT ? fragment_shader_readtex_3point : fragment_shader_readtex);
	g_readtex_ms_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_readtex_ms);
	g_dither_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_dither);
#endif // GLESX

	noiseTex.init();
	g_monochrome_image_program = createShaderProgram(default_vertex_shader, zelda_monochrome_fragment_shader);
	glUseProgram(g_monochrome_image_program);
	const int texLoc = glGetUniformLocation(g_monochrome_image_program, "uColorImage");
	glUniform1i(texLoc, 0);
	glUseProgram(0);

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		g_calc_depth_shader_object = _createShader(GL_FRAGMENT_SHADER, depth_compare_shader_float);

	InitZlutTexture();
	InitShadowMapShader();
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

void DestroyShaderCombiner() {
	strFragmentShader.clear();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteShader(g_vertex_shader_object);
	g_vertex_shader_object = 0;
	glDeleteShader(g_vertex_shader_object_notex);
	g_vertex_shader_object_notex = 0;
#ifndef GLESX
	glDeleteShader(g_calc_light_shader_object);
	g_calc_light_shader_object = 0;
	glDeleteShader(g_calc_mipmap_shader_object);
	g_calc_mipmap_shader_object = 0;
	glDeleteShader(g_readtex_shader_object);
	g_readtex_shader_object = 0;
	glDeleteShader(g_readtex_ms_shader_object);
	g_readtex_ms_shader_object = 0;
	glDeleteShader(g_calc_noise_shader_object);
	g_calc_noise_shader_object = 0;
	glDeleteShader(g_dither_shader_object);
	g_dither_shader_object = 0;
	glDeleteShader(g_calc_depth_shader_object);
	g_calc_depth_shader_object = 0;
#endif // GLESX

	glDeleteProgram(g_monochrome_image_program);
	g_monochrome_image_program = 0;
	noiseTex.destroy();

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	DestroyZlutTexture();
	DestroyShadowMapShader();
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

ShaderCombiner::ShaderCombiner()
{
	m_program = glCreateProgram();
	_locate_attributes();
}

ShaderCombiner::ShaderCombiner(Combiner & _color, Combiner & _alpha, const gDPCombine & _combine) : m_combine(_combine)
{
	char strCombiner[1024];
	m_nInputs = compileCombiner(_color, _alpha, strCombiner);

	if (usesTexture()) {
		strFragmentShader.assign(fragment_shader_header_common_variables);
		strFragmentShader.append(fragment_shader_header_common_functions);
	}
	else {
		strFragmentShader.assign(fragment_shader_header_common_variables_notex);
		strFragmentShader.append(fragment_shader_header_common_functions_notex);
	}
	strFragmentShader.append(fragment_shader_header_main);
	const bool bUseLod = usesLOD();
	if (bUseLod) {
		strFragmentShader.append("  lowp vec4 readtex0, readtex1; \n");
		strFragmentShader.append("  lowp float lod_frac = mipmap(readtex0, readtex1);	\n");
	} else {
#ifdef GL_MULTISAMPLING_SUPPORT
		if (usesTile(0)) {
			if (config.video.multisampling > 0) {
				strFragmentShader.append("  lowp vec4 readtex0; \n");
				strFragmentShader.append("  if (uMSTexEnabled[0] == 0) readtex0 = readTex(uTex0, vTexCoord0, uFbMonochrome[0], uFbFixedAlpha[0] != 0); \n");
				strFragmentShader.append("  else readtex0 = readTexMS(uMSTex0, vTexCoord0, uFbMonochrome[0], uFbFixedAlpha[0] != 0); \n");
			} else
				strFragmentShader.append("  lowp vec4 readtex0 = readTex(uTex0, vTexCoord0, uFbMonochrome[0], uFbFixedAlpha[0] != 0); \n");
		}
		if (usesTile(1)) {
			if (config.video.multisampling > 0) {
				strFragmentShader.append("  lowp vec4 readtex1; \n");
				strFragmentShader.append("  if (uMSTexEnabled[1] == 0) readtex1 = readTex(uTex1, vTexCoord1, uFbMonochrome[1], uFbFixedAlpha[1] != 0); \n");
				strFragmentShader.append("  else readtex1 = readTexMS(uMSTex1, vTexCoord1, uFbMonochrome[1], uFbFixedAlpha[1] != 0); \n");
			} else
				strFragmentShader.append("  lowp vec4 readtex1 = readTex(uTex1, vTexCoord1, uFbMonochrome[1], uFbFixedAlpha[1] != 0); \n");
		}
#else
		if (usesTile(0))
			strFragmentShader.append("  lowp vec4 readtex0 = readTex(uTex0, vTexCoord0, uFbMonochrome[0], uFbFixedAlpha[0] != 0); \n");
		if (usesTile(1))
			strFragmentShader.append("  lowp vec4 readtex1 = readTex(uTex1, vTexCoord1, uFbMonochrome[1], uFbFixedAlpha[1] != 0); \n");
#endif // GL_MULTISAMPLING_SUPPORT
	}
	const bool bUseHWLight = config.generalEmulation.enableHWLighting != 0 && GBI.isHWLSupported() && usesShadeColor();
	if (bUseHWLight)
		strFragmentShader.append("  calc_light(vNumLights, vShadeColor.rgb, input_color); \n");
	else
		strFragmentShader.append("  input_color = vShadeColor.rgb;\n");
	strFragmentShader.append("  vec_color = vec4(input_color, vShadeColor.a); \n");
	strFragmentShader.append(strCombiner);

	strFragmentShader.append(
		"  if (uEnableAlphaTest != 0) {				\n"
		"    lowp float alphaTestValue = (uAlphaCompareMode == 3 && alpha2 > 0.0) ? snoise() : uAlphaTestValue;	\n"
		"    if  (alpha2 < alphaTestValue) discard;	\n"
		"  }										\n"
		);

	if (config.generalEmulation.enableNoise != 0) {
		strFragmentShader.append(
			"  if (uColorDitherMode == 2) colorNoiseDither(snoise(), color2);	\n"
			"  if (uAlphaDitherMode == 2) alphaNoiseDither(snoise(), alpha2);	\n"
		);
	}

	strFragmentShader.append(
		"  lowp int fogUsage = uFogUsage;			\n"
		"  if (fogUsage >= 256) fogUsage -= 256;	\n"
		"  if (fogUsage == 2) fragColor = vec4(color2, uFogColor.a);				\n"
		"  else if (fogUsage == 3) fragColor = uFogColor;							\n"
		"  else if (fogUsage == 4) fragColor = vec4(color2, uFogColor.a*alpha2);	\n"
		"  else fragColor = vec4(color2, alpha2);									\n"
	);

	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		strFragmentShader.append("  if (!depth_compare()) discard; \n");

	strFragmentShader.append(
		"  if (uFogUsage == 257) \n"
		"    fragColor.rgb = mix(fragColor.rgb, uFogColor.rgb, vFogFragCoord); \n"
		"  if (uGammaCorrectionEnabled != 0) \n"
		"    fragColor.rgb = sqrt(fragColor.rgb); \n"
		);

	strFragmentShader.append(fragment_shader_end);

	if (config.generalEmulation.enableNoise == 0)
		strFragmentShader.append(fragment_shader_dummy_noise);

	if (bUseLod && config.generalEmulation.enableLOD == 0)
		strFragmentShader.append(fragment_shader_fake_mipmap);

#ifdef GLESX
	if (bUseHWLight)
		strFragmentShader.append(fragment_shader_calc_light);
	if (bUseLod) {
		if (config.generalEmulation.enableLOD != 0)
			strFragmentShader.append(fragment_shader_mipmap);
	} else if (usesTexture()) {
		strFragmentShader.append(config.texture.bilinearMode == BILINEAR_3POINT ? fragment_shader_readtex_3point : fragment_shader_readtex);
#ifdef GL_MULTISAMPLING_SUPPORT
		if (config.video.multisampling > 0)
			strFragmentShader.append(fragment_shader_readtex_ms);
#endif
	}
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		strFragmentShader.append(depth_compare_shader_float);
#endif
	if (config.generalEmulation.enableNoise != 0) {
		strFragmentShader.append(fragment_shader_noise);
		strFragmentShader.append(fragment_shader_dither);
	}
#endif

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
#ifndef GLESX
	if (bUseHWLight)
		glAttachShader(m_program, g_calc_light_shader_object);
	if (bUseLod) {
		if (config.generalEmulation.enableLOD != 0)
			glAttachShader(m_program, g_calc_mipmap_shader_object);
	} else if (usesTexture()) {
		glAttachShader(m_program, g_readtex_shader_object);
		if (config.video.multisampling > 0)
			glAttachShader(m_program, g_readtex_ms_shader_object);
	}
	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		glAttachShader(m_program, g_calc_depth_shader_object);
	if (config.generalEmulation.enableNoise != 0) {
		glAttachShader(m_program, g_calc_noise_shader_object);
		glAttachShader(m_program, g_dither_shader_object);
	}
#endif
	if (CombinerInfo::get().isShaderCacheSupported())
		glProgramParameteri(m_program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
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
	LocateUniform(uFogMode);
	LocateUniform(uFogUsage);
	LocateUniform(uAlphaCompareMode);
	LocateUniform(uAlphaDitherMode);
	LocateUniform(uColorDitherMode);
	LocateUniform(uGammaCorrectionEnabled);
	LocateUniform(uEnableLod);
	LocateUniform(uEnableAlphaTest);
	LocateUniform(uEnableDepth);
	LocateUniform(uEnableDepthCompare)
	LocateUniform(uEnableDepthUpdate);
	LocateUniform(uDepthMode);
	LocateUniform(uDepthSource);
	LocateUniform(uFbMonochrome);
	LocateUniform(uFbFixedAlpha);
	LocateUniform(uMaxTile)
	LocateUniform(uTextureDetail);
	LocateUniform(uTexturePersp);
	LocateUniform(uTextureFilterMode);
	LocateUniform(uSpecialBlendMode);

	LocateUniform(uFogAlpha);
	LocateUniform(uMinLod);
	LocateUniform(uDeltaZ);
	LocateUniform(uAlphaTestValue);

	LocateUniform(uRenderState);

	LocateUniform(uScreenScale);
	LocateUniform(uDepthScale);
	LocateUniform(uFogScale);

#ifdef GL_MULTISAMPLING_SUPPORT
	LocateUniform(uMSTex0);
	LocateUniform(uMSTex1);
	LocateUniform(uMSTexEnabled);
	LocateUniform(uMSAASamples);
	LocateUniform(uMSAAScale);
#endif
}

void ShaderCombiner::_locate_attributes() const {
	glBindAttribLocation(m_program, SC_POSITION, "aPosition");
	glBindAttribLocation(m_program, SC_COLOR, "aColor");
	glBindAttribLocation(m_program, SC_TEXCOORD0, "aTexCoord0");
	glBindAttribLocation(m_program, SC_TEXCOORD1, "aTexCoord1");
	glBindAttribLocation(m_program, SC_NUMLIGHTS, "aNumLights");
}

void ShaderCombiner::update(bool _bForce) {
	glUseProgram(m_program);

	if (_bForce) {
		m_uniforms.uTexNoise.set(g_noiseTexIndex, true);
		if (usesTexture()) {
			m_uniforms.uTex0.set(0, true);
			m_uniforms.uTex1.set(1, true);
#ifdef GL_MULTISAMPLING_SUPPORT
			m_uniforms.uMSTex0.set(g_MSTex0Index + 0, true);
			m_uniforms.uMSTex1.set(g_MSTex0Index + 1, true);
			m_uniforms.uMSAASamples.set(config.video.multisampling, true);
			m_uniforms.uMSAAScale.set(1.0f / (float)config.video.multisampling, true);
			m_uniforms.uMSTexEnabled.set(0, 0, true);
#endif
		}

		updateFBInfo(true);
		updateRenderState(true);
	}

	updateGammaCorrection(_bForce);
	updateFogMode(_bForce);
	updateDitherMode(_bForce);
	updateLOD(_bForce);
	updateTextureInfo(_bForce);
	updateAlphaTestInfo(_bForce);
	updateDepthInfo(_bForce);
}

void ShaderCombiner::updateRenderState(bool _bForce)
{
	m_uniforms.uRenderState.set(video().getRender().getRenderState(), _bForce);
}

void ShaderCombiner::updateGammaCorrection(bool _bForce)
{
	m_uniforms.uGammaCorrectionEnabled.set(*REG.VI_STATUS & 8, _bForce);
}

void ShaderCombiner::updateFogMode(bool _bForce)
{
	const u32 blender = (gDP.otherMode.l >> 16);
	const int nFogBlendEnabled = config.generalEmulation.enableFog != 0 && gSP.fog.multiplier >= 0 && (gDP.otherMode.c1_m1a == 3 || gDP.otherMode.c1_m2a == 3 || gDP.otherMode.c2_m1a == 3 || gDP.otherMode.c2_m2a == 3) ? 256 : 0;
	int nFogUsage = ((gSP.geometryMode & G_FOG) != 0) ? 1 : 0;
	int nSpecialBlendMode = 0;
	switch (blender) {
	case 0x0150:
	case 0x0D18:
		nFogUsage = gDP.otherMode.cycleType == G_CYC_2CYCLE ? 2 : 0;
		break;
	case 0x0440:
		nFogUsage = gDP.otherMode.cycleType == G_CYC_1CYCLE ? 2 : 0;
		break;
	case 0xC912:
		nFogUsage = 2;
		break;
	case 0xF550:
		nFogUsage = 3;
		break;
	case 0x0550:
		nFogUsage = 4;
		break;
	case 0x0382:
	case 0x0091:
		// Mace
		// CLR_IN * A_IN + CLR_BL * 1MA
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE)
			nSpecialBlendMode = 1;
		break;
	case 0xA500:
		// Bomberman 2
		// CLR_BL * A_FOG + CLR_IN * 1MA
		if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
			nSpecialBlendMode = 2;
			nFogUsage = 5;
		}
		break;
	case 0x07C2:
		// Conker BFD shadow
		// CLR_IN * A_FOG + CLR_FOG * 1MA
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
			nSpecialBlendMode = 3;
			nFogUsage = 5;
		}
		break;
		/* Brings troubles with Roadsters sky
		case 0xc702:
		// Donald Duck
		// clr_fog*a_fog + clr_in*1ma
		nFogUsage = 5;
		nSpecialBlendMode = 2;
		break;
		*/
	}

	int nFogMode = 0; // Normal
	if (nFogUsage == 0) {
		switch (blender) {
		case 0xC410:
		case 0xC411:
		case 0xF500:
			nFogMode = 1; // fog blend
			nFogUsage = 1;
			break;
		case 0x04D1:
			nFogMode = 2; // inverse fog blend
			nFogUsage = 1;
			break;
		}
	}

	m_uniforms.uSpecialBlendMode.set(nSpecialBlendMode, _bForce);
	m_uniforms.uFogUsage.set(nFogUsage | nFogBlendEnabled, _bForce);
	m_uniforms.uFogMode.set(nFogMode, _bForce);
	if (nFogUsage + nFogMode != 0) {
		m_uniforms.uFogScale.set((float)gSP.fog.multiplier / 256.0f, (float)gSP.fog.offset / 256.0f, _bForce);
		m_uniforms.uFogAlpha.set(gDP.fogColor.a, _bForce);
	}
}

void ShaderCombiner::updateDitherMode(bool _bForce)
{
	if (gDP.otherMode.cycleType < G_CYC_COPY) {
		m_uniforms.uAlphaCompareMode.set(gDP.otherMode.alphaCompare, _bForce);
		m_uniforms.uAlphaDitherMode.set(gDP.otherMode.alphaDither, _bForce);
		m_uniforms.uColorDitherMode.set(gDP.otherMode.colorDither, _bForce);
	} else {
		m_uniforms.uAlphaCompareMode.set(0, _bForce);
		m_uniforms.uAlphaDitherMode.set(0, _bForce);
		m_uniforms.uColorDitherMode.set(0, _bForce);
	}

	const int nDither = (gDP.otherMode.cycleType < G_CYC_COPY) && (gDP.otherMode.colorDither == G_CD_NOISE || gDP.otherMode.alphaDither == G_AD_NOISE || gDP.otherMode.alphaCompare == G_AC_DITHER) ? 1 : 0;
	if ((m_nInputs & (1 << NOISE)) + nDither != 0) {
		m_uniforms.uScreenScale.set(video().getScaleX(), video().getScaleY(), _bForce);
		noiseTex.update();
	}
}

void ShaderCombiner::updateLOD(bool _bForce)
{
	if (!usesLOD())
		return;

	m_uniforms.uMinLod.set(gDP.primColor.m, _bForce);
	m_uniforms.uMaxTile.set(gSP.texture.level, _bForce);

	if (config.generalEmulation.enableLOD != 0) {
		const int uCalcLOD = (gDP.otherMode.textureLOD == G_TL_LOD) ? 1 : 0;
		m_uniforms.uEnableLod.set(uCalcLOD, _bForce);
		m_uniforms.uScreenScale.set(video().getScaleX(), video().getScaleY(), _bForce);
		m_uniforms.uTextureDetail.set(gDP.otherMode.textureDetail, _bForce);
	}
}

void ShaderCombiner::updateTextureInfo(bool _bForce) {
	m_uniforms.uTexturePersp.set(gDP.otherMode.texturePersp, _bForce);
	if (config.texture.bilinearMode == BILINEAR_3POINT)
		m_uniforms.uTextureFilterMode.set(gDP.otherMode.textureFilter | (gSP.objRendermode&G_OBJRM_BILERP), _bForce);
}

void ShaderCombiner::updateFBInfo(bool _bForce) {
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
		nMSTex0Enabled = cache.current[0]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
	}
	if (cache.current[1] != NULL && cache.current[1]->frameBufferTexture != CachedTexture::fbNone) {
		if (cache.current[1]->size == G_IM_SIZ_8b) {
			nFbMonochromeMode1 = 1;
			if (gDP.otherMode.imageRead == 0)
				nFbFixedAlpha1 = 1;
		} else if (gSP.textureTile[1]->size == G_IM_SIZ_16b && gSP.textureTile[1]->format == G_IM_FMT_IA)
			nFbMonochromeMode1 = 2;
		nMSTex1Enabled = cache.current[1]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
	}
	m_uniforms.uFbMonochrome.set(nFbMonochromeMode0, nFbMonochromeMode1, _bForce);
	m_uniforms.uFbFixedAlpha.set(nFbFixedAlpha0, nFbFixedAlpha1, _bForce);
	m_uniforms.uMSTexEnabled.set(nMSTex0Enabled, nMSTex1Enabled, _bForce);

	gDP.changed &= ~CHANGED_FB_TEXTURE;
}

void ShaderCombiner::updateDepthInfo(bool _bForce) {
	if (RSP.bLLE)
		m_uniforms.uDepthScale.set(0.5f, 0.5f, _bForce);
	else
		m_uniforms.uDepthScale.set(gSP.viewport.vscale[2], gSP.viewport.vtrans[2], _bForce);

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
		m_uniforms.uAlphaTestValue.set(0.0f, _bForce);
	} else if (gDP.otherMode.cycleType == G_CYC_COPY) {
		if (gDP.otherMode.alphaCompare & G_AC_THRESHOLD) {
			m_uniforms.uEnableAlphaTest.set(1, _bForce);
			m_uniforms.uAlphaTestValue.set(0.5f, _bForce);
		} else {
			m_uniforms.uEnableAlphaTest.set(0, _bForce);
			m_uniforms.uAlphaTestValue.set(0.0f, _bForce);
		}
	} else if (((gDP.otherMode.alphaCompare & G_AC_THRESHOLD) != 0) && (gDP.otherMode.alphaCvgSel == 0) && (gDP.otherMode.forceBlender == 0 || gDP.blendColor.a > 0))	{
		m_uniforms.uEnableAlphaTest.set(1, _bForce);
		m_uniforms.uAlphaTestValue.set(max(gDP.blendColor.a, 1.0f / 256.0f), _bForce);
	} else if ((gDP.otherMode.alphaCompare == G_AC_DITHER) && (gDP.otherMode.alphaCvgSel == 0))	{
		m_uniforms.uEnableAlphaTest.set(1, _bForce);
		m_uniforms.uAlphaTestValue.set(0.0f, _bForce);
	} else if (gDP.otherMode.cvgXAlpha != 0)	{
		m_uniforms.uEnableAlphaTest.set(1, _bForce);
		m_uniforms.uAlphaTestValue.set(0.125f, _bForce);
	} else {
		m_uniforms.uEnableAlphaTest.set(0, _bForce);
		m_uniforms.uAlphaTestValue.set(0.0f, _bForce);
	}
}

std::ostream & operator<< (std::ostream & _os, const ShaderCombiner & _combiner)
{
	GLint  binaryLength;
	glGetProgramiv(_combiner.m_program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

	if (binaryLength < 1)
		return _os;

	std::vector<char> binary(binaryLength);

	if (binary.size() == 0)
		return _os;

	GLenum binaryFormat;
	glGetProgramBinary(_combiner.m_program, binaryLength, &binaryLength, &binaryFormat, binary.data());
	if (isGLError())
		return _os;

	_os.write((char*)&_combiner.m_combine.mux, sizeof(_combiner.m_combine.mux));
	_os.write((char*)&_combiner.m_nInputs, sizeof(_combiner.m_nInputs));
	_os.write((char*)&binaryFormat, sizeof(binaryFormat));
	_os.write((char*)&binaryLength, sizeof(binaryLength));
	_os.write(binary.data(), binaryLength);
	return _os;
}

std::istream & operator>> (std::istream & _is, ShaderCombiner & _combiner)
{
	_is.read((char*)&_combiner.m_combine.mux, sizeof(_combiner.m_combine.mux));
	_is.read((char*)&_combiner.m_nInputs, sizeof(_combiner.m_nInputs));
	GLenum binaryFormat;
	GLint  binaryLength;
	_is.read((char*)&binaryFormat, sizeof(binaryFormat));
	_is.read((char*)&binaryLength, sizeof(binaryLength));
	std::vector<char> binary(binaryLength);
	_is.read(binary.data(), binaryLength);

	glProgramBinary(_combiner.m_program, binaryFormat, binary.data(), binaryLength);
	assert(checkProgramLinkStatus(_combiner.m_program));
	_combiner._locateUniforms();
	return _is;
}

void ShaderCombiner::getShaderCombinerOptionsSet(std::vector<u32> & _vecOptions)
{
	// WARNING: Shader Storage format version must be increased after any change in this function.
	_vecOptions.push_back(config.video.multisampling > 0 ? 1 : 0);
	_vecOptions.push_back(config.texture.bilinearMode);
	_vecOptions.push_back(config.generalEmulation.enableHWLighting);
	_vecOptions.push_back(config.generalEmulation.enableNoise);
	_vecOptions.push_back(config.generalEmulation.enableLOD);
	_vecOptions.push_back(config.frameBufferEmulation.N64DepthCompare);
}

#ifdef GL_IMAGE_TEXTURES_SUPPORT
void SetDepthFogCombiner()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

	if (g_paletteCRC256 != gDP.paletteCRC256) {
		g_paletteCRC256 = gDP.paletteCRC256;

#ifdef GLESX
		u32 palette[256];
#else
		u16 palette[256];
#endif
		u16 *src = (u16*)&TMEM[256];
		for (int i = 0; i < 256; ++i)
			palette[i] = swapword(src[i*4]);
		glBindImageTexture(TlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);
		glBindTexture(GL_TEXTURE_2D, g_tlut_tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, fboFormats.lutFormat, fboFormats.lutType, palette);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindImageTexture(TlutImageUnit, g_tlut_tex, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);
	}

	glUseProgram(g_draw_shadow_map_program);
	int loc = glGetUniformLocation(g_draw_shadow_map_program, "uFogColor");
	if (loc >= 0)
		glUniform4fv(loc, 1, &gDP.fogColor.r);

	gDP.changed |= CHANGED_COMBINE;
}
#endif // GL_IMAGE_TEXTURES_SUPPORT

void SetMonochromeCombiner() {
	glUseProgram(g_monochrome_image_program);
	gDP.changed |= CHANGED_COMBINE;
}
