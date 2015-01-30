#include <assert.h>

#include "N64.h"
#include "gSP.h"
#include "PostProcessor.h"
#include "GLSLCombiner.h"

const char * vertexShader = 
"#version 330 core										\n"
"in highp vec2 aPosition;								\n"
"in highp vec2 aTexCoord;								\n"
"out mediump vec2 vTexCoord;							\n"
"void main(){                                           \n"
"gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);\n"
"vTexCoord = aTexCoord;                                 \n"
"}                                                      \n"
;

static const char* bloomShader =
"#version 330 core																		\n"
"in mediump vec2 vTexCoord;																\n"
"layout(binding = 0) uniform sampler2D bgl_RenderedTexture;                             \n"
"out lowp vec4 fragColor;																\n"
"void main()                                                                            \n"
"{                                                                                      \n"
"   vec4 sum = vec4(0);                                                                 \n"
"   vec2 texcoord = vTexCoord;															\n"
"   int j;                                                                              \n"
"   int i;                                                                              \n"
"                                                                                       \n"
"   for( i= -4 ; i < 4; i++)                                                            \n"
"   {                                                                                   \n"
"        for (j = -3; j < 3; j++)                                                       \n"
"        {                                                                              \n"
"            sum += texture2D(bgl_RenderedTexture, texcoord + vec2(j, i)*0.004) * 0.25; \n"
"        }                                                                              \n"
"   }                                                                                   \n"
"    if (texture2D(bgl_RenderedTexture, texcoord).r < 0.3)		                        \n"
"    {                                                                                  \n"
"       fragColor = sum*sum*0.012 + texture2D(bgl_RenderedTexture, texcoord);    	\n"
"    }                                                                                  \n"
"    else                                                                               \n"
"    {                                                                                  \n"
"        if (texture2D(bgl_RenderedTexture, texcoord).r < 0.5)                          \n"
"        {                                                                              \n"
"            fragColor = sum*sum*0.009 + texture2D(bgl_RenderedTexture, texcoord);   \n"
"        }                                                                              \n"
"        else                                                                           \n"
"        {                                                                              \n"
"            fragColor = sum*sum*0.0075 + texture2D(bgl_RenderedTexture, texcoord);  \n"
"        }                                                                              \n"
"    }                                                                                  \n"
"}                                                                                      \n"
;

static const char* copyShader =
"#version 330 core																		\n"
"in mediump vec2 vTexCoord;																\n"
"layout(binding = 0) uniform sampler2D bgl_RenderedTexture;                             \n"
"out lowp vec4 fragColor;																\n"
"void main()                                                                            \n"
"{                                                                                      \n"
"  fragColor = texture2D(bgl_RenderedTexture, vTexCoord);								\n"
"}                                                                                      \n"
;

static
GLuint _createShaderProgram(const char * _strVertex, const char * _strFragment)
{
	GLuint vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_object, 1, &_strVertex, NULL);
	glCompileShader(vertex_shader_object);

	GLuint fragment_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_object, 1, &_strFragment, NULL);
	glCompileShader(fragment_shader_object);

	GLuint program = glCreateProgram();
	glBindAttribLocation(program, SC_POSITION, "aPosition");
	glBindAttribLocation(program, SC_TEXCOORD0, "aTexCoord");
	glAttachShader(program, vertex_shader_object);
	glAttachShader(program, fragment_shader_object);
	glLinkProgram(program);
	glDeleteShader(vertex_shader_object);
	glDeleteShader(fragment_shader_object);
	return program;
}


void PostProcessor::init()
{
	m_bloomProgram = _createShaderProgram(vertexShader, bloomShader);
	m_copyProgram = _createShaderProgram(vertexShader, copyShader);

	m_pTexture = textureCache().addFrameBufferTexture();
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = TRUE;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = video().getWidth();
	m_pTexture->realHeight = video().getHeight();
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight * 4;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_pTexture->realWidth, m_pTexture->realHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTexture->glName, 0);
	GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);
	// check if everything is OK
	assert(checkFBO());
	assert(!isGLError());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

}

void PostProcessor::destroy()
{
	if (m_bloomProgram != 0)
		glDeleteProgram(m_bloomProgram);

	if (m_copyProgram != 0)
		glDeleteProgram(m_copyProgram);

	if (m_FBO != 0)
		glDeleteFramebuffers(1, &m_FBO);

	if (m_pTexture != NULL)
		textureCache().removeFrameBufferTexture(m_pTexture);
}

PostProcessor & PostProcessor::get()
{
	static PostProcessor processor;
	return processor;
}

void _setGLState() {
	glDisable(GL_SCISSOR_TEST);
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
}

void PostProcessor::processTexture(CachedTexture * _pTexture)
{
	_setGLState();

	textureCache().activateTexture(0, _pTexture);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glUseProgram(m_bloomProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	textureCache().activateTexture(0, m_pTexture);
	GLuint copyFBO = 0;
	glGenFramebuffers(1, &copyFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, copyFBO);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _pTexture->glName, 0);
	assert(checkFBO());
	glUseProgram(m_copyProgram);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &copyFBO);
	video().getRender().dropRenderState();
	glUseProgram(0);
	gSP.changed = CHANGED_VIEWPORT | CHANGED_TEXTURE;
}
