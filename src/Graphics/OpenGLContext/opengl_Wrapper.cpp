#include "opengl_Wrapper.h"

namespace opengl {

	bool FunctionWrapper::m_threaded_wrapper = false;
	bool FunctionWrapper::m_shutdown = false;
	int FunctionWrapper::m_swapBuffersQueued = 0;
	std::thread FunctionWrapper::m_commandExecutionThread;
	std::mutex FunctionWrapper::m_condvarMutex;
	std::condition_variable FunctionWrapper::m_condition;
	BlockingQueue<std::shared_ptr<OpenGlCommand>> FunctionWrapper::m_commandQueue;

	void FunctionWrapper::executeCommand(std::shared_ptr<OpenGlCommand> _command)
	{
		m_commandQueue.push(_command);
		_command->waitOnCommand();
	}

	void FunctionWrapper::executePriorityCommand(std::shared_ptr<OpenGlCommand> _command)
	{
		m_commandQueue.pushBack(_command);
		_command->waitOnCommand();
	}

	void FunctionWrapper::commandLoop(void)
	{
		while (!m_shutdown || m_commandQueue.size() != 0) {
			std::shared_ptr<OpenGlCommand> command;

			if (m_commandQueue.tryPop(command, std::chrono::milliseconds(10))) {
				command->performCommand();
			}
		}
	}

	void FunctionWrapper::setThreadedMode(u32 _threaded)
	{
#ifdef GL_DEBUG
		_threaded = 0;
#endif

		if (_threaded == 1) {
			m_threaded_wrapper = true;
			m_shutdown = false;
			m_commandExecutionThread = std::thread(&FunctionWrapper::commandLoop);
		} else {
			m_threaded_wrapper = false;
			m_shutdown = true;
		}
	}

	void FunctionWrapper::glBlendFunc(GLenum sfactor, GLenum dfactor)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBlendFuncCommand::get(sfactor, dfactor));
		else
			g_glBlendFunc(sfactor, dfactor);
	}

	void FunctionWrapper::glPixelStorei(GLenum pname, GLint param)
	{
		if (m_threaded_wrapper)
			executeCommand(GlPixelStoreiCommand::get(pname, param));
		else
			g_glPixelStorei(pname, param);
	}

	void FunctionWrapper::glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	{
		if (m_threaded_wrapper)
			executeCommand(GlClearColorCommand::get(red, green, blue, alpha));
		else
			g_glClearColor(red, green, blue, alpha);
	}

	void FunctionWrapper::glCullFace(GLenum mode)
	{
		if (m_threaded_wrapper)
			executeCommand(GlCullFaceCommand::get(mode));
		else
			g_glCullFace(mode);
	}

	void FunctionWrapper::glDepthFunc(GLenum func)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDepthFuncCommand::get(func));
		else
			g_glDepthFunc(func);
	}

	void FunctionWrapper::glDepthMask(GLboolean flag)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDepthMaskCommand::get(flag));
		else
			g_glDepthMask(flag);
	}

	void FunctionWrapper::glDisable(GLenum cap)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDisableCommand::get(cap));
		else
			g_glDisable(cap);
	}

	void FunctionWrapper::glEnable(GLenum cap)
	{
		if (m_threaded_wrapper)
			executeCommand(GlEnableCommand::get(cap));
		else
			g_glEnable(cap);
	}

	void FunctionWrapper::glPolygonOffset(GLfloat factor, GLfloat units)
	{
		if (m_threaded_wrapper)
			executeCommand(GlPolygonOffsetCommand::get(factor, units));
		else
			g_glPolygonOffset(factor, units);
	}

	void FunctionWrapper::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		if (m_threaded_wrapper)
			executeCommand(GlScissorCommand::get(x, y, width, height));
		else
			g_glScissor(x, y, width, height);
	}

	void FunctionWrapper::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		if (m_threaded_wrapper)
			executeCommand(GlViewportCommand::get(x, y, width, height));
		else
			g_glViewport(x, y, width, height);
	}

	void FunctionWrapper::glBindTexture(GLenum target, GLuint texture)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindTextureCommand::get(target, texture));
		else
			g_glBindTexture(target, texture);
	}

	void FunctionWrapper::glTexParameteri(GLenum target, GLenum pname, GLint param)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTexParameteriCommand::get(target, pname, param));
		else
			g_glTexParameteri(target, pname, param);
	}

	void FunctionWrapper::glGetIntegerv(GLenum pname, GLint* data)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetIntegervCommand::get(pname, data));
		else
			g_glGetIntegerv(pname, data);
	}

	const GLubyte* FunctionWrapper::glGetString(GLenum name)
	{
		const GLubyte* returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlGetStringCommand::get(name, returnValue));
		else
			returnValue = g_glGetString(name);

		return returnValue;
	}

	void FunctionWrapper::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
	{
		if (m_threaded_wrapper)
			executeCommand(GlReadPixelsCommand::get(x, y, width, height, format, type, pixels));
		else
			g_glReadPixels(x, y, width, height, format, type, pixels);
	}

	void FunctionWrapper::glReadPixelsAsync(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type)
	{
		if (m_threaded_wrapper)
			executeCommand(GlReadPixelsAsyncCommand::get(x, y, width, height, format, type));
		else
			g_glReadPixels(x, y, width, height, format, type, nullptr);
	}

	void FunctionWrapper::glTexSubImage2DBuffered(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::size_t offset)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTexSubImage2DBufferedCommand::get(target, level, xoffset, yoffset, width, height, format, type, offset));
		else
			g_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, (const GLvoid *)offset);
	}

	void FunctionWrapper::glDrawArrays(GLenum mode, GLint first, GLsizei count)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDrawArraysCommand::get(mode, first, count));
		else
			g_glDrawArrays(mode, first, count);
	}

	void FunctionWrapper::glDrawArraysUnbuffered(GLenum mode, GLint first, GLsizei count, std::unique_ptr<std::vector<char>> data)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDrawArraysUnbufferedCommand::get(mode, first, count, std::move(data)));
		else {
			auto command = GlDrawArraysUnbufferedCommand::get(mode, first, count, std::move(data));
			command->performCommandSingleThreaded();
		}
	}

	GLenum FunctionWrapper::glGetError(void)
	{
#ifdef GL_DEBUG
		GLenum returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlGetErrorCommand::get(returnValue));
		else
			returnValue = g_glGetError();

		return returnValue;
#else
		return GL_NO_ERROR;
#endif
	}

	void FunctionWrapper::glDrawElementsNotThreadSafe(GLenum mode, GLsizei count, GLenum type, const void *indices)
	{
		g_glDrawElements(mode, count, type, indices);
	}

	void FunctionWrapper::glLineWidth(GLfloat width)
	{
		if (m_threaded_wrapper)
			executeCommand(GlLineWidthCommand::get(width));
		else
			g_glLineWidth(width);
	}

	void FunctionWrapper::glClear(GLbitfield mask)
	{
		if (m_threaded_wrapper)
			executeCommand(GlClearCommand::get(mask));
		else
			g_glClear(mask);
	}

	void FunctionWrapper::glGetFloatv(GLenum pname, GLfloat* data)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlGetFloatvCommand::get(pname, data));
		else
			g_glGetFloatv(pname, data);
	}

	void FunctionWrapper::glDeleteTextures(GLsizei n, std::unique_ptr<GLuint[]> textures)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteTexturesCommand::get(n, std::move(textures)));
		else
			g_glDeleteTextures(n, textures.get());
	}

	void FunctionWrapper::glGenTextures(GLsizei n, GLuint* textures)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlGenTexturesCommand::get(n, textures));
		else
			g_glGenTextures(n, textures);
	}

	void FunctionWrapper::glTexParameterf(GLenum target, GLenum pname, GLfloat param)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTexParameterfCommand::get(target, pname, param));
		else
			g_glTexParameterf(target, pname, param);
	}

	void FunctionWrapper::glActiveTexture(GLenum texture)
	{
		if (m_threaded_wrapper)
			executeCommand(GlActiveTextureCommand::get(texture));
		else
			g_glActiveTexture(texture);
	}

	void FunctionWrapper::glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBlendColorCommand::get(red, green, blue, alpha));
		else
			g_glBlendColor(red, green, blue, alpha);
	}

	void FunctionWrapper::glReadBuffer(GLenum src)
	{
		if (m_threaded_wrapper)
			executeCommand(GlReadBufferCommand::get(src));
		else
			g_glReadBuffer(src);
	}

	GLuint FunctionWrapper::glCreateShader(GLenum type)
	{
		GLuint returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlCreateShaderCommand::get(type, returnValue));
		else
			returnValue = g_glCreateShader(type);

		return returnValue;
	}

	void FunctionWrapper::glCompileShader(GLuint shader)
	{
		if (m_threaded_wrapper)
			executeCommand(GlCompileShaderCommand::get(shader));
		else
			g_glCompileShader(shader);
	}

	void FunctionWrapper::glShaderSource(GLuint shader, const std::string& string)
	{
		if (m_threaded_wrapper)
			executeCommand(GlShaderSourceCommand::get(shader, string));
		else {
			const GLchar* strShaderData = string.data();
			g_glShaderSource(shader, 1, &strShaderData, nullptr);
		}
	}

	GLuint FunctionWrapper::glCreateProgram(void)
	{
		GLuint returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlCreateProgramCommand::get(returnValue));
		else
			returnValue = g_glCreateProgram();

		return returnValue;
	}

	void FunctionWrapper::glAttachShader(GLuint program, GLuint shader)
	{
		if (m_threaded_wrapper)
			executeCommand(GlAttachShaderCommand::get(program, shader));
		else
			g_glAttachShader(program, shader);
	}

	void FunctionWrapper::glLinkProgram(GLuint program)
	{
		if (m_threaded_wrapper)
			executeCommand(GlLinkProgramCommand::get(program));
		else
			g_glLinkProgram(program);
	}

	void FunctionWrapper::glUseProgram(GLuint program)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUseProgramCommand::get(program));
		else
			g_glUseProgram(program);
	}

	GLint FunctionWrapper::glGetUniformLocation(GLuint program, const GLchar *name)
	{
		GLint returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlGetUniformLocationCommand::get(program, name, returnValue));
		else
			returnValue = g_glGetUniformLocation(program, name);

		return returnValue;
	}

	void FunctionWrapper::glUniform1i(GLint location, GLint v0)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform1iCommand::get(location, v0));
		else
			g_glUniform1i(location, v0);
	}

	void FunctionWrapper::glUniform1f(GLint location, GLfloat v0)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform1fCommand::get(location, v0));
		else
			g_glUniform1f(location, v0);
	}

	void FunctionWrapper::glUniform2f(GLint location, GLfloat v0, GLfloat v1)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform2fCommand::get(location, v0, v1));
		else
			g_glUniform2f(location, v0, v1);
	}

	void FunctionWrapper::glUniform2i(GLint location, GLint v0, GLint v1)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform2iCommand::get(location, v0, v1));
		else
			g_glUniform2i(location, v0, v1);
	}

	void FunctionWrapper::glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform4iCommand::get(location, v0, v1, v2, v3));
		else
			g_glUniform4i(location, v0, v1, v2, v3);
	}


	void FunctionWrapper::glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform4fCommand::get(location, v0, v1, v2, v3));
		else
			g_glUniform4f(location, v0, v1, v2, v3);
	}

	void FunctionWrapper::glUniform3fv(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform3fvCommand::get(location, count, std::move(value)));
		else
			g_glUniform3fv(location, count, value.get());
	}

	void FunctionWrapper::glUniform4fv(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniform4fvCommand::get(location, count, std::move(value)));
		else
			g_glUniform4fv(location, count, value.get());
	}

	void FunctionWrapper::glDetachShader(GLuint program, GLuint shader)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDetachShaderCommand::get(program, shader));
		else
			g_glDetachShader(program, shader);
	}

	void FunctionWrapper::glDeleteShader(GLuint shader)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteShaderCommand::get(shader));
		else
			g_glDeleteShader(shader);
	}

	void FunctionWrapper::glDeleteProgram(GLuint program)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteProgramCommand::get(program));
		else
			g_glDeleteProgram(program);
	}

	void FunctionWrapper::glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar *infoLog)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetProgramInfoLogCommand::get(program, bufSize, length, infoLog));
		else
			g_glGetProgramInfoLog(program, bufSize, length, infoLog);
	}

	void FunctionWrapper::glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar *infoLog)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetShaderInfoLogCommand::get(shader, bufSize, length, infoLog));
		else
			g_glGetShaderInfoLog(shader, bufSize, length, infoLog);
	}

	void FunctionWrapper::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetShaderivCommand::get(shader, pname, params));
		else
			g_glGetShaderiv(shader, pname, params);
	}

	void FunctionWrapper::glGetProgramiv(GLuint program, GLenum pname, GLint* params)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetProgramivCommand::get(program, pname, params));
		else
			g_glGetProgramiv(program, pname, params);
	}

	void FunctionWrapper::glEnableVertexAttribArray(GLuint index)
	{
		if (m_threaded_wrapper)
			executeCommand(GlEnableVertexAttribArrayCommand::get(index));
		else
			g_glEnableVertexAttribArray(index);
	}

	void FunctionWrapper::glDisableVertexAttribArray(GLuint index)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDisableVertexAttribArrayCommand::get(index));
		else
			g_glDisableVertexAttribArray(index);
	}

	void FunctionWrapper::glVertexAttribPointerBuffered(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, std::size_t offset)
	{
		if (m_threaded_wrapper)
			executeCommand(GlVertexAttribPointerBufferedCommand::get(index, size, type, normalized, stride, offset));
		else
			g_glVertexAttribPointer(index, size, type, normalized, stride, (const GLvoid *)(offset));
	}

	void FunctionWrapper::glVertexAttribPointerNotThreadSafe(GLuint index, GLint size, GLenum type, GLboolean normalized,
		GLsizei stride, const void *pointer)
	{
		g_glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	}

	void FunctionWrapper::glVertexAttribPointerUnbuffered(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
		std::size_t offset)
	{
		if (m_threaded_wrapper)
			executeCommand(GlVertexAttribPointerUnbufferedCommand::get(index, size, type, normalized, stride, offset));
		else {
			auto command = GlVertexAttribPointerUnbufferedCommand::get(index, size, type, normalized, stride, offset);
			command->performCommandSingleThreaded();
		}
	}

	void FunctionWrapper::glBindAttribLocation(GLuint program, GLuint index, const std::string& name)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindAttribLocationCommand::get(program, index, std::move(name)));
		else
			g_glBindAttribLocation(program, index, name.data());
	}

	void FunctionWrapper::glVertexAttrib1f(GLuint index, GLfloat x)
	{
		if (m_threaded_wrapper)
			executeCommand(GlVertexAttrib1fCommand::get(index, x));
		else
			g_glVertexAttrib1f(index, x);
	}

	void FunctionWrapper::glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
	{
		if (m_threaded_wrapper)
			executeCommand(GlVertexAttrib4fCommand::get(index, x, y, z, w));
		else
			g_glVertexAttrib4f(index, x, y, z, w);
	}

	void FunctionWrapper::glVertexAttrib4fv(GLuint index, std::unique_ptr<GLfloat[]> v)
	{
		if (m_threaded_wrapper)
			executeCommand(GlVertexAttrib4fvCommand::get(index, std::move(v)));
		else
			g_glVertexAttrib4fv(index, v.get());
	}

	void FunctionWrapper::glDepthRangef(GLfloat n, GLfloat f)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDepthRangefCommand::get(n, f));
		else
			g_glDepthRangef(n, f);
	}

	void FunctionWrapper::glClearDepthf(GLfloat d)
	{
		if (m_threaded_wrapper)
			executeCommand(GlClearDepthfCommand::get(d));
		else
			g_glClearDepthf(d);
	}

	void FunctionWrapper::glDrawBuffers(GLsizei n, std::unique_ptr<GLenum[]> bufs)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDrawBuffersCommand::get(n, std::move(bufs)));
		else
			g_glDrawBuffers(n, bufs.get());
	}

	void FunctionWrapper::glGenFramebuffers(GLsizei n, GLuint* framebuffers)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlGenFramebuffersCommand::get(n, framebuffers));
		else
			g_glGenFramebuffers(n, framebuffers);
	}

	void FunctionWrapper::glBindFramebuffer(GLenum target, GLuint framebuffer)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindFramebufferCommand::get(target, framebuffer));
		else
			g_glBindFramebuffer(target, framebuffer);
	}

	void FunctionWrapper::glDeleteFramebuffers(GLsizei n, std::unique_ptr<GLuint[]> framebuffers)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteFramebuffersCommand::get(n, std::move(framebuffers)));
		else
			g_glDeleteFramebuffers(n, framebuffers.get());
	}

	void FunctionWrapper::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
	{
		if (m_threaded_wrapper)
			executeCommand(GlFramebufferTexture2DCommand::get(target, attachment, textarget, texture, level));
		else
			g_glFramebufferTexture2D(target, attachment, textarget, texture, level);
	}

	void FunctionWrapper::glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTexImage2DMultisampleCommand::get(target, samples, internalformat, width, height, fixedsamplelocations));
		else
			g_glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
	}

	void FunctionWrapper::glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTexStorage2DMultisampleCommand::get(target, samples, internalformat, width, height, fixedsamplelocations));
		else
			g_glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
	}

	void FunctionWrapper::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlGenRenderbuffersCommand::get(n, renderbuffers));
		else
			g_glGenRenderbuffers(n, renderbuffers);
	}

	void FunctionWrapper::glBindRenderbuffer(GLenum target, GLuint renderbuffer)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindRenderbufferCommand::get(target, renderbuffer));
		else
			g_glBindRenderbuffer(target, renderbuffer);
	}

	void FunctionWrapper::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
	{
		if (m_threaded_wrapper)
			executeCommand(GlRenderbufferStorageCommand::get(target, internalformat, width, height));
		else
			g_glRenderbufferStorage(target, internalformat, width, height);
	}

	void FunctionWrapper::glDeleteRenderbuffers(GLsizei n, std::unique_ptr<GLuint[]> renderbuffers)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteRenderbuffersCommand::get(n, std::move(renderbuffers)));
		else
			g_glDeleteRenderbuffers(n, renderbuffers.get());
	}

	void FunctionWrapper::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
	{
		if (m_threaded_wrapper)
			executeCommand(GlFramebufferRenderbufferCommand::get(target, attachment, renderbuffertarget, renderbuffer));
		else
			g_glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
	}

	GLenum FunctionWrapper::glCheckFramebufferStatus(GLenum target)
	{
#ifdef GL_DEBUG
		GLenum returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlCheckFramebufferStatusCommand::get(target, returnValue));
		else
			returnValue = g_glCheckFramebufferStatus(target);

		return returnValue;
#else
		return GL_FRAMEBUFFER_COMPLETE;
#endif
	}

	void FunctionWrapper::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBlitFramebufferCommand::get(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
		else
			g_glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	}

	void FunctionWrapper::glGenVertexArrays(GLsizei n, GLuint* arrays)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlGenVertexArraysCommand::get(n, arrays));
		else
			g_glGenVertexArrays(n, arrays);
	}

	void FunctionWrapper::glBindVertexArray(GLuint array)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindVertexArrayCommand::get(array));
		else
			g_glBindVertexArray(array);
	}

	void FunctionWrapper::glDeleteVertexArrays(GLsizei n, std::unique_ptr<GLuint[]> arrays)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteVertexArraysCommand::get(n, std::move(arrays)));
		else
			g_glDeleteVertexArrays(n, arrays.get());
	}

	void FunctionWrapper::glGenBuffers(GLsizei n, GLuint* buffers)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlGenBuffersCommand::get(n, buffers));
		else
			g_glGenBuffers(n, buffers);
	}

	void FunctionWrapper::glBindBuffer(GLenum target, GLuint buffer)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindBufferCommand::get(target, buffer));
		else
			g_glBindBuffer(target, buffer);
	}

	void FunctionWrapper::glMapBuffer(GLenum target, GLenum access)
	{
		if (m_threaded_wrapper)
			executeCommand(GlMapBufferCommand::get(target, access));
		else
			g_glMapBuffer(target, access);
	}

	void* FunctionWrapper::glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
	{
		GLubyte* returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlMapBufferRangeCommand::get(target, offset, length, access, returnValue));
		else
			returnValue = reinterpret_cast<GLubyte*>(g_glMapBufferRange(target, offset, length, access));

		return returnValue;
	}

	void FunctionWrapper::glMapBufferRangeWriteAsync(GLenum target, GLuint buffer, GLintptr offset, u32 length, GLbitfield access, std::unique_ptr<u8[]> data)
	{
		if (m_threaded_wrapper)
			executeCommand(GlMapBufferRangeWriteAsyncCommand::get(target, buffer, offset, length, access, std::move(data)));
		else {
			auto command = GlMapBufferRangeWriteAsyncCommand::get(target, buffer, offset, length, access, std::move(data));
			command->performCommandSingleThreaded();
		}
	}

	std::shared_ptr<std::vector<u8>> FunctionWrapper::glMapBufferRangeReadAsync(GLenum target, GLuint buffer, GLintptr offset, u32 length, GLbitfield access)
	{
		if (m_threaded_wrapper)
			executeCommand(GlMapBufferRangeReadAsyncCommand::get(target, buffer, offset, length, access));
		else {
			auto command = GlMapBufferRangeReadAsyncCommand::get(target, buffer, offset, length, access);
			command->performCommandSingleThreaded();
		}
		return GlMapBufferRangeReadAsyncCommand::getData(buffer, length);
	}

	GLboolean FunctionWrapper::glUnmapBuffer(GLenum target)
	{
		GLboolean returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlUnmapBufferCommand::get(target, returnValue));
		else
			returnValue = g_glUnmapBuffer(target);

		return returnValue;
	}

	void FunctionWrapper::glUnmapBufferAsync(GLenum target)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUnmapBufferAsyncCommand::get(target));
		else
			g_glUnmapBuffer(target);
	}

	void FunctionWrapper::glDeleteBuffers(GLsizei n, std::unique_ptr<GLuint[]> buffers)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteBuffersCommand::get(n, std::move(buffers)));
		else
			g_glDeleteBuffers(n, buffers.get());
	}

	void FunctionWrapper::glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindImageTextureCommand::get(unit, texture, level, layered, layer, access, format));
		else
			g_glBindImageTexture(unit, texture, level, layered, layer, access, format);
	}

	void FunctionWrapper::glMemoryBarrier(GLbitfield barriers)
	{
		if (m_threaded_wrapper)
			executeCommand(GlMemoryBarrierCommand::get(barriers));
		else
			g_glMemoryBarrier(barriers);
	}

	const GLubyte* FunctionWrapper::glGetStringi(GLenum name, GLuint index)
	{
		const GLubyte* returnValue;

		if (m_threaded_wrapper)
			executePriorityCommand(GlGetStringiCommand::get(name, index, returnValue));
		else
			returnValue = g_glGetStringi(name, index);

		return returnValue;
	}

	void FunctionWrapper::glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, std::unique_ptr<GLenum[]> attachments)
	{
		if (m_threaded_wrapper)
			executeCommand(GlInvalidateFramebufferCommand::get(target, numAttachments, std::move(attachments)));
		else
			g_glInvalidateFramebuffer(target, numAttachments, attachments.get());
	}

	GLsync FunctionWrapper::glFenceSync(GLenum condition, GLbitfield flags)
	{
		GLsync returnValue;

		if (m_threaded_wrapper)
			executePriorityCommand(GlFenceSyncCommand::get(condition, flags, returnValue));
		else
			returnValue = g_glFenceSync(condition, flags);

		return returnValue;
	}

	void FunctionWrapper::glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlClientWaitSyncCommand::get(sync, flags, timeout));
		else
			g_glClientWaitSync(sync, flags, timeout);
	}

	void FunctionWrapper::glDeleteSync(GLsync sync)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDeleteSyncCommand::get(sync));
		else
			g_glDeleteSync(sync);
	}

	GLuint FunctionWrapper::glGetUniformBlockIndex(GLuint program, GLchar *uniformBlockName)
	{
		GLuint returnValue;

		if (m_threaded_wrapper)
			executeCommand(GlGetUniformBlockIndexCommand::get(program, uniformBlockName, returnValue));
		else
			returnValue = g_glGetUniformBlockIndex(program, uniformBlockName);

		return returnValue;
	}

	void FunctionWrapper::glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
	{
		if (m_threaded_wrapper)
			executeCommand(GlUniformBlockBindingCommand::get(program, uniformBlockIndex, uniformBlockBinding));
		else
			g_glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
	}

	void FunctionWrapper::glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetActiveUniformBlockivCommand::get(program, uniformBlockIndex, pname, params));
		else
			g_glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
	}

	void FunctionWrapper::glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint* uniformIndices)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetUniformIndicesCommand::get(program, uniformCount, uniformNames, uniformIndices));
		else
			g_glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
	}

	void FunctionWrapper::glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint* params)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetActiveUniformsivCommand::get(program, uniformCount, uniformIndices, pname, params));
		else
			g_glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
	}

	void FunctionWrapper::glBindBufferBase(GLenum target, GLuint index, GLuint buffer)
	{
		if (m_threaded_wrapper)
			executeCommand(GlBindBufferBaseCommand::get(target, index, buffer));
		else
			g_glBindBufferBase(target, index, buffer);
	}

	void FunctionWrapper::glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void *binary)
	{
		if (m_threaded_wrapper)
			executeCommand(GlGetProgramBinaryCommand::get(program, bufSize, length, binaryFormat, binary));
		else
			g_glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
	}

	void FunctionWrapper::glProgramParameteri(GLuint program, GLenum pname, GLint value)
	{
		if (m_threaded_wrapper)
			executeCommand(GlProgramParameteriCommand::get(program, pname, value));
		else
			g_glProgramParameteri(program, pname, value);
	}

	void FunctionWrapper::glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTexStorage2DCommand::get(target, levels, internalformat, width, height));
		else
			g_glTexStorage2D(target, levels, internalformat, width, height);
	}

	void FunctionWrapper::glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTextureStorage2DCommand::get(texture, levels, internalformat, width, height));
		else
			g_glTextureStorage2D(texture, levels, internalformat, width, height);
	}

	void FunctionWrapper::glTextureSubImage2DBuffered(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, std::size_t offset)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTextureSubImage2DBufferedCommand::get(texture, level, xoffset, yoffset, width, height, format, type, offset));
		else
			g_glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, (const GLvoid* )offset);
	}

	void FunctionWrapper::glTextureStorage2DMultisample(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTextureStorage2DMultisampleCommand::get(texture, target, samples, internalformat, width, height, fixedsamplelocations));
		else
			g_glTextureStorage2DMultisample(texture, target, samples, internalformat, width, height, fixedsamplelocations);
	}

	void FunctionWrapper::glTextureParameteri(GLuint texture, GLenum pname, GLint param)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTextureParameteriCommand::get(texture, pname, param));
		else
			g_glTextureParameteri(texture, pname, param);
	}

	void FunctionWrapper::glTextureParameterf(GLuint texture, GLenum pname, GLfloat param)
	{
		if (m_threaded_wrapper)
			executeCommand(GlTextureParameterfCommand::get(texture, pname, param));
		else
			g_glTextureParameterf(texture, pname, param);
	}

	void FunctionWrapper::glCreateTextures(GLenum target, GLsizei n, GLuint* textures)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlCreateTexturesCommand::get(target, n, textures));
		else
			g_glCreateTextures(target, n, textures);
	}

	void FunctionWrapper::glCreateBuffers(GLsizei n, GLuint* buffers)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlCreateBuffersCommand::get(n, buffers));
		else
			g_glCreateBuffers(n, buffers);
	}

	void FunctionWrapper::glCreateFramebuffers(GLsizei n, GLuint* framebuffers)
	{
		if (m_threaded_wrapper)
			executePriorityCommand(GlCreateFramebuffersCommand::get(n, framebuffers));
		else
			g_glCreateFramebuffers(n, framebuffers);
	}

	void FunctionWrapper::glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level)
	{
		if (m_threaded_wrapper)
			executeCommand(GlNamedFramebufferTextureCommand::get(framebuffer, attachment, texture, level));
		else
			g_glNamedFramebufferTexture(framebuffer, attachment, texture, level);
	}

	void FunctionWrapper::glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const char* indices, GLint basevertex)
	{
		if (m_threaded_wrapper)
			executeCommand(GlDrawElementsBaseVertexCommand::get(mode, count, type, std::move(indices), basevertex));
		else
			g_glDrawElementsBaseVertex(mode, count, type, std::move(indices), basevertex);
	}

	void FunctionWrapper::glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
	{
		if (m_threaded_wrapper)
			executeCommand(GlFlushMappedBufferRangeCommand::get(target, offset, length));
		else
			g_glFlushMappedBufferRange(target, offset, length);
	}

	void FunctionWrapper::glFinish(void)
	{
		if (m_threaded_wrapper)
			executeCommand(GlFinishCommand::get());
		else
			g_glFinish();
	}

#ifdef MUPENPLUSAPI

	void FunctionWrapper::CoreVideo_Init(void)
	{
		if (m_threaded_wrapper)
			executeCommand(CoreVideoInitCommand::get());
		else
			CoreVideoInitCommand::get()->performCommandSingleThreaded();
	}

	void FunctionWrapper::CoreVideo_Quit(void)
	{
		if (m_threaded_wrapper)
			executeCommand(CoreVideoQuitCommand::get());
		else
			CoreVideoQuitCommand::get()->performCommandSingleThreaded();

		m_shutdown = true;

		if (m_threaded_wrapper) {
			m_condition.notify_all();
			m_commandExecutionThread.join();
		}
	}

	m64p_error FunctionWrapper::CoreVideo_SetVideoMode(int screenWidth, int screenHeight, int bitsPerPixel, m64p_video_mode mode, m64p_video_flags flags)
	{
		m64p_error returnValue;

		if (m_threaded_wrapper)
			executeCommand(CoreVideoSetVideoModeCommand::get(screenWidth, screenHeight, bitsPerPixel, mode, flags, returnValue));
		else
			CoreVideoSetVideoModeCommand::get(screenWidth, screenHeight, bitsPerPixel, mode, flags, returnValue)->performCommandSingleThreaded();

		return returnValue;
	}

	void FunctionWrapper::CoreVideo_GL_SetAttribute(m64p_GLattr attribute, int value)
	{
		if (m_threaded_wrapper)
			executeCommand(CoreVideoGLSetAttributeCommand::get(attribute, value));
		else
			CoreVideoGLSetAttributeCommand::get(attribute, value)->performCommandSingleThreaded();
	}

	void FunctionWrapper::CoreVideo_GL_GetAttribute(m64p_GLattr attribute, int *value)
	{
		if (m_threaded_wrapper)
			executeCommand(CoreVideoGLGetAttributeCommand::get(attribute, value));
		else
			CoreVideoGLGetAttributeCommand::get(attribute, value)->performCommandSingleThreaded();
	}

	void FunctionWrapper::CoreVideo_GL_SwapBuffers(void)
	{
		++m_swapBuffersQueued;

		if (m_threaded_wrapper)
			executeCommand(CoreVideoGLSwapBuffersCommand::get([]{ReduceSwapBuffersQueued();}));
		else
			CoreVideoGLSwapBuffersCommand::get([]{ReduceSwapBuffersQueued();})->performCommandSingleThreaded();
	}
#else
	bool FunctionWrapper::windowsStart(void)
	{
		bool returnValue;

		if (m_threaded_wrapper)
			executeCommand(WindowsStartCommand::get(returnValue));
		else
			WindowsStartCommand::get(returnValue)->performCommandSingleThreaded();

		return returnValue;
	}

	void FunctionWrapper::windowsStop(void)
	{
		if (m_threaded_wrapper)
			executeCommand(WindowsStopCommand::get());
		else
			WindowsStopCommand::get()->performCommandSingleThreaded();

		m_shutdown = true;

		if (m_threaded_wrapper) {
			m_condition.notify_all();
			m_commandExecutionThread.join();
		}
	}

	void FunctionWrapper::windowsSwapBuffers(void)
	{
		++m_swapBuffersQueued;

		if (m_threaded_wrapper)
			executeCommand(WindowsSwapBuffersCommand::get([]{ReduceSwapBuffersQueued();}));
		else
			WindowsSwapBuffersCommand::get([]{ReduceSwapBuffersQueued();})->performCommandSingleThreaded();
	}

#endif

	void FunctionWrapper::ReduceSwapBuffersQueued(void)
	{
		--m_swapBuffersQueued;

		if (m_swapBuffersQueued <= MAX_SWAP) {
			m_condition.notify_all();
		}
	}

	void FunctionWrapper::WaitForSwapBuffersQueued(void)
	{
		std::unique_lock<std::mutex> lock(m_condvarMutex);

		if (!m_shutdown && m_swapBuffersQueued > MAX_SWAP) {
			m_condition.wait(lock, []{return FunctionWrapper::m_swapBuffersQueued <= MAX_SWAP;});
		}
	}
}
