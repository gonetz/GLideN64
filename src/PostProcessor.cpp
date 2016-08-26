#include <assert.h>

#include "N64.h"
#include "gSP.h"
#include "PostProcessor.h"
#include "FrameBuffer.h"
#include "GLSLCombiner.h"
#include "ShaderUtils.h"
#include "Config.h"

#if defined(GLES3_1)
#define SHADER_VERSION "#version 310 es \n"
#elif defined(GLES3)
#define SHADER_VERSION "#version 300 es \n"
#elif defined(GLES2)
#define SHADER_VERSION "#version 100 \n"
#else
#define SHADER_VERSION "#version 330 core \n"
#endif

#ifdef GLES2
#define FRAGMENT_SHADER_END "  gl_FragColor = fragColor; \n"
#else
#define FRAGMENT_SHADER_END "\n"
#endif

#ifdef ANDROID
PostProcessor PostProcessor::processor;
#endif

static const float vert[] =
{
	-1.0, -1.0, +0.0, +0.0,
	+1.0, -1.0, +1.0, +0.0,
	-1.0, +1.0, +0.0, +1.0,
	+1.0, +1.0, +1.0, +1.0
};

static const char * vertexShader =
SHADER_VERSION
"#if (__VERSION__ > 120)						\n"
"# define IN in									\n"
"# define OUT out								\n"
"#else											\n"
"# define IN attribute							\n"
"# define OUT varying							\n"
"#endif // __VERSION							\n"
"IN highp vec2 aPosition;								\n"
"IN highp vec2 aTexCoord;								\n"
"OUT mediump vec2 vTexCoord;							\n"
"void main(){                                           \n"
"gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);\n"
"vTexCoord = aTexCoord;                                 \n"
"}                                                      \n"
;

static const char* extractBloomShader =
SHADER_VERSION
"#if (__VERSION__ > 120)		\n"
"# define IN in					\n"
"# define OUT out				\n"
"# define texture2D texture		\n"
"#else							\n"
"# define IN varying			\n"
"# define OUT					\n"
"#endif // __VERSION __			\n"
"IN mediump vec2 vTexCoord;                               \n"
"uniform sampler2D Sample0;				               \n"
"OUT lowp vec4 fragColor;								\n"
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
FRAGMENT_SHADER_END
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
"#if (__VERSION__ > 120)		\n"
"# define IN in					\n"
"# define OUT out				\n"
"# define texture2D texture		\n"
"#else							\n"
"# define IN varying			\n"
"# define OUT					\n"
"#endif // __VERSION __			\n"
// Uniform variables.
"uniform sampler2D Sample0;																                                                            \n"
"uniform mediump vec2 TexelSize;                                                                                                                    \n"
"                                                                                                                                                   \n"
"uniform lowp int Orientation;                                                                                                                      \n"
"uniform lowp int BlurAmount;                                                                                                                       \n"
"uniform lowp float BlurScale;                                                                                                                      \n"
"uniform lowp float BlurStrength;                                                                                                                   \n"
"                                                                                                                                                   \n"
"IN mediump vec2 vTexCoord;																								                            \n"
"OUT lowp vec4 fragColor;																															\n"
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
"		for (lowp int i = 0; i < BlurAmount; ++i)                                                                                                \n"
"		{                                                                                                                                   \n"
"			mediump float offset = float(i) - halfBlur;                                                                                     \n"
"			colour += texture2D(Sample0, vTexCoord + vec2(offset * TexelSize.x * BlurScale, 0.0)) * Gaussian(offset * strength, deviation); \n"
"		}                                                                                                                                   \n"
"	}                                                                                                                                       \n"
"	else                                                                                                                                    \n"
"	{                                                                                                                                       \n"
"		// Vertical blur                                                                                                                    \n"
"		for (lowp int i = 0; i < BlurAmount; ++i)                                                                                                \n"
"		{                                                                                                                                   \n"
"			mediump float offset = float(i) - halfBlur;                                                                                     \n"
"			colour += texture2D(Sample0, vTexCoord + vec2(0.0, offset * TexelSize.y * BlurScale)) * Gaussian(offset * strength, deviation); \n"
"		}                                                                                                                                   \n"
"	}                                                                                                                                       \n"
"	                                                                                                                                        \n"
"	// Apply colour                                                                                                                         \n"
"	fragColor = clamp(colour, 0.0, 1.0);                                                                                                    \n"
"	fragColor.a = 1.0;                                                                                                                      \n"
FRAGMENT_SHADER_END
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
"#if (__VERSION__ > 120)		\n"
"# define IN in					\n"
"# define OUT out				\n"
"# define texture2D texture		\n"
"#else							\n"
"# define IN varying			\n"
"# define OUT					\n"
"#endif // __VERSION __			\n"
// Uniform variables.
"uniform sampler2D Sample0;						                                                         \n"
"uniform sampler2D Sample1;											                                      \n"
"uniform lowp int BlendMode;                                                                              \n"
"                                                                                                         \n"
"IN mediump vec2 vTexCoord;														                            \n"
"OUT lowp vec4 fragColor;																					\n"
"                                                                                                         \n"
// Fragment shader entry.
"void main ()                                                                                             \n"
"{                                                                                                        \n"
"	lowp vec4 dst = texture2D(Sample0, vTexCoord); // rendered scene                                      \n"
"	lowp vec4 src = texture2D(Sample1, vTexCoord); // glowmap                                             \n"
"                                                                                                         \n"
"   if (BlendMode == 0) {																								\n"
"		// Additive blending (strong result, high overexposure)															\n"
"	     fragColor = min(src + dst, 1.0);																				\n"
"		 fragColor.a = 1.0;																								\n"
"   } else if (BlendMode == 1) {																						\n"
"		 fragColor = clamp((src + dst) - (src * dst), 0.0, 1.0);														\n"
"		 fragColor.a = 1.0;																							    \n"
"   } else if (BlendMode == 2) {																						\n"
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
"   } else  {																											\n"
"		// Show just the glow map																						\n"
"		 fragColor = src;																								\n"
"	}																													\n"
FRAGMENT_SHADER_END
"}																														\n"
;

static const char* gammaCorrectionShader =
SHADER_VERSION
"#if (__VERSION__ > 120)													\n"
"# define IN in																\n"
"# define OUT out															\n"
"# define texture2D texture													\n"
"#else																		\n"
"# define IN varying														\n"
"# define OUT																\n"
"#endif // __VERSION __														\n"
"IN mediump vec2 vTexCoord;													\n"
"uniform sampler2D Sample0;													\n"
"uniform lowp float uGammaCorrectionLevel;									\n"
"OUT lowp vec4 fragColor;													\n"
"																			\n"
"void main()																\n"
"{																			\n"
"    fragColor = texture2D(Sample0, vTexCoord);								\n"
"    fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / uGammaCorrectionLevel));	\n"
FRAGMENT_SHADER_END
"}																			\n"
;

static
GLuint _createShaderProgram(const char * _strVertex, const char * _strFragment)
{
	GLuint vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_object, 1, &_strVertex, nullptr);
	glCompileShader(vertex_shader_object);
	assert(checkShaderCompileStatus(vertex_shader_object));

	GLuint fragment_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_object, 1, &_strFragment, nullptr);
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
void _initTexture(CachedTexture * pTexture)
{
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pTexture->realWidth, pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static
CachedTexture * _createTexture()
{
	CachedTexture * pTexture = textureCache().addFrameBufferTexture();
	_initTexture(pTexture);
	return pTexture;
}

static
void _initFBO(GLuint _FBO, CachedTexture * _pTexture)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _FBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _pTexture->glName, 0);
	assert(checkFBO());
}

static
GLuint _createFBO(CachedTexture * _pTexture)
{
	GLuint FBO;
	glGenFramebuffers(1, &FBO);
	_initFBO(FBO, _pTexture);
	return FBO;
}

PostProcessor::PostProcessor()
	: m_extractBloomProgram(0)
	, m_seperableBlurProgram(0)
	, m_glowProgram(0)
	, m_bloomProgram(0)
	, m_gammaCorrectionProgram(0)
	, m_pResultBuffer(nullptr)
	, m_FBO_glowMap(0)
	, m_FBO_blur(0)
	, m_pTextureOriginal(nullptr)
	, m_pTextureGlowMap(nullptr)
	, m_pTextureBlur(nullptr)
{}

void PostProcessor::_initCommon()
{
	m_pResultBuffer = new FrameBuffer();
	_initTexture(m_pResultBuffer->m_pTexture);
	_initFBO(m_pResultBuffer->m_FBO, m_pResultBuffer->m_pTexture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void PostProcessor::_initGammaCorrection()
{
	m_gammaCorrectionProgram = _createShaderProgram(vertexShader, gammaCorrectionShader);
	glUseProgram(m_gammaCorrectionProgram);
	int loc = glGetUniformLocation(m_gammaCorrectionProgram, "Sample0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_gammaCorrectionProgram, "uGammaCorrectionLevel");
	assert(loc >= 0);
	const f32 gammaLevel = (config.gammaCorrection.force != 0) ? config.gammaCorrection.level : 2.0f;
	glUniform1f(loc, gammaLevel);
	glUseProgram(0);
}

void PostProcessor::_initBlur()
{
	m_extractBloomProgram = _createShaderProgram(vertexShader, extractBloomShader);
	glUseProgram(m_extractBloomProgram);
	int loc = glGetUniformLocation(m_extractBloomProgram, "Sample0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_extractBloomProgram, "ThresholdLevel");
	assert(loc >= 0);
	glUniform1i(loc, config.bloomFilter.thresholdLevel);

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
}

void PostProcessor::init()
{
#ifndef GLESX
	glGenBuffers(1, &pp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, pp_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*16, vert, GL_STATIC_DRAW);
#endif
	_initCommon();
	_initGammaCorrection();
	if (config.bloomFilter.enable != 0)
		_initBlur();
}

void PostProcessor::_destroyCommon()
{
#ifndef GLESX
	glDeleteBuffers(1, &pp_vbo);
#endif
	delete m_pResultBuffer;
	m_pResultBuffer = nullptr;

	m_pTextureOriginal = nullptr;
}

void PostProcessor::_destroyGammaCorrection()
{
	if (m_gammaCorrectionProgram != 0)
		glDeleteProgram(m_gammaCorrectionProgram);
	m_gammaCorrectionProgram = 0;
}

void PostProcessor::_destroyBlur()
{
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

	if (m_FBO_glowMap != 0)
		glDeleteFramebuffers(1, &m_FBO_glowMap);
	m_FBO_glowMap = 0;

	if (m_FBO_blur != 0)
		glDeleteFramebuffers(1, &m_FBO_blur);
	m_FBO_blur = 0;

	if (m_pTextureGlowMap != nullptr)
		textureCache().removeFrameBufferTexture(m_pTextureGlowMap);
	m_pTextureGlowMap = nullptr;

	if (m_pTextureBlur != nullptr)
		textureCache().removeFrameBufferTexture(m_pTextureBlur);
	m_pTextureBlur = nullptr;
}


void PostProcessor::destroy()
{
	_destroyBlur();
	_destroyGammaCorrection();
	_destroyCommon();
}

PostProcessor & PostProcessor::get()
{
#ifndef ANDROID
	static PostProcessor processor;
#endif
	return processor;
}

void PostProcessor::_setGLState() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);


	glEnableVertexAttribArray(SC_POSITION);
	glEnableVertexAttribArray(SC_TEXCOORD0);
#ifndef GLESX
	glBindBuffer(GL_ARRAY_BUFFER, pp_vbo);
	glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (float*)NULL);
	glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (float*)NULL + 2);
#else
	glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (float*)vert);
	glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (float*)vert + 2);
#endif
	glDisableVertexAttribArray(SC_COLOR);
	glDisableVertexAttribArray(SC_TEXCOORD1);
	glDisableVertexAttribArray(SC_NUMLIGHTS);
	glDisableVertexAttribArray(SC_MODIFY);
	glViewport(0, 0, video().getWidth(), video().getHeight());
	glScissor(0, 0, m_pResultBuffer->m_pTexture->realWidth, m_pResultBuffer->m_pTexture->realHeight);
	gSP.changed |= CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_RENDERMODE | CHANGED_SCISSOR;
}

void PostProcessor::_preDraw(FrameBuffer * _pBuffer)
{
	_setGLState();
	OGLVideo & ogl = video();

	m_pResultBuffer->m_width = _pBuffer->m_width;
	m_pResultBuffer->m_height = _pBuffer->m_height;
	m_pResultBuffer->m_scaleX = ogl.getScaleX();
	m_pResultBuffer->m_scaleY = ogl.getScaleY();
#ifdef GLES2
	m_pTextureOriginal = _pBuffer->m_pTexture;
#else
	if (_pBuffer->m_pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
		_pBuffer->resolveMultisampledTexture(true);
		m_pTextureOriginal = _pBuffer->m_pResolveTexture;
	} else
		m_pTextureOriginal = _pBuffer->m_pTexture;
#endif

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void PostProcessor::_postDraw()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	video().getRender().dropRenderState();
	glUseProgram(0);
}

FrameBuffer * PostProcessor::doBlur(FrameBuffer * _pBuffer)
{
	if (_pBuffer == nullptr)
		return nullptr;

	if (config.bloomFilter.enable == 0)
		return _pBuffer;

	_preDraw(_pBuffer);

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

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pResultBuffer->m_FBO);
	textureCache().activateTexture(0, m_pTextureOriginal);
	textureCache().activateTexture(1, m_pTextureGlowMap);
	glUseProgram(m_glowProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	_postDraw();
	return m_pResultBuffer;
}

FrameBuffer * PostProcessor::doGammaCorrection(FrameBuffer * _pBuffer)
{
	if (_pBuffer == nullptr)
		return nullptr;

	if (((*REG.VI_STATUS & 8) | config.gammaCorrection.force) == 0)
		return _pBuffer;


	_preDraw(_pBuffer);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pResultBuffer->m_FBO);
	textureCache().activateTexture(0, m_pTextureOriginal);
	glUseProgram(m_gammaCorrectionProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	_postDraw();
	return m_pResultBuffer;
}
