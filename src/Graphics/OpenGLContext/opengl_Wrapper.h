#pragma once

#include "GLFunctions.h"
#include "BlockingQueue.h"
#include "opengl_WrappedFunctions.h"
#include <thread>

#ifdef MUPENPLUSAPI
#include <mupenplus/GLideN64_mupenplus.h>
#endif

namespace opengl {

	class FunctionWrapper
	{
	private:
		static void executeCommand(std::shared_ptr<OpenGlCommand> _command);

		static void executePriorityCommand(std::shared_ptr<OpenGlCommand> _command);

		static void commandLoop(void);

		static BlockingQueue<std::shared_ptr<OpenGlCommand>> m_commandQueue;

		static bool m_threaded_wrapper;
		static bool m_shutdown;
		static int m_swapBuffersQueued;
		static std::thread m_commandExecutionThread;
		static std::mutex m_condvarMutex;
		static std::condition_variable m_condition;

		static const int MAX_SWAP = 2;

	public:
		static void setThreadedMode(void);

		static void glBlendFunc(GLenum sfactor, GLenum dfactor);
		static void glPixelStorei(GLenum pname, GLint param);
		static void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
		static void glCullFace(GLenum mode);
		static void glDepthFunc(GLenum func);
		static void glDepthMask(GLboolean flag);
		static void glDisable(GLenum cap);
		static void glEnable(GLenum cap);
		static void glPolygonOffset(GLfloat factor, GLfloat units);

		static void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
		static void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
		static void glBindTexture(GLenum target, GLuint texture);
		template <class pixelType>
		static void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels);
		static void glTexParameteri(GLenum target, GLenum pname, GLint param);
		static void glGetIntegerv(GLenum pname, GLint* data);
		static const GLubyte* glGetString(GLenum name);

		static void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
		static void glReadPixelsAsync(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type);
		template <class pixelType>
		static void glTexSubImage2DUnbuffered(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels);
		static void glTexSubImage2DBuffered(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::size_t offset);
		static void glDrawArrays(GLenum mode, GLint first, GLsizei count);
		static void glDrawArraysUnbuffered(GLenum mode, GLint first, GLsizei count, std::unique_ptr<std::vector<char>> data);
		static GLenum glGetError(void);
		static void glDrawElementsNotThreadSafe(GLenum mode, GLsizei count, GLenum type, const void *indices);
		template <class indiceType>
		static void glDrawElementsUnbuffered(GLenum mode, GLsizei count, GLenum type, std::unique_ptr<indiceType[]> indices, std::unique_ptr<std::vector<char>> data);
		static void glLineWidth(GLfloat width);
		static void glClear(GLbitfield mask);
		static void glGetFloatv(GLenum pname, GLfloat* data);
		static void glDeleteTextures(GLsizei n, std::unique_ptr<GLuint[]> textures);
		static void glGenTextures(GLsizei n, GLuint* textures);
		static void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
		static void glActiveTexture(GLenum texture);
		static void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
		static void glReadBuffer(GLenum src);

		static GLuint glCreateShader(GLenum type);
		static void glCompileShader(GLuint shader);
		static void glShaderSource(GLuint shader, const std::string& string);
		static GLuint glCreateProgram(void);
		static void glAttachShader(GLuint program, GLuint shader);
		static void glLinkProgram(GLuint program);
		static void glUseProgram(GLuint program);
		static GLint glGetUniformLocation(GLuint program, const GLchar *name);
		static void glUniform1i(GLint location, GLint v0);
		static void glUniform1f(GLint location, GLfloat v0);
		static void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
		static void glUniform2i(GLint location, GLint v0, GLint v1);
		static void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

		static void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
		static void glUniform3fv(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value);
		static void glUniform4fv(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value);
		static void glDetachShader(GLuint program, GLuint shader);
		static void glDeleteShader(GLuint shader);
		static void glDeleteProgram(GLuint program);
		static void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar *infoLog);
		static void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar *infoLog);
		static void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
		static void glGetProgramiv(GLuint program, GLenum pname, GLint* params);

		static void glEnableVertexAttribArray(GLuint index);
		static void glDisableVertexAttribArray(GLuint index);
		static void glVertexAttribPointerBuffered(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, std::size_t offset);
		static void glVertexAttribPointerNotThreadSafe(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
		static void glVertexAttribPointerUnbuffered(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, std::size_t offset);
		static void glBindAttribLocation(GLuint program, GLuint index, const std::string& name);
		static void glVertexAttrib1f(GLuint index, GLfloat x);
		static void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
		static void glVertexAttrib4fv(GLuint index, std::unique_ptr<GLfloat[]> v);

		static void glDepthRangef(GLfloat n, GLfloat f);
		static void glClearDepthf(GLfloat d);

		static void glDrawBuffers(GLsizei n, std::unique_ptr<GLenum[]> bufs);
		static void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
		static void glBindFramebuffer(GLenum target, GLuint framebuffer);
		static void glDeleteFramebuffers(GLsizei n, std::unique_ptr<GLuint[]> framebuffers);
		static void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
		static void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
		static void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
		static void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
		static void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
		static void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
		static void glDeleteRenderbuffers(GLsizei n, std::unique_ptr<GLuint[]> renderbuffers);
		static void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
		static GLenum glCheckFramebufferStatus(GLenum target);
		static void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
		static void glGenVertexArrays(GLsizei n, GLuint* arrays);
		static void glBindVertexArray(GLuint array);
		static void glDeleteVertexArrays(GLsizei n, std::unique_ptr<GLuint[]> arrays);
		static void glGenBuffers(GLsizei n, GLuint* buffers);
		static void glBindBuffer(GLenum target, GLuint buffer);
		template <class dataType>
		static void glBufferData(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLenum usage);
		static void glMapBuffer(GLenum target, GLenum access);
		static void* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
		static void glMapBufferRangeWriteAsync(GLenum target, GLuint buffer, GLintptr offset, u32 length, GLbitfield access, std::unique_ptr<u8[]> data);
		static std::shared_ptr<std::vector<u8>> glMapBufferRangeReadAsync(GLenum target, GLuint buffer, GLintptr offset, u32 length, GLbitfield access);
		static GLboolean glUnmapBuffer(GLenum target);
		static void glUnmapBufferAsync(GLenum target);
		static void glDeleteBuffers(GLsizei n, std::unique_ptr<GLuint[]> buffers);
		static void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
		static void glMemoryBarrier(GLbitfield barriers);
		static const GLubyte* glGetStringi(GLenum name, GLuint index);
		static void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, std::unique_ptr<GLenum[]> attachments);
		template <class dataType>
		static void glBufferStorage(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLbitfield flags);
		static GLsync glFenceSync(GLenum condition, GLbitfield flags);
		static void glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
		static void glDeleteSync(GLsync sync);

		static GLuint glGetUniformBlockIndex(GLuint program, GLchar *uniformBlockName);
		static void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
		static void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
		static void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint* uniformIndices);
		static void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint* params);
		static void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
		template <class dataType>
		static void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, std::unique_ptr<dataType[]> data);

		static void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void *binary);
		template <class dataType>
		static void glProgramBinary(GLuint program, GLenum binaryFormat, std::unique_ptr<dataType[]> binary, GLsizei length);
		static void glProgramParameteri(GLuint program, GLenum pname, GLint value);

		static void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
		static void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
		template <class pixelType>
		static void glTextureSubImage2DUnbuffered(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels);
		static void glTextureSubImage2DBuffered(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::size_t offset);
		static void glTextureStorage2DMultisample(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
		static void glTextureParameteri(GLuint texture, GLenum pname, GLint param);
		static void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param);
		static void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);
		static void glCreateBuffers(GLsizei n, GLuint* buffers);
		static void glCreateFramebuffers(GLsizei n, GLuint* framebuffers);
		static void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
		static void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const char* indices, GLint basevertex);
		static void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
		static void glFinish(void);

#ifdef MUPENPLUSAPI
		//Vid_ext functions
		static void CoreVideo_Init(void);
		static void CoreVideo_Quit(void);
		static m64p_error CoreVideo_SetVideoMode(int screenWidth, int screenHeight, int bitsPerPixel, m64p_video_mode mode, m64p_video_flags flags);
		static void CoreVideo_GL_SetAttribute(m64p_GLattr attribute, int value);
		static void CoreVideo_GL_GetAttribute(m64p_GLattr attribute, int *value);
		static void CoreVideo_GL_SwapBuffers(void);
#else
		//Windows GL context functions
		static bool windowsStart(void);
		static void windowsStop(void);
		static void windowsSwapBuffers(void);
#endif

		static void ReduceSwapBuffersQueued(void);
		static void WaitForSwapBuffersQueued(void);
	};

	template <class pixelType>
	void  FunctionWrapper::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexImage2DCommand<pixelType>>(target, level, internalformat, width, height, border, format, type, std::move(pixels)));
		else
			g_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels.get());
	}

	template <class pixelType>
	void  FunctionWrapper::glTexSubImage2DUnbuffered(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexSubImage2DUnbufferedCommand<pixelType>>(target, level, xoffset, yoffset, width, height, format, type, std::move(pixels)));
		else
			g_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels.get());
	}

	template <class indiceType>
	void  FunctionWrapper::glDrawElementsUnbuffered(GLenum mode, GLsizei count, GLenum type, std::unique_ptr<indiceType[]> indices, std::unique_ptr<std::vector<char>> data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDrawElementsUnbufferedCommand<indiceType>>(mode, count, type, std::move(indices), std::move(data)));
		else
			std::make_shared<GlDrawElementsUnbufferedCommand<indiceType>>(mode, count, type, std::move(indices), std::move(data))->performCommandSingleThreaded();
	}

	template <class dataType>
	void  FunctionWrapper::glBufferData(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLenum usage)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBufferDataCommand<dataType>>(target, size, std::move(data), usage));
		else
			g_glBufferData(target, size, data.get(), usage);
	}

	template <class dataType>
	void  FunctionWrapper::glBufferStorage(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLbitfield flags)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBufferStorageCommand<dataType>>(target, size, std::move(data), flags));
		else
			g_glBufferStorage(target, size, data.get(), flags);
	}

	template <class dataType>
	void  FunctionWrapper::glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, std::unique_ptr<dataType[]> data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBufferSubDataCommand<dataType>>(target, offset, size, std::move(data)));
		else
			g_glBufferSubData(target, offset, size, data.get());
	}

	template <class dataType>
	void  FunctionWrapper::glProgramBinary(GLuint program, GLenum binaryFormat, std::unique_ptr<dataType[]> binary, GLsizei length)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlProgramBinaryCommand<dataType>>(program, binaryFormat, std::move(binary), length));
		else
			g_glProgramBinary(program, binaryFormat, binary.get(), length);
	}

	template <class pixelType>
	void  FunctionWrapper::glTextureSubImage2DUnbuffered(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureSubImage2DUnbufferedCommand<pixelType>>(texture, level, xoffset, yoffset, width, height, format, type, std::move(pixels)));
		else
			g_glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels.get());
	}
}
