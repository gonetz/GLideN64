#include <assert.h>
#include <stdio.h>
#include "ShaderUtils.h"
#include "Log.h"

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
const char* fragment_shader_blender =
// Mace
"  if (uSpecialBlendMode == 1)													\n"
"		color1 = color1 * alpha1 + uBlendColor.rgb * (1.0 - alpha1);			\n"
// Bomberman2
"  else if (uSpecialBlendMode == 2)												\n"
"		color1 = uBlendColor.rgb * uFogColor.a + color1 * (1.0 - uFogColor.a);	\n"
// Conker BFD
"  else if (uSpecialBlendMode == 3)												\n"
"		color1 = color1 * uFogColor.a + uFogColor.rgb * (1.0 - uFogColor.a);	\n"
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
void _correctSecondStageParams(CombinerStage & _stage) {
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctSecondStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctSecondStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctSecondStageParam(_stage.op[i].param3);
	}
}

static
int _compileCombiner(const CombinerStage & _stage, const char** _Input, char * _strCombiner) {
	char buf[128];
	bool bBracketOpen = false;
	int nRes = 0;
	for (int i = 0; i < _stage.numOps; ++i) {
		switch (_stage.op[i].op) {
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
			}
			else
				sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
			strcat(_strCombiner, buf);
			nRes |= 1 << _stage.op[i].param1;
			break;
		case ADD:
			if (bBracketOpen) {
				sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
				bBracketOpen = false;
			}
			else
				sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
			strcat(_strCombiner, buf);
			nRes |= 1 << _stage.op[i].param1;
			break;
		case MUL:
			if (bBracketOpen) {
				sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
				bBracketOpen = false;
			}
			else
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

int compileCombiner(Combiner & _color, Combiner & _alpha, char * _strShader)
{
	if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
		_correctFirstStageParams(_alpha.stage[0]);
		_correctFirstStageParams(_color.stage[0]);
	}
	strcpy(_strShader, "  alpha1 = ");
	int nInputs = _compileCombiner(_alpha.stage[0], AlphaInput, _strShader);
	strcat(_strShader, "  color1 = ");
	nInputs |= _compileCombiner(_color.stage[0], ColorInput, _strShader);
	strcat(_strShader, fragment_shader_blender);

	strcat(_strShader, "  combined_color = vec4(color1, alpha1); \n");
	if (_alpha.numStages == 2) {
		strcat(_strShader, "  alpha2 = ");
		_correctSecondStageParams(_alpha.stage[1]);
		nInputs |= _compileCombiner(_alpha.stage[1], AlphaInput, _strShader);
	}
	else
		strcat(_strShader, "  alpha2 = alpha1; \n");
	if (_color.numStages == 2) {
		strcat(_strShader, "  color2 = ");
		_correctSecondStageParams(_color.stage[1]);
		nInputs |= _compileCombiner(_color.stage[1], ColorInput, _strShader);
	}
	else
		strcat(_strShader, "  color2 = color1; \n");
	return nInputs;
}