#ifndef __LINUX__
# include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <assert.h>
#else
# include "winlnxdefs.h"
# include <stdlib.h> // malloc()
#endif
#include "OpenGL.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "Noise_shader.h"

static GLhandleARB g_vertex_shader_object;
static GLhandleARB g_lod_program;
static GLhandleARB g_lod_clear_program;
static GLhandleARB g_lod_program_test;
static GLuint g_lod_fbo = 0;
static GLuint g_lod_tex = 0;

static
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

const char *ColorInput_1cycle[] = {
	"combined_color.rgb",
	"readtex0.rgb",
	"readtex1.rgb",
	"prim_color.rgb",
	"vec_color.rgb",
	"env_color.rgb",
	"center_color.rgb",
	"scale_color.rgb",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"prim_color.a",
	"vec_color.a",
	"env_color.a",
	"lod_frac", // TODO: emulate lod_fraction
	"vec3(prim_lod)",
	"vec3(0.5 + 0.5*snoise(noiseCoord2D))",
	"vec3(k4)",
	"vec3(k5)",
	"vec3(1.0)",
	"vec3(0.0)"
};

const char *ColorInput_2cycle[] = {
	"combined_color.rgb",
	"readtex1.rgb",
	"readtex0.rgb",
	"prim_color.rgb",
	"vec_color.rgb",
	"env_color.rgb",
	"center_color.rgb",
	"scale_color.rgb",
	"combined_color.a",
	"readtex1.a",
	"readtex0.a",
	"prim_color.a",
	"vec_color.a",
	"env_color.a",
	"lod_frac", // TODO: emulate lod_fraction
	"vec3(prim_lod)",
	"vec3(0.5 + 0.5*snoise(noiseCoord2D))",
	"vec3(k4)",
	"vec3(k5)",
	"vec3(1.0)",
	"vec3(0.0)"
};

const char *AlphaInput_1cycle[] = {
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"prim_color.a",
	"vec_color.a",
	"env_color.a",
	"center_color.a",
	"scale_color.a",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"prim_color.a",
	"vec_color.a",
	"env_color.a",
	"lod_frac", // TODO: emulate lod_fraction
	"prim_lod",
	"1.0",
	"k4",
	"k5",
	"1.0",
	"0.0"
};

const char *AlphaInput_2cycle[] = {
	"combined_color.a",
	"readtex1.a",
	"readtex0.a",
	"prim_color.a",
	"vec_color.a",
	"env_color.a",
	"center_color.a",
	"scale_color.a",
	"combined_color.a",
	"readtex1.a",
	"readtex0.a",
	"prim_color.a",
	"vec_color.a",
	"env_color.a",
	"lod_frac", // TODO: emulate lod_fraction
	"prim_lod",
	"1.0",
	"k4",
	"k5",
	"1.0",
	"0.0"
};

static const char* fragment_shader_header_common_variables =
"uniform sampler2D texture0;	\n"
"uniform sampler2D texture1;	\n"
"uniform vec4 prim_color;		\n"
"uniform vec4 env_color;		\n"
"uniform vec4 center_color;		\n"
"uniform vec4 scale_color;		\n"
"uniform float k4;				\n"
"uniform float k5;				\n"
"uniform float prim_lod;		\n"
"uniform int dither_enabled;	\n"
"uniform int fog_enabled;		\n"
"varying vec4 secondary_color;	\n"
"varying vec2 noiseCoord2D;		\n"
"vec3 input_color;				\n"
;

static const char* fragment_shader_header_lod_variables =
"uniform int lod_enabled;		\n"
"uniform float lod_x_scale;		\n"
"uniform float lod_y_scale;		\n"
"uniform float min_lod;			\n"
"uniform int max_tile;			\n"
"uniform int texture_detail;	\n"
"uniform sampler2D lod_texture;	\n"
;

static const char* fragment_shader_header_common_functions =
"															\n"
"float calc_light() {										\n"
"  input_color = gl_Color.rgb;								\n"
"  if (int(secondary_color.r) == 0)							\n"
"     return 1.0;											\n"
"  float full_intensity = 0.0;								\n"
"  int nLights = int(secondary_color.r);					\n"
"  input_color = vec3(gl_LightSource[nLights].ambient);		\n"
"  vec3 lightDir, lightColor;								\n"
"  float intensity;											\n"
"  vec3 n = normalize(gl_Color.rgb);						\n"
"  for (int i = 0; i < nLights; i++)	{					\n"
"    lightDir = vec3(gl_LightSource[i].position);			\n"
"    intensity = max(dot(n,lightDir),0.0);					\n"
"    full_intensity += intensity;							\n"
"    lightColor = vec3(gl_LightSource[i].ambient)*intensity;\n"
"    input_color += lightColor;								\n"
"  };														\n"
"  return full_intensity;									\n"
"}															\n"
"															\n"
"float snoise(vec2 v);										\n"
#ifdef USE_TOONIFY
"void toonify(in float intensity);	\n"
#endif
;

static const char* fragment_shader_calc_lod =
"														\n"
"vec2 fetchTex(in ivec2 screenpos) { \n"
  // look up result from previous render pass in the texture
"  vec4 color = texelFetch(lod_texture, screenpos, 0);	\n"
"  return vec2(color);									\n"
"}														\n"
"														\n"
"float calc_lod() {										\n"
"  if (lod_enabled == 0)								\n"
"    return prim_lod;									\n"
  // convert fragment position to integers
"  int x0 = int(gl_FragCoord.x);						\n"
"  int y0 = int(gl_FragCoord.y);						\n"
"  float lod = 0.0;										\n"
"  vec2 lodtex0 = 255.0*fetchTex(ivec2(x0, y0));		\n"
"  lodtex0.x *= lod_x_scale;							\n"
"  lodtex0.y *= lod_y_scale;							\n"
"  vec2 lodtex1 = 255.0*fetchTex(ivec2(x0+1, y0));		\n"
"  if (length(lodtex1) > 0.0) {							\n"
"    lodtex1.x *= lod_x_scale;							\n"
"    lodtex1.y *= lod_y_scale;							\n"
"    lod = distance(lodtex0, lodtex1);					\n"
"  }													\n"
"  if (lod < 1.0) {										\n"
"    lodtex1 = 255.0*fetchTex(ivec2(x0, y0+1));			\n"
"    if (length(lodtex1) > 0.0) {						\n"
"      lodtex1.x *= lod_x_scale;						\n"
"      lodtex1.y *= lod_y_scale;						\n"
"      lod = distance(lodtex0, lodtex1);				\n"
"    }													\n"
"  }													\n"
"  if (lod < 1.0) {										\n"
"    lodtex1 = 255.0*fetchTex(ivec2(x0-1, y0));			\n"
"    if (length(lodtex1) > 0.0) {						\n"
"      lodtex1.x *= lod_x_scale;						\n"
"      lodtex1.y *= lod_y_scale;						\n"
"      lod = distance(lodtex0, lodtex1);				\n"
"    }													\n"
"  }													\n"
"  if (lod < 1.0) {										\n"
"    lodtex1 = 255.0*fetchTex(ivec2(x0, y0-1));			\n"
"    if (length(lodtex1) > 0.0) {						\n"
"      lodtex1.x *= lod_x_scale;						\n"
"      lodtex1.y *= lod_y_scale;						\n"
"      lod = distance(lodtex0, lodtex1);				\n"
"    }													\n"
"  }													\n"
"  if (texture_detail > 0 && lod < min_lod)				\n"
"    lod = min_lod;										\n"
"  if (lod < 1.0)										\n"
"    return 0.0;										\n"
"  float tile = min(float(max_tile), floor(log2(floor(lod)))); \n"
"  return fract(lod/pow(2.0, tile));					\n"
"}														\n"
;

static const char* fragment_shader_header_main =
"													\n"
"layout(pixel_center_integer) in vec4 gl_FragCoord; \n"
"void main()						\n"
"{									\n"
"  vec4 vec_color, combined_color;	\n"
"  float alpha1, alpha2;			\n"
"  vec3 color1, color2;				\n"
;

#ifdef USE_TOONIFY
static const char* fragment_shader_toonify =
"																	\n"
"void toonify(in float intensity) {									\n"
"   if (intensity > 0.5)											\n"
"	   return;														\n"
"	else if (intensity > 0.125)										\n"
"		gl_FragColor = vec4(vec3(gl_FragColor)*0.5, gl_FragColor.a);\n"
"	else															\n"
"		gl_FragColor = vec4(vec3(gl_FragColor)*0.2, gl_FragColor.a);\n"
"}																	\n"
;
#endif

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
"uniform float time;											\n"
"varying vec2 noiseCoord2D;										\n"
"varying vec4 secondary_color;                                  \n"
"void main()                                                    \n"
"{                                                              \n"
"  gl_Position = ftransform();                                  \n"
"  gl_FrontColor = gl_Color;                                    \n"
"  gl_TexCoord[0] = gl_MultiTexCoord0;                          \n"
"  gl_TexCoord[1] = gl_MultiTexCoord1;                          \n"
"  gl_FogFragCoord = (gl_Fog.end - gl_FogCoord) * gl_Fog.scale;	\n"
"  gl_FogFragCoord = clamp(gl_FogFragCoord, 0.0, 1.0);			\n"
"  secondary_color = gl_SecondaryColor;							\n"
"  noiseCoord2D = gl_Vertex.xy + vec2(0.0, time);				\n"
"}                                                              \n"
;

static const char* lod_vertex_shader =
"varying vec4 secondary_color;                                  \n"
"void main()                                                    \n"
"{                                                              \n"
"  gl_Position = ftransform();                                  \n"
"  gl_FrontColor = gl_Color;                                    \n"
"  secondary_color = gl_SecondaryColor;							\n"
"}                                                              \n"
;

static const char* lod_fragment_shader =
"varying vec4 secondary_color;      \n"
"layout (location = 0) out vec4 texCoordOut;	\n"
"void main()						\n"
"{texCoordOut = vec4(secondary_color.g, secondary_color.b, 0.0, 1.0);} \n"
;

static const char* lod_clear_fragment_shader =
"varying vec4 secondary_color;      \n"
"layout (location = 0) out vec4 texCoordOut;	\n"
"void main()						\n"
"{texCoordOut = vec4(0.0, 0.0, 0.0, 1.0);} \n"
;

static const char* lod_fragment_shader_test =
// texture with the previous render pass
"uniform sampler2D mytex; \n"
"layout(pixel_center_integer) in vec4 gl_FragCoord; \n"
"vec2 fetchTex(in ivec2 screenpos) {						\n"
  // look up result from previous render pass in the texture
"  vec4 color = texelFetch(mytex, screenpos, 0);			\n"
"  return vec2(color);										\n"
"}															\n"
"vec2 fetchCoord(in ivec2 screenpos) {						\n"
  // look up result from previous render pass in the texture
"  vec4 color = texelFetch(mytex, screenpos, 0);			\n"
"  return vec2(color.b, color.a);							\n"
"}															\n"
"void main()												\n"
#if 0
"{															\n"
"  int x0 = int(gl_FragCoord.x);							\n"
"  int y0 = int(gl_FragCoord.y);							\n"
"  float maxDist = 0.0;										\n"
"  vec2 lodtex0 = 255.0*fetchTex(ivec2(x0, y0));			\n"
"  vec2 lodtex1 = 255.0*fetchTex(ivec2(x0+1, y0));			\n"
"  if (length(lodtex1) > 0.0) {								\n"
"     float dist = distance(lodtex0, lodtex1);				\n"
"     maxDist = max(dist, maxDist);							\n"
"  }														\n"
"  if (maxDist < 1.0) {										\n"
"    lodtex1 = 255.0*fetchTex(ivec2(x0, y0+1));				\n"
"    if (length(lodtex1) > 0.0) {							\n"
"       float dist = distance(lodtex0, lodtex1);			\n"
"       maxDist = max(dist, maxDist);						\n"
"    }														\n"
"  }														\n"
"  if (maxDist < 1.0) {										\n"
"    lodtex1 = 255.0*fetchTex(ivec2(x0-1, y0));				\n"
"    if (length(lodtex1) > 0.0) {							\n"
"       float dist = distance(lodtex0, lodtex1);			\n"
"       maxDist = max(dist, maxDist);						\n"
"    }														\n"
"  }														\n"
"  if (maxDist < 1.0) {										\n"
"    lodtex1 = 255.0*fetchTex(ivec2(x0, y0-1));				\n"
"    if (length(lodtex1) > 0.0) {							\n"
"       float dist = distance(lodtex0, lodtex1);			\n"
"       maxDist = max(dist, maxDist);						\n"
"    }														\n"
"  }														\n"
"  float f_lod = 1.0;											\n"
"  if (maxDist >= 1.0)										\n"
"     f_lod = fract(maxDist);								\n"
//"  maxDist = lodtex0.x;\n"
//"  maxDist = abs(lodtex0.y - lodtex1.y);\n"
//"  if (maxDist < 1.0) maxDist = abs(lodtex0.y - lodtex1.y);\n"
//"  maxDist = distance(lodtex0,lodtex1);\n"
//"  float f_lod = fract(maxDist);							\n"
//"  float f_lod = maxDist/4.0;							\n"
"  gl_FragColor = vec4(f_lod, 0.0, 0.0, 1.0);				\n"
"}															\n"
#else
"{ \n"
  // convert fragment position to integers
"  ivec2 screenpos = ivec2(gl_FragCoord.xy); \n"
  // look up result from previous render pass in the texture
#if 0
"  vec4 color = 8.0*texelFetch(mytex, screenpos, 0); \n"
"  gl_FragColor = vec4(color.r, color.g, 0.0, 1.0); \n"
#else
"  vec4 color = 255.0*texelFetch(mytex, screenpos, 0); \n"
"  gl_FragColor = vec4(fract(color.r), fract(color.g), 0.0, 1.0); \n"
#endif
  // now use the value from the previous render pass ...
//"  gl_FragColor = color; \n"
"} \n"
#endif
;

void InitGLSLCombiner()
{
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);

	g_vertex_shader_object = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSourceARB(g_vertex_shader_object, 1, &vertex_shader, NULL);
	glCompileShaderARB(g_vertex_shader_object);

	GLhandleARB lod_vertex_shader_object = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSourceARB(lod_vertex_shader_object, 1, &lod_vertex_shader, NULL);
	glCompileShaderARB(lod_vertex_shader_object);
	GLhandleARB lod_fragment_shader_object = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(lod_fragment_shader_object, 1, &lod_fragment_shader, NULL);
	glCompileShaderARB(lod_fragment_shader_object);
	g_lod_program = glCreateProgramObjectARB();
	glAttachObjectARB(g_lod_program, lod_vertex_shader_object);
	glAttachObjectARB(g_lod_program, lod_fragment_shader_object);
	glLinkProgramARB(g_lod_program);

	GLhandleARB lod_clear_fragment_shader_object = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(lod_clear_fragment_shader_object, 1, &lod_clear_fragment_shader, NULL);
	glCompileShaderARB(lod_clear_fragment_shader_object);
	g_lod_clear_program = glCreateProgramObjectARB();
	glAttachObjectARB(g_lod_clear_program, lod_vertex_shader_object);
	glAttachObjectARB(g_lod_clear_program, lod_clear_fragment_shader_object);
	glLinkProgramARB(g_lod_clear_program);
#ifdef LOD_TEST
	GLhandleARB lod_vertex_shader_object2 = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glShaderSourceARB(lod_vertex_shader_object2, 1, &lod_vertex_shader, NULL);
	glCompileShaderARB(lod_vertex_shader_object2);
	GLhandleARB lod_fragment_shader_object2 = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(lod_fragment_shader_object2, 1, &lod_fragment_shader_test, NULL);
	glCompileShaderARB(lod_fragment_shader_object2);
	g_lod_program_test = glCreateProgramObjectARB();
	glAttachObjectARB(g_lod_program_test, lod_vertex_shader_object2);
	glAttachObjectARB(g_lod_program_test, lod_fragment_shader_object2);
	glLinkProgramARB(g_lod_program_test);

#ifdef _DEBUG
	int log_length;
	glGetObjectParameterivARB(g_lod_program_test, GL_OBJECT_LINK_STATUS_ARB , &log_length);
	if(!log_length)
	{
		const int nLogSize = 1024;
		char shader_log[nLogSize];
		glGetInfoLogARB(lod_fragment_shader_object2, 
			nLogSize, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
		glGetInfoLogARB(lod_vertex_shader_object2, nLogSize, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
		glGetInfoLogARB(g_lod_program_test, 
			nLogSize, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
	}
#endif
#endif // LOD_TEST

#ifdef FBO
	// generate a framebuffer 
	glGenFramebuffers(1, &g_lod_fbo);
	// bind it as the target for rendering commands
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_lod_fbo);

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &g_lod_tex);
	glBindTexture(GL_TEXTURE_2D, g_lod_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RG16F,
						OGL.width <= 1024 ? 1024 : 2048,
						OGL.height <= 1024 ? 1024 : 2048,
						0, GL_RG, GL_FLOAT,
						NULL); 
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, g_lod_tex, 0);

	// check if everything is OK
	GLenum e = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	switch (e) {
		case GL_FRAMEBUFFER_UNDEFINED:
			printf("FBO Undefined\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
			printf("FBO Incomplete Attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
			printf("FBO Missing Attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
			printf("FBO Incomplete Draw Buffer\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED :
			printf("FBO Unsupported\n");
			break;
		case GL_FRAMEBUFFER_COMPLETE:
			printf("FBO OK\n");
			break;
		default:
			printf("FBO Problem?\n");
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif

#ifdef FBO_OLD
	// generate a framebuffer 
	glGenFramebuffersEXT(1, &g_lod_fbo);
	
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &g_lod_tex);
	glBindTexture(GL_TEXTURE_2D, g_lod_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA16F,
						OGL.width <= 1024 ? 1024 : 2048,
						OGL.height <= 1024 ? 1024 : 2048,
						0, GL_RGBA, GL_FLOAT,
						NULL); 
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_lod_fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_lod_tex, 0);

	// check if everything is OK
	GLenum e = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch (e) {
		 case GL_FRAMEBUFFER_COMPLETE_EXT:
		   printf("framebuffer complete!\n");
		   break;
		 case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		   printf("framebuffer GL_FRAMEBUFFER_UNSUPPORTED_EXT\n");
			/* you gotta choose different formats */
		   break;
		 case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
		   printf("framebuffer INCOMPLETE_ATTACHMENT\n");
		   break;
		 case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
		   printf("framebuffer FRAMEBUFFER_MISSING_ATTACHMENT\n");
		   break;
		 case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
		   printf("framebuffer FRAMEBUFFER_DIMENSIONS\n");
		   break;
		 case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
		   printf("framebuffer INCOMPLETE_FORMATS\n");
		   break;
		 case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
		   printf("framebuffer INCOMPLETE_DRAW_BUFFER\n");
		   break;
		 case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
		   printf("framebuffer INCOMPLETE_READ_BUFFER\n");
		   break;
		 case GL_FRAMEBUFFER_BINDING_EXT:
		   printf("framebuffer BINDING_EXT\n");
		   break;
		 default:
		   break;
		   /* programming error; will fail on all hardware */ \
		   /*assert(0);*/ \
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif
}

void DestroyGLSLCombiner() {
	if (g_lod_tex > 0)
		glDeleteTextures(1, &g_lod_tex);
#ifdef FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &g_lod_fbo);
#elif defined(FBO_OLD)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDeleteFramebuffersEXT(1, &g_lod_fbo);
#endif
}

static
int CompileCombiner(const CombinerStage & _stage, const char** _Input, char * _fragment_shader) {
	char buf[128];
	bool bBracketOpen = false;
	int nRes = 0;
	for (int i = 0; i < _stage.numOps; ++i) {
		switch(_stage.op[i].op) {
			case LOAD:
				sprintf(buf, "(%s ", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				bBracketOpen = true;
				nRes |= 1 << _stage.op[i].param1;
				break;
			case SUB:
				if (bBracketOpen) {
					sprintf(buf, "- %s)", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				nRes |= 1 << _stage.op[i].param1;
				break;
			case ADD:
				if (bBracketOpen) {
					sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				nRes |= 1 << _stage.op[i].param1;
				break;
			case MUL:
				if (bBracketOpen) {
					sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
					bBracketOpen = false;
				} else
					sprintf(buf, "*%s", _Input[_stage.op[i].param1]);
				strcat(_fragment_shader, buf);
				nRes |= 1 << _stage.op[i].param1;
				break;
			case INTER:
				sprintf(buf, "mix(%s, %s, %s)", _Input[_stage.op[0].param2], _Input[_stage.op[0].param1], _Input[_stage.op[0].param3]);
				strcat(_fragment_shader, buf);
				nRes |= 1 << _stage.op[i].param1;
				nRes |= 1 << _stage.op[i].param2;
				nRes |= 1 << _stage.op[i].param3;
				break;

				//			default:
				//				assert(false);
		}
	}
	if (bBracketOpen)
		strcat(_fragment_shader, ")");
	strcat(_fragment_shader, "; \n");
	return nRes;
}

GLSLCombiner::GLSLCombiner(Combiner *_color, Combiner *_alpha) {
	m_vertexShaderObject = g_vertex_shader_object;

	char *fragment_shader = (char*)malloc(8192);
	strcpy(fragment_shader, fragment_shader_header_common_variables);

	char strCombiner[256];
	strcpy(strCombiner, "  alpha1 = ");
	m_nInputs = CompileCombiner(_alpha->stage[0], AlphaInput_1cycle, strCombiner);
	strcat(strCombiner, "  color1 = ");
	m_nInputs |= CompileCombiner(_color->stage[0], ColorInput_1cycle, strCombiner);
	strcat(strCombiner, "  combined_color = vec4(color1, alpha1); \n");
	if (_alpha->numStages == 2) {
		strcat(strCombiner, "  alpha2 = ");
		m_nInputs |= CompileCombiner(_alpha->stage[1], AlphaInput_2cycle, strCombiner);
	} else
		strcat(strCombiner, "  alpha2 = alpha1; \n");
	if (_color->numStages == 2) {
		strcat(strCombiner, "  color2 = ");
		m_nInputs |= CompileCombiner(_color->stage[1], ColorInput_2cycle, strCombiner);
	} else
		strcat(strCombiner, "  color2 = color1; \n");

	bool bUseLod = (m_nInputs & (1<<LOD_FRACTION)) > 0;
	if (bUseLod)
		strcat(fragment_shader, fragment_shader_header_lod_variables);
	strcat(fragment_shader, fragment_shader_header_common_functions);
	if (bUseLod)
		strcat(fragment_shader, fragment_shader_calc_lod);
	strcat(fragment_shader, fragment_shader_header_main);
	strcat(fragment_shader, "  if (dither_enabled > 0) \n");
	strcat(fragment_shader, "    if (snoise(noiseCoord2D) < 0.0) discard; \n");
	if (bUseLod)
		strcat(fragment_shader, "  float lod_frac = calc_lod();		\n");
	if ((m_nInputs & ((1<<TEXEL0)|(1<<TEXEL1)|(1<<TEXEL0_ALPHA)|(1<<TEXEL1_ALPHA))) > 0) {
		strcat(fragment_shader, fragment_shader_readtex0color);
		strcat(fragment_shader, fragment_shader_readtex1color);
	} else {
		assert(strstr(strCombiner, "readtex") == 0);
	}
	strcat(fragment_shader, "  float intensity = calc_light(); \n");
	strcat(fragment_shader, "  vec_color = vec4(input_color, gl_Color.a); \n");
	strcat(fragment_shader, strCombiner);
	strcat(fragment_shader, "  gl_FragColor = vec4(color2, alpha2); \n");
#ifdef USE_TOONIFY
	strcat(fragment_shader, "  toonify(intensity); \n");
#endif
	strcat(fragment_shader, "  if (fog_enabled > 0) \n");
	strcat(fragment_shader, "    gl_FragColor = vec4(mix(gl_Fog.color.rgb, gl_FragColor.rgb, gl_FogFragCoord), gl_FragColor.a); \n");
	strcat(fragment_shader, fragment_shader_end);
	strcat(fragment_shader, noise_fragment_shader);

#ifdef USE_TOONIFY
	strcat(fragment_shader, fragment_shader_toonify);
#endif

	m_fragmentShaderObject = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glShaderSourceARB(m_fragmentShaderObject, 1, (const GLcharARB**)&fragment_shader, NULL);
	free(fragment_shader);

	glCompileShaderARB(m_fragmentShaderObject);

	m_programObject = glCreateProgramObjectARB();
	glAttachObjectARB(m_programObject, m_fragmentShaderObject);
	glAttachObjectARB(m_programObject, m_vertexShaderObject);
	glLinkProgramARB(m_programObject);
}

void GLSLCombiner::Set() {
	combiner.usesT0 = FALSE;
	combiner.usesT1 = FALSE;
	combiner.usesLOD = (m_nInputs & (1<<LOD_FRACTION)) > 0 ? TRUE : FALSE;

	combiner.vertex.color = COMBINED;
	combiner.vertex.alpha = COMBINED;
	combiner.vertex.secondaryColor = LIGHT;

	glUseProgramObjectARB(m_programObject);

	int texture0_location = glGetUniformLocationARB(m_programObject, "texture0");
	if (texture0_location != -1) {
		glUniform1iARB(texture0_location, 0);
		combiner.usesT0 = TRUE;
	}

	int texture1_location = glGetUniformLocationARB(m_programObject, "texture1");
	if (texture1_location != -1) {
		glUniform1iARB(texture1_location, 1);
		combiner.usesT1 = TRUE;
	}

	UpdateColors();

#ifdef _DEBUG
	int log_length;
	glGetObjectParameterivARB(m_programObject, GL_OBJECT_LINK_STATUS_ARB , &log_length);
	if(!log_length)
	{
		const int nLogSize = 1024;
		char shader_log[nLogSize];
		glGetInfoLogARB(m_fragmentShaderObject, 
			nLogSize, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
		glGetInfoLogARB(m_vertexShaderObject, nLogSize, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
		glGetInfoLogARB(m_programObject, 
			nLogSize, &log_length, shader_log);
		if(log_length) 
			display_warning(shader_log);
	}
#endif
}

void GLSLCombiner::UpdateColors() {
	int prim_color_location = glGetUniformLocationARB(m_programObject, "prim_color");
	glUniform4fARB(prim_color_location, gDP.primColor.r, gDP.primColor.g, gDP.primColor.b, gDP.primColor.a);

	int env_color_location = glGetUniformLocationARB(m_programObject, "env_color");
	glUniform4fARB(env_color_location, gDP.envColor.r, gDP.envColor.g, gDP.envColor.b, gDP.envColor.a);

	int prim_lod_location = glGetUniformLocationARB(m_programObject, "prim_lod");
	glUniform1fARB(prim_lod_location, gDP.primColor.l);

	if (combiner.usesLOD) {
		BOOL bCalcLOD = gDP.otherMode.textureLOD == G_TL_LOD;
		int lod_en_location = glGetUniformLocationARB(m_programObject, "lod_enabled");
		glUniform1iARB(lod_en_location, bCalcLOD);
		if (bCalcLOD) {
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, g_lod_tex);
			int lod_texture_location = glGetUniformLocationARB(m_programObject, "lod_texture");
			glUniform1iARB(lod_texture_location, 2);
			int scale_x_location = glGetUniformLocationARB(m_programObject, "lod_x_scale");
			glUniform1fARB(scale_x_location, OGL.scaleX);
			int scale_y_location = glGetUniformLocationARB(m_programObject, "lod_y_scale");
			glUniform1fARB(scale_y_location, OGL.scaleY);
			int min_lod_location = glGetUniformLocationARB(m_programObject, "min_lod");
			glUniform1fARB(min_lod_location, gDP.primColor.m);
			int max_tile_location = glGetUniformLocationARB(m_programObject, "max_tile");
			glUniform1iARB(max_tile_location, gSP.texture.level);
			int texture_detail_location = glGetUniformLocationARB(m_programObject, "texture_detail");
			glUniform1iARB(texture_detail_location, gDP.otherMode.textureDetail);
		}
	}
	
	int nDither = (gDP.otherMode.colorDither == 2 || gDP.otherMode.alphaDither == 2 || gDP.otherMode.alphaCompare == 3) ? 1 : 0;
	int dither_location = glGetUniformLocationARB(m_programObject, "dither_enabled");
	glUniform1iARB(dither_location, nDither);

	if ((m_nInputs & (1<<NOISE)) + nDither > 0) {
		int time_location = glGetUniformLocationARB(m_programObject, "time");
		glUniform1fARB(time_location, (float)(rand()&255));
	}

	int fog_location = glGetUniformLocationARB(m_programObject, "fog_enabled");
	glUniform1iARB(fog_location, (gSP.geometryMode & G_FOG) > 0 ? 1 : 0);
}

#include "VI.h"

void OGL_UpdateCullFace();
void OGL_UpdateViewport();
void OGL_ClearColorBuffer( float *color );

void drawFBO()
{
	glUseProgramObjectARB(0);
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, VI.width, VI.height, 0, 1.0f, -1.0f );
	glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_lod_tex);
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
	glColor4f( 1, 1, 1, 1 );
	const float s0 = 1.0f;//640.0f/1024.0f;
	const float t0 = 1.0f;//480.0f/1024.0f;


	glBegin( GL_QUADS );
		glMultiTexCoord2fARB( GL_TEXTURE2_ARB, 0, 0 );
		glVertex4f( 0, 0, 1.0f, 1.0f );

		glMultiTexCoord2fARB( GL_TEXTURE2_ARB, s0, 0 );
		glVertex4f( 320, 0, 1.0f, 1.0f );

		glMultiTexCoord2fARB( GL_TEXTURE2_ARB, s0, t0 );
		glVertex4f( 320, 240, 1.0f, 1.0f );

		glMultiTexCoord2fARB( GL_TEXTURE2_ARB, 0, t0 );
		glVertex4f( 0, 240, 1.0f, 1.0f );
	glEnd();

	glLoadIdentity();
	OGL_UpdateCullFace();
	OGL_UpdateViewport();
	
}


void GLSL_CalcLOD() {
	glDisable( GL_DEPTH_TEST );
#ifdef FBO
	// bind a framebuffer object
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_lod_fbo);
	// Set Drawing buffers
	GLuint attachments[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1,  attachments);
	float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	OGL_ClearColorBuffer(clear_color );
#elif defined(FBO_OLD)
	// bind a framebuffer object
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_lod_fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_lod_tex, 0);
	float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	OGL_ClearColorBuffer(clear_color );
#endif	
	glUseProgramObjectARB(g_lod_program);
	glDrawArrays( GL_TRIANGLES, 0, OGL.numVertices );
#ifdef FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif defined(FBO_OLD)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif

#if defined(FBO) || defined(FBO_OLD)
//	drawFBO();
#endif

#if defined(LOD_TEST)
	glActiveTextureARB(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_lod_tex);
	glUseProgramObjectARB(g_lod_program_test);
	int texture2_location = glGetUniformLocationARB(g_lod_program_test, "mytex");
	if (texture2_location != -1)
		glUniform1iARB(texture2_location, 2);
	glDrawArrays( GL_TRIANGLES, 0, OGL.numVertices );
#endif

	if (gSP.geometryMode & G_ZBUFFER)
		glEnable( GL_DEPTH_TEST );

	Combiner_SetCombine( gDP.combine.mux );
}

void GLSL_PostCalcLOD() {
	glDisable( GL_DEPTH_TEST );
#ifdef FBO
	// bind a framebuffer object
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_lod_fbo);
	// Set Drawing buffers
	GLuint attachments[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1,  attachments);
	float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	OGL_ClearColorBuffer(clear_color );
#elif defined(FBO_OLD)
	// bind a framebuffer object
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_lod_fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_lod_tex, 0);
	float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	OGL_ClearColorBuffer(clear_color );
#endif	
	glUseProgramObjectARB(g_lod_clear_program);
	glDrawArrays( GL_TRIANGLES, 0, OGL.numVertices );
#ifdef FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif defined(FBO_OLD)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif
	Combiner_SetCombine( gDP.combine.mux );
}
