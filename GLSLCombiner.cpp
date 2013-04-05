#ifndef __LINUX__
# include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#else
# include "winlnxdefs.h"
# include <stdlib.h> // malloc()
#endif
#include "OpenGL.h"
#include "Combiner.h"
#include "GLSLCombiner.h"

void display_warning(const char *text, ...)
{
	static int first_message = 100;
	if (first_message)
	{
		char buf[1000];

		va_list ap;

		va_start(ap, text);
		vsprintf(buf, text, ap);
		va_end(ap);
		first_message--;
	}
}

const char *ColorInput[] = {
	"combined_color.rgb",
	"readtex0.rgb",
	"readtex1.rgb",
	"prim_color.rgb",
	"gl_Color.rgb",
	"env_color.rgb",
	"center_color.rgb",
	"scale_color.rgb",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"prim_color.a",
	"gl_Color.a",
	"env_color.a",
	"vec3(0.5)", // TODO: emulate lod_fraction
	"vec3(prim_lod)",
	"vec3(1.0)", // TODO: emulate noise
	"vec3(k4)",
	"vec3(k5)",
	"vec3(1.0)",
	"vec3(0.0)"
};

const char *AlphaInput[] = {
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"prim_color.a",
	"gl_Color.a",
	"env_color.a",
	"center_color.a",
	"scale_color.a",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"prim_color.a",
	"gl_Color.a",
	"env_color.a",
	"0.5", // TODO: emulate lod_fraction
	"prim_lod",
	"1.0", // TODO: emulate noise
	"k4",
	"k5",
	"1.0",
	"0.0"
};

static GLhandleARB vertex_shader_object;

static const char* fragment_shader_header =
/*
"uniform sampler2D texture0;       \n"
"uniform sampler2D texture1;       \n"
"uniform sampler2D ditherTex;      \n"
"uniform vec4 constant_color;      \n"
"uniform vec4 ccolor0;             \n"
"uniform vec4 ccolor1;             \n"
"uniform vec4 chroma_color;        \n"
"uniform float lambda;             \n"
"varying vec4 fogValue;            \n"
"                                  \n"
"void test_chroma(vec4 ctexture1); \n"
"                                  \n"
"                                  \n"
"void main()                       \n"
"{                                 \n"
;
*/
"uniform sampler2D texture0;		\n"
"uniform sampler2D texture1;		\n"
"uniform vec4 prim_color;			\n"
"uniform vec4 env_color;			\n"
"uniform vec4 center_color;			\n"
"uniform vec4 scale_color;			\n"
"uniform float k4;					\n"
"uniform float k5;					\n"
"uniform float prim_lod;			\n"
"									\n"
"void main()						\n"
"{									\n"
"  vec4 combined_color;				\n"
"  float alpha1, alpha2;			\n"
"  vec3 color1, color2;				\n"
;
/*
// using gl_FragCoord is terribly slow on ATI and varying variables don't work for some unknown
// reason, so we use the unused components of the texture2 coordinates
static const char* fragment_shader_dither =
"  float dithx = (gl_TexCoord[2].b + 1.0)*0.5*1000.0; \n"
"  float dithy = (gl_TexCoord[2].a + 1.0)*0.5*1000.0; \n"
"  if(texture2D(ditherTex, vec2((dithx-32.0*floor(dithx/32.0))/32.0, \n"
"                               (dithy-32.0*floor(dithy/32.0))/32.0)).a > 0.5) discard; \n"
;

static const char* fragment_shader_depth =
"  gl_FragDepth = dot(texture2D(texture0, vec2(gl_TexCoord[0])), vec4(32*64*32/65536.0, 64*32/65536.0, 32/65536.0, 0))*0.5 + 0.5; \n"
;

static const char* fragment_shader_bw =
"  vec4 readtex0 = texture2D(texture0, vec2(gl_TexCoord[0])); \n"
"  gl_FragColor = vec4(vec3(readtex0.b),                      \n"
"                 readtex0.r + readtex0.g * 8.0 / 256.0);     \n"
;

static const char* fragment_shader_readtex0bw =
"  vec4 readtex0 = texture2D(texture0, vec2(gl_TexCoord[0])); \n"
"  readtex0 = vec4(vec3(readtex0.b),                          \n"
"                  readtex0.r + readtex0.g * 8.0 / 256.0);    \n"
;
static const char* fragment_shader_readtex0bw_2 =
"  vec4 readtex0 = vec4(dot(texture2D(texture0, vec2(gl_TexCoord[0])), vec4(1.0/3, 1.0/3, 1.0/3, 0)));                        \n"
;

static const char* fragment_shader_readtex1bw =
"  vec4 readtex1 = texture2D(texture1, vec2(gl_TexCoord[1])); \n"
"  readtex1 = vec4(vec3(readtex1.b),                          \n"
"                  readtex1.r + readtex1.g * 8.0 / 256.0);    \n"
;
static const char* fragment_shader_readtex1bw_2 =
"  vec4 readtex1 = vec4(dot(texture2D(texture1, vec2(gl_TexCoord[1])), vec4(1.0/3, 1.0/3, 1.0/3, 0)));                        \n"
;
*/

static const char* fragment_shader_default =
//"  gl_FragColor = texture2D(texture0, gl_TexCoord[0].st); \n"
//"  gl_FragColor = gl_Color; \n"
"  vec4 color = texture2D(texture0, gl_TexCoord[0].st); \n"
"  gl_FragColor = gl_Color*color; \n"
;

static const char* fragment_shader_readtex0color =
"  vec4 readtex0 = texture2D(texture0, gl_TexCoord[0].st); \n"
;

static const char* fragment_shader_readtex1color =
"  vec4 readtex1 = texture2D(texture1, gl_TexCoord[1].st); \n"
;

static const char* fragment_shader_end =
"}                               \n"
;

static const char* vertex_shader =
#if 0
//"varying vec4 fogValue;                                         \n"
"                                                               \n"
"void main()                                                    \n"
"{                                                              \n"
"  gl_Position = ftransform();                                  \n"
"  gl_FrontColor = gl_Color;                                    \n"
"  gl_TexCoord[0] = gl_MultiTexCoord0;                          \n"
"  gl_TexCoord[1] = gl_MultiTexCoord1;                          \n"
"  float f = (gl_Fog.end - gl_SecondaryColor.r) * gl_Fog.scale; \n" // fog value passed through secondary color (workaround ATI bug)
"  f = clamp(f, 0.0, 1.0);                                      \n"
//"  gl_TexCoord[0].b = f;                                        \n" // various data passed through
//"  gl_TexCoord[2].b = gl_Vertex.x;                              \n" // texture coordinates
//"  gl_TexCoord[2].a = gl_Vertex.y;                              \n" // again it is the only way
"}                                                              \n" // i've found to get it working fast with ATI drivers
;
#else
"void main()                                                    \n"
"{                                                              \n"
"  gl_Position = ftransform();                                  \n"
"  gl_FrontColor = gl_Color;                                    \n"
"  gl_TexCoord[0] = gl_MultiTexCoord0;                          \n"
"  gl_TexCoord[1] = gl_MultiTexCoord1;                          \n"
"  gl_FogFragCoord = (gl_Fog.end - gl_FogCoord) * gl_Fog.scale;	\n"
"}                                                              \n"
;
#endif

static char shader_log[2048];

void InitGLSLCombiner()
{
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);

	vertex_shader_object = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSourceARB(vertex_shader_object, 1, &vertex_shader, NULL);
	glCompileShaderARB(vertex_shader_object);
}

void CompileCombiner(const CombinerStage & _stage, const char** _Input, char * _fragment_shader) {
	char buf[128];
	bool bBracketOpen = false;
	for (int i = 0; i < _stage.numOps; ++i) {
		switch(_stage.op[i].op) {
			case LOAD:
				sprintf(buf, "(%s ", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				bBracketOpen = true;
				break;
			case SUB:
				if (bBracketOpen) {
					sprintf(buf, "- %s)", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				break;
			case ADD:
				if (bBracketOpen) {
					sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				break;
			case MUL:
				if (bBracketOpen) {
					sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "*%s", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				break;
			case INTER:
				sprintf(buf, "mix(%s, %s, %s)", ColorInput[_stage.op[0].param2], _Input[_stage.op[0].param1], _Input[_stage.op[0].param3]);
				strcat(_fragment_shader, buf);
				break;

				//			default:
				//				assert(false);
		}
	}
	if (bBracketOpen)
		strcat(_fragment_shader, ")");
	strcat(_fragment_shader, "; \n");
}

GLSLCombiner * CompileGLSLCominer(Combiner *_color, Combiner *_alpha) {
	GLSLCombiner *glslCombiner = (GLSLCombiner*)malloc(sizeof(GLSLCombiner));
	glslCombiner->vertex_shader_object = vertex_shader_object;

	char *fragment_shader = (char*)malloc(4096);

	strcpy(fragment_shader, fragment_shader_header);

	strcat(fragment_shader, fragment_shader_readtex0color);
	strcat(fragment_shader, fragment_shader_readtex1color);
#if 1
	strcat(fragment_shader, "  alpha1 = ");
	CompileCombiner(_alpha->stage[0], AlphaInput, fragment_shader);
	strcat(fragment_shader, "  color1 = ");
	CompileCombiner(_color->stage[0], ColorInput, fragment_shader);
	strcat(fragment_shader, "  combined_color = vec4(color1, alpha1); \n");

	if (_alpha->numStages == 2) {
		strcat(fragment_shader, "  alpha2 = ");
		CompileCombiner(_alpha->stage[1], AlphaInput, fragment_shader);
	} else
		strcat(fragment_shader, "  alpha2 = alpha1; \n");

	if (_color->numStages == 2) {
		strcat(fragment_shader, "  color2 = ");
		CompileCombiner(_color->stage[1], ColorInput, fragment_shader);
	} else
		strcat(fragment_shader, "  color2 = color1; \n");

	strcat(fragment_shader, "  gl_FragColor = vec4(color2, alpha2); \n");
	if (gSP.geometryMode & G_FOG)
		strcat(fragment_shader, "  gl_FragColor = vec4(mix(gl_Fog.color.rgb, gl_FragColor.rgb, gl_FogFragCoord), gl_FragColor.a); \n");
#else
//	strcat(fragment_shader, fragment_shader_default);
	strcat(fragment_shader, "gl_FragColor = vec4(env_color); \n");
#endif

	strcat(fragment_shader, fragment_shader_end);

	glslCombiner->fragment_shader_object = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(glslCombiner->fragment_shader_object, 1, (const GLcharARB**)&fragment_shader, NULL);
	free(fragment_shader);

	glCompileShaderARB(glslCombiner->fragment_shader_object);

	glslCombiner->program_object = glCreateProgramObjectARB();
	glAttachObjectARB(glslCombiner->program_object, glslCombiner->fragment_shader_object);
	glAttachObjectARB(glslCombiner->program_object, glslCombiner->vertex_shader_object);
	glLinkProgramARB(glslCombiner->program_object);
	return glslCombiner;
}

void SetGLSLCombiner(GLSLCombiner *glslCombiner) {
	combiner.usesT0 = FALSE;
	combiner.usesT1 = FALSE;
	combiner.usesNoise = FALSE;

	combiner.vertex.color = COMBINED;
	combiner.vertex.secondaryColor = COMBINED;
	combiner.vertex.alpha = COMBINED;

	glUseProgramObjectARB(glslCombiner->program_object);

	int texture0_location = glGetUniformLocationARB(glslCombiner->program_object, "texture0");
	if (texture0_location != -1) {
		glUniform1iARB(texture0_location, 0);
		combiner.usesT0 = TRUE;
	}

	int texture1_location = glGetUniformLocationARB(glslCombiner->program_object, "texture1");
	if (texture1_location != -1) {
		glUniform1iARB(texture1_location, 1);
		combiner.usesT1 = TRUE;
	}

	int log_length;
	glGetObjectParameterivARB(glslCombiner->program_object, GL_OBJECT_LINK_STATUS_ARB , &log_length);
	if(!log_length)
	{
		glGetInfoLogARB(glslCombiner->fragment_shader_object, 
			2048, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
		glGetInfoLogARB(vertex_shader_object, 2048, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
		glGetInfoLogARB(glslCombiner->program_object, 
			2048, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
	}
}

void UpdateGLSLCombinerColors(GLSLCombiner * _glslCombiner) {
//	if (int chroma_color_location = glGetUniformLocationARB(_glslCombiner->program_object, "chroma_color") != -1)
//		glUniform4fARB(chroma_color_location, chroma_color[0], chroma_color[1],	chroma_color[2], chroma_color[3]);

	int prim_color_location = glGetUniformLocationARB(_glslCombiner->program_object, "prim_color");
	glUniform4fARB(prim_color_location, gDP.primColor.r, gDP.primColor.g, gDP.primColor.b, gDP.primColor.a);

	int env_color_location = glGetUniformLocationARB(_glslCombiner->program_object, "env_color");
	glUniform4fARB(env_color_location, gDP.envColor.r, gDP.envColor.g, gDP.envColor.b, gDP.envColor.a);

	int prim_lod_location = glGetUniformLocationARB(_glslCombiner->program_object, "prim_lod");
	glUniform1fARB(prim_lod_location, gDP.primColor.l);
	
	/*
	if(dither_enabled)
	{
	ditherTex_location = glGetUniformLocationARB(program_object, "ditherTex");
	glUniform1iARB(ditherTex_location, 2);
	}

	set_lambda();
	number_of_programs++;
	*/  
}

