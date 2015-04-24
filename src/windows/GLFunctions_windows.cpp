#include "GLideN64_Windows.h"
#include "../OpenGL.h"

// GLSL functions
PFNGLCREATESHADERPROC glCreateShader;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETPROGRAMIVPROC glGetProgramiv;

PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;

// multitexture functions
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLDEPTHRANGEFPROC glDepthRangef;
PFNGLCLEARDEPTHFPROC glClearDepthf;

PFNGLDRAWBUFFERSPROC glDrawBuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;
PFNGLMEMORYBARRIERPROC glMemoryBarrier;

PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv;
PFNGLGETUNIFORMINDICESPROC glGetUniformIndices;
PFNGLGETACTIVEUNIFORMSIVPROC glGetActiveUniformsiv;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLBUFFERSUBDATAPROC glBufferSubData;

void initGLFunctions()
{
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
	glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");

	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
	glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)wglGetProcAddress("glVertexAttrib4f");
	glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib4fv");

	glActiveTexture	= (PFNGLACTIVETEXTUREPROC)wglGetProcAddress( "glActiveTexture" );
	glDepthRangef = (PFNGLDEPTHRANGEFPROC)wglGetProcAddress( "glDepthRangef" );
	glClearDepthf = (PFNGLCLEARDEPTHFPROC)wglGetProcAddress( "glClearDepthf" );

	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress( "glDrawBuffers" );
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress( "glBindFramebuffer" );
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress( "glDeleteFramebuffers" );
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress( "glGenFramebuffers" );
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress( "glFramebufferTexture2D" );
	glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)wglGetProcAddress("glTexImage2DMultisample");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress( "glGenRenderbuffers" );
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress( "glBindRenderbuffer" );
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress( "glRenderbufferStorage" );
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress( "glFramebufferRenderbuffer" );
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress( "glDeleteRenderbuffers" );
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress( "glCheckFramebufferStatus" );
	glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)wglGetProcAddress( "glBlitFramebuffer" );
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress( "glGenBuffers" );
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress( "glBindBuffer" );
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress( "glBufferData" );
	glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress( "glMapBuffer" );
	glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)wglGetProcAddress("glMapBufferRange");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress( "glUnmapBuffer" );
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress( "glDeleteBuffers" );
	glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)wglGetProcAddress( "glBindImageTexture" );
	glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)wglGetProcAddress( "glMemoryBarrier" );

	glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)wglGetProcAddress("glGetUniformBlockIndex");
	glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)wglGetProcAddress("glUniformBlockBinding");
	glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)wglGetProcAddress("glGetActiveUniformBlockiv");
	glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)wglGetProcAddress("glGetUniformIndices");
	glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)wglGetProcAddress("glGetActiveUniformsiv");
	glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
}
