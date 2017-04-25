#ifndef GLFUNCTIONS_H
#define GLFUNCTIONS_H

#ifdef OS_WINDOWS
#include <windows.h>
#elif defined(OS_LINUX)
#include <winlnxdefs.h>
#endif

#ifdef EGL
#include <GL/glcorearb.h>
#elif defined(OS_MAC_OS_X)
#include <OpenGL/OpenGL.h>
#include <stddef.h>
#include <OpenGL/gl3.h>

#elif defined(OS_IOS)
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
// Add missing type defintions for iOS
typedef double GLclampd;
typedef double GLdouble;
// These will get redefined by other GL headers.
#undef GL_DRAW_FRAMEBUFFER_BINDING
#undef GL_COPY_READ_BUFFER_BINDING
#undef GL_COPY_WRITE_BUFFER_BINDING
#include <GL/glcorearb.h>
#else
#include <GL/gl.h>
#endif

#include <GL/glext.h>
#include <stdexcept>
#include <sstream>
#include "Log.h"

#define IS_GL_FUNCTION_VALID(proc_name) g_##proc_name != nullptr

#if defined(EGL) || defined(OS_IOS)
extern PFNGLBLENDFUNCPROC g_glBlendFunc;
extern PFNGLPIXELSTOREIPROC g_glPixelStorei;
extern PFNGLCLEARCOLORPROC g_glClearColor;
extern PFNGLCULLFACEPROC g_glCullFace;
extern PFNGLDEPTHFUNCPROC g_glDepthFunc;
extern PFNGLDEPTHMASKPROC g_glDepthMask;
extern PFNGLDISABLEPROC g_glDisable;
extern PFNGLENABLEPROC g_glEnable;
extern PFNGLPOLYGONOFFSETPROC g_glPolygonOffset;
extern PFNGLSCISSORPROC g_glScissor;
extern PFNGLVIEWPORTPROC g_glViewport;
extern PFNGLBINDTEXTUREPROC g_glBindTexture;
extern PFNGLTEXIMAGE2DPROC g_glTexImage2D;
extern PFNGLTEXPARAMETERIPROC g_glTexParameteri;
extern PFNGLGETINTEGERVPROC g_glGetIntegerv;
extern PFNGLGETSTRINGPROC g_glGetString;
extern PFNGLREADPIXELSPROC g_glReadPixels;
extern PFNGLTEXSUBIMAGE2DPROC g_glTexSubImage2D;
extern PFNGLDRAWARRAYSPROC g_glDrawArrays;
extern PFNGLGETERRORPROC g_glGetError;
extern PFNGLDRAWELEMENTSPROC g_glDrawElements;
extern PFNGLLINEWIDTHPROC g_glLineWidth;
extern PFNGLCLEARPROC g_glClear;
extern PFNGLGETFLOATVPROC g_glGetFloatv;
extern PFNGLDELETETEXTURESPROC g_glDeleteTextures;
extern PFNGLGENTEXTURESPROC g_glGenTextures;
extern PFNGLTEXPARAMETERFPROC g_glTexParameterf;
extern PFNGLACTIVETEXTUREPROC g_glActiveTexture;
extern PFNGLBLENDCOLORPROC g_glBlendColor;
extern PFNGLREADBUFFERPROC g_glReadBuffer;
extern PFNGLFINISHPROC g_glFinish;
#else
#define g_glBlendFunc(...) ::glBlendFunc(__VA_ARGS__)
#define g_glPixelStorei(...) ::glPixelStorei(__VA_ARGS__)
#define g_glClearColor(...) ::glClearColor(__VA_ARGS__)
#define g_glCullFace(...) ::glCullFace(__VA_ARGS__)
#define g_glDepthFunc(...) ::glDepthFunc(__VA_ARGS__)
#define g_glDepthMask(...) ::glDepthMask(__VA_ARGS__)
#define g_glDisable(...) ::glDisable(__VA_ARGS__)
#define g_glEnable(...) ::glEnable(__VA_ARGS__)
#define g_glPolygonOffset(...) ::glPolygonOffset(__VA_ARGS__)
#define g_glScissor(...) ::glScissor(__VA_ARGS__)
#define g_glViewport(...) ::glViewport(__VA_ARGS__)
#define g_glBindTexture(...) ::glBindTexture(__VA_ARGS__)
#define g_glTexImage2D(...) ::glTexImage2D(__VA_ARGS__)
#define g_glTexParameteri(...) ::glTexParameteri(__VA_ARGS__)
#define g_glGetIntegerv(...) ::glGetIntegerv(__VA_ARGS__)
#define g_glGetString(...) ::glGetString(__VA_ARGS__)
#define g_glReadPixels(...) ::glReadPixels(__VA_ARGS__)
#define g_glTexSubImage2D(...) ::glTexSubImage2D(__VA_ARGS__)
#define g_glDrawArrays(...) ::glDrawArrays(__VA_ARGS__)
#define g_glGetError(...) ::glGetError(__VA_ARGS__)
#define g_glDrawElements(...) ::glDrawElements(__VA_ARGS__)
#define g_glLineWidth(...) ::glLineWidth(__VA_ARGS__)
#define g_glClear(...) ::glClear(__VA_ARGS__)
#define g_glGetFloatv(...) ::glGetFloatv(__VA_ARGS__)
#define g_glDeleteTextures(...) ::glDeleteTextures(__VA_ARGS__)
#define g_glGenTextures(...) ::glGenTextures(__VA_ARGS__)
#define g_glTexParameterf(...) ::glTexParameterf(__VA_ARGS__)
#ifndef OS_WINDOWS
#define g_glActiveTexture(...) ::glActiveTexture(__VA_ARGS__)
#define g_glBlendColor(...) ::glBlendColor(__VA_ARGS__)
#endif
#define g_glReadBuffer(...) ::glReadBuffer(__VA_ARGS__)
#define g_glFinish(...) ::glFinish(__VA_ARGS__)
#endif

#ifdef OS_WINDOWS
extern PFNGLACTIVETEXTUREPROC g_glActiveTexture;
extern PFNGLBLENDCOLORPROC g_glBlendColor;
#endif

extern PFNGLCREATESHADERPROC g_glCreateShader;
extern PFNGLCOMPILESHADERPROC g_glCompileShader;
extern PFNGLSHADERSOURCEPROC g_glShaderSource;
extern PFNGLCREATEPROGRAMPROC g_glCreateProgram;
extern PFNGLATTACHSHADERPROC g_glAttachShader;
extern PFNGLLINKPROGRAMPROC g_glLinkProgram;
extern PFNGLUSEPROGRAMPROC g_glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC g_glGetUniformLocation;
extern PFNGLUNIFORM1IPROC g_glUniform1i;
extern PFNGLUNIFORM1FPROC g_glUniform1f;
extern PFNGLUNIFORM2FPROC g_glUniform2f;
extern PFNGLUNIFORM2IPROC g_glUniform2i;
extern PFNGLUNIFORM4IPROC g_glUniform4i;

extern PFNGLUNIFORM4FPROC g_glUniform4f;
extern PFNGLUNIFORM3FVPROC g_glUniform3fv;
extern PFNGLUNIFORM4FVPROC g_glUniform4fv;
extern PFNGLDETACHSHADERPROC g_glDetachShader;
extern PFNGLDELETESHADERPROC g_glDeleteShader;
extern PFNGLDELETEPROGRAMPROC g_glDeleteProgram;
extern PFNGLGETPROGRAMINFOLOGPROC g_glGetProgramInfoLog;
extern PFNGLGETSHADERINFOLOGPROC g_glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC g_glGetShaderiv;
extern PFNGLGETPROGRAMIVPROC g_glGetProgramiv;

extern PFNGLENABLEVERTEXATTRIBARRAYPROC g_glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC g_glDisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC g_glVertexAttribPointer;
extern PFNGLBINDATTRIBLOCATIONPROC g_glBindAttribLocation;
extern PFNGLVERTEXATTRIB1FPROC g_glVertexAttrib1f;
extern PFNGLVERTEXATTRIB4FPROC g_glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC g_glVertexAttrib4fv;

extern PFNGLDEPTHRANGEFPROC g_glDepthRangef;
extern PFNGLCLEARDEPTHFPROC g_glClearDepthf;

extern PFNGLDRAWBUFFERSPROC g_glDrawBuffers;
extern PFNGLGENFRAMEBUFFERSPROC g_glGenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC g_glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC g_glDeleteFramebuffers;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC g_glFramebufferTexture2D;
extern PFNGLTEXIMAGE2DMULTISAMPLEPROC g_glTexImage2DMultisample;
extern PFNGLTEXSTORAGE2DMULTISAMPLEPROC g_glTexStorage2DMultisample;
extern PFNGLGENRENDERBUFFERSPROC g_glGenRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC g_glBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEPROC g_glRenderbufferStorage;
extern PFNGLDELETERENDERBUFFERSPROC g_glDeleteRenderbuffers;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC g_glFramebufferRenderbuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC g_glCheckFramebufferStatus;
extern PFNGLBLITFRAMEBUFFERPROC g_glBlitFramebuffer;
extern PFNGLGENVERTEXARRAYSPROC g_glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC g_glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC g_glDeleteVertexArrays;
extern PFNGLGENBUFFERSPROC g_glGenBuffers;
extern PFNGLBINDBUFFERPROC g_glBindBuffer;
extern PFNGLBUFFERDATAPROC g_glBufferData;
extern PFNGLMAPBUFFERPROC g_glMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC g_glMapBufferRange;
extern PFNGLUNMAPBUFFERPROC g_glUnmapBuffer;
extern PFNGLDELETEBUFFERSPROC g_glDeleteBuffers;
extern PFNGLBINDIMAGETEXTUREPROC g_glBindImageTexture;
extern PFNGLMEMORYBARRIERPROC g_glMemoryBarrier;
extern PFNGLGETSTRINGIPROC g_glGetStringi;
extern PFNGLINVALIDATEFRAMEBUFFERPROC g_glInvalidateFramebuffer;
extern PFNGLBUFFERSTORAGEPROC g_glBufferStorage;
extern PFNGLFENCESYNCPROC g_glFenceSync;
extern PFNGLCLIENTWAITSYNCPROC g_glClientWaitSync;
extern PFNGLDELETESYNCPROC g_glDeleteSync;

extern PFNGLGETUNIFORMBLOCKINDEXPROC g_glGetUniformBlockIndex;
extern PFNGLUNIFORMBLOCKBINDINGPROC g_glUniformBlockBinding;
extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC g_glGetActiveUniformBlockiv;
extern PFNGLGETUNIFORMINDICESPROC g_glGetUniformIndices;
extern PFNGLGETACTIVEUNIFORMSIVPROC g_glGetActiveUniformsiv;
extern PFNGLBINDBUFFERBASEPROC g_glBindBufferBase;
extern PFNGLBUFFERSUBDATAPROC g_glBufferSubData;

extern PFNGLGETPROGRAMBINARYPROC g_glGetProgramBinary;
extern PFNGLPROGRAMBINARYPROC g_glProgramBinary;
extern PFNGLPROGRAMPARAMETERIPROC g_glProgramParameteri;

extern PFNGLTEXSTORAGE2DPROC g_glTexStorage2D;
extern PFNGLTEXTURESTORAGE2DPROC g_glTextureStorage2D;
extern PFNGLTEXTURESUBIMAGE2DPROC g_glTextureSubImage2D;
extern PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC g_glTextureStorage2DMultisample;
extern PFNGLTEXTUREPARAMETERIPROC g_glTextureParameteri;
extern PFNGLTEXTUREPARAMETERFPROC g_glTextureParameterf;
extern PFNGLCREATETEXTURESPROC g_glCreateTextures;
extern PFNGLCREATEBUFFERSPROC g_glCreateBuffers;
extern PFNGLCREATEFRAMEBUFFERSPROC g_glCreateFramebuffers;
extern PFNGLNAMEDFRAMEBUFFERTEXTUREPROC g_glNamedFramebufferTexture;
extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC g_glDrawRangeElementsBaseVertex;
extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC g_glFlushMappedBufferRange;
extern PFNGLTEXTUREBARRIERPROC g_glTextureBarrier;
extern PFNGLTEXTUREBARRIERNVPROC g_glTextureBarrierNV;
extern PFNGLCLEARBUFFERFVPROC g_glClearBufferfv;
extern PFNGLENABLEIPROC g_glEnablei;
extern PFNGLDISABLEIPROC g_glDisablei;

void initGLFunctions();

#endif // GLFUNCTIONS_H
