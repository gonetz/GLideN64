#include "GLFunctions.h"

#ifdef GL_GLEXT_PROTOTYPES
GLValidFunctions g_GLValidFunctions;

void initGLFunctions()
{
	g_GLValidFunctions = { 0 };
}
#else
#include "Config.h"

extern "C"
{
#define GL_APIENTRY __stdcall
	void GL_APIENTRY GL_BlendFunc(GLenum sfactor, GLenum dfactor);
	void GL_APIENTRY GL_BlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void GL_APIENTRY GL_PixelStorei(GLenum pname, GLint param);
	void GL_APIENTRY GL_ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void GL_APIENTRY GL_CullFace(GLenum mode);
	void GL_APIENTRY GL_DepthFunc(GLenum func);
	void GL_APIENTRY GL_DepthMask(GLboolean flag);
	void GL_APIENTRY GL_Disable(GLenum cap);
	void GL_APIENTRY GL_Enable(GLenum cap);
	void GL_APIENTRY GL_PolygonOffset(GLfloat factor, GLfloat units);
	void GL_APIENTRY GL_Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
	void GL_APIENTRY GL_Viewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void GL_APIENTRY GL_BindTexture(GLenum target, GLuint texture);
	void GL_APIENTRY GL_TexImage2D(GLenum target,
		GLint level,
		GLint internalformat,
		GLsizei width,
		GLsizei height,
		GLint border,
		GLenum format,
		GLenum type,
		const void* pixels);
	void GL_APIENTRY GL_TexParameteri(GLenum target, GLenum pname, GLint param);
	void GL_APIENTRY GL_GetIntegerv(GLenum pname, GLint* data);
	const GLubyte* GL_APIENTRY GL_GetString(GLenum name);
	void GL_APIENTRY GL_ReadPixels(GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		void* pixels);
	void GL_APIENTRY GL_TexSubImage2D(GLenum target,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		const void* pixels);
	void GL_APIENTRY GL_DrawArrays(GLenum mode, GLint first, GLsizei count);
	GLenum GL_APIENTRY GL_GetError();
	void GL_APIENTRY GL_DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
	void GL_APIENTRY GL_LineWidth(GLfloat width);
	void GL_APIENTRY GL_Clear(GLbitfield mask);
	void GL_APIENTRY GL_GetFloatv(GLenum pname, GLfloat* data);
	void GL_APIENTRY GL_DeleteTextures(GLsizei n, const GLuint* textures);
	void GL_APIENTRY GL_GenTextures(GLsizei n, GLuint* textures);
	void GL_APIENTRY GL_TexParameterf(GLenum target, GLenum pname, GLfloat param);
	void GL_APIENTRY GL_ActiveTexture(GLenum texture);
	void GL_APIENTRY GL_BlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void GL_APIENTRY GL_ReadBuffer(GLenum src);
	void GL_APIENTRY GL_Finish();
	GLuint GL_APIENTRY GL_CreateShader(GLenum type);
	void GL_APIENTRY GL_CompileShader(GLuint shader);
	void GL_APIENTRY GL_ShaderSource(GLuint shader,
		GLsizei count,
		const GLchar* const* string,
		const GLint* length);
	GLuint GL_APIENTRY GL_CreateProgram();
	void GL_APIENTRY GL_AttachShader(GLuint program, GLuint shader);
	void GL_APIENTRY GL_LinkProgram(GLuint program);
	void GL_APIENTRY GL_UseProgram(GLuint program);
	GLint GL_APIENTRY GL_GetUniformLocation(GLuint program, const GLchar* name);
	void GL_APIENTRY GL_Uniform1i(GLint location, GLint v0);
	void GL_APIENTRY GL_Uniform1f(GLint location, GLfloat v0);
	void GL_APIENTRY GL_Uniform2f(GLint location, GLfloat v0, GLfloat v1);
	void GL_APIENTRY GL_Uniform2i(GLint location, GLint v0, GLint v1);
	void GL_APIENTRY GL_Uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void GL_APIENTRY GL_Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void GL_APIENTRY GL_Uniform3fv(GLint location, GLsizei count, const GLfloat* value);
	void GL_APIENTRY GL_Uniform4fv(GLint location, GLsizei count, const GLfloat* value);
	void GL_APIENTRY GL_DetachShader(GLuint program, GLuint shader);
	void GL_APIENTRY GL_DeleteShader(GLuint shader);
	void GL_APIENTRY GL_DeleteProgram(GLuint program);
	void GL_APIENTRY GL_GetProgramInfoLog(GLuint program,
		GLsizei bufSize,
		GLsizei* length,
		GLchar* infoLog);
	void GL_APIENTRY GL_GetShaderInfoLog(GLuint shader,
		GLsizei bufSize,
		GLsizei* length,
		GLchar* infoLog);
	void GL_APIENTRY GL_GetShaderiv(GLuint shader, GLenum pname, GLint* params);
	void GL_APIENTRY GL_GetProgramiv(GLuint program, GLenum pname, GLint* params);
	void GL_APIENTRY GL_EnableVertexAttribArray(GLuint index);
	void GL_APIENTRY GL_DisableVertexAttribArray(GLuint index);
	void GL_APIENTRY GL_VertexAttribPointer(GLuint index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		const void* pointer);
	void GL_APIENTRY GL_BindAttribLocation(GLuint program, GLuint index, const GLchar* name);
	void GL_APIENTRY GL_VertexAttrib1f(GLuint index, GLfloat x);
	void GL_APIENTRY GL_VertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void GL_APIENTRY GL_VertexAttrib4fv(GLuint index, const GLfloat* v);
	void GL_APIENTRY GL_DepthRangef(GLfloat n, GLfloat f);
	void GL_APIENTRY GL_ClearDepthf(GLfloat d);
	void GL_APIENTRY GL_DrawBuffers(GLsizei n, const GLenum* bufs);
	void GL_APIENTRY GL_BindFramebuffer(GLenum target, GLuint framebuffer);
	void GL_APIENTRY GL_DeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
	void GL_APIENTRY GL_GenFramebuffers(GLsizei n, GLuint* framebuffers);
	void GL_APIENTRY GL_FramebufferTexture2D(GLenum target,
		GLenum attachment,
		GLenum textarget,
		GLuint texture,
		GLint level);
	void GL_APIENTRY GL_TexImage2DMultisample(GLenum target,
		GLsizei samples,
		GLenum internalformat,
		GLsizei width,
		GLsizei height,
		GLboolean fixedsamplelocations);
	void GL_APIENTRY GL_TexStorage2DMultisample(GLenum target,
		GLsizei samples,
		GLenum internalformat,
		GLsizei width,
		GLsizei height,
		GLboolean fixedsamplelocations);
	void GL_APIENTRY GL_GenRenderbuffers(GLsizei n, GLuint* renderbuffers);
	void GL_APIENTRY GL_BindRenderbuffer(GLenum target, GLuint renderbuffer);
	void GL_APIENTRY GL_RenderbufferStorage(GLenum target,
		GLenum internalformat,
		GLsizei width,
		GLsizei height);
	void GL_APIENTRY GL_FramebufferRenderbuffer(GLenum target,
		GLenum attachment,
		GLenum renderbuffertarget,
		GLuint renderbuffer);
	void GL_APIENTRY GL_DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
	GLenum GL_APIENTRY GL_CheckFramebufferStatus(GLenum target);
	void GL_APIENTRY GL_BlitFramebuffer(GLint srcX0,
		GLint srcY0,
		GLint srcX1,
		GLint srcY1,
		GLint dstX0,
		GLint dstY0,
		GLint dstX1,
		GLint dstY1,
		GLbitfield mask,
		GLenum filter);
	void GL_APIENTRY GL_GenVertexArrays(GLsizei n, GLuint* arrays);
	void GL_APIENTRY GL_BindVertexArray(GLuint array);
	void GL_APIENTRY GL_DeleteVertexArrays(GLsizei n, const GLuint* arrays);
	void GL_APIENTRY GL_GenBuffers(GLsizei n, GLuint* buffers);
	void GL_APIENTRY GL_BindBuffer(GLenum target, GLuint buffer);
	void GL_APIENTRY GL_BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
	void* GL_APIENTRY GL_MapBuffer(GLenum target, GLenum access);
	void* GL_APIENTRY GL_MapBufferRange(GLenum target,
		GLintptr offset,
		GLsizeiptr length,
		GLbitfield access);
	GLboolean GL_APIENTRY GL_UnmapBuffer(GLenum target);
	void GL_APIENTRY GL_DeleteBuffers(GLsizei n, const GLuint* buffers);
	void GL_APIENTRY GL_BindImageTexture(GLuint unit,
		GLuint texture,
		GLint level,
		GLboolean layered,
		GLint layer,
		GLenum access,
		GLenum format);
	void GL_APIENTRY GL_MemoryBarrier(GLbitfield barriers);
	const GLubyte* GL_APIENTRY GL_GetStringi(GLenum name, GLuint index);
	void GL_APIENTRY GL_InvalidateFramebuffer(GLenum target,
		GLsizei numAttachments,
		const GLenum* attachments);
	void GL_APIENTRY GL_BufferStorage(GLenum target,
		GLsizeiptr size,
		const void* data,
		GLbitfield flags);
	GLsync GL_APIENTRY GL_FenceSync(GLenum condition, GLbitfield flags);
	GLenum GL_APIENTRY GL_ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
	void GL_APIENTRY GL_DeleteSync(GLsync sync);
	GLuint GL_APIENTRY GL_GetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName);
	void GL_APIENTRY GL_UniformBlockBinding(GLuint program,
		GLuint uniformBlockIndex,
		GLuint uniformBlockBinding);
	void GL_APIENTRY GL_GetActiveUniformBlockiv(GLuint program,
		GLuint uniformBlockIndex,
		GLenum pname,
		GLint* params);
	void GL_APIENTRY GL_GetUniformIndices(GLuint program,
		GLsizei uniformCount,
		const GLchar* const* uniformNames,
		GLuint* uniformIndices);
	void GL_APIENTRY GL_GetActiveUniformsiv(GLuint program,
		GLsizei uniformCount,
		const GLuint* uniformIndices,
		GLenum pname,
		GLint* params);
	void GL_APIENTRY GL_BindBufferBase(GLenum target, GLuint index, GLuint buffer);
	void GL_APIENTRY GL_BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
	void GL_APIENTRY GL_GetProgramBinary(GLuint program,
		GLsizei bufSize,
		GLsizei* length,
		GLenum* binaryFormat,
		void* binary);
	void GL_APIENTRY GL_ProgramBinary(GLuint program,
		GLenum binaryFormat,
		const void* binary,
		GLsizei length);
	void GL_APIENTRY GL_ProgramParameteri(GLuint program, GLenum pname, GLint value);
	void GL_APIENTRY GL_TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	void GL_APIENTRY GL_TextureStorage2D(GLuint texture,
		GLsizei levels,
		GLenum internalformat,
		GLsizei width,
		GLsizei height);
	void GL_APIENTRY GL_TextureSubImage2D(GLuint texture,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		const void* pixels);
	void GL_APIENTRY GL_TextureStorage2DMultisample(GLuint texture,
		GLsizei samples,
		GLenum internalformat,
		GLsizei width,
		GLsizei height,
		GLboolean fixedsamplelocations);
	void GL_APIENTRY GL_TextureParameteri(GLuint texture, GLenum pname, GLint param);
	void GL_APIENTRY GL_TextureParameterf(GLuint texture, GLenum pname, GLfloat param);
	void GL_APIENTRY GL_CreateTextures(GLenum target, GLsizei n, GLuint* textures);
	void GL_APIENTRY GL_CreateBuffers(GLsizei n, GLuint* buffers);
	void GL_APIENTRY GL_CreateFramebuffers(GLsizei n, GLuint* framebuffers);
	void GL_APIENTRY GL_NamedFramebufferTexture(GLuint framebuffer,
		GLenum attachment,
		GLuint texture,
		GLint level);
	void GL_APIENTRY GL_DrawRangeElementsBaseVertex(GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		const void* indices,
		GLint basevertex);
	void GL_APIENTRY GL_FlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
	void GL_APIENTRY GL_TextureBarrier();
	void GL_APIENTRY GL_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value);
	void GL_APIENTRY GL_Enablei(GLenum target, GLuint index);
	void GL_APIENTRY GL_Disablei(GLenum target, GLuint index);
	void GL_APIENTRY GL_EGLImageTargetTexture2DOES(GLenum target, void* image);
	
#undef glBlendFunc
#undef glPixelStorei
#undef glClearColor
#undef glCullFace
#undef glDepthFunc
#undef glDepthMask
#undef glDisable
#undef glEnable
#undef glPolygonOffset
#undef glScissor
#undef glViewport
#undef glBindTexture
#undef glTexImage2D
#undef glTexParameteri
#undef glGetIntegerv
#undef glGetString
#undef glReadPixels
#undef glTexSubImage2D
#undef glDrawArrays
#undef glGetError
#undef glDrawElements
#undef glLineWidth
#undef glClear
#undef glGetFloatv
#undef glDeleteTextures
#undef glGenTextures
#undef glTexParameterf
#undef glActiveTexture
#undef glBlendColor
#undef glReadBuffer
#undef glFinish
	void WINAPI glBlendFunc(
		GLenum sfactor,
		GLenum dfactor
	);
	void WINAPI glPixelStorei(
		GLenum pname,
		GLint  param
	);
	void WINAPI glClearColor(
		GLclampf red,
		GLclampf green,
		GLclampf blue,
		GLclampf alpha
	);
	void WINAPI glCullFace(
		GLenum mode
	);
	void WINAPI glDepthFunc(
		GLenum func
	);
	void WINAPI glDepthMask(
		GLboolean flag
	);
	void WINAPI glDisable(
		GLenum cap
	);
	void WINAPI glEnable(
		GLenum cap
	);
	void WINAPI glPolygonOffset(
		GLfloat factor,
		GLfloat units
	);
	void WINAPI glScissor(
		GLint   x,
		GLint   y,
		GLsizei width,
		GLsizei height
	);
	void WINAPI glViewport(
		GLint   x,
		GLint   y,
		GLsizei width,
		GLsizei height
	);
	void WINAPI glBindTexture(
		GLenum target,
		GLuint texture
	);
	void WINAPI glTexImage2D(
		GLenum  target,
		GLint   level,
		GLint   internalformat,
		GLsizei width,
		GLsizei height,
		GLint   border,
		GLenum  format,
		GLenum  type,
		const GLvoid* pixels
	);
	void WINAPI glTexParameteri(
		GLenum target,
		GLenum pname,
		GLint  param
	);
	void WINAPI glGetIntegerv(
		GLenum pname,
		GLint* params
	);
	const GLubyte* WINAPI glGetString(
		GLenum name
	);
	void WINAPI glReadPixels(
		GLint   x,
		GLint   y,
		GLsizei width,
		GLsizei height,
		GLenum  format,
		GLenum  type,
		GLvoid* pixels
	);
	void WINAPI glTexSubImage2D(
		GLenum  target,
		GLint   level,
		GLint   xoffset,
		GLint   yoffset,
		GLsizei width,
		GLsizei height,
		GLenum  format,
		GLenum  type,
		const GLvoid* pixels
	);
	void WINAPI glDrawArrays(
		GLenum  mode,
		GLint   first,
		GLsizei count
	);
	GLenum WINAPI glGetError(void);
	void WINAPI glDrawElements(
		GLenum  mode,
		GLsizei count,
		GLenum  type,
		const GLvoid* indices
	);
	void WINAPI glLineWidth(
		GLfloat width
	);
	void WINAPI glClear(
		GLbitfield mask
	);
	void WINAPI glGetFloatv(
		GLenum  pname,
		GLfloat* params
	);
	void WINAPI glDeleteTextures(
		GLsizei n,
		const GLuint* textures
	);
	void WINAPI glGenTextures(
		GLsizei n,
		GLuint* textures
	);
	void WINAPI glTexParameterf(
		GLenum  target,
		GLenum  pname,
		GLfloat param
	);
	void WINAPI glReadBuffer(
		GLenum mode
	);
	void WINAPI glFinish(void);
}

#ifdef OS_WINDOWS

#define glGetProcAddress wglGetProcAddress
#define GL_GET_PROC_ADR(proc_type, proc_name) g_##proc_name = (proc_type) glGetProcAddress(#proc_name)

#elif defined(VERO4K) || defined(ODROID) || defined(VC)

#define GL_GET_PROC_ADR(proc_type, proc_name) g_##proc_name = (proc_type) dlsym(gles2so, #proc_name);

#elif defined(EGL)

#define glGetProcAddress eglGetProcAddress
#define GL_GET_PROC_ADR(proc_type, proc_name) g_##proc_name = (proc_type) glGetProcAddress(#proc_name)

#elif defined(OS_LINUX)

#include <X11/Xutil.h>
typedef XID GLXContextID;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXPbuffer;
typedef XID GLXWindow;
typedef XID GLXFBConfigID;
typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glxext.h>
#define glGetProcAddress glXGetProcAddress
#define GL_GET_PROC_ADR(proc_type, proc_name) g_##proc_name = (proc_type) glGetProcAddress((const GLubyte*)#proc_name)

#elif defined(OS_MAC_OS_X)
#include <dlfcn.h>

static void* AppleGLGetProcAddress (const char *name)
{
	static void* image = NULL;
	if (NULL == image)
		image = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);

	return (image ? dlsym(image, name) : NULL);
}
#define glGetProcAddress AppleGLGetProcAddress
#define GL_GET_PROC_ADR(proc_type, proc_name) g_##proc_name = (proc_type) glGetProcAddress(#proc_name)

#elif defined(OS_IOS)
#include <dlfcn.h>

static void* IOSGLGetProcAddress (const char *name)
{
    return dlsym(RTLD_DEFAULT, name);
}

#define glGetProcAddress IOSGLGetProcAddress
#define GL_GET_PROC_ADR(proc_type, proc_name) g_##proc_name = (proc_type)glGetProcAddress(#proc_name)

#endif

//GL Fucntions

#if defined(OS_WINDOWS) && !defined(EGL)
PFNGLACTIVETEXTUREPROC g_glActiveTexture;
PFNGLBLENDCOLORPROC g_glBlendColor;
#endif

#if defined(EGL) || defined(OS_IOS)
PFNGLBLENDFUNCPROC g_glBlendFunc;
PFNGLPIXELSTOREIPROC g_glPixelStorei;
PFNGLCLEARCOLORPROC g_glClearColor;
PFNGLCULLFACEPROC g_glCullFace;
PFNGLDEPTHFUNCPROC g_glDepthFunc;
PFNGLDEPTHMASKPROC g_glDepthMask;
PFNGLDISABLEPROC g_glDisable;
PFNGLENABLEPROC g_glEnable;
PFNGLPOLYGONOFFSETPROC g_glPolygonOffset;
PFNGLSCISSORPROC g_glScissor;
PFNGLVIEWPORTPROC g_glViewport;
PFNGLBINDTEXTUREPROC g_glBindTexture;
PFNGLTEXIMAGE2DPROC g_glTexImage2D;
PFNGLTEXPARAMETERIPROC g_glTexParameteri;
PFNGLGETINTEGERVPROC g_glGetIntegerv;
PFNGLGETSTRINGPROC g_glGetString;
PFNGLREADPIXELSPROC g_glReadPixels;
PFNGLTEXSUBIMAGE2DPROC g_glTexSubImage2D;
PFNGLDRAWARRAYSPROC g_glDrawArrays;
PFNGLGETERRORPROC g_glGetError;
PFNGLDRAWELEMENTSPROC g_glDrawElements;
PFNGLLINEWIDTHPROC g_glLineWidth;
PFNGLCLEARPROC g_glClear;
PFNGLGETFLOATVPROC g_glGetFloatv;
PFNGLDELETETEXTURESPROC g_glDeleteTextures;
PFNGLGENTEXTURESPROC g_glGenTextures;
PFNGLTEXPARAMETERFPROC g_glTexParameterf;
PFNGLACTIVETEXTUREPROC g_glActiveTexture;
PFNGLBLENDCOLORPROC g_glBlendColor;
PFNGLREADBUFFERPROC g_glReadBuffer;
PFNGLFINISHPROC g_glFinish;
#if defined(OS_ANDROID)
PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC g_eglGetNativeClientBufferANDROID;
#endif
#endif
PFNGLCREATESHADERPROC g_glCreateShader;
PFNGLCOMPILESHADERPROC g_glCompileShader;
PFNGLSHADERSOURCEPROC g_glShaderSource;
PFNGLCREATEPROGRAMPROC g_glCreateProgram;
PFNGLATTACHSHADERPROC g_glAttachShader;
PFNGLLINKPROGRAMPROC g_glLinkProgram;
PFNGLUSEPROGRAMPROC g_glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC g_glGetUniformLocation;
PFNGLUNIFORM1IPROC g_glUniform1i;
PFNGLUNIFORM1FPROC g_glUniform1f;
PFNGLUNIFORM2FPROC g_glUniform2f;
PFNGLUNIFORM2IPROC g_glUniform2i;
PFNGLUNIFORM4IPROC g_glUniform4i;
PFNGLUNIFORM4FPROC g_glUniform4f;
PFNGLUNIFORM3FVPROC g_glUniform3fv;
PFNGLUNIFORM4FVPROC g_glUniform4fv;
PFNGLDETACHSHADERPROC g_glDetachShader;
PFNGLDELETESHADERPROC g_glDeleteShader;
PFNGLDELETEPROGRAMPROC g_glDeleteProgram;
PFNGLGETPROGRAMINFOLOGPROC g_glGetProgramInfoLog;
PFNGLGETSHADERINFOLOGPROC g_glGetShaderInfoLog;
PFNGLGETSHADERIVPROC g_glGetShaderiv;
PFNGLGETPROGRAMIVPROC g_glGetProgramiv;

PFNGLENABLEVERTEXATTRIBARRAYPROC g_glEnableVertexAttribArray;
PFNGLDISABLEVERTEXATTRIBARRAYPROC g_glDisableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC g_glVertexAttribPointer;
PFNGLBINDATTRIBLOCATIONPROC g_glBindAttribLocation;
PFNGLVERTEXATTRIB1FPROC g_glVertexAttrib1f;
PFNGLVERTEXATTRIB4FPROC g_glVertexAttrib4f;
PFNGLVERTEXATTRIB4FVPROC g_glVertexAttrib4fv;

// multitexture functions
PFNGLDEPTHRANGEFPROC g_glDepthRangef;
PFNGLCLEARDEPTHFPROC g_glClearDepthf;

PFNGLDRAWBUFFERSPROC g_glDrawBuffers;
PFNGLBINDFRAMEBUFFERPROC g_glBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC g_glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC g_glGenFramebuffers;
PFNGLFRAMEBUFFERTEXTURE2DPROC g_glFramebufferTexture2D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC g_glTexImage2DMultisample;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC g_glTexStorage2DMultisample;
PFNGLGENRENDERBUFFERSPROC g_glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC g_glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC g_glRenderbufferStorage;
PFNGLFRAMEBUFFERRENDERBUFFERPROC g_glFramebufferRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC g_glDeleteRenderbuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC g_glCheckFramebufferStatus;
PFNGLBLITFRAMEBUFFERPROC g_glBlitFramebuffer;
PFNGLGENVERTEXARRAYSPROC g_glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC g_glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC g_glDeleteVertexArrays;
PFNGLGENBUFFERSPROC g_glGenBuffers;
PFNGLBINDBUFFERPROC g_glBindBuffer;
PFNGLBUFFERDATAPROC g_glBufferData;
PFNGLMAPBUFFERPROC g_glMapBuffer;
PFNGLMAPBUFFERRANGEPROC g_glMapBufferRange;
PFNGLUNMAPBUFFERPROC g_glUnmapBuffer;
PFNGLDELETEBUFFERSPROC g_glDeleteBuffers;
PFNGLBINDIMAGETEXTUREPROC g_glBindImageTexture;
PFNGLMEMORYBARRIERPROC g_glMemoryBarrier;
PFNGLGETSTRINGIPROC g_glGetStringi;
PFNGLINVALIDATEFRAMEBUFFERPROC g_glInvalidateFramebuffer;
PFNGLBUFFERSTORAGEPROC g_glBufferStorage;
PFNGLFENCESYNCPROC g_glFenceSync;
PFNGLCLIENTWAITSYNCPROC g_glClientWaitSync;
PFNGLDELETESYNCPROC g_glDeleteSync;

PFNGLGETUNIFORMBLOCKINDEXPROC g_glGetUniformBlockIndex;
PFNGLUNIFORMBLOCKBINDINGPROC g_glUniformBlockBinding;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC g_glGetActiveUniformBlockiv;
PFNGLGETUNIFORMINDICESPROC g_glGetUniformIndices;
PFNGLGETACTIVEUNIFORMSIVPROC g_glGetActiveUniformsiv;
PFNGLBINDBUFFERBASEPROC g_glBindBufferBase;
PFNGLBUFFERSUBDATAPROC g_glBufferSubData;

PFNGLGETPROGRAMBINARYPROC g_glGetProgramBinary;
PFNGLPROGRAMBINARYPROC g_glProgramBinary;
PFNGLPROGRAMPARAMETERIPROC g_glProgramParameteri;

PFNGLTEXSTORAGE2DPROC g_glTexStorage2D;
PFNGLTEXTURESTORAGE2DPROC g_glTextureStorage2D;
PFNGLTEXTURESUBIMAGE2DPROC g_glTextureSubImage2D;
PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC g_glTextureStorage2DMultisample;
PFNGLTEXTUREPARAMETERIPROC g_glTextureParameteri;
PFNGLTEXTUREPARAMETERFPROC g_glTextureParameterf;
PFNGLCREATETEXTURESPROC g_glCreateTextures;
PFNGLCREATEBUFFERSPROC g_glCreateBuffers;
PFNGLCREATEFRAMEBUFFERSPROC g_glCreateFramebuffers;
PFNGLNAMEDFRAMEBUFFERTEXTUREPROC g_glNamedFramebufferTexture;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC g_glDrawRangeElementsBaseVertex;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC g_glFlushMappedBufferRange;
PFNGLTEXTUREBARRIERPROC g_glTextureBarrier;
PFNGLTEXTUREBARRIERNVPROC g_glTextureBarrierNV;
PFNGLCLEARBUFFERFVPROC g_glClearBufferfv;
PFNGLENABLEIPROC g_glEnablei;
PFNGLDISABLEIPROC g_glDisablei;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC g_glEGLImageTargetTexture2DOES;

void initGLFunctions()
{
	if (config.angle.renderer == config.arOpenGL)
	{
#ifdef VC
		void* gles2so = dlopen("/opt/vc/lib/libbrcmGLESv2.so", RTLD_NOW);
#elif defined(ODROID)
		void* gles2so = dlopen("/usr/lib/arm-linux-gnueabihf/libGLESv2.so", RTLD_NOW);
#elif defined(VERO4K)
		void* gles2so = dlopen("/opt/vero3/lib/libGLESv2.so", RTLD_NOW);
#endif

		g_glBlendFunc = glBlendFunc;
		g_glPixelStorei = glPixelStorei;
		g_glClearColor = glClearColor;
		g_glCullFace = glCullFace;
		g_glDepthFunc = glDepthFunc;
		g_glDepthMask = glDepthMask;
		g_glDisable = glDisable;
		g_glEnable = glEnable;
		g_glPolygonOffset = glPolygonOffset;
		g_glScissor = glScissor;
		g_glViewport = glViewport;
		g_glBindTexture = glBindTexture;
		g_glTexImage2D = glTexImage2D;
		g_glTexParameteri = glTexParameteri;
		g_glGetIntegerv = glGetIntegerv;
		g_glGetString = glGetString;
		g_glReadPixels = glReadPixels;
		g_glTexSubImage2D = glTexSubImage2D;
		g_glDrawArrays = glDrawArrays;
		g_glGetError = glGetError;
		g_glDrawElements = glDrawElements;
		g_glLineWidth = glLineWidth;
		g_glClear = glClear;
		g_glGetFloatv = glGetFloatv;
		g_glDeleteTextures = glDeleteTextures;
		g_glGenTextures = glGenTextures;
		g_glTexParameterf = glTexParameterf;
		g_glReadBuffer = glReadBuffer;
		g_glFinish = glFinish;

		GL_GET_PROC_ADR(PFNGLACTIVETEXTUREPROC, glActiveTexture);
		GL_GET_PROC_ADR(PFNGLBLENDCOLORPROC, glBlendColor);

		GL_GET_PROC_ADR(PFNGLCREATESHADERPROC, glCreateShader);
		GL_GET_PROC_ADR(PFNGLCOMPILESHADERPROC, glCompileShader);
		GL_GET_PROC_ADR(PFNGLSHADERSOURCEPROC, glShaderSource);
		GL_GET_PROC_ADR(PFNGLCREATEPROGRAMPROC, glCreateProgram);
		GL_GET_PROC_ADR(PFNGLATTACHSHADERPROC, glAttachShader);
		GL_GET_PROC_ADR(PFNGLLINKPROGRAMPROC, glLinkProgram);
		GL_GET_PROC_ADR(PFNGLUSEPROGRAMPROC, glUseProgram);
		GL_GET_PROC_ADR(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
		GL_GET_PROC_ADR(PFNGLUNIFORM1IPROC, glUniform1i);
		GL_GET_PROC_ADR(PFNGLUNIFORM1FPROC, glUniform1f);
		GL_GET_PROC_ADR(PFNGLUNIFORM2FPROC, glUniform2f);
		GL_GET_PROC_ADR(PFNGLUNIFORM2IPROC, glUniform2i);
		GL_GET_PROC_ADR(PFNGLUNIFORM4IPROC, glUniform4i);
		GL_GET_PROC_ADR(PFNGLUNIFORM4FPROC, glUniform4f);
		GL_GET_PROC_ADR(PFNGLUNIFORM3FVPROC, glUniform3fv);
		GL_GET_PROC_ADR(PFNGLUNIFORM4FVPROC, glUniform4fv);
		GL_GET_PROC_ADR(PFNGLDETACHSHADERPROC, glDetachShader);
		GL_GET_PROC_ADR(PFNGLDELETESHADERPROC, glDeleteShader);
		GL_GET_PROC_ADR(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
		GL_GET_PROC_ADR(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
		GL_GET_PROC_ADR(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
		GL_GET_PROC_ADR(PFNGLGETSHADERIVPROC, glGetShaderiv);
		GL_GET_PROC_ADR(PFNGLGETPROGRAMIVPROC, glGetProgramiv);

		GL_GET_PROC_ADR(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
		GL_GET_PROC_ADR(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
		GL_GET_PROC_ADR(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
		GL_GET_PROC_ADR(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation);
		GL_GET_PROC_ADR(PFNGLVERTEXATTRIB1FPROC, glVertexAttrib1f);
		GL_GET_PROC_ADR(PFNGLVERTEXATTRIB4FPROC, glVertexAttrib4f);
		GL_GET_PROC_ADR(PFNGLVERTEXATTRIB4FVPROC, glVertexAttrib4fv);

		GL_GET_PROC_ADR(PFNGLDEPTHRANGEFPROC, glDepthRangef);
		GL_GET_PROC_ADR(PFNGLCLEARDEPTHFPROC, glClearDepthf);

		GL_GET_PROC_ADR(PFNGLDRAWBUFFERSPROC, glDrawBuffers);
		GL_GET_PROC_ADR(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
		GL_GET_PROC_ADR(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
		GL_GET_PROC_ADR(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
		GL_GET_PROC_ADR(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
		GL_GET_PROC_ADR(PFNGLTEXIMAGE2DMULTISAMPLEPROC, glTexImage2DMultisample);
		GL_GET_PROC_ADR(PFNGLTEXSTORAGE2DMULTISAMPLEPROC, glTexStorage2DMultisample);
		GL_GET_PROC_ADR(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers);
		GL_GET_PROC_ADR(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer);
		GL_GET_PROC_ADR(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage);
		GL_GET_PROC_ADR(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer);
		GL_GET_PROC_ADR(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers);
		GL_GET_PROC_ADR(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
		GL_GET_PROC_ADR(PFNGLBLITFRAMEBUFFERPROC, glBlitFramebuffer);
		GL_GET_PROC_ADR(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
		GL_GET_PROC_ADR(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
		GL_GET_PROC_ADR(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
		GL_GET_PROC_ADR(PFNGLGENBUFFERSPROC, glGenBuffers);
		GL_GET_PROC_ADR(PFNGLBINDBUFFERPROC, glBindBuffer);
		GL_GET_PROC_ADR(PFNGLBUFFERDATAPROC, glBufferData);
		GL_GET_PROC_ADR(PFNGLMAPBUFFERPROC, glMapBuffer);
		GL_GET_PROC_ADR(PFNGLMAPBUFFERRANGEPROC, glMapBufferRange);
		GL_GET_PROC_ADR(PFNGLUNMAPBUFFERPROC, glUnmapBuffer);
		GL_GET_PROC_ADR(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
		GL_GET_PROC_ADR(PFNGLBINDIMAGETEXTUREPROC, glBindImageTexture);
		GL_GET_PROC_ADR(PFNGLMEMORYBARRIERPROC, glMemoryBarrier);
		GL_GET_PROC_ADR(PFNGLGETSTRINGIPROC, glGetStringi);
		GL_GET_PROC_ADR(PFNGLINVALIDATEFRAMEBUFFERPROC, glInvalidateFramebuffer);
		GL_GET_PROC_ADR(PFNGLBUFFERSTORAGEPROC, glBufferStorage);
		GL_GET_PROC_ADR(PFNGLFENCESYNCPROC, glFenceSync);
		GL_GET_PROC_ADR(PFNGLCLIENTWAITSYNCPROC, glClientWaitSync);
		GL_GET_PROC_ADR(PFNGLDELETESYNCPROC, glDeleteSync);

		GL_GET_PROC_ADR(PFNGLGETUNIFORMBLOCKINDEXPROC, glGetUniformBlockIndex);
		GL_GET_PROC_ADR(PFNGLUNIFORMBLOCKBINDINGPROC, glUniformBlockBinding);
		GL_GET_PROC_ADR(PFNGLGETACTIVEUNIFORMBLOCKIVPROC, glGetActiveUniformBlockiv);
		GL_GET_PROC_ADR(PFNGLGETUNIFORMINDICESPROC, glGetUniformIndices);
		GL_GET_PROC_ADR(PFNGLGETACTIVEUNIFORMSIVPROC, glGetActiveUniformsiv);
		GL_GET_PROC_ADR(PFNGLBINDBUFFERBASEPROC, glBindBufferBase);
		GL_GET_PROC_ADR(PFNGLBUFFERSUBDATAPROC, glBufferSubData);

		GL_GET_PROC_ADR(PFNGLGETPROGRAMBINARYPROC, glGetProgramBinary);
		GL_GET_PROC_ADR(PFNGLPROGRAMBINARYPROC, glProgramBinary);
		GL_GET_PROC_ADR(PFNGLPROGRAMPARAMETERIPROC, glProgramParameteri);

		GL_GET_PROC_ADR(PFNGLTEXSTORAGE2DPROC, glTexStorage2D);
		GL_GET_PROC_ADR(PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D);
		GL_GET_PROC_ADR(PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D);
		GL_GET_PROC_ADR(PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC, glTextureStorage2DMultisample);

		GL_GET_PROC_ADR(PFNGLTEXTUREPARAMETERIPROC, glTextureParameteri);
		GL_GET_PROC_ADR(PFNGLTEXTUREPARAMETERFPROC, glTextureParameterf);
		GL_GET_PROC_ADR(PFNGLCREATETEXTURESPROC, glCreateTextures);
		GL_GET_PROC_ADR(PFNGLCREATEBUFFERSPROC, glCreateBuffers);
		GL_GET_PROC_ADR(PFNGLCREATEFRAMEBUFFERSPROC, glCreateFramebuffers);
		GL_GET_PROC_ADR(PFNGLNAMEDFRAMEBUFFERTEXTUREPROC, glNamedFramebufferTexture);
		GL_GET_PROC_ADR(PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC, glDrawRangeElementsBaseVertex);
		GL_GET_PROC_ADR(PFNGLFLUSHMAPPEDBUFFERRANGEPROC, glFlushMappedBufferRange);
		GL_GET_PROC_ADR(PFNGLTEXTUREBARRIERPROC, glTextureBarrier);
		GL_GET_PROC_ADR(PFNGLTEXTUREBARRIERNVPROC, glTextureBarrierNV);
		GL_GET_PROC_ADR(PFNGLCLEARBUFFERFVPROC, glClearBufferfv);
		GL_GET_PROC_ADR(PFNGLENABLEIPROC, glEnablei);
		GL_GET_PROC_ADR(PFNGLDISABLEIPROC, glDisablei);
		GL_GET_PROC_ADR(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC, glEGLImageTargetTexture2DOES);
	}
	else
	{
		g_glBlendFunc = GL_BlendFunc;

		g_glPixelStorei = GL_PixelStorei;
		g_glClearColor = GL_ClearColor;
		g_glCullFace = GL_CullFace;
		g_glDepthFunc = GL_DepthFunc;
		g_glDepthMask = GL_DepthMask;
		g_glDisable = GL_Disable;
		g_glEnable = GL_Enable;
		g_glPolygonOffset = GL_PolygonOffset;
		g_glScissor = GL_Scissor;
		g_glViewport = GL_Viewport;
		g_glBindTexture = GL_BindTexture;
		g_glTexImage2D = GL_TexImage2D;
		g_glTexParameteri = GL_TexParameteri;
		g_glGetIntegerv = GL_GetIntegerv;
		g_glGetString = GL_GetString;
		g_glReadPixels = GL_ReadPixels;
		g_glTexSubImage2D = GL_TexSubImage2D;
		g_glDrawArrays = GL_DrawArrays;
		g_glGetError = GL_GetError;
		g_glDrawElements = GL_DrawElements;
		g_glLineWidth = GL_LineWidth;
		g_glClear = GL_Clear;
		g_glGetFloatv = GL_GetFloatv;
		g_glDeleteTextures = GL_DeleteTextures;
		g_glGenTextures = GL_GenTextures;

		g_glTexParameterf = GL_TexParameterf;
		g_glActiveTexture = GL_ActiveTexture;
		g_glBlendColor = GL_BlendColor;
		g_glReadBuffer = GL_ReadBuffer;
		g_glFinish = GL_Finish;
		g_glCreateShader = GL_CreateShader;
		g_glCompileShader = GL_CompileShader;
		g_glShaderSource = GL_ShaderSource;
		g_glCreateProgram = GL_CreateProgram;
		g_glAttachShader = GL_AttachShader;
		g_glLinkProgram = GL_LinkProgram;
		g_glUseProgram = GL_UseProgram;
		g_glGetUniformLocation = GL_GetUniformLocation;
		g_glUniform1i = GL_Uniform1i;
		g_glUniform1f = GL_Uniform1f;
		g_glUniform2f = GL_Uniform2f;
		g_glUniform2i = GL_Uniform2i;
		g_glUniform4i = GL_Uniform4i;
		g_glUniform4f = GL_Uniform4f;
		g_glUniform3fv = GL_Uniform3fv;
		g_glUniform4fv = GL_Uniform4fv;
		g_glDetachShader = GL_DetachShader;
		g_glDeleteShader = GL_DeleteShader;
		g_glDeleteProgram = GL_DeleteProgram;
		g_glGetProgramInfoLog = GL_GetProgramInfoLog;
		g_glGetShaderInfoLog = GL_GetShaderInfoLog;
		g_glGetShaderiv = GL_GetShaderiv;
		g_glGetProgramiv = GL_GetProgramiv;

		g_glEnableVertexAttribArray = GL_EnableVertexAttribArray;
		g_glDisableVertexAttribArray = GL_DisableVertexAttribArray;
		g_glVertexAttribPointer = GL_VertexAttribPointer;
		g_glBindAttribLocation = GL_BindAttribLocation;
		g_glVertexAttrib1f = GL_VertexAttrib1f;
		g_glVertexAttrib4f = GL_VertexAttrib4f;
		g_glVertexAttrib4fv = GL_VertexAttrib4fv;

		g_glDepthRangef = GL_DepthRangef;
		g_glClearDepthf = GL_ClearDepthf;

		g_glDrawBuffers = GL_DrawBuffers;
		g_glBindFramebuffer = GL_BindFramebuffer;
		g_glDeleteFramebuffers = GL_DeleteFramebuffers;
		g_glGenFramebuffers = GL_GenFramebuffers;
		g_glFramebufferTexture2D = GL_FramebufferTexture2D;
		g_glTexImage2DMultisample = GL_TexImage2DMultisample;
		g_glTexStorage2DMultisample = GL_TexStorage2DMultisample;
		g_glGenRenderbuffers = GL_GenRenderbuffers;
		g_glBindRenderbuffer = GL_BindRenderbuffer;
		g_glRenderbufferStorage = GL_RenderbufferStorage;
		g_glFramebufferRenderbuffer = GL_FramebufferRenderbuffer;
		g_glDeleteRenderbuffers = GL_DeleteRenderbuffers;
		g_glCheckFramebufferStatus = GL_CheckFramebufferStatus;
		g_glBlitFramebuffer = GL_BlitFramebuffer;
		g_glGenVertexArrays = GL_GenVertexArrays;
		g_glBindVertexArray = GL_BindVertexArray;
		g_glDeleteVertexArrays = GL_DeleteVertexArrays;
		g_glGenBuffers = GL_GenBuffers;
		g_glBindBuffer = GL_BindBuffer;
		g_glBufferData = GL_BufferData;
		g_glMapBuffer = GL_MapBuffer;
		g_glMapBufferRange = GL_MapBufferRange;
		g_glUnmapBuffer = GL_UnmapBuffer;
		g_glDeleteBuffers = GL_DeleteBuffers;
		g_glBindImageTexture = GL_BindImageTexture;
		g_glMemoryBarrier = GL_MemoryBarrier;
		g_glGetStringi = GL_GetStringi;
		g_glInvalidateFramebuffer = GL_InvalidateFramebuffer;
		g_glBufferStorage = GL_BufferStorage;
		g_glFenceSync = GL_FenceSync;
		g_glClientWaitSync = GL_ClientWaitSync;
		g_glDeleteSync = GL_DeleteSync;

		g_glGetUniformBlockIndex = GL_GetUniformBlockIndex;
		g_glUniformBlockBinding = GL_UniformBlockBinding;
		g_glGetActiveUniformBlockiv = GL_GetActiveUniformBlockiv;
		g_glGetUniformIndices = GL_GetUniformIndices;
		g_glGetActiveUniformsiv = GL_GetActiveUniformsiv;
		g_glBindBufferBase = GL_BindBufferBase;
		g_glBufferSubData = GL_BufferSubData;

		g_glGetProgramBinary = GL_GetProgramBinary;
		g_glProgramBinary = GL_ProgramBinary;
		g_glProgramParameteri = GL_ProgramParameteri;

		g_glTexStorage2D = GL_TexStorage2D;
		g_glTextureStorage2D = GL_TextureStorage2D;
		g_glTextureSubImage2D = GL_TextureSubImage2D;
		g_glTextureStorage2DMultisample = nullptr; // GL_TextureStorage2DMultisample;

		g_glTextureParameteri = GL_TextureParameteri;
		g_glTextureParameterf = GL_TextureParameterf;
		g_glCreateTextures = GL_CreateTextures;
		g_glCreateBuffers = GL_CreateBuffers;
		g_glCreateFramebuffers = GL_CreateFramebuffers;
		g_glNamedFramebufferTexture = GL_NamedFramebufferTexture;
		g_glDrawRangeElementsBaseVertex = GL_DrawRangeElementsBaseVertex;
		g_glFlushMappedBufferRange = GL_FlushMappedBufferRange;
		g_glTextureBarrier = GL_TextureBarrier;
		g_glTextureBarrierNV = nullptr;
		g_glClearBufferfv = GL_ClearBufferfv;
		g_glEnablei = GL_Enablei;
		g_glDisablei = GL_Disablei;
		g_glEGLImageTargetTexture2DOES = GL_EGLImageTargetTexture2DOES;
	}
}
#endif
