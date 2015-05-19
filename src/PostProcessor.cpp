#include <assert.h>

#include "N64.h"
#include "gSP.h"
#include "PostProcessor.h"
#include "FrameBuffer.h"
#include "GLSLCombiner.h"
#include "ShaderUtils.h"
#include "Config.h"

#ifdef GLES3
#define SHADER_VERSION "#version 300 es \n"
#else
#define SHADER_VERSION "#version 330 core \n"
#endif

static const char * vertexShader =
SHADER_VERSION
"in highp vec2 aPosition;								\n"
"in highp vec2 aTexCoord;								\n"
"out mediump vec2 vTexCoord;							\n"
"void main(){                                           \n"
"gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);\n"
"vTexCoord = aTexCoord;                                 \n"
"}                                                      \n"
;

static const char* copyShader =
SHADER_VERSION
"in mediump vec2 vTexCoord;                             \n"
"uniform sampler2D Sample0;				                \n"
"out lowp vec4 fragColor;								\n"
"                                                       \n"
"void main()                                            \n"
"{                                                      \n"
"    fragColor = texture2D(Sample0, vTexCoord);         \n"
"}							                            \n"
;

static const char* extractBloomShader =
SHADER_VERSION
"in mediump vec2 vTexCoord;                               \n"
"uniform sampler2D Sample0;				               \n"
"out lowp vec4 fragColor;								\n"
"                                                         \n"
"uniform lowp int ThresholdLevel;                         \n"
"                                                         \n"
"void main()                                              \n"
"{                                                        \n"
"    lowp vec4 color = texture2D(Sample0, vTexCoord);         \n"
"                                                         \n"
"    mediump float lum = dot(vec4(0.30, 0.59, 0.11, 0.0), color);\n"
"    mediump float scale = lum;									\n"
"   lowp int level = clamp(ThresholdLevel, 2, 6);				\n"
"	for (int i = 1; i < level; ++i)                             \n"
"     scale *= lum;												\n"
"    fragColor = scale*color;									\n"
"	fragColor.a = 1.0;											\n"
"}							                                    \n"
;

static const char* seperableBlurShader =
/// Author:		Nathaniel Meyer
///
/// Copyright:	Nutty Software
///				http://www.nutty.ca
///
/// Fragment shader for performing a seperable blur on the specified texture.
SHADER_VERSION
// Uniform variables.
"uniform sampler2D Sample0;																                                                            \n"
"uniform mediump vec2 TexelSize;                                                                                                                    \n"
"                                                                                                                                                   \n"
"uniform lowp int Orientation;                                                                                                                      \n"
"uniform lowp int BlurAmount;                                                                                                                       \n"
"uniform lowp float BlurScale;                                                                                                                      \n"
"uniform lowp float BlurStrength;                                                                                                                   \n"
"                                                                                                                                                   \n"
"in mediump vec2 vTexCoord;																								                            \n"
"out lowp vec4 fragColor;																															\n"
"                                                                                                                                                   \n"
// Gets the Gaussian value in the first dimension.
// "x" Distance from origin on the x-axis.
// "deviation" Standard deviation.
// returns The gaussian value on the x-axis.
"mediump float Gaussian (in mediump float x, in mediump float deviation)                                                                            \n"
"{                                                                                                                                                  \n"
"	return (1.0 / sqrt(2.0 * 3.141592 * deviation)) * exp(-((x * x) / (2.0 * deviation)));	                                                        \n"
"}                                                                                                                                                  \n"
"                                                                                                                                                   \n"
// Fragment shader entry.
"void main ()                                                                                                                                       \n"
"{                                                                                                                                                  \n"
"	// Locals                                                                                                                                   \n"
"	mediump float halfBlur = float(BlurAmount) * 0.5;                                                                                           \n"
"	mediump vec4 colour = vec4(0.0);                                                                                                            \n"
"	                                                                                                                                            \n"
"	// Gaussian deviation                                                                                                                       \n"
"	mediump float deviation = halfBlur * 0.35;                                                                                                  \n"
"	deviation *= deviation;                                                                                                                     \n"
"	mediump float strength = 1.0 - BlurStrength;                                                                                                \n"
"	                                                                                                                                            \n"
"	if ( Orientation == 0 )                                                                                                                     \n"
"	{                                                                                                                                           \n"
"		// Horizontal blur                                                                                                                  \n"
"		for (int i = 0; i < BlurAmount; ++i)                                                                                                \n"
"		{                                                                                                                                   \n"
"			mediump float offset = float(i) - halfBlur;                                                                                     \n"
"			colour += texture2D(Sample0, vTexCoord + vec2(offset * TexelSize.x * BlurScale, 0.0)) * Gaussian(offset * strength, deviation); \n"
"		}                                                                                                                                   \n"
"	}                                                                                                                                       \n"
"	else                                                                                                                                    \n"
"	{                                                                                                                                       \n"
"		// Vertical blur                                                                                                                    \n"
"		for (int i = 0; i < BlurAmount; ++i)                                                                                                \n"
"		{                                                                                                                                   \n"
"			mediump float offset = float(i) - halfBlur;                                                                                     \n"
"			colour += texture2D(Sample0, vTexCoord + vec2(0.0, offset * TexelSize.y * BlurScale)) * Gaussian(offset * strength, deviation); \n"
"		}                                                                                                                                   \n"
"	}                                                                                                                                       \n"
"	                                                                                                                                        \n"
"	// Apply colour                                                                                                                         \n"
"	fragColor = clamp(colour, 0.0, 1.0);                                                                                                    \n"
"	fragColor.a = 1.0;                                                                                                                      \n"
"}                                                                                                                                          \n"
;

static const char* glowShader =
/// Author:		Nathaniel Meyer
///
/// Copyright:	Nutty Software
///				http://www.nutty.ca
///
/// Fragment shader for blending two textures using an algorithm that overlays the glowmap.
SHADER_VERSION
// Uniform variables.
"uniform sampler2D Sample0;						                                                         \n"
"uniform sampler2D Sample1;											                                      \n"
"uniform lowp int BlendMode;                                                                              \n"
"                                                                                                         \n"
"in mediump vec2 vTexCoord;														                            \n"
"out lowp vec4 fragColor;																					\n"
"                                                                                                         \n"
// Fragment shader entry.
"void main ()                                                                                             \n"
"{                                                                                                        \n"
"	lowp vec4 dst = texture2D(Sample0, vTexCoord); // rendered scene                                      \n"
"	lowp vec4 src = texture2D(Sample1, vTexCoord); // glowmap                                             \n"
"                                                                                                         \n"
"   switch (BlendMode) {																								\n"
"      case 0:																											\n"
"		// Additive blending (strong result, high overexposure)															\n"
"	     fragColor = min(src + dst, 1.0);																				\n"
"		 fragColor.a = 1.0;																								\n"
"      break;																											\n"
"      case 1:																											\n"
"		 fragColor = clamp((src + dst) - (src * dst), 0.0, 1.0);														\n"
"		 fragColor.a = 1.0;																							    \n"
"      break;																											\n"
"      case 2:																											\n"
"		 src = (src * 0.5) + 0.5;																						\n"
"																														\n"
"        if (src.x <= 0.5)																								\n"
"          fragColor.x = dst.x - (1.0 - 2.0 * src.x) * dst.x * (1.0 - dst.x);											\n"
"        else if ((src.x > 0.5) && (dst.x <= 0.25))																		\n"
"          fragColor.x = dst.x + (2.0 * src.x - 1.0) * (4.0 * dst.x * (4.0 * dst.x + 1.0) * (dst.x - 1.0) + 7.0 * dst.x);\n"
"        else																											\n"
"          fragColor.x = dst.x + (2.0 * src.x - 1.0) * (sqrt(dst.x) - dst.x);											\n"
"        if (src.y <= 0.5)																								\n"
"          fragColor.y = dst.y - (1.0 - 2.0 * src.y) * dst.y * (1.0 - dst.y);											\n"
"        else if ((src.y > 0.5) && (dst.y <= 0.25))																		\n"
"          fragColor.y = dst.y + (2.0 * src.y - 1.0) * (4.0 * dst.y * (4.0 * dst.y + 1.0) * (dst.y - 1.0) + 7.0 * dst.y);\n"
"        else																											\n"
"          fragColor.y = dst.y + (2.0 * src.y - 1.0) * (sqrt(dst.y) - dst.y);											\n"
"        if (src.z <= 0.5)																								\n"
"          fragColor.z = dst.z - (1.0 - 2.0 * src.z) * dst.z * (1.0 - dst.z);											\n"
"        else if ((src.z > 0.5) && (dst.z <= 0.25))																		\n"
"          fragColor.z = dst.z + (2.0 * src.z - 1.0) * (4.0 * dst.z * (4.0 * dst.z + 1.0) * (dst.z - 1.0) + 7.0 * dst.z);\n"
"        else																											\n"
"          fragColor.z = dst.z + (2.0 * src.z - 1.0) * (sqrt(dst.z) - dst.z);											\n"
"		 fragColor.a = 1.0;																								\n"
"      break;																											\n"
"      default:																											\n"
"		// Show just the glow map																						\n"
"		 fragColor = src;																								\n"
"      break;																											\n"
"	}																													\n"
"}																														\n"
;

static
GLuint _createShaderProgram(const char * _strVertex, const char * _strFragment)
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
	glBindAttribLocation(program, SC_TEXCOORD0, "aTexCoord");
	glAttachShader(program, vertex_shader_object);
	glAttachShader(program, fragment_shader_object);
	glLinkProgram(program);
	glDeleteShader(vertex_shader_object);
	glDeleteShader(fragment_shader_object);
	assert(checkProgramLinkStatus(program));
	return program;
}

static
CachedTexture * _createTexture()
{
	CachedTexture * pTexture = textureCache().addFrameBufferTexture();
	pTexture->format = G_IM_FMT_RGBA;
	pTexture->clampS = 1;
	pTexture->clampT = 1;
	pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	pTexture->maskS = 0;
	pTexture->maskT = 0;
	pTexture->mirrorS = 0;
	pTexture->mirrorT = 0;
	pTexture->realWidth = video().getWidth();
	pTexture->realHeight = video().getHeight();
	pTexture->textureBytes = pTexture->realWidth * pTexture->realHeight * 4;
	textureCache().addFrameBufferTextureSize(pTexture->textureBytes);
	glBindTexture(GL_TEXTURE_2D, pTexture->glName);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pTexture->realWidth, pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	return pTexture;
}

static
GLuint _createFBO(CachedTexture * _pTexture)
{
	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _pTexture->glName, 0);
	assert(checkFBO());
	return FBO;
}

void PostProcessor::init()
{
	m_pTextureOriginal = _createTexture();
	m_FBO_original = _createFBO(m_pTextureOriginal);

	m_extractBloomProgram = _createShaderProgram(vertexShader, extractBloomShader);
	glUseProgram(m_extractBloomProgram);
	int loc = glGetUniformLocation(m_extractBloomProgram, "Sample0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_extractBloomProgram, "ThresholdLevel");
	assert(loc >= 0);
	glUniform1i(loc, config.bloomFilter.thresholdLevel);

#ifdef GLES2
	m_copyProgram = _createShaderProgram(vertexShader, copyShader);
	glUseProgram(m_copyProgram);
	loc = glGetUniformLocation(m_copyProgram, "Sample0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
#endif

	m_seperableBlurProgram = _createShaderProgram(vertexShader, seperableBlurShader);
	glUseProgram(m_seperableBlurProgram);
	loc = glGetUniformLocation(m_seperableBlurProgram, "Sample0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_seperableBlurProgram, "TexelSize");
	assert(loc >= 0);
	glUniform2f(loc, 1.0f / video().getWidth(), 1.0f / video().getHeight());
	loc = glGetUniformLocation(m_seperableBlurProgram, "Orientation");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_seperableBlurProgram, "BlurAmount");
	assert(loc >= 0);
	glUniform1i(loc, config.bloomFilter.blurAmount);
	loc = glGetUniformLocation(m_seperableBlurProgram, "BlurScale");
	assert(loc >= 0);
	glUniform1f(loc, 1.0f);
	loc = glGetUniformLocation(m_seperableBlurProgram, "BlurStrength");
	assert(loc >= 0);
	glUniform1f(loc, config.bloomFilter.blurStrength/100.0f);

	m_glowProgram = _createShaderProgram(vertexShader, glowShader);
	glUseProgram(m_glowProgram);
	loc = glGetUniformLocation(m_glowProgram, "Sample0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_glowProgram, "Sample1");
	assert(loc >= 0);
	glUniform1i(loc, 1);
	loc = glGetUniformLocation(m_glowProgram, "BlendMode");
	assert(loc >= 0);
	glUniform1i(loc, config.bloomFilter.blendMode);

	m_pTextureGlowMap = _createTexture();
	m_pTextureBlur = _createTexture();

	m_FBO_glowMap = _createFBO(m_pTextureGlowMap);
	m_FBO_blur = _createFBO(m_pTextureBlur);

	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void PostProcessor::destroy()
{
	if (m_copyProgram != 0)
		glDeleteProgram(m_copyProgram);
	m_copyProgram = 0;
	if (m_extractBloomProgram != 0)
		glDeleteProgram(m_extractBloomProgram);
	m_extractBloomProgram = 0;
	if (m_seperableBlurProgram != 0)
		glDeleteProgram(m_seperableBlurProgram);
	m_seperableBlurProgram = 0;
	if (m_glowProgram != 0)
		glDeleteProgram(m_glowProgram);
	m_glowProgram = 0;
	if (m_bloomProgram != 0)
		glDeleteProgram(m_bloomProgram);
	m_bloomProgram = 0;

	if (m_FBO_original != 0)
		glDeleteFramebuffers(1, &m_FBO_original);
	m_FBO_original = 0;
	if (m_FBO_glowMap != 0)
		glDeleteFramebuffers(1, &m_FBO_glowMap);
	m_FBO_glowMap = 0;
	if (m_FBO_blur != 0)
		glDeleteFramebuffers(1, &m_FBO_blur);
	m_FBO_blur = 0;

	if (m_pTextureOriginal != NULL)
		textureCache().removeFrameBufferTexture(m_pTextureOriginal);
	m_pTextureOriginal = NULL;
	if (m_pTextureGlowMap != NULL)
		textureCache().removeFrameBufferTexture(m_pTextureGlowMap);
	m_pTextureGlowMap = NULL;
	if (m_pTextureBlur != NULL)
		textureCache().removeFrameBufferTexture(m_pTextureBlur);
	m_pTextureBlur = NULL;
}

PostProcessor & PostProcessor::get()
{
	static PostProcessor processor;
	return processor;
}

void _setGLState() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	static const float vert[] =
	{
		-1.0, -1.0, +0.0, +0.0,
		+1.0, -1.0, +1.0, +0.0,
		-1.0, +1.0, +0.0, +1.0,
		+1.0, +1.0, +1.0, +1.0
	};

	glEnableVertexAttribArray(SC_POSITION);
	glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (float*)vert);
	glEnableVertexAttribArray(SC_TEXCOORD0);
	glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (float*)vert + 2);
	glDisableVertexAttribArray(SC_COLOR);
	glDisableVertexAttribArray(SC_TEXCOORD1);
	glDisableVertexAttribArray(SC_NUMLIGHTS);
	glViewport(0, 0, video().getWidth(), video().getHeight());
	gSP.changed |= CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_RENDERMODE;
}

void PostProcessor::process(FrameBuffer * _pBuffer)
{
	if (config.bloomFilter.enable == 0)
		return;

	if (_pBuffer == NULL || _pBuffer->m_postProcessed)
		return;

	_pBuffer->m_postProcessed = true;

	_setGLState();
	OGLVideo & ogl = video();

#ifdef GLES2
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO_original);
	textureCache().activateTexture(0, _pBuffer->m_pTexture);
	glUseProgram(m_copyProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _pBuffer->m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO_original);
	glBlitFramebuffer(
		0, 0, ogl.getWidth(), ogl.getHeight(),
		0, 0, ogl.getWidth(), ogl.getHeight(),
		GL_COLOR_BUFFER_BIT, GL_LINEAR
	);
#endif

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO_glowMap);
	textureCache().activateTexture(0, m_pTextureOriginal);
	glUseProgram(m_extractBloomProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO_blur);
	textureCache().activateTexture(0, m_pTextureGlowMap);
	glUseProgram(m_seperableBlurProgram);
	int loc = glGetUniformLocation(m_seperableBlurProgram, "Orientation");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO_glowMap);
	textureCache().activateTexture(0, m_pTextureBlur);
	glUniform1i(loc, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _pBuffer->m_FBO);
	textureCache().activateTexture(0, m_pTextureOriginal);
	textureCache().activateTexture(1, m_pTextureGlowMap);
	glUseProgram(m_glowProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	video().getRender().dropRenderState();
	glUseProgram(0);
}
