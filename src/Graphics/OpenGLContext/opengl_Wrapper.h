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
	public:
		static void setThreadedMode(void);

		static void glBlendFunc(const GLenum& sfactor, const GLenum& dfactor);
		static void glPixelStorei(const GLenum& pname, const GLint& param);
		static void glClearColor(const GLfloat& red, const GLfloat& green, const GLfloat& blue, const GLfloat& alpha);
		static void glCullFace(const GLenum& mode);
		static void glDepthFunc(const GLenum& func);
		static void glDepthMask(const GLboolean& flag);
		static void glDisable(const GLenum& cap);
		static void glEnable(const GLenum& cap);
		static void glPolygonOffset(const GLfloat& factor, const GLfloat& units);

		static void glScissor(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height);
		static void glViewport(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height);
		static void glBindTexture(const GLenum& target, const GLuint& texture);
		template <class pixelType>
		static void glTexImage2D(const GLenum& target, const GLint& level, const GLint& internalformat, const GLsizei& width, const GLsizei& height, const GLint& border, const GLenum& format, const GLenum& type, std::unique_ptr<pixelType[]> pixels);
		static void glTexParameteri(const GLenum& target, const GLenum& pname, const GLint& param);
		static void glGetIntegerv(const GLenum& pname, GLint* data);
		static const GLubyte* glGetString(const GLenum& name);

		static void glReadPixels(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, void *pixels);
		static void glReadPixelsAsync(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type);
		template <class pixelType>
		static void glTexSubImage2DUnbuffered(const GLenum& target, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::unique_ptr<pixelType[]> pixels);
		static void glTexSubImage2DBuffered(const GLenum& target, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::size_t offset);
		static void glDrawArrays(const GLenum& mode, const GLint& first, const GLsizei& count);
		static void glDrawArraysUnbuffered(const GLenum& mode, const GLint& first, const GLsizei& count, std::unique_ptr<std::vector<char>> data);
		static GLenum glGetError(void);
		static void glDrawElementsNotThreadSafe(const GLenum& mode, const GLsizei& count, const GLenum& type, const void *indices);
		template <class indiceType>
		static void glDrawElementsUnbuffered(const GLenum& mode, const GLsizei& count, const GLenum& type, std::unique_ptr<indiceType[]> indices, std::unique_ptr<std::vector<char>> data);
		static void glLineWidth(const GLfloat& width);
		static void glClear(const GLbitfield& mask);
		static void glGetFloatv(const GLenum& pname, GLfloat* data);
		static void glDeleteTextures(const GLsizei& n, std::unique_ptr<GLuint[]> textures);
		static void glGenTextures(const GLsizei& n, GLuint* textures);
		static void glTexParameterf(const GLenum& target, const GLenum& pname, const GLfloat& param);
		static void glActiveTexture(const GLenum& texture);
		static void glBlendColor(const GLfloat& red, const GLfloat& green, const GLfloat& blue, const GLfloat& alpha);
		static void glReadBuffer(const GLenum& src);

		static GLuint glCreateShader(const GLenum& type);
		static void glCompileShader(const GLuint& shader);
		static void glShaderSource(const GLuint& shader, const std::string& string);
		static GLuint glCreateProgram(void);
		static void glAttachShader(const GLuint& program, const GLuint& shader);
		static void glLinkProgram(const GLuint& program);
		static void glUseProgram(const GLuint& program);
		static GLint glGetUniformLocation(const GLuint& program, const GLchar *name);
		static void glUniform1i(const GLint& location, const GLint& v0);
		static void glUniform1f(const GLint& location, const GLfloat& v0);
		static void glUniform2f(const GLint& location, const GLfloat& v0, const GLfloat& v1);
		static void glUniform2i(const GLint& location, const GLint& v0, const GLint& v1);
		static void glUniform4i(const GLint& location, const GLint& v0, const GLint& v1, const GLint& v2, const GLint& v3);

		static void glUniform4f(const GLint& location, const GLfloat& v0, const GLfloat& v1, const GLfloat& v2, const GLfloat& v3);
		static void glUniform3fv(const GLint& location, const GLsizei& count, std::unique_ptr<GLfloat[]> value);
		static void glUniform4fv(const GLint& location, const GLsizei& count, std::unique_ptr<GLfloat[]> value);
		static void glDetachShader(const GLuint& program, const GLuint& shader);
		static void glDeleteShader(const GLuint& shader);
		static void glDeleteProgram(const GLuint& program);
		static void glGetProgramInfoLog(const GLuint& program, const GLsizei& bufSize, GLsizei* length, GLchar *infoLog);
		static void glGetShaderInfoLog(const GLuint& shader, const GLsizei& bufSize, GLsizei* length, GLchar *infoLog);
		static void glGetShaderiv(const GLuint& shader, const GLenum& pname, GLint* params);
		static void glGetProgramiv(const GLuint& program, const GLenum& pname, GLint* params);

		static void glEnableVertexAttribArray(const GLuint& index);
		static void glDisableVertexAttribArray(const GLuint& index);
		static void glVertexAttribPointerBuffered(const GLuint& index, const GLint& size, const GLenum& type, const GLboolean& normalized, const GLsizei& stride, std::size_t offset);
		static void glVertexAttribPointerNotThreadSafe(const GLuint& index, const GLint& size, const GLenum& type, const GLboolean& normalized, const GLsizei& stride, const void *pointer);
		static void glVertexAttribPointerUnbuffered(const GLuint& index, const GLint& size, const GLenum& type, const GLboolean& normalized, const GLsizei& stride, std::size_t offset);
		static void glBindAttribLocation(const GLuint& program, const GLuint& index, const std::string& name);
		static void glVertexAttrib1f(const GLuint& index, const GLfloat& x);
		static void glVertexAttrib4f(const GLuint& index, const GLfloat& x, const GLfloat& y, const GLfloat& z, const GLfloat& w);
		static void glVertexAttrib4fv(const GLuint& index, std::unique_ptr<GLfloat[]> v);

		static void glDepthRangef(const GLfloat& n, const GLfloat& f);
		static void glClearDepthf(const GLfloat& d);

		static void glDrawBuffers(const GLsizei& n, std::unique_ptr<GLenum[]> bufs);
		static void glGenFramebuffers(const GLsizei& n, GLuint* framebuffers);
		static void glBindFramebuffer(const GLenum& target, const GLuint& framebuffer);
		static void glDeleteFramebuffers(const GLsizei& n, std::unique_ptr<GLuint[]> framebuffers);
		static void glFramebufferTexture2D(const GLenum& target, const GLenum& attachment, const GLenum& textarget, const GLuint& texture, const GLint& level);
		static void glTexImage2DMultisample(const GLenum& target, const GLsizei& samples, const GLenum& internalformat, const GLsizei& width, const GLsizei& height, const GLboolean& fixedsamplelocations);
		static void glTexStorage2DMultisample(const GLenum& target, const GLsizei& samples, const GLenum& internalformat, const GLsizei& width, const GLsizei& height, const GLboolean& fixedsamplelocations);
		static void glGenRenderbuffers(const GLsizei& n, GLuint* renderbuffers);
		static void glBindRenderbuffer(const GLenum& target, const GLuint& renderbuffer);
		static void glRenderbufferStorage(const GLenum& target, const GLenum& internalformat, const GLsizei& width, const GLsizei& height);
		static void glDeleteRenderbuffers(const GLsizei& n, std::unique_ptr<GLuint[]> renderbuffers);
		static void glFramebufferRenderbuffer(const GLenum& target, const GLenum& attachment, const GLenum& renderbuffertarget, const GLuint& renderbuffer);
		static GLenum glCheckFramebufferStatus(const GLenum& target);
		static void glBlitFramebuffer(const GLint& srcX0, const GLint& srcY0, const GLint& srcX1, const GLint& srcY1, const GLint& dstX0, const GLint& dstY0, const GLint& dstX1, const GLint& dstY1, const GLbitfield& mask, const GLenum& filter);
		static void glGenVertexArrays(const GLsizei& n, GLuint* arrays);
		static void glBindVertexArray(const GLuint& array);
		static void glDeleteVertexArrays(const GLsizei& n, std::unique_ptr<GLuint[]> arrays);
		static void glGenBuffers(GLsizei n, GLuint* buffers);
		static void glBindBuffer(const GLenum& target, const GLuint& buffer);
		template <class dataType>
		static void glBufferData(const GLenum& target, const GLsizeiptr& size, std::unique_ptr<dataType[]> data, const GLenum& usage);
		static void glMapBuffer(const GLenum& target, const GLenum& access);
		static void* glMapBufferRange(const GLenum& target, const GLintptr& offset, const GLsizeiptr& length, const GLbitfield& access);
		static void glMapBufferRangeWriteAsync(const GLenum& target, const GLuint& buffer, const GLintptr& offset, u32 length, const GLbitfield& access, std::unique_ptr<u8[]> data);
		static void* glMapBufferRangeReadAsync(const GLenum& target, const GLuint& buffer, const GLintptr& offset, u32 length, const GLbitfield& access);
		static GLboolean glUnmapBuffer(const GLenum& target);
		static void glUnmapBufferAsync(const GLenum& target);
		static void glDeleteBuffers(GLsizei n, std::unique_ptr<GLuint[]> buffers);
		static void glBindImageTexture(const GLuint& unit, const GLuint& texture, const GLint& level, const GLboolean& layered, const GLint& layer, const GLenum& access, const GLenum& format);
		static void glMemoryBarrier(const GLbitfield& barriers);
		static const GLubyte* glGetStringi(const GLenum& name, const GLuint& index);
		static void glInvalidateFramebuffer(const GLenum& target, const GLsizei& numAttachments, std::unique_ptr<GLenum[]> attachments);
		template <class dataType>
		static void glBufferStorage(const GLenum& target, const GLsizeiptr& size, std::unique_ptr<dataType[]> data, const GLbitfield& flags);
		static GLsync glFenceSync(const GLenum& condition, const GLbitfield& flags);
		static void glClientWaitSync(const GLsync& sync, const GLbitfield& flags, const GLuint64& timeout);
		static void glDeleteSync(const GLsync& sync);

		static GLuint glGetUniformBlockIndex(const GLuint& program, GLchar *uniformBlockName);
		static void glUniformBlockBinding(const GLuint& program, const GLuint& uniformBlockIndex, const GLuint& uniformBlockBinding);
		static void glGetActiveUniformBlockiv(const GLuint& program, const GLuint& uniformBlockIndex, const GLenum& pname, GLint* params);
		static void glGetUniformIndices(const GLuint& program, const GLsizei& uniformCount, const GLchar *const*uniformNames, GLuint* uniformIndices);
		static void glGetActiveUniformsiv(const GLuint& program, const GLsizei& uniformCount, const GLuint *uniformIndices, const GLenum& pname, GLint* params);
		static void glBindBufferBase(const GLenum& target, const GLuint& index, const GLuint& buffer);
		template <class dataType>
		static void glBufferSubData(const GLenum& target, const GLintptr& offset, const GLsizeiptr& size, std::unique_ptr<dataType[]> data);

		static void glGetProgramBinary(const GLuint& program, const GLsizei& bufSize, GLsizei* length, GLenum* binaryFormat, void *binary);
		template <class dataType>
		static void glProgramBinary(const GLuint& program, const GLenum& binaryFormat, std::unique_ptr<dataType[]> binary, const GLsizei& length);
		static void glProgramParameteri(const GLuint& program, const GLenum& pname, const GLint& value);

		static void glTexStorage2D(const GLenum& target, const GLsizei& levels, const GLenum& internalformat, const GLsizei& width, const GLsizei& height);
		static void glTextureStorage2D(const GLuint& texture, const GLsizei& levels, const GLenum& internalformat, const GLsizei& width, const GLsizei& height);
		template <class pixelType>
		static void glTextureSubImage2DUnbuffered(const GLuint& texture, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::unique_ptr<pixelType[]> pixels);
		static void glTextureSubImage2DBuffered(const GLuint& texture, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::size_t offset);
		static void glTextureStorage2DMultisample(const GLuint& texture, const GLenum& target, const GLsizei& samples, const GLenum& internalformat, const GLsizei& width, const GLsizei& height, const GLboolean& fixedsamplelocations);
		static void glTextureParameteri(const GLuint& texture, const GLenum& pname, const GLint& param);
		static void glTextureParameterf(const GLuint& texture, const GLenum& pname, const GLfloat& param);
		static void glCreateTextures(const GLenum& target, const GLsizei& n, GLuint* textures);
		static void glCreateBuffers(const GLsizei& n, GLuint* buffers);
		static void glCreateFramebuffers(const GLsizei& n, GLuint* framebuffers);
		static void glNamedFramebufferTexture(const GLuint& framebuffer, const GLenum& attachment, const GLuint& texture, const GLint& level);
		static void glDrawElementsBaseVertex(const GLenum& mode, const GLsizei& count, const GLenum& type, const char* indices, const GLint& basevertex);
		static void glFlushMappedBufferRange(const GLenum& target, const GLintptr& offset, const GLsizeiptr& length);
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
	void  FunctionWrapper::glTexImage2D(const GLenum& target, const GLint& level, const GLint& internalformat, const GLsizei& width, const GLsizei& height, const GLint& border, const GLenum& format, const GLenum& type, std::unique_ptr<pixelType[]> pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexImage2DCommand<pixelType>>(target, level, internalformat, width, height, border, format, type, std::move(pixels)));
		else
			g_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels.get());
	}

	template <class pixelType>
	void  FunctionWrapper::glTexSubImage2DUnbuffered(const GLenum& target, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::unique_ptr<pixelType[]> pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexSubImage2DUnbufferedCommand<pixelType>>(target, level, xoffset, yoffset, width, height, format, type, std::move(pixels)));
		else
			g_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels.get());
	}

	template <class indiceType>
	void  FunctionWrapper::glDrawElementsUnbuffered(const GLenum& mode, const GLsizei& count, const GLenum& type, std::unique_ptr<indiceType[]> indices, std::unique_ptr<std::vector<char>> data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDrawElementsUnbufferedCommand<indiceType>>(mode, count, type, std::move(indices), std::move(data)));
		else
			std::make_shared<GlDrawElementsUnbufferedCommand<indiceType>>(mode, count, type, std::move(indices), std::move(data))->performCommandSingleThreaded();
	}

	template <class dataType>
	void  FunctionWrapper::glBufferData(const GLenum& target, const GLsizeiptr& size, std::unique_ptr<dataType[]> data, const GLenum& usage)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBufferDataCommand<dataType>>(target, size, std::move(data), usage));
		else
			g_glBufferData(target, size, data.get(), usage);
	}

	template <class dataType>
	void  FunctionWrapper::glBufferStorage(const GLenum& target, const GLsizeiptr& size, std::unique_ptr<dataType[]> data, const GLbitfield& flags)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBufferStorageCommand<dataType>>(target, size, std::move(data), flags));
		else
			g_glBufferStorage(target, size, data.get(), flags);
	}

	template <class dataType>
	void  FunctionWrapper::glBufferSubData(const GLenum& target, const GLintptr& offset, const GLsizeiptr& size, std::unique_ptr<dataType[]> data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBufferSubDataCommand<dataType>>(target, offset, size, std::move(data)));
		else
			g_glBufferSubData(target, offset, size, data.get());
	}

	template <class dataType>
	void  FunctionWrapper::glProgramBinary(const GLuint& program, const GLenum& binaryFormat, std::unique_ptr<dataType[]> binary, const GLsizei& length)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlProgramBinaryCommand<dataType>>(program, binaryFormat, std::move(binary), length));
		else
			g_glProgramBinary(program, binaryFormat, binary.get(), length);
	}

	template <class pixelType>
	void  FunctionWrapper::glTextureSubImage2DUnbuffered(const GLuint& texture, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::unique_ptr<pixelType[]> pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureSubImage2DUnbufferedCommand<pixelType>>(texture, level, xoffset, yoffset, width, height, format, type, std::move(pixels)));
		else
			g_glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels.get());
	}
}
