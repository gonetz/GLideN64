#include <assert.h>
#include <stdio.h>
#include <string>

#include "N64.h"
#include "OpenGL.h"
#include "Config.h"
#include "GLSLCombiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "RSP.h"
#include "VI.h"
#include "Log.h"

#include "Shaders.h"

using namespace std;

static GLuint  g_vertex_shader_object;
static GLuint  g_vertex_shader_object_notex;
static GLuint  g_calc_light_shader_object;
static GLuint  g_calc_mipmap_shader_object;
static GLuint  g_calc_noise_shader_object;
static GLuint  g_calc_depth_shader_object;
static GLuint  g_readtex_shader_object;
static GLuint  g_dither_shader_object;

GLuint g_monochrome_image_program = 0;
#ifdef GL_IMAGE_TEXTURES_SUPPORT
GLuint g_draw_shadow_map_program = 0;
static GLuint g_zlut_tex = 0;
GLuint g_tlut_tex = 0;
static u32 g_paletteCRC256 = 0;
#endif // GL_IMAGE_TEXTURES_SUPPORT

#ifndef GLESX
#define GL_RED16 GL_R16
#else
#define GL_RED16 GL_R16UI
#endif

static std::string strFragmentShader;

static const GLsizei nShaderLogSize = 1024;
bool checkShaderCompileStatus(GLuint obj)
{
	GLint status;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
	if(status == GL_FALSE) {
		GLchar shader_log[nShaderLogSize];
		GLsizei nLogSize = nShaderLogSize;
		glGetShaderInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		shader_log[nLogSize] = 0;
		LOG(LOG_ERROR, "shader_compile error: %s\n", shader_log);
		return false;
	}
	return true;
}

bool checkProgramLinkStatus(GLuint obj)
{
	GLint status;
	glGetProgramiv(obj, GL_LINK_STATUS, &status);
	if(status == GL_FALSE) {
		GLsizei nLogSize = nShaderLogSize;
		GLchar shader_log[nShaderLogSize];
		glGetProgramInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		LOG(LOG_ERROR, "shader_link error: %s\n", shader_log);
		return false;
	}
	return true;
}

class NoiseTexture
{
public:
	NoiseTexture() : m_pTexture(NULL), m_PBO(0), m_DList(0) {}
	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture;
#ifndef GLES2
	GLuint m_PBO;
#else
	GLubyte* m_PBO;
#endif
	u32 m_DList;
} noiseTex;

void NoiseTexture::init()
{
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
#ifdef GLES2
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Generate Pixel Buffer Object. Initialize it later
#ifndef GLES2
	glGenBuffers(1, &m_PBO);
#endif
}

void NoiseTexture::destroy()
{
	if (m_pTexture != NULL) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = NULL;
	}
#ifndef GLES2
	glDeleteBuffers(1, &m_PBO);
	m_PBO = 0;
#endif
}

void NoiseTexture::update()
{
	if (m_DList == RSP.DList)
		return;
	const u32 dataSize = VI.width*VI.height;
	if (dataSize == 0)
		return;
#ifndef GLES2
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, NULL, GL_DYNAMIC_DRAW);
	isGLError();
	GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
	isGLError();
#else
	m_PBO = (GLubyte*)malloc(dataSize);
	GLubyte* ptr = m_PBO;
	PBOBinder binder(m_PBO);
#endif // GLES2
	if (ptr == NULL)
		return;
	for (u32 y = 0; y < VI.height; ++y)	{
		for (u32 x = 0; x < VI.width; ++x)
			ptr[x + y*VI.width] = rand()&0xFF;
	}
#ifndef GLES2
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
#endif

	glActiveTexture(GL_TEXTURE0 + g_noiseTexIndex);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
#ifndef GLES2
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VI.width, VI.height, GL_RED, GL_UNSIGNED_BYTE, 0);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VI.width, VI.height, GL_LUMINANCE, GL_UNSIGNED_BYTE, m_PBO);
#endif
	m_DList = RSP.DList;
}


#ifdef GL_IMAGE_TEXTURES_SUPPORT
static
void InitZlutTexture()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

	const u16 * const zLUT = depthBufferList().getZLUT();
	glGenTextures(1, &g_zlut_tex);
	glBindTexture(GL_TEXTURE_2D, g_zlut_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED16,
		512, 512, 0, GL_RED, GL_UNSIGNED_SHORT,
		zLUT);
	glBindImageTexture(ZlutImageUnit, g_zlut_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
}

static
void DestroyZlutTexture()
{
	if (!video().getRender().isImageTexturesSupported())
		return;
	glBindImageTexture(ZlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
	if (g_zlut_tex > 0) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &g_zlut_tex);
		g_zlut_tex = 0;
	}
}

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment)
{
	GLuint vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_object, 1, &_strVertex, NULL);
	glCompileShader(vertex_shader_object);
	assert(checkShaderCompileStatus(vertex_shader_object));

	GLuint fragment_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_object, 1, &_strFragment, NULL);
	glCompileShader(fragment_shader_object);
	assert(checkShaderCompileStatus(fragment_shader_object));

	GLuint program = glCreateProgram();
	glBindAttribLocation(program, SC_POSITION, "aPosition");
	glAttachShader(program, vertex_shader_object);
	glAttachShader(program, fragment_shader_object);
	glLinkProgram(program);
	glDeleteShader(vertex_shader_object);
	glDeleteShader(fragment_shader_object);
	assert(checkProgramLinkStatus(program));
	return program;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED16, 256, 1, 0, GL_RED, GL_UNSIGNED_SHORT, NULL);

	g_draw_shadow_map_program = createShaderProgram(default_vertex_shader, shadow_map_fragment_shader_float);
	g_monochrome_image_program = createShaderProgram(default_vertex_shader, zelda_monochrome_fragment_shader);
}

static
void DestroyShadowMapShader()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

	glBindImageTexture(TlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);

	if (g_tlut_tex > 0) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &g_tlut_tex);
		g_tlut_tex = 0;
	}
	glDeleteProgram(g_draw_shadow_map_program);
	g_draw_shadow_map_program = 0;
	glDeleteProgram(g_monochrome_image_program);
	g_monochrome_image_program = 0;
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
	g_readtex_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_readtex);
	g_dither_shader_object = _createShader(GL_FRAGMENT_SHADER, fragment_shader_dither);
#endif // GLESX

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		g_calc_depth_shader_object = _createShader(GL_FRAGMENT_SHADER, depth_compare_shader_float);

	InitZlutTexture();
	InitShadowMapShader();
	noiseTex.init();
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
	glDeleteShader(g_calc_noise_shader_object);
	g_calc_noise_shader_object = 0;
	glDeleteShader(g_dither_shader_object);
	g_dither_shader_object = 0;
	glDeleteShader(g_calc_depth_shader_object);
	g_calc_depth_shader_object = 0;
#endif // GLESX

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	noiseTex.destroy();
	DestroyZlutTexture();
	DestroyShadowMapShader();
#endif // GL_IMAGE_TEXTURES_SUPPORT
}

const char *ColorInput[] = {
	"combined_color.rgb",
	"readtex0.rgb",
	"readtex1.rgb",
	"uPrimColor.rgb",
	"vec_color.rgb",
	"uEnvColor.rgb",
	"uCenterColor.rgb",
	"uScaleColor.rgb",
	"combined_color.a",
	"vec3(readtex0.a)",
	"vec3(readtex1.a)",
	"vec3(uPrimColor.a)",
	"vec3(vec_color.a)",
	"vec3(uEnvColor.a)",
	"vec3(lod_frac)",
	"vec3(uPrimLod)",
	"vec3(0.5 + 0.5*snoise())",
	"vec3(uK4)",
	"vec3(uK5)",
	"vec3(1.0)",
	"vec3(0.0)"
};

const char *AlphaInput[] = {
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"uPrimColor.a",
	"vec_color.a",
	"uEnvColor.a",
	"uCenterColor.a",
	"uScaleColor.a",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"uPrimColor.a",
	"vec_color.a",
	"uEnvColor.a",
	"lod_frac",
	"uPrimLod",
	"0.5 + 0.5*snoise()",
	"uK4",
	"uK5",
	"1.0",
	"0.0"
};

inline
int CorrectFirstStageParam(int _param)
{
	switch (_param) {
		case TEXEL1:
		return TEXEL0;
		case TEXEL1_ALPHA:
		return TEXEL0_ALPHA;
	}
	return _param;
}

static
void CorrectFirstStageParams(CombinerStage & _stage)
{
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = CorrectFirstStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = CorrectFirstStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = CorrectFirstStageParam(_stage.op[i].param3);
	}
}

inline
int CorrectSecondStageParam(int _param)
{
	switch (_param) {
		case TEXEL0:
			return TEXEL1;
		case TEXEL1:
			return TEXEL0;
		case TEXEL0_ALPHA:
			return TEXEL1_ALPHA;
		case TEXEL1_ALPHA:
			return TEXEL0_ALPHA;
	}
	return _param;
}

static
void CorrectSecondStageParams(CombinerStage & _stage) {
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = CorrectSecondStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = CorrectSecondStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = CorrectSecondStageParam(_stage.op[i].param3);
	}
}

static
int CompileCombiner(const CombinerStage & _stage, const char** _Input, char * _strCombiner) {
	char buf[128];
	bool bBracketOpen = false;
	int nRes = 0;
	for (int i = 0; i < _stage.numOps; ++i) {
		switch(_stage.op[i].op) {
			case LOAD:
				sprintf(buf, "(%s ", _Input[_stage.op[i].param1]);
				strcat(_strCombiner, buf);
				bBracketOpen = true;
				nRes |= 1 << _stage.op[i].param1;
				break;
			case SUB:
				if (bBracketOpen) {
					sprintf(buf, "- %s)", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
				strcat(_strCombiner, buf);
				nRes |= 1 << _stage.op[i].param1;
				break;
			case ADD:
				if (bBracketOpen) {
					sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
				strcat(_strCombiner, buf);
				nRes |= 1 << _stage.op[i].param1;
				break;
			case MUL:
				if (bBracketOpen) {
					sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "*%s", _Input[_stage.op[i].param1]);
				strcat(_strCombiner, buf);
				nRes |= 1 << _stage.op[i].param1;
				break;
			case INTER:
				sprintf(buf, "mix(%s, %s, %s)", _Input[_stage.op[0].param2], _Input[_stage.op[0].param1], _Input[_stage.op[0].param3]);
				strcat(_strCombiner, buf);
				nRes |= 1 << _stage.op[i].param1;
				nRes |= 1 << _stage.op[i].param2;
				nRes |= 1 << _stage.op[i].param3;
				break;

				//			default:
				//				assert(false);
		}
	}
	if (bBracketOpen)
		strcat(_strCombiner, ")");
	strcat(_strCombiner, "; \n");
	return nRes;
}

ShaderCombiner::ShaderCombiner(Combiner & _color, Combiner & _alpha, const gDPCombine & _combine) : m_combine(_combine)
{
	if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
		CorrectFirstStageParams(_alpha.stage[0]);
		CorrectFirstStageParams(_color.stage[0]);
	}
	char strCombiner[1024];
	strcpy(strCombiner, "  alpha1 = ");
	m_nInputs = CompileCombiner(_alpha.stage[0], AlphaInput, strCombiner);
	strcat(strCombiner, "  color1 = ");
	m_nInputs |= CompileCombiner(_color.stage[0], ColorInput, strCombiner);
	strcat(strCombiner, fragment_shader_blender);

	strcat(strCombiner, "  combined_color = vec4(color1, alpha1); \n");
	if (_alpha.numStages == 2) {
		strcat(strCombiner, "  alpha2 = ");
		CorrectSecondStageParams(_alpha.stage[1]);
		m_nInputs |= CompileCombiner(_alpha.stage[1], AlphaInput, strCombiner);
	} else
		strcat(strCombiner, "  alpha2 = alpha1; \n");
	if (_color.numStages == 2) {
		strcat(strCombiner, "  color2 = ");
		CorrectSecondStageParams(_color.stage[1]);
		m_nInputs |= CompileCombiner(_color.stage[1], ColorInput, strCombiner);
	} else
		strcat(strCombiner, "  color2 = color1; \n");

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
				strFragmentShader.append("  if (uMSTexEnabled[0] == 0) readtex0 = readTex(uTex0, vTexCoord0, uFb8Bit[0] != 0, uFbFixedAlpha[0] != 0); \n");
				strFragmentShader.append("  else readtex0 = readTexMS(uMSTex0, vTexCoord0, uFb8Bit[0] != 0, uFbFixedAlpha[0] != 0); \n");
			} else
				strFragmentShader.append("  lowp vec4 readtex0 = readTex(uTex0, vTexCoord0, uFb8Bit[0] != 0, uFbFixedAlpha[0] != 0); \n");
		}
		if (usesTile(1)) {
			if (config.video.multisampling > 0) {
				strFragmentShader.append("  lowp vec4 readtex1; \n");
				strFragmentShader.append("  if (uMSTexEnabled[1] == 0) readtex1 = readTex(uTex1, vTexCoord1, uFb8Bit[1] != 0, uFbFixedAlpha[1] != 0); \n");
				strFragmentShader.append("  else readtex1 = readTexMS(uMSTex1, vTexCoord1, uFb8Bit[1] != 0, uFbFixedAlpha[1] != 0); \n");
			} else
				strFragmentShader.append("  lowp vec4 readtex1 = readTex(uTex1, vTexCoord1, uFb8Bit[1] != 0, uFbFixedAlpha[1] != 0); \n");
		}
#else
		if (usesTile(0))
			strFragmentShader.append("  lowp vec4 readtex0 = readTex(uTex0, vTexCoord0, uFb8Bit[0] != 0, uFbFixedAlpha[0] != 0); \n");
		if (usesTile(1))
			strFragmentShader.append("  lowp vec4 readtex1 = readTex(uTex1, vTexCoord1, uFb8Bit[1] != 0, uFbFixedAlpha[1] != 0); \n");
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
		"  switch (uFogUsage&255) {								\n"
		"	case 2:												\n"
		"		fragColor = vec4(color2, uFogColor.a);			\n"
		"	break;												\n"
		"	case 3:												\n"
		"		fragColor = uFogColor;							\n"
		"	break;												\n"
		"	case 4:												\n"
		"		fragColor = vec4(color2, uFogColor.a*alpha2);	\n"
		"	break;												\n"
		"	default:											\n"
		"		fragColor = vec4(color2, alpha2);				\n"
		"	break;												\n"
		"  }													\n"
	);

	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		strFragmentShader.append("  if (!depth_compare()) discard; \n");

#ifdef USE_TOONIFY
	strFragmentShader.append("  toonify(intensity); \n");
#endif
	strFragmentShader.append(
		"	if (uFogUsage == 257) \n"
		"		fragColor = vec4(mix(fragColor.rgb, uFogColor.rgb, vFogFragCoord), fragColor.a); \n"
		"	if (uGammaCorrectionEnabled != 0) \n"
		"		fragColor = vec4(sqrt(fragColor.rgb), fragColor.a); \n"
	);

	strFragmentShader.append(fragment_shader_end);

	if (config.generalEmulation.enableNoise == 0)
		strFragmentShader.append(fragment_shader_dummy_noise);

#ifdef USE_TOONIFY
	strFragmentShader.append(fragment_shader_toonify);
#endif

#ifdef GLESX
	if (bUseHWLight)
		strFragmentShader.append(fragment_shader_calc_light);
	if (bUseLod)
		strFragmentShader.append(fragment_shader_mipmap);
	else if (usesTexture())
		strFragmentShader.append(fragment_shader_readtex);
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
	if (bUseLod)
		glAttachShader(m_program, g_calc_mipmap_shader_object);
	else if (usesTexture())
		glAttachShader(m_program, g_readtex_shader_object);
	if (video().getRender().isImageTexturesSupported() && config.frameBufferEmulation.N64DepthCompare != 0)
		glAttachShader(m_program, g_calc_depth_shader_object);
	if (config.generalEmulation.enableNoise != 0) {
		glAttachShader(m_program, g_calc_noise_shader_object);
		glAttachShader(m_program, g_dither_shader_object);
	}
#endif
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
	LocateUniform(uFb8Bit);
	LocateUniform(uFbFixedAlpha);
	LocateUniform(uMaxTile)
	LocateUniform(uTextureDetail);
	LocateUniform(uTexturePersp);
	LocateUniform(uTextureFilterMode);
	LocateUniform(uSpecialBlendMode);

	LocateUniform(uFogAlpha);
	LocateUniform(uPrimitiveLod);
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
	if (usesLOD()) {
		int uCalcLOD = (config.generalEmulation.enableLOD && gDP.otherMode.textureLOD == G_TL_LOD) ? 1 : 0;
		m_uniforms.uEnableLod.set(uCalcLOD, _bForce);
		if (uCalcLOD) {
			m_uniforms.uScreenScale.set(video().getScaleX(), video().getScaleY(), _bForce);
			m_uniforms.uPrimitiveLod.set(gDP.primColor.l, _bForce);
			m_uniforms.uMinLod.set(gDP.primColor.m, _bForce);
			m_uniforms.uMaxTile.set(gSP.texture.level, _bForce);
			m_uniforms.uTextureDetail.set(gDP.otherMode.textureDetail, _bForce);
		}
	}
}

void ShaderCombiner::updateTextureInfo(bool _bForce) {
	m_uniforms.uTexturePersp.set(gDP.otherMode.texturePersp, _bForce);
	m_uniforms.uTextureFilterMode.set(config.texture.bilinearMode == BILINEAR_3POINT ? gDP.otherMode.textureFilter | (gSP.objRendermode&G_OBJRM_BILERP) : 0, _bForce);
}

void ShaderCombiner::updateFBInfo(bool _bForce) {
	if (!usesTexture())
		return;

	int nFb8bitMode0 = 0, nFb8bitMode1 = 0;
	int nFbFixedAlpha0 = 0, nFbFixedAlpha1 = 0;
	int nMSTex0Enabled = 0, nMSTex1Enabled = 0;
	TextureCache & cache = textureCache();
	if (cache.current[0] != NULL && cache.current[0]->frameBufferTexture != CachedTexture::fbNone) {
		if (cache.current[0]->size == G_IM_SIZ_8b) {
			nFb8bitMode0 = 1;
			if (gDP.otherMode.imageRead == 0)
				nFbFixedAlpha0 = 1;
		}
		nMSTex0Enabled = cache.current[0]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
	}
	if (cache.current[1] != NULL && cache.current[1]->frameBufferTexture != CachedTexture::fbNone) {
		if (cache.current[1]->size == G_IM_SIZ_8b) {
			nFb8bitMode1 = 1;
			if (gDP.otherMode.imageRead == 0)
				nFbFixedAlpha1 = 1;
		}
		nMSTex1Enabled = cache.current[1]->frameBufferTexture == CachedTexture::fbMultiSample ? 1 : 0;
	}
	m_uniforms.uFb8Bit.set(nFb8bitMode0, nFb8bitMode1, _bForce);
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
	} else if (gDP.otherMode.cvgXAlpha != 0)	{
		m_uniforms.uEnableAlphaTest.set(1, _bForce);
		m_uniforms.uAlphaTestValue.set(0.125f, _bForce);
	} else {
		m_uniforms.uEnableAlphaTest.set(0, _bForce);
		m_uniforms.uAlphaTestValue.set(0.0f, _bForce);
	}
}

#ifdef GL_IMAGE_TEXTURES_SUPPORT
void SetDepthFogCombiner()
{
	if (!video().getRender().isImageTexturesSupported())
		return;

	if (g_paletteCRC256 != gDP.paletteCRC256) {
		g_paletteCRC256 = gDP.paletteCRC256;
		u16 palette[256];
		u16 *src = (u16*)&TMEM[256];
		for (int i = 0; i < 256; ++i)
			palette[i] = swapword(src[i*4]);
		glBindImageTexture(TlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
		glBindTexture(GL_TEXTURE_2D, g_tlut_tex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RED, GL_UNSIGNED_SHORT, palette);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindImageTexture(TlutImageUnit, g_tlut_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
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

/*======================UniformBlock==========================*/
#ifdef GL_UNIFORMBLOCK_SUPPORT

static
const char * strTextureUniforms[UniformBlock::tuTotal] = {
	"uTexScale",
	"uTexMask",
	"uTexOffset",
	"uCacheScale",
	"uCacheOffset",
	"uCacheShiftScale",
	"uCacheFrameBuffer"
};

static
const char * strColorUniforms[UniformBlock::cuTotal] = {
	"uFogColor",
	"uCenterColor",
	"uScaleColor",
	"uBlendColor",
	"uEnvColor",
	"uPrimColor",
	"uPrimLod",
	"uK4",
	"uK5"
};

static
const char * strLightUniforms[UniformBlock::luTotal] = {
	"uLightDirection",
	"uLightColor"
};

UniformBlock::UniformBlock() : m_currentBuffer(0)
{
}

UniformBlock::~UniformBlock()
{
}

void UniformBlock::_initTextureBuffer(GLuint _program)
{
	const GLint blockSize = m_textureBlock.initBuffer(_program, "TextureBlock", strTextureUniforms);
	if (blockSize == 0)
		return;
	m_textureBlockData.resize(blockSize);
	GLbyte * pData = m_textureBlockData.data();
	memset(pData, 0, blockSize);
	glBindBuffer(GL_UNIFORM_BUFFER, m_textureBlock.m_buffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_textureBlock.m_blockBindingPoint, m_textureBlock.m_buffer);
	updateTextureParameters();
}

void UniformBlock::_initColorsBuffer(GLuint _program)
{
	const GLint blockSize = m_colorsBlock.initBuffer(_program, "ColorsBlock", strColorUniforms);
	if (blockSize == 0)
		return;
	m_colorsBlockData.resize(blockSize);
	GLbyte * pData = m_colorsBlockData.data();
	memset(pData, 0, blockSize);
	memcpy(pData + m_colorsBlock.m_offsets[cuFogColor], &gDP.fogColor.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuCenterColor], &gDP.key.center.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuScaleColor], &gDP.key.scale.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuEnvColor], &gDP.envColor.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuPrimColor], &gDP.primColor.r, sizeof(f32)* 4);
	*(f32*)(pData + m_colorsBlock.m_offsets[cuPrimLod]) = gDP.primColor.l;
	*(f32*)(pData + m_colorsBlock.m_offsets[cuK4]) = gDP.convert.k4*0.0039215689f;
	*(f32*)(pData + m_colorsBlock.m_offsets[cuK5]) = gDP.convert.k5*0.0039215689f;

	glBindBuffer(GL_UNIFORM_BUFFER, m_colorsBlock.m_buffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, pData, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_colorsBlock.m_blockBindingPoint, m_colorsBlock.m_buffer);
	m_currentBuffer = m_colorsBlock.m_buffer;
}

void UniformBlock::_initLightBuffer(GLuint _program)
{
	const GLint blockSize = m_lightBlock.initBuffer(_program, "LightBlock", strLightUniforms);
	if (blockSize == 0)
		return;
	m_lightBlockData.resize(blockSize);
	GLbyte * pData = m_lightBlockData.data();
	memset(pData, 0, blockSize);
	glBindBuffer(GL_UNIFORM_BUFFER, m_lightBlock.m_buffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_lightBlock.m_blockBindingPoint, m_lightBlock.m_buffer);
	updateLightParameters();
}

bool UniformBlock::_isDataChanged(void * _pBuffer, const void * _pData, u32 _dataSize)
{
	u32 * pSrc = (u32*)_pData;
	u32 * pDst = (u32*)_pBuffer;
	u32 cnt = _dataSize / 4;
	for (u32 i = 0; i < cnt; ++i) {
		if (pSrc[i] != pDst[i]) {
			memcpy(_pBuffer, _pData, _dataSize);
			return true;
		}
	}
	return false;
}

void UniformBlock::bindWithShaderCombiner(ShaderCombiner * _pCombiner)
{
	const GLuint program = _pCombiner->m_program;
	if (_pCombiner->usesTexture()) {
		if (m_textureBlock.m_buffer == 0)
			_initTextureBuffer(program);
		else {
			const GLint blockIndex = glGetUniformBlockIndex(program, "TextureBlock");
			if (blockIndex != GL_INVALID_INDEX)
				glUniformBlockBinding(program, blockIndex, m_textureBlock.m_blockBindingPoint);
		}
	}

	if (m_colorsBlock.m_buffer == 0)
		_initColorsBuffer(program);
	else {
		const GLint blockIndex = glGetUniformBlockIndex(program, "ColorsBlock");
		if (blockIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(program, blockIndex, m_colorsBlock.m_blockBindingPoint);
	}

	if (_pCombiner->usesShadeColor() && config.generalEmulation.enableHWLighting != 0) {
		if (m_lightBlock.m_buffer == 0)
			_initLightBuffer(program);
		else {
			const GLint blockIndex = glGetUniformBlockIndex(program, "LightBlock");
			if (blockIndex != GL_INVALID_INDEX)
				glUniformBlockBinding(program, blockIndex, m_lightBlock.m_blockBindingPoint);
		}
	}
}

void UniformBlock::setColorData(ColorUniforms _index, u32 _dataSize, const void * _data)
{
	if (m_colorsBlock.m_buffer == 0)
		return;
	if (!_isDataChanged(m_colorsBlockData.data() + m_colorsBlock.m_offsets[_index], _data, _dataSize))
		return;

	if (m_currentBuffer != m_colorsBlock.m_buffer) {
		m_currentBuffer = m_colorsBlock.m_buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_colorsBlock.m_buffer);
	}
	glBufferSubData(GL_UNIFORM_BUFFER, m_colorsBlock.m_offsets[_index], _dataSize, _data);
}

void UniformBlock::updateTextureParameters()
{
	if (m_textureBlock.m_buffer == 0)
		return;

	GLbyte * pData = m_textureBlockData.data();
	f32 texScale[4] = { gSP.texture.scales, gSP.texture.scalet, 0, 0 };
	memcpy(pData + m_textureBlock.m_offsets[tuTexScale], texScale, m_textureBlock.m_offsets[tuTexMask] - m_textureBlock.m_offsets[tuTexScale]);

	f32 texOffset[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	f32 texMask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	if (gSP.textureTile[0] != NULL) {
		if (gSP.textureTile[0]->textureMode != TEXTUREMODE_BGIMAGE && gSP.textureTile[0]->textureMode != TEXTUREMODE_FRAMEBUFFER_BG) {
			texOffset[0] = gSP.textureTile[0]->fuls;
			texOffset[1] = gSP.textureTile[0]->fult;
			texMask[0] = gSP.textureTile[0]->masks > 0 ? (float)(1 << gSP.textureTile[0]->masks) : 0.0f;
			texMask[1] = gSP.textureTile[0]->maskt > 0 ? (float)(1 << gSP.textureTile[0]->maskt) : 0.0f;
		}
	}
	if (gSP.textureTile[1] != 0) {
		texOffset[4] = gSP.textureTile[1]->fuls;
		texOffset[5] = gSP.textureTile[1]->fult;
		texMask[4] = gSP.textureTile[1]->masks > 0 ? (float)(1 << gSP.textureTile[1]->masks) : 0.0f;
		texMask[5] = gSP.textureTile[1]->maskt > 0 ? (float)(1 << gSP.textureTile[1]->maskt) : 0.0f;
	}
	memcpy(pData + m_textureBlock.m_offsets[tuTexMask], texMask, m_textureBlock.m_offsets[tuTexOffset] - m_textureBlock.m_offsets[tuTexMask]);
	memcpy(pData + m_textureBlock.m_offsets[tuTexOffset], texOffset, m_textureBlock.m_offsets[tuCacheScale] - m_textureBlock.m_offsets[tuTexOffset]);

	f32 texCacheScale[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	f32 texCacheOffset[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	f32 texCacheShiftScale[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	GLint texCacheFrameBuffer[4] = { 0, 0, 0, 0 };
	TextureCache & cache = textureCache();
	if (cache.current[0]) {
		texCacheScale[0] = cache.current[0]->scaleS;
		texCacheScale[1] = cache.current[0]->scaleT;
		texCacheOffset[0] = cache.current[0]->offsetS;
		texCacheOffset[1] = cache.current[0]->offsetT;

		f32 shiftScaleS = 1.0f;
		f32 shiftScaleT = 1.0f;
		getTextureShiftScale(0, cache, shiftScaleS, shiftScaleT);
		texCacheShiftScale[0] = shiftScaleS;
		texCacheShiftScale[1] = shiftScaleT;
		texCacheFrameBuffer[0] = cache.current[0]->frameBufferTexture;
	}
	if (cache.current[1]) {
		texCacheScale[4] = cache.current[1]->scaleS;
		texCacheScale[5] = cache.current[1]->scaleT;
		texCacheOffset[4] = cache.current[1]->offsetS;
		texCacheOffset[5] = cache.current[1]->offsetT;

		f32 shiftScaleS = 1.0f;
		f32 shiftScaleT = 1.0f;
		getTextureShiftScale(1, cache, shiftScaleS, shiftScaleT);
		texCacheShiftScale[4] = shiftScaleS;
		texCacheShiftScale[5] = shiftScaleT;
		texCacheFrameBuffer[1] = cache.current[1]->frameBufferTexture;
	}
	memcpy(pData + m_textureBlock.m_offsets[tuCacheScale], texCacheScale, m_textureBlock.m_offsets[tuCacheOffset] - m_textureBlock.m_offsets[tuCacheScale]);
	memcpy(pData + m_textureBlock.m_offsets[tuCacheOffset], texCacheOffset, m_textureBlock.m_offsets[tuCacheShiftScale] - m_textureBlock.m_offsets[tuCacheOffset]);
	memcpy(pData + m_textureBlock.m_offsets[tuCacheShiftScale], texCacheShiftScale, m_textureBlock.m_offsets[tuCacheFrameBuffer] - m_textureBlock.m_offsets[tuCacheShiftScale]);
	memcpy(pData + m_textureBlock.m_offsets[tuCacheFrameBuffer], texCacheFrameBuffer, m_textureBlockData.size() - m_textureBlock.m_offsets[tuCacheFrameBuffer]);

	if (m_currentBuffer != m_textureBlock.m_buffer) {
		m_currentBuffer = m_textureBlock.m_buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_textureBlock.m_buffer);
	}
	glBufferSubData(GL_UNIFORM_BUFFER, m_textureBlock.m_offsets[tuTexScale], m_textureBlockData.size(), pData);
}

void UniformBlock::updateLightParameters()
{
	if (m_lightBlock.m_buffer == 0)
		return;

	GLbyte * pData = m_lightBlockData.data();
	const u32 arraySize = m_lightBlock.m_offsets[luLightColor] / 8;
	for (s32 i = 0; i <= gSP.numLights; ++i) {
		memcpy(pData + m_lightBlock.m_offsets[luLightDirection] + arraySize*i, &gSP.lights[i].x, arraySize);
		memcpy(pData + m_lightBlock.m_offsets[luLightColor] + arraySize*i, &gSP.lights[i].r, arraySize);
	}
	if (m_currentBuffer != m_lightBlock.m_buffer) {
		m_currentBuffer = m_lightBlock.m_buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_lightBlock.m_buffer);
	}
	glBufferSubData(GL_UNIFORM_BUFFER, m_lightBlock.m_offsets[luLightDirection], m_lightBlockData.size(), pData);
}

#else


#define LocateUniform2(A) \
	location.A.loc = glGetUniformLocation(program, #A);

void UniformBlock::bindWithShaderCombiner(ShaderCombiner * _pCombiner)
{
	const u64 mux = _pCombiner->getMux();
	const GLuint program = _pCombiner->m_program;
	m_uniforms.emplace(mux, program);
	UniformBlockLocation & location = m_uniforms.at(mux);

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

void UniformBlock::_updateColorUniforms(UniformBlockLocation & _location, bool _bForce)
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


void UniformBlock::_updateTextureUniforms(UniformBlockLocation & _location, bool _bUsesT0, bool _bUsesT1, bool _bForce)
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
			nFB[t] = cache.current[t]->frameBufferTexture;
		}
	}

	_location.uCacheFrameBuffer.set(nFB[0], nFB[1], _bForce);
	_location.uTexScale.set(gSP.texture.scales, gSP.texture.scalet, _bForce);
}

void UniformBlock::_updateLightUniforms(UniformBlockLocation & _location, bool _bForce)
{
	for (s32 i = 0; i <= gSP.numLights; ++i) {
		_location.uLightDirection[i].set(&gSP.lights[i].x, _bForce);
		_location.uLightColor[i].set(&gSP.lights[i].r, _bForce);
	}
}

void UniformBlock::updateUniforms(ShaderCombiner * _pCombiner)
{
	UniformBlockLocation & location = m_uniforms.at(_pCombiner->getMux());

	_updateColorUniforms(location, false);

	OGLRender::RENDER_STATE rs = video().getRender().getRenderState();
	if ((rs == OGLRender::rsTriangle || rs == OGLRender::rsLine) && _pCombiner->usesTexture())
		_updateTextureUniforms(location, _pCombiner->usesTile(0), _pCombiner->usesTile(1), false);

	if (config.generalEmulation.enableHWLighting != 0 && GBI.isHWLSupported() && _pCombiner->usesShadeColor())
		_updateLightUniforms(location, false);
}

#endif // GL_UNIFORMBLOCK_SUPPORT
