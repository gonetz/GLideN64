#include "../OpenGL.h"

#ifndef EGL
#ifdef OS_WINDOWS
#define glGetProcAddress wglGetProcAddress
#else
#error Must not compile for this OS
#endif
#else // EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#define glGetProcAddress eglGetProcAddress
//GL Fucntions
PFNGLBLENDFUNCPROC glBlendFunc;
PFNGLPIXELSTOREIPROC glPixelStorei;
PFNGLCLEARCOLORPROC glClearColor;
PFNGLCULLFACEPROC glCullFace;
PFNGLDEPTHFUNCPROC glDepthFunc;
PFNGLDEPTHMASKPROC glDepthMask;
PFNGLDISABLEPROC glDisable;
PFNGLENABLEPROC glEnable;
PFNGLPOLYGONOFFSETPROC glPolygonOffset;
PFNGLSCISSORPROC glScissor;
PFNGLVIEWPORTPROC glViewport;
PFNGLBINDTEXTUREPROC glBindTexture;
PFNGLTEXIMAGE2DPROC glTexImage2D;
PFNGLTEXPARAMETERIPROC glTexParameteri;
PFNGLGETINTEGERVPROC glGetIntegerv;
PFNGLGETSTRINGPROC glGetString;
PFNGLREADPIXELSPROC glReadPixels;
PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
PFNGLDRAWARRAYSPROC glDrawArrays;
PFNGLGETERRORPROC glGetError;
PFNGLDRAWELEMENTSPROC glDrawElements;
PFNGLLINEWIDTHPROC glLineWidth;
PFNGLCLEARPROC glClear;
PFNGLGETFLOATVPROC glGetFloatv;
PFNGLDELETETEXTURESPROC glDeleteTextures;
PFNGLGENTEXTURESPROC glGenTextures;
PFNGLTEXPARAMETERFPROC glTexParameterf;

#endif // EGL

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
PFNGLUNIFORM4IPROC glUniform4i;
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
PFNGLBLENDCOLORPROC glBlendColor;

PFNGLDRAWBUFFERSPROC glDrawBuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexStorage2DMultisample;
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
PFNGLGETSTRINGIPROC glGetStringi;
PFNGLINVALIDATEFRAMEBUFFERPROC glInvalidateFramebuffer;
PFNGLBUFFERSTORAGEPROC glBufferStorage;
PFNGLFENCESYNCPROC glFenceSync;
PFNGLCLIENTWAITSYNCPROC glClientWaitSync;
PFNGLDELETESYNCPROC glDeleteSync;

PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv;
PFNGLGETUNIFORMINDICESPROC glGetUniformIndices;
PFNGLGETACTIVEUNIFORMSIVPROC glGetActiveUniformsiv;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLBUFFERSUBDATAPROC glBufferSubData;

PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
PFNGLPROGRAMBINARYPROC glProgramBinary;
PFNGLPROGRAMPARAMETERIPROC glProgramParameteri;

PFNGLTEXSTORAGE2DPROC glTexStorage2D;

void initGLFunctions()
{
#ifdef EGL
	glBlendFunc = (PFNGLBLENDFUNCPROC)glGetProcAddress("glBlendFunc");
	glPixelStorei = (PFNGLPIXELSTOREIPROC)glGetProcAddress("glPixelStorei");
	glClearColor = (PFNGLCLEARCOLORPROC)glGetProcAddress("glClearColor");
	glCullFace = (PFNGLCULLFACEPROC)glGetProcAddress("glCullFace");
	glDepthFunc = (PFNGLDEPTHFUNCPROC)glGetProcAddress("glDepthFunc");
	glDepthMask = (PFNGLDEPTHMASKPROC)glGetProcAddress("glDepthMask");
	glDisable = (PFNGLDISABLEPROC)glGetProcAddress("glDisable");
	glEnable = (PFNGLENABLEPROC)glGetProcAddress("glEnable");
	glPolygonOffset = (PFNGLPOLYGONOFFSETPROC)glGetProcAddress("glPolygonOffset");
	glScissor = (PFNGLSCISSORPROC)glGetProcAddress("glScissor");
	glViewport = (PFNGLVIEWPORTPROC)glGetProcAddress("glViewport");
	glBindTexture = (PFNGLBINDTEXTUREPROC)glGetProcAddress("glBindTexture");
	glTexImage2D = (PFNGLTEXIMAGE2DPROC)glGetProcAddress("glTexImage2D");
	glTexParameteri = (PFNGLTEXPARAMETERIPROC)glGetProcAddress("glTexParameteri");
	glGetIntegerv = (PFNGLGETINTEGERVPROC)glGetProcAddress("glGetIntegerv");
	glGetString = (PFNGLGETSTRINGPROC)glGetProcAddress("glGetString");
	glReadPixels = (PFNGLREADPIXELSPROC)glGetProcAddress("glReadPixels");
	glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)glGetProcAddress("glTexSubImage2D");
	glDrawArrays = (PFNGLDRAWARRAYSPROC)glGetProcAddress("glDrawArrays");
	glGetError = (PFNGLGETERRORPROC)glGetProcAddress("glGetError");
	glDrawElements = (PFNGLDRAWELEMENTSPROC)glGetProcAddress("glDrawElements");
	glLineWidth = (PFNGLLINEWIDTHPROC)glGetProcAddress("glLineWidth");
	glClear = (PFNGLCLEARPROC)glGetProcAddress("glClear");
	glGetFloatv = (PFNGLGETFLOATVPROC)glGetProcAddress("glGetFloatv");
	glDeleteTextures = (PFNGLDELETETEXTURESPROC)glGetProcAddress("glDeleteTextures");
	glGenTextures = (PFNGLGENTEXTURESPROC)glGetProcAddress("glGenTextures");
	glTexParameterf = (PFNGLTEXPARAMETERFPROC)glGetProcAddress("glTexParameterf");
#endif

	glCreateShader = (PFNGLCREATESHADERPROC)glGetProcAddress("glCreateShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)glGetProcAddress("glCompileShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)glGetProcAddress("glShaderSource");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)glGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)glGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)glGetProcAddress("glLinkProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)glGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glGetProcAddress("glGetUniformLocation");
	glUniform1i = (PFNGLUNIFORM1IPROC)glGetProcAddress("glUniform1i");
	glUniform1f = (PFNGLUNIFORM1FPROC)glGetProcAddress("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)glGetProcAddress("glUniform2f");
	glUniform2i = (PFNGLUNIFORM2IPROC)glGetProcAddress("glUniform2i");
	glUniform4i = (PFNGLUNIFORM4IPROC)glGetProcAddress("glUniform4i");
	glUniform4f = (PFNGLUNIFORM4FPROC)glGetProcAddress("glUniform4f");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)glGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)glGetProcAddress("glUniform4fv");
	glDetachShader = (PFNGLDETACHSHADERPROC)glGetProcAddress("glDetachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)glGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glGetProcAddress("glDeleteProgram");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)glGetProcAddress("glGetShaderiv");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glGetProcAddress("glGetProgramiv");

	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)glGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glGetProcAddress("glVertexAttribPointer");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)glGetProcAddress("glBindAttribLocation");
	glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)glGetProcAddress("glVertexAttrib4f");
	glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)glGetProcAddress("glVertexAttrib4fv");

	glActiveTexture	= (PFNGLACTIVETEXTUREPROC)glGetProcAddress( "glActiveTexture" );
	glDepthRangef = (PFNGLDEPTHRANGEFPROC)glGetProcAddress( "glDepthRangef" );
	glClearDepthf = (PFNGLCLEARDEPTHFPROC)glGetProcAddress( "glClearDepthf" );
	glBlendColor = (PFNGLBLENDCOLORPROC)glGetProcAddress( "glBlendColor" );

	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)glGetProcAddress( "glDrawBuffers" );
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)glGetProcAddress( "glBindFramebuffer" );
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)glGetProcAddress( "glDeleteFramebuffers" );
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)glGetProcAddress( "glGenFramebuffers" );
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)glGetProcAddress( "glFramebufferTexture2D" );
	glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)glGetProcAddress("glTexImage2DMultisample");
	glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)glGetProcAddress("glTexStorage2DMultisample");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)glGetProcAddress( "glGenRenderbuffers" );
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)glGetProcAddress( "glBindRenderbuffer" );
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)glGetProcAddress( "glRenderbufferStorage" );
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glGetProcAddress( "glFramebufferRenderbuffer" );
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)glGetProcAddress( "glDeleteRenderbuffers" );
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glGetProcAddress( "glCheckFramebufferStatus" );
	glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)glGetProcAddress( "glBlitFramebuffer" );
	glGenBuffers = (PFNGLGENBUFFERSPROC)glGetProcAddress( "glGenBuffers" );
	glBindBuffer = (PFNGLBINDBUFFERPROC)glGetProcAddress( "glBindBuffer" );
	glBufferData = (PFNGLBUFFERDATAPROC)glGetProcAddress( "glBufferData" );
	glMapBuffer = (PFNGLMAPBUFFERPROC)glGetProcAddress( "glMapBuffer" );
	glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)glGetProcAddress("glMapBufferRange");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)glGetProcAddress( "glUnmapBuffer" );
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glGetProcAddress( "glDeleteBuffers" );
	glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)glGetProcAddress( "glBindImageTexture" );
	glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)glGetProcAddress( "glMemoryBarrier" );
	glGetStringi = (PFNGLGETSTRINGIPROC)glGetProcAddress("glGetStringi");
	glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)glGetProcAddress("glInvalidateFramebuffer");
	glBufferStorage = (PFNGLBUFFERSTORAGEPROC)glGetProcAddress("glBufferStorage"); //For GLES 3.0 and 3.1 this is glBufferStorageEXT
	glFenceSync = (PFNGLFENCESYNCPROC)glGetProcAddress("glFenceSync");
	glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)glGetProcAddress("glClientWaitSync");
	glDeleteSync = (PFNGLDELETESYNCPROC)glGetProcAddress("glDeleteSync");

	glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)glGetProcAddress("glGetUniformBlockIndex");
	glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)glGetProcAddress("glUniformBlockBinding");
	glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)glGetProcAddress("glGetActiveUniformBlockiv");
	glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)glGetProcAddress("glGetUniformIndices");
	glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)glGetProcAddress("glGetActiveUniformsiv");
	glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)glGetProcAddress("glBindBufferBase");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)glGetProcAddress("glBufferSubData");

	glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)glGetProcAddress("glGetProgramBinary");
	glProgramBinary = (PFNGLPROGRAMBINARYPROC)glGetProcAddress("glProgramBinary");
	glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)glGetProcAddress("glProgramParameteri");

	glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)glGetProcAddress("glTexStorage2D");
}
