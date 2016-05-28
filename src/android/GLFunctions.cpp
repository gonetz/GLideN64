#include "../OpenGL.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

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

PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
PFNGLPROGRAMBINARYPROC glProgramBinary;
PFNGLPROGRAMPARAMETERIPROC glProgramParameteri;

void initGLFunctions()
{
#ifdef EGL
	glBlendFunc = (PFNGLBLENDFUNCPROC)eglGetProcAddress("glBlendFunc");
	glPixelStorei = (PFNGLPIXELSTOREIPROC)eglGetProcAddress("glPixelStorei");
	glClearColor = (PFNGLCLEARCOLORPROC)eglGetProcAddress("glClearColor");
	glCullFace = (PFNGLCULLFACEPROC)eglGetProcAddress("glCullFace");
	glDepthFunc = (PFNGLDEPTHFUNCPROC)eglGetProcAddress("glDepthFunc");
	glDepthMask = (PFNGLDEPTHMASKPROC)eglGetProcAddress("glDepthMask");
	glDisable = (PFNGLDISABLEPROC)eglGetProcAddress("glDisable");
	glEnable = (PFNGLENABLEPROC)eglGetProcAddress("glEnable");
	glPolygonOffset = (PFNGLPOLYGONOFFSETPROC)eglGetProcAddress("glPolygonOffset");
	glScissor = (PFNGLSCISSORPROC)eglGetProcAddress("glScissor");
	glViewport = (PFNGLVIEWPORTPROC)eglGetProcAddress("glViewport");
	glBindTexture = (PFNGLBINDTEXTUREPROC)eglGetProcAddress("glBindTexture");
	glTexImage2D = (PFNGLTEXIMAGE2DPROC)eglGetProcAddress("glTexImage2D");
	glTexParameteri = (PFNGLTEXPARAMETERIPROC)eglGetProcAddress("glTexParameteri");
	glGetIntegerv = (PFNGLGETINTEGERVPROC)eglGetProcAddress("glGetIntegerv");
	glGetString = (PFNGLGETSTRINGPROC)eglGetProcAddress("glGetString");
	glReadPixels = (PFNGLREADPIXELSPROC)eglGetProcAddress("glReadPixels");
	glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC)eglGetProcAddress("glTexSubImage2D");
	glDrawArrays = (PFNGLDRAWARRAYSPROC)eglGetProcAddress("glDrawArrays");
	glGetError = (PFNGLGETERRORPROC)eglGetProcAddress("glGetError");
	glDrawElements = (PFNGLDRAWELEMENTSPROC)eglGetProcAddress("glDrawElements");
	glLineWidth = (PFNGLLINEWIDTHPROC)eglGetProcAddress("glLineWidth");
	glClear = (PFNGLCLEARPROC)eglGetProcAddress("glClear");
	glGetFloatv = (PFNGLGETFLOATVPROC)eglGetProcAddress("glGetFloatv");
	glDeleteTextures = (PFNGLDELETETEXTURESPROC)eglGetProcAddress("glDeleteTextures");
	glGenTextures = (PFNGLGENTEXTURESPROC)eglGetProcAddress("glGenTextures");
	glTexParameterf = (PFNGLTEXPARAMETERFPROC)eglGetProcAddress("glTexParameterf");
#endif

	glCreateShader = (PFNGLCREATESHADERPROC)eglGetProcAddress("glCreateShader");
	glCompileShader = (PFNGLCOMPILESHADERPROC)eglGetProcAddress("glCompileShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)eglGetProcAddress("glShaderSource");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)eglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)eglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)eglGetProcAddress("glLinkProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)eglGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)eglGetProcAddress("glGetUniformLocation");
	glUniform1i = (PFNGLUNIFORM1IPROC)eglGetProcAddress("glUniform1i");
	glUniform1f = (PFNGLUNIFORM1FPROC)eglGetProcAddress("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)eglGetProcAddress("glUniform2f");
	glUniform2i = (PFNGLUNIFORM2IPROC)eglGetProcAddress("glUniform2i");
	glUniform4i = (PFNGLUNIFORM4IPROC)eglGetProcAddress("glUniform4i");
	glUniform4f = (PFNGLUNIFORM4FPROC)eglGetProcAddress("glUniform4f");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)eglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)eglGetProcAddress("glUniform4fv");
	glDetachShader = (PFNGLDETACHSHADERPROC)eglGetProcAddress("glDetachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)eglGetProcAddress("glDeleteShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)eglGetProcAddress("glDeleteProgram");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)eglGetProcAddress("glGetProgramInfoLog");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)eglGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)eglGetProcAddress("glGetShaderiv");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)eglGetProcAddress("glGetProgramiv");

	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)eglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)eglGetProcAddress("glDisableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)eglGetProcAddress("glVertexAttribPointer");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)eglGetProcAddress("glBindAttribLocation");
	glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)eglGetProcAddress("glVertexAttrib4f");
	glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)eglGetProcAddress("glVertexAttrib4fv");

	glActiveTexture	= (PFNGLACTIVETEXTUREPROC)eglGetProcAddress( "glActiveTexture" );
	glDepthRangef = (PFNGLDEPTHRANGEFPROC)eglGetProcAddress( "glDepthRangef" );
	glClearDepthf = (PFNGLCLEARDEPTHFPROC)eglGetProcAddress( "glClearDepthf" );
	glBlendColor = (PFNGLBLENDCOLORPROC)eglGetProcAddress( "glBlendColor" );

	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)eglGetProcAddress( "glDrawBuffers" );
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)eglGetProcAddress( "glBindFramebuffer" );
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)eglGetProcAddress( "glDeleteFramebuffers" );
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)eglGetProcAddress( "glGenFramebuffers" );
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)eglGetProcAddress( "glFramebufferTexture2D" );
	glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)eglGetProcAddress("glTexImage2DMultisample");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)eglGetProcAddress( "glGenRenderbuffers" );
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)eglGetProcAddress( "glBindRenderbuffer" );
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)eglGetProcAddress( "glRenderbufferStorage" );
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)eglGetProcAddress( "glFramebufferRenderbuffer" );
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)eglGetProcAddress( "glDeleteRenderbuffers" );
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)eglGetProcAddress( "glCheckFramebufferStatus" );
	glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)eglGetProcAddress( "glBlitFramebuffer" );
	glGenBuffers = (PFNGLGENBUFFERSPROC)eglGetProcAddress( "glGenBuffers" );
	glBindBuffer = (PFNGLBINDBUFFERPROC)eglGetProcAddress( "glBindBuffer" );
	glBufferData = (PFNGLBUFFERDATAPROC)eglGetProcAddress( "glBufferData" );
	glMapBuffer = (PFNGLMAPBUFFERPROC)eglGetProcAddress( "glMapBuffer" );
	glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)eglGetProcAddress("glMapBufferRange");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)eglGetProcAddress( "glUnmapBuffer" );
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)eglGetProcAddress( "glDeleteBuffers" );
	glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)eglGetProcAddress( "glBindImageTexture" );
	glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)eglGetProcAddress( "glMemoryBarrier" );

	glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)eglGetProcAddress("glGetUniformBlockIndex");
	glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)eglGetProcAddress("glUniformBlockBinding");
	glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)eglGetProcAddress("glGetActiveUniformBlockiv");
	glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)eglGetProcAddress("glGetUniformIndices");
	glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)eglGetProcAddress("glGetActiveUniformsiv");
	glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)eglGetProcAddress("glBindBufferBase");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)eglGetProcAddress("glBufferSubData");

	glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)eglGetProcAddress("glGetProgramBinary");
	glProgramBinary = (PFNGLPROGRAMBINARYPROC)eglGetProcAddress("glProgramBinary");
	glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)eglGetProcAddress("glProgramParameteri");
}
