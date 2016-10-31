#include <assert.h>
#include <stdio.h>
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

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment)
{
	GLuint vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_object, 1, &_strVertex, nullptr);
	glCompileShader(vertex_shader_object);
	assert(checkShaderCompileStatus(vertex_shader_object));

	if (!checkShaderCompileStatus(vertex_shader_object))
		logErrorShader(GL_VERTEX_SHADER, _strVertex);

	GLuint fragment_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_object, 1, &_strFragment, nullptr);
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

int compileCombiner(Combiner & _color, Combiner & _alpha, std::string & _strShader)
{
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE) {
		_correctFirstStageParams(_alpha.stage[0]);
		_correctFirstStageParams(_color.stage[0]);
	}
	_strShader.append("  alpha1 = ");
	int nInputs = _compileCombiner(_alpha.stage[0], AlphaInput, _strShader);

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

	// Simulate N64 color clamp.
	_strShader.append("  lowp vec4 cmbRes = vec4(color2, alpha2);\n");
	_strShader.append("  lowp vec4 clampedColor = cmbRes + (-cmbRes)*step(cmbRes, vec4(0.0)) + step(cmbRes, vec4(-0.51)); \n");

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
