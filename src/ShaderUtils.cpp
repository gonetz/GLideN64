#include <assert.h>
#include <stdio.h>
#include <sstream>
#include "ShaderUtils.h"
#include "Config.h"
#include "Log.h"

#ifdef VC
#include "RaspberryPi/ShaderUtils_VC.h"
#endif

static const GLsizei nShaderLogSize = 1024;
bool checkShaderCompileStatus(GLuint obj)
{
	GLint status;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
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
	if (status == GL_FALSE) {
		GLsizei nLogSize = nShaderLogSize;
		GLchar shader_log[nShaderLogSize];
		glGetProgramInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		LOG(LOG_ERROR, "shader_link error: %s\n", shader_log);
		return false;
	}
	return true;
}

void logErrorShader(GLenum _shaderType, const std::string & _strShader)
{
	LOG(LOG_ERROR, "Error in %s shader", _shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment");

	const int max = 800;
	int pos = 0;

	while(pos < _strShader.length() ) {

		if (_strShader.length() - pos < max) {
			LOG(LOG_ERROR, "%s", _strShader.substr(pos).data());
		} else {
			LOG(LOG_ERROR, "%s", _strShader.substr(pos, max).data());
		}
		pos += max;
	}
}

const std::string &  getHeader()
{
	static std::string strHeader;
	if (!strHeader.empty())
		return strHeader;

	OGLVideo & ogl = video();
	OGLRender & render = ogl.getRender();
	std::stringstream headerStream;
#if defined(GLES2)
	headerStream << "#version 100 " << std::endl;
	if (config.generalEmulation.enableLOD > 0) {
		headerStream << "#extension GL_EXT_shader_texture_lod : enable " << std::endl \
			<< "#extension GL_OES_standard_derivatives : enable " << std::endl;
	}
#elif defined(GLESX)
	headerStream << "#version " << render.majorVersion << render.minorVersion << "0 es " << std::endl;
#else
	headerStream << "#version " << render.majorVersion << render.minorVersion << "0 core " << std::endl;
	if (video().getRender().isImageTexturesSupported() && ((render.majorVersion * 10) + render.minorVersion) < 42)
		headerStream << "#extension GL_ARB_shader_image_load_store : enable " << std::endl \
			<< "#extension GL_ARB_shading_language_420pack : enable " << std::endl;
#endif
	strHeader = headerStream.str();
	return strHeader;
}

const char* addGLSLVersion(const char* shaderString)
{
	static std::string strResult;
	strResult = getHeader();
	strResult.append(shaderString);
	return strResult.c_str();
}

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment)
{
	GLuint vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
	const char* vertex_shader_string = addGLSLVersion(_strVertex);
	glShaderSource(vertex_shader_object, 1, &vertex_shader_string, nullptr);
	glCompileShader(vertex_shader_object);
	assert(checkShaderCompileStatus(vertex_shader_object));

	if (!checkShaderCompileStatus(vertex_shader_object))
		logErrorShader(GL_VERTEX_SHADER, _strVertex);

	GLuint fragment_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragment_shader_string = addGLSLVersion(_strFragment);
	glShaderSource(fragment_shader_object, 1, &fragment_shader_string, nullptr);
	glCompileShader(fragment_shader_object);
	assert(checkShaderCompileStatus(fragment_shader_object));

	if (!checkShaderCompileStatus(fragment_shader_object))
		logErrorShader(GL_VERTEX_SHADER, _strFragment);

	GLuint program = glCreateProgram();
	glBindAttribLocation(program, SC_POSITION, "aPosition");
	glBindAttribLocation(program, SC_TEXCOORD0, "aTexCoord0");
	glBindAttribLocation(program, SC_TEXCOORD1, "aTexCoord1");
	glAttachShader(program, vertex_shader_object);
	glAttachShader(program, fragment_shader_object);
	glLinkProgram(program);
	glDeleteShader(vertex_shader_object);
	glDeleteShader(fragment_shader_object);
	assert(checkProgramLinkStatus(program));
	return program;
}

#ifndef VC
static
const char* fragment_shader_blender1 =
"  if (uForceBlendCycle1 != 0) {						\n"
"    muxPM[0] = clampedColor;							\n"
"    muxA[0] = clampedColor.a;							\n"
"    muxB[0] = 1.0 - muxA[uBlendMux1[1]];				\n"
"    lowp vec4 blend1 = (muxPM[uBlendMux1[0]] * muxA[uBlendMux1[1]]) + (muxPM[uBlendMux1[2]] * muxB[uBlendMux1[3]]);	\n"
"    clampedColor.rgb = clamp(blend1.rgb, 0.0, 1.0);	\n"
"  }													\n"
;

static
const char* fragment_shader_blender2 =
"  if (uForceBlendCycle2 != 0) {						\n"
"    muxPM[0] = clampedColor;							\n"
"    muxA[0] = clampedColor.a;							\n"
"    muxB[0] = 1.0 - muxA[uBlendMux2[1]];				\n"
"    lowp vec4 blend2 = muxPM[uBlendMux2[0]] * muxA[uBlendMux2[1]] + muxPM[uBlendMux2[2]] * muxB[uBlendMux2[3]];	\n"
"    clampedColor.rgb = clamp(blend2.rgb, 0.0, 1.0);	\n"
"  }													\n"
;
#endif

/*
// N64 color wrap and clamp on floats
// See https://github.com/gonetz/GLideN64/issues/661 for reference
if (c < -1.0) return c + 2.0;

if (c < -0.5) return 1;

if (c < 0.0) return 0;

if (c > 2.0) return c - 2.0;

if (c > 1.5) return 0;

if (c > 1.0) return 1;

return c;
*/
const char* fragment_shader_clamp =
"  lowp vec4 wrappedColor = cmbRes + 2.0 * step(cmbRes, vec4(-0.51)) - 2.0*step(vec4(1.51), cmbRes); \n"
"  lowp vec4 clampedColor = clamp(wrappedColor, 0.0, 1.0); \n"
;

/*
N64 sign-extension for C component of combiner
if (c > 1.0)
return c - 2.0;

return c;
*/
const char* fragment_shader_sign_extend_color_c =
"  color1 = color1 - 2.0*(vec3(1.0) - step(color1, vec3(1.0)));	\n"
;
const char* fragment_shader_sign_extend_alpha_c =
"  alpha1 = alpha1 - 2.0*(1.0 - step(alpha1, 1.0));					\n"
;

/*
N64 sign-extension for ABD components of combiner
if (c > 1.5)
return c - 2.0;

if (c < -0.5)
return c + 2.0;

return c;
*/
const char* fragment_shader_sign_extend_color_abd =
"  color1 = color1 + 2.0*step(color1, vec3(-0.51)) - 2.0*step(vec3(1.51), color1); \n"
;
const char* fragment_shader_sign_extend_alpha_abd =
"  alpha1 = alpha1 + 2.0*step(alpha1, -0.51) - 2.0*step(1.51, alpha1); \n"
;

static
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

static
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
int correctFirstStageParam(int _param)
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
void _correctFirstStageParams(CombinerStage & _stage)
{
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctFirstStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctFirstStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctFirstStageParam(_stage.op[i].param3);
	}
}

inline
int correctSecondStageParam(int _param)
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
bool needClampColor() {
	return gDP.otherMode.cycleType <= G_CYC_2CYCLE;
}

static
bool combinedColorC(const gDPCombine & _combine) {
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE)
		return false;
	return _combine.mRGB1 == G_CCMUX_COMBINED;
}

static
bool combinedAlphaC(const gDPCombine & _combine) {
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE)
		return false;
	return _combine.mA1 == G_ACMUX_COMBINED;
}

static
bool combinedColorABD(const gDPCombine & _combine) {
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE)
		return false;
	if (_combine.aRGB1 == G_CCMUX_COMBINED)
		return true;
	if (_combine.saRGB1 == G_CCMUX_COMBINED || _combine.sbRGB1 == G_CCMUX_COMBINED)
		return _combine.mRGB1 != G_CCMUX_0;
	return false;
}

static
bool combinedAlphaABD(const gDPCombine & _combine) {
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE)
		return false;
	if (_combine.aA1 == G_ACMUX_COMBINED)
		return true;
	if (_combine.saA1 == G_ACMUX_COMBINED || _combine.sbA1 == G_ACMUX_COMBINED)
		return _combine.mA1 != G_ACMUX_0;
	return false;
}

static
void _correctSecondStageParams(CombinerStage & _stage) {
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctSecondStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctSecondStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctSecondStageParam(_stage.op[i].param3);
	}
}

static
int _compileCombiner(const CombinerStage & _stage, const char** _Input, std::string & _strShader) {
	char buf[128];
	bool bBracketOpen = false;
	int nRes = 0;
	for (int i = 0; i < _stage.numOps; ++i) {
		switch (_stage.op[i].op) {
		case LOAD:
			sprintf(buf, "(%s ", _Input[_stage.op[i].param1]);
			_strShader += buf;
			bBracketOpen = true;
			nRes |= 1 << _stage.op[i].param1;
			break;
		case SUB:
			if (bBracketOpen) {
				sprintf(buf, "- %s)", _Input[_stage.op[i].param1]);
				bBracketOpen = false;
			}
			else
				sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
			_strShader += buf;
			nRes |= 1 << _stage.op[i].param1;
			break;
		case ADD:
			if (bBracketOpen) {
				sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
				bBracketOpen = false;
			}
			else
				sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
			_strShader += buf;
			nRes |= 1 << _stage.op[i].param1;
			break;
		case MUL:
			if (bBracketOpen) {
				sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
				bBracketOpen = false;
			}
			else
				sprintf(buf, "*%s", _Input[_stage.op[i].param1]);
			_strShader += buf;
			nRes |= 1 << _stage.op[i].param1;
			break;
		case INTER:
			sprintf(buf, "mix(%s, %s, %s)", _Input[_stage.op[0].param2], _Input[_stage.op[0].param1], _Input[_stage.op[0].param3]);
			_strShader += buf;
			nRes |= 1 << _stage.op[i].param1;
			nRes |= 1 << _stage.op[i].param2;
			nRes |= 1 << _stage.op[i].param3;
			break;

			//			default:
			//				assert(false);
		}
	}
	if (bBracketOpen)
		_strShader.append(")");
	_strShader.append("; \n");
	return nRes;
}

int compileCombiner(const gDPCombine & _combine, Combiner & _color, Combiner & _alpha, std::string & _strShader)
{
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE) {
		_correctFirstStageParams(_alpha.stage[0]);
		_correctFirstStageParams(_color.stage[0]);
	}
	_strShader.append("  alpha1 = ");
	int nInputs = _compileCombiner(_alpha.stage[0], AlphaInput, _strShader);
	// Simulate N64 color sign-extend.
	if (combinedAlphaC(_combine))
		_strShader.append(fragment_shader_sign_extend_alpha_c);
	else if (combinedAlphaABD(_combine))
		_strShader.append(fragment_shader_sign_extend_alpha_abd);

	_strShader.append(
		"  if (uEnableAlphaTest != 0) {							\n"
		"    lowp float alphaTestValue = (uAlphaCompareMode == 3) ? snoise() : uAlphaTestValue;	\n"
	    "    lowp float alphaValue;								\n"
		"    if ((uAlphaCvgSel != 0) && (uCvgXAlpha == 0)) {	\n"
	    "      alphaValue = 0.125;								\n"
	    "    } else {											\n"
	    "      alphaValue = clamp(alpha1, 0.0, 1.0);			\n"
	    "    }													\n"
		"    if (alphaValue < alphaTestValue) discard;			\n"
		"  }													\n"
		);

	_strShader.append("  color1 = ");
	nInputs |= _compileCombiner(_color.stage[0], ColorInput, _strShader);
	// Simulate N64 color sign-extend.
	if (combinedColorC(_combine))
		_strShader.append(fragment_shader_sign_extend_color_c);
	else if (combinedColorABD(_combine))
		_strShader.append(fragment_shader_sign_extend_color_abd);

	_strShader.append("  combined_color = vec4(color1, alpha1); \n");
	if (_alpha.numStages == 2) {
		_strShader.append("  alpha2 = ");
		_correctSecondStageParams(_alpha.stage[1]);
		nInputs |= _compileCombiner(_alpha.stage[1], AlphaInput, _strShader);
	}
	else
		_strShader.append("  alpha2 = alpha1; \n");

	_strShader.append("  if (uCvgXAlpha != 0 && alpha2 < 0.125) discard; \n");

	if (_color.numStages == 2) {
		_strShader.append("  color2 = ");
		_correctSecondStageParams(_color.stage[1]);
		nInputs |= _compileCombiner(_color.stage[1], ColorInput, _strShader);
	}
	else
		_strShader.append("  color2 = color1; \n");

	_strShader.append("  lowp vec4 cmbRes = vec4(color2, alpha2);\n");
	// Simulate N64 color clamp.
	if (needClampColor())
		_strShader.append(fragment_shader_clamp);
	else
		_strShader.append("  lowp vec4 clampedColor = clamp(cmbRes, 0.0, 1.0);\n");

#ifndef GLES2
	if (config.generalEmulation.enableNoise != 0) {
		_strShader.append(
			"  if (uColorDitherMode == 2) colorNoiseDither(snoise(), clampedColor.rgb);	\n"
			"  if (uAlphaDitherMode == 2) alphaNoiseDither(snoise(), clampedColor.a);	\n"
			);
	}
#endif

	if (config.generalEmulation.enableLegacyBlending == 0) {
		if (gDP.otherMode.cycleType <= G_CYC_2CYCLE)
			_strShader.append(fragment_shader_blender1);
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE)
			_strShader.append(fragment_shader_blender2);

		_strShader.append(
			"  fragColor = clampedColor;	\n"
			);
	} else {
		_strShader.append(
			"  fragColor = clampedColor;	\n"
			"  if (uFogUsage == 1) \n"
			"    fragColor.rgb = mix(fragColor.rgb, uFogColor.rgb, vShadeColor.a); \n"
			);
	}

	return nInputs;
}
