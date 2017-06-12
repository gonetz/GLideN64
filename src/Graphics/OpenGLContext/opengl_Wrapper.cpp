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
		while(!m_shutdown || m_commandQueue.size() != 0)
		{
			std::shared_ptr<OpenGlCommand> command;

			if(m_commandQueue.tryPop(command, std::chrono::milliseconds(10))) {
				command->performCommand();
			}
		}
	}

	void FunctionWrapper::setThreadedMode(void)
	{
		m_threaded_wrapper = true;

		m_commandExecutionThread = std::thread(&FunctionWrapper::commandLoop);
	}

	void FunctionWrapper::glBlendFunc(const GLenum& sfactor, const GLenum& dfactor)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBlendFuncCommand>(sfactor, dfactor));
		else
			g_glBlendFunc(sfactor, dfactor);
	}

	void FunctionWrapper::glPixelStorei(const GLenum& pname, const GLint& param)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlPixelStoreiCommand>(pname, param));
		else
			g_glPixelStorei(pname, param);
	}

	void FunctionWrapper::glClearColor(const GLfloat& red, const GLfloat& green, const GLfloat& blue, const GLfloat& alpha)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlClearColorCommand>(red, green, blue, alpha));
		else
			g_glClearColor(red, green, blue, alpha);
	}

	void FunctionWrapper::glCullFace(const GLenum& mode)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlCullFaceCommand>(mode));
		else
			g_glCullFace(mode);
	}

	void FunctionWrapper::glDepthFunc(const GLenum& func)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDepthFuncCommand>(func));
		else
			g_glDepthFunc(func);
	}

	void FunctionWrapper::glDepthMask(const GLboolean& flag)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDepthMaskCommand>(flag));
		else
			g_glDepthMask(flag);
	}

	void FunctionWrapper::glDisable(const GLenum& cap)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDisableCommand>(cap));
		else
			g_glDisable(cap);
	}

	void FunctionWrapper::glEnable(const GLenum& cap)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlEnableCommand>(cap));
		else
			g_glEnable(cap);
	}

	void FunctionWrapper::glPolygonOffset(const GLfloat& factor, const GLfloat& units)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlPolygonOffsetCommand>(factor, units));
		else
			g_glPolygonOffset(factor, units);
	}

	void FunctionWrapper::glScissor(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlScissorCommand>(x, y, width, height));
		else
			g_glScissor(x, y, width, height);
	}

	void FunctionWrapper::glViewport(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlViewportCommand>(x, y, width, height));
		else
			g_glViewport(x, y, width, height);
	}

	void FunctionWrapper::glBindTexture(const GLenum& target, const GLuint& texture)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindTextureCommand>(target, texture));
		else
			g_glBindTexture(target, texture);
	}

	void FunctionWrapper::glTexParameteri(const GLenum& target, const GLenum& pname, const GLint& param)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexParameteriCommand>(target, pname, param));
		else
			g_glTexParameteri(target, pname, param);
	}

	void FunctionWrapper::glGetIntegerv(const GLenum& pname, GLint* data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetIntegervCommand>(pname, data));
		else
			g_glGetIntegerv(pname, data);
	}

	const GLubyte* FunctionWrapper::glGetString(const GLenum& name)
	{
		const GLubyte* returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetStringCommand>(name, returnValue));
		else
			returnValue = g_glGetString(name);

		return returnValue;
	}

	void FunctionWrapper::glReadPixels(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, void *pixels)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlReadPixelsCommand>(x, y, width, height, format, type, pixels));
		else
			g_glReadPixels(x, y, width, height, format, type, pixels);
	}

	void FunctionWrapper::glReadPixelsAsync(const GLint& x, const GLint& y, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlReadPixelsAsyncCommand>(x, y, width, height, format, type));
		else
			g_glReadPixels(x, y, width, height, format, type, nullptr);
	}

	void FunctionWrapper::glTexSubImage2DBuffered(const GLenum& target, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::size_t offset)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexSubImage2DBufferedCommand>(target, level, xoffset, yoffset, width, height, format, type, offset));
		else
			g_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, (const GLvoid *)offset);
	}

	void FunctionWrapper::glDrawArrays(const GLenum& mode, const GLint& first, const GLsizei& count)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDrawArraysCommand>(mode, first, count));
		else
			g_glDrawArrays(mode, first, count);
	}

	void FunctionWrapper::glDrawArraysUnbuffered(const GLenum& mode, const GLint& first, const GLsizei& count, std::unique_ptr<std::vector<char>> data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDrawArraysUnbufferedCommand>(mode, first, count, std::move(data)));
		else {
			auto command = std::make_shared<GlDrawArraysUnbufferedCommand>(mode, first, count, std::move(data));
			command->performCommandSingleThreaded();
		}
	}

	GLenum FunctionWrapper::glGetError(void)
	{
#ifdef GL_DEBUG
		GLenum returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetErrorCommand>(returnValue));
		else
			returnValue = g_glGetError();

		return returnValue;
#else
		return GL_NO_ERROR;
#endif
	}

	void FunctionWrapper::glDrawElementsNotThreadSafe(const GLenum& mode, const GLsizei& count, const GLenum& type, const void *indices)
	{
		g_glDrawElements(mode, count, type, indices);
	}

	void FunctionWrapper::glLineWidth(const GLfloat& width)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlLineWidthCommand>(width));
		else
			g_glLineWidth(width);
	}

	void FunctionWrapper::glClear(const GLbitfield& mask)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlClearCommand>(mask));
		else
			g_glClear(mask);
	}

	void FunctionWrapper::glGetFloatv(const GLenum& pname, GLfloat* data)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGetFloatvCommand>(pname, data));
		else
			g_glGetFloatv(pname, data);
	}

	void FunctionWrapper::glDeleteTextures(const GLsizei& n, std::unique_ptr<GLuint[]> textures)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteTexturesCommand>(n, std::move(textures)));
		else
			g_glDeleteTextures(n, textures.get());
	}

	void FunctionWrapper::glGenTextures(const GLsizei& n, GLuint* textures)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGenTexturesCommand>(n, textures));
		else
			g_glGenTextures(n, textures);
	}

	void FunctionWrapper::glTexParameterf(const GLenum& target, const GLenum& pname, const GLfloat& param)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexParameterfCommand>(target, pname, param));
		else
			g_glTexParameterf(target, pname, param);
	}

	void FunctionWrapper::glActiveTexture(const GLenum& texture)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlActiveTextureCommand>(texture));
		else
			g_glActiveTexture(texture);
	}

	void FunctionWrapper::glBlendColor(const GLfloat& red, const GLfloat& green, const GLfloat& blue, const GLfloat& alpha)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBlendColorCommand>(red, green, blue, alpha));
		else
			g_glBlendColor(red, green, blue, alpha);
	}

	void FunctionWrapper::glReadBuffer(const GLenum& src)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlReadBufferCommand>(src));
		else
			g_glReadBuffer(src);
	}

	GLuint FunctionWrapper::glCreateShader(const GLenum& type)
	{
		GLuint returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlCreateShaderCommand>(type, returnValue));
		else
			returnValue = g_glCreateShader(type);

		return returnValue;
	}

	void FunctionWrapper::glCompileShader(const GLuint& shader)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlCompileShaderCommand>(shader));
		else
			g_glCompileShader(shader);
	}

	void FunctionWrapper::glShaderSource(const GLuint& shader, const std::string& string)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlShaderSourceCommand>(shader, string));
		else {
			const GLchar* strShaderData = string.data();
			g_glShaderSource(shader, 1, &strShaderData, nullptr);
		}
	}

	GLuint FunctionWrapper::glCreateProgram(void)
	{
		GLuint returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlCreateProgramCommand>(returnValue));
		else
			returnValue = g_glCreateProgram();

		return returnValue;
	}

	void FunctionWrapper::glAttachShader(const GLuint& program, const GLuint& shader)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlAttachShaderCommand>(program, shader));
		else
			g_glAttachShader(program, shader);
	}

	void FunctionWrapper::glLinkProgram(const GLuint& program)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlLinkProgramCommand>(program));
		else
			g_glLinkProgram(program);
	}

	void FunctionWrapper::glUseProgram(const GLuint& program)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUseProgramCommand>(program));
		else
			g_glUseProgram(program);
	}

	GLint FunctionWrapper::glGetUniformLocation(const GLuint& program, const GLchar *name)
	{
		GLint returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetUniformLocationCommand>(program, name, returnValue));
		else
			returnValue = g_glGetUniformLocation(program, name);

		return returnValue;
	}

	void FunctionWrapper::glUniform1i(const GLint& location, const GLint& v0)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform1iCommand>(location, v0));
		else
			g_glUniform1i(location, v0);
	}

	void FunctionWrapper::glUniform1f(const GLint& location, const GLfloat& v0)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform1fCommand>(location, v0));
		else
			g_glUniform1f(location, v0);
	}

	void FunctionWrapper::glUniform2f(const GLint& location, const GLfloat& v0, const GLfloat& v1)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform2fCommand>(location, v0, v1));
		else
			g_glUniform2f(location, v0, v1);
	}

	void FunctionWrapper::glUniform2i(const GLint& location, const GLint& v0, const GLint& v1)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform2iCommand>(location, v0, v1));
		else
			g_glUniform2i(location, v0, v1);
	}

	void FunctionWrapper::glUniform4i(const GLint& location, const GLint& v0, const GLint& v1, const GLint& v2, const GLint& v3)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform4iCommand>(location, v0, v1, v2, v3));
		else
			g_glUniform4i(location, v0, v1, v2, v3);
	}


	void FunctionWrapper::glUniform4f(const GLint& location, const GLfloat& v0, const GLfloat& v1, const GLfloat& v2, const GLfloat& v3)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform4fCommand>(location, v0, v1, v2, v3));
		else
			g_glUniform4f(location, v0, v1, v2, v3);
	}

	void FunctionWrapper::glUniform3fv(const GLint& location, const GLsizei& count, std::unique_ptr<GLfloat[]> value)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform3fvCommand>(location, count, std::move(value)));
		else
			g_glUniform3fv(location, count, value.get());
	}

	void FunctionWrapper::glUniform4fv(const GLint& location, const GLsizei& count, std::unique_ptr<GLfloat[]> value)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniform4fvCommand>(location, count, std::move(value)));
		else
			g_glUniform4fv(location, count, value.get());
	}

	void FunctionWrapper::glDetachShader(const GLuint& program, const GLuint& shader)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDetachShaderCommand>(program, shader));
		else
			g_glDetachShader(program, shader);
	}

	void FunctionWrapper::glDeleteShader(const GLuint& shader)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteShaderCommand>(shader));
		else
			g_glDeleteShader(shader);
	}

	void FunctionWrapper::glDeleteProgram(const GLuint& program)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteProgramCommand>(program));
		else
			g_glDeleteProgram(program);
	}

	void FunctionWrapper::glGetProgramInfoLog(const GLuint& program, const GLsizei& bufSize, GLsizei* length, GLchar *infoLog)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetProgramInfoLogCommand>(program, bufSize, length, infoLog));
		else
			g_glGetProgramInfoLog(program, bufSize, length, infoLog);
	}

	void FunctionWrapper::glGetShaderInfoLog(const GLuint& shader, const GLsizei& bufSize, GLsizei* length, GLchar *infoLog)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetShaderInfoLogCommand>(shader, bufSize, length, infoLog));
		else
			g_glGetShaderInfoLog(shader, bufSize, length, infoLog);
	}

	void FunctionWrapper::glGetShaderiv(const GLuint& shader, const GLenum& pname, GLint* params)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetShaderivCommand>(shader, pname, params));
		else
			g_glGetShaderiv(shader, pname, params);
	}

	void FunctionWrapper::glGetProgramiv(const GLuint& program, const GLenum& pname, GLint* params)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetProgramivCommand>(program, pname, params));
		else
			g_glGetProgramiv(program, pname, params);
	}

	void FunctionWrapper::glEnableVertexAttribArray(const GLuint& index)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlEnableVertexAttribArrayCommand>(index));
		else
			g_glEnableVertexAttribArray(index);
	}

	void FunctionWrapper::glDisableVertexAttribArray(const GLuint& index)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDisableVertexAttribArrayCommand>(index));
		else
			g_glDisableVertexAttribArray(index);
	}

	void FunctionWrapper::glVertexAttribPointerBuffered(const GLuint& index, const GLint& size, const GLenum& type, const GLboolean& normalized, const GLsizei& stride, std::size_t offset)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlVertexAttribPointerBufferedCommand>(index, size, type, normalized, stride, offset));
		else
			g_glVertexAttribPointer(index, size, type, normalized, stride, (const GLvoid *)(offset));
	}

	void FunctionWrapper::glVertexAttribPointerNotThreadSafe(const GLuint& index, const GLint& size, const GLenum& type, const GLboolean& normalized,
		const GLsizei& stride, const void *pointer)
	{
		g_glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	}

	void FunctionWrapper::glVertexAttribPointerUnbuffered(const GLuint& index, const GLint& size, const GLenum& type, const GLboolean& normalized, const GLsizei& stride,
		std::size_t offset)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlVertexAttribPointerUnbufferedCommand>(index, size, type, normalized, stride, offset));
		else {
			auto command = std::make_shared<GlVertexAttribPointerUnbufferedCommand>(index, size, type, normalized, stride, offset);
			command->performCommandSingleThreaded();
		}
	}

	void FunctionWrapper::glBindAttribLocation(const GLuint& program, const GLuint& index, const std::string& name)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindAttribLocationCommand>(program, index, std::move(name)));
		else
			g_glBindAttribLocation(program, index, name.data());
	}

	void FunctionWrapper::glVertexAttrib1f(const GLuint& index, const GLfloat& x)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlVertexAttrib1fCommand>(index, x));
		else
			g_glVertexAttrib1f(index, x);
	}

	void FunctionWrapper::glVertexAttrib4f(const GLuint& index, const GLfloat& x, const GLfloat& y, const GLfloat& z, const GLfloat& w)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlVertexAttrib4fCommand>(index, x, y, z, w));
		else
			g_glVertexAttrib4f(index, x, y, z, w);
	}

	void FunctionWrapper::glVertexAttrib4fv(const GLuint& index, std::unique_ptr<GLfloat[]> v)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlVertexAttrib4fvCommand>(index, std::move(v)));
		else
			g_glVertexAttrib4fv(index, v.get());
	}

	void FunctionWrapper::glDepthRangef(const GLfloat& n, const GLfloat& f)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDepthRangefCommand>(n, f));
		else
			g_glDepthRangef(n, f);
	}

	void FunctionWrapper::glClearDepthf(const GLfloat& d)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlClearDepthfCommand>(d));
		else
			g_glClearDepthf(d);
	}

	void FunctionWrapper::glDrawBuffers(const GLsizei& n, std::unique_ptr<GLenum[]> bufs)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDrawBuffersCommand>(n, std::move(bufs)));
		else
			g_glDrawBuffers(n, bufs.get());
	}

	void FunctionWrapper::glGenFramebuffers(const GLsizei& n, GLuint* framebuffers)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGenFramebuffersCommand>(n, framebuffers));
		else
			g_glGenFramebuffers(n, framebuffers);
	}

	void FunctionWrapper::glBindFramebuffer(const GLenum& target, const GLuint& framebuffer)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindFramebufferCommand>(target, framebuffer));
		else
			g_glBindFramebuffer(target, framebuffer);
	}

	void FunctionWrapper::glDeleteFramebuffers(const GLsizei& n, std::unique_ptr<GLuint[]> framebuffers)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteFramebuffersCommand>(n, std::move(framebuffers)));
		else
			g_glDeleteFramebuffers(n, framebuffers.get());
	}

	void FunctionWrapper::glFramebufferTexture2D(const GLenum& target, const GLenum& attachment, const GLenum& textarget, const GLuint& texture, const GLint& level)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlFramebufferTexture2DCommand>(target, attachment, textarget, texture, level));
		else
			g_glFramebufferTexture2D(target, attachment, textarget, texture, level);
	}

	void FunctionWrapper::glTexImage2DMultisample(const GLenum& target, const GLsizei& samples, const GLenum& internalformat, const GLsizei& width, const GLsizei& height, const GLboolean& fixedsamplelocations)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexImage2DMultisampleCommand>(target, samples, internalformat, width, height, fixedsamplelocations));
		else
			g_glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
	}

	void FunctionWrapper::glTexStorage2DMultisample(const GLenum& target, const GLsizei& samples, const GLenum& internalformat, const GLsizei& width, const GLsizei& height, const GLboolean& fixedsamplelocations)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexStorage2DMultisampleCommand>(target, samples, internalformat, width, height, fixedsamplelocations));
		else
			g_glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
	}

	void FunctionWrapper::glGenRenderbuffers(const GLsizei& n, GLuint* renderbuffers)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGenRenderbuffersCommand>(n, renderbuffers));
		else
			g_glGenRenderbuffers(n, renderbuffers);
	}

	void FunctionWrapper::glBindRenderbuffer(const GLenum& target, const GLuint& renderbuffer)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindRenderbufferCommand>(target, renderbuffer));
		else
			g_glBindRenderbuffer(target, renderbuffer);
	}

	void FunctionWrapper::glRenderbufferStorage(const GLenum& target, const GLenum& internalformat, const GLsizei& width, const GLsizei& height)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlRenderbufferStorageCommand>(target, internalformat, width, height));
		else
			g_glRenderbufferStorage(target, internalformat, width, height);
	}

	void FunctionWrapper::glDeleteRenderbuffers(const GLsizei& n, std::unique_ptr<GLuint[]> renderbuffers)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteRenderbuffersCommand>(n, std::move(renderbuffers)));
		else
			g_glDeleteRenderbuffers(n, renderbuffers.get());
	}

	void FunctionWrapper::glFramebufferRenderbuffer(const GLenum& target, const GLenum& attachment, const GLenum& renderbuffertarget, const GLuint& renderbuffer)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlFramebufferRenderbufferCommand>(target, attachment, renderbuffertarget, renderbuffer));
		else
			g_glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
	}

	GLenum FunctionWrapper::glCheckFramebufferStatus(const GLenum& target)
	{
#ifdef GL_DEBUG
		GLenum returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlCheckFramebufferStatusCommand>(target, returnValue));
		else
			returnValue = g_glCheckFramebufferStatus(target);

		return returnValue;
#else
		return GL_FRAMEBUFFER_COMPLETE;
#endif
	}

	void FunctionWrapper::glBlitFramebuffer(const GLint& srcX0, const GLint& srcY0, const GLint& srcX1, const GLint& srcY1, const GLint& dstX0, const GLint& dstY0, const GLint& dstX1, const GLint& dstY1, const GLbitfield& mask, const GLenum& filter)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBlitFramebufferCommand>(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
		else
			g_glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	}

	void FunctionWrapper::glGenVertexArrays(const GLsizei& n, GLuint* arrays)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGenVertexArraysCommand>(n, arrays));
		else
			g_glGenVertexArrays(n, arrays);
	}

	void FunctionWrapper::glBindVertexArray(const GLuint& array)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindVertexArrayCommand>(array));
		else
			g_glBindVertexArray(array);
	}

	void FunctionWrapper::glDeleteVertexArrays(const GLsizei& n, std::unique_ptr<GLuint[]> arrays)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteVertexArraysCommand>(n, std::move(arrays)));
		else
			g_glDeleteVertexArrays(n, arrays.get());
	}

	void FunctionWrapper::glGenBuffers(GLsizei n, GLuint* buffers)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGenBuffersCommand>(n, buffers));
		else
			g_glGenBuffers(n, buffers);
	}

	void FunctionWrapper::glBindBuffer(const GLenum& target, const GLuint& buffer)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindBufferCommand>(target, buffer));
		else
			g_glBindBuffer(target, buffer);
	}

	void FunctionWrapper::glMapBuffer(const GLenum& target, const GLenum& access)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlMapBufferCommand>(target, access));
		else
			g_glMapBuffer(target, access);
	}

	void* FunctionWrapper::glMapBufferRange(const GLenum& target, const GLintptr& offset, const GLsizeiptr& length, const GLbitfield& access)
	{
		GLubyte* returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlMapBufferRangeCommand>(target, offset, length, access, returnValue));
		else
			returnValue = reinterpret_cast<GLubyte*>(g_glMapBufferRange(target, offset, length, access));

		return returnValue;
	}

	void FunctionWrapper::glMapBufferRangeWriteAsync(const GLenum& target, const GLuint& buffer, const GLintptr& offset, u32 length, const GLbitfield& access, std::unique_ptr<u8[]> data)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlMapBufferRangeWriteAsyncCommand>(target, buffer, offset, length, access, std::move(data)));
		else {
			auto command = std::make_shared<GlMapBufferRangeWriteAsyncCommand>(target, buffer, offset, length, access, std::move(data));
			command->performCommandSingleThreaded();
		}
	}

	void* FunctionWrapper::glMapBufferRangeReadAsync(const GLenum& target, const GLuint& buffer, const GLintptr& offset, u32 length, const GLbitfield& access)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlMapBufferRangeReadAsyncCommand>(target, buffer, offset, length, access));
		else {
			auto command = std::make_shared<GlMapBufferRangeReadAsyncCommand>(target, buffer, offset, length, access);
			command->performCommandSingleThreaded();
		}
		return GlMapBufferRangeReadAsyncCommand::getData(buffer);
	}

	GLboolean FunctionWrapper::glUnmapBuffer(const GLenum& target)
	{
		GLboolean returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUnmapBufferCommand>(target, returnValue));
		else
			returnValue = g_glUnmapBuffer(target);

		return returnValue;
	}

	void FunctionWrapper::glUnmapBufferAsync(const GLenum& target)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUnmapBufferAsyncCommand>(target));
		else
			g_glUnmapBuffer(target);
	}

	void FunctionWrapper::glDeleteBuffers(GLsizei n, std::unique_ptr<GLuint[]> buffers)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteBuffersCommand>(n, std::move(buffers)));
		else
			g_glDeleteBuffers(n, buffers.get());
	}

	void FunctionWrapper::glBindImageTexture(const GLuint& unit, const GLuint& texture, const GLint& level, const GLboolean& layered, const GLint& layer, const GLenum& access, const GLenum& format)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindImageTextureCommand>(unit, texture, level, layered, layer, access, format));
		else
			g_glBindImageTexture(unit, texture, level, layered, layer, access, format);
	}

	void FunctionWrapper::glMemoryBarrier(const GLbitfield& barriers)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlMemoryBarrierCommand>(barriers));
		else
			g_glMemoryBarrier(barriers);
	}

	const GLubyte* FunctionWrapper::glGetStringi(const GLenum& name, const GLuint& index)
	{
		const GLubyte* returnValue;

		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlGetStringiCommand>(name, index, returnValue));
		else
			returnValue = g_glGetStringi(name, index);

		return returnValue;
	}

	void FunctionWrapper::glInvalidateFramebuffer(const GLenum& target, const GLsizei& numAttachments, std::unique_ptr<GLenum[]> attachments)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlInvalidateFramebufferCommand>(target, numAttachments, std::move(attachments)));
		else
			g_glInvalidateFramebuffer(target, numAttachments, attachments.get());
	}

	GLsync FunctionWrapper::glFenceSync(const GLenum& condition, const GLbitfield& flags)
	{
		GLsync returnValue;

		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlFenceSyncCommand>(condition, flags, returnValue));
		else
			returnValue = g_glFenceSync(condition, flags);

		return returnValue;
	}

	void FunctionWrapper::glClientWaitSync(const GLsync& sync, const GLbitfield& flags, const GLuint64& timeout)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlClientWaitSyncCommand>(sync, flags, timeout));
		else
			g_glClientWaitSync(sync, flags, timeout);
	}

	void FunctionWrapper::glDeleteSync(const GLsync& sync)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDeleteSyncCommand>(sync));
		else
			g_glDeleteSync(sync);
	}

	GLuint FunctionWrapper::glGetUniformBlockIndex(const GLuint& program, GLchar *uniformBlockName)
	{
		GLuint returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetUniformBlockIndexCommand>(program, uniformBlockName, returnValue));
		else
			returnValue = g_glGetUniformBlockIndex(program, uniformBlockName);

		return returnValue;
	}

	void FunctionWrapper::glUniformBlockBinding(const GLuint& program, const GLuint& uniformBlockIndex, const GLuint& uniformBlockBinding)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlUniformBlockBindingCommand>(program, uniformBlockIndex, uniformBlockBinding));
		else
			g_glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
	}

	void FunctionWrapper::glGetActiveUniformBlockiv(const GLuint& program, const GLuint& uniformBlockIndex, const GLenum& pname, GLint* params)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetActiveUniformBlockivCommand>(program, uniformBlockIndex, pname, params));
		else
			g_glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
	}

	void FunctionWrapper::glGetUniformIndices(const GLuint& program, const GLsizei& uniformCount, const GLchar *const*uniformNames, GLuint* uniformIndices)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetUniformIndicesCommand>(program, uniformCount, uniformNames, uniformIndices));
		else
			g_glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
	}

	void FunctionWrapper::glGetActiveUniformsiv(const GLuint& program, const GLsizei& uniformCount, const GLuint *uniformIndices, const GLenum& pname, GLint* params)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetActiveUniformsivCommand>(program, uniformCount, uniformIndices, pname, params));
		else
			g_glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
	}

	void FunctionWrapper::glBindBufferBase(const GLenum& target, const GLuint& index, const GLuint& buffer)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlBindBufferBaseCommand>(target, index, buffer));
		else
			g_glBindBufferBase(target, index, buffer);
	}

	void FunctionWrapper::glGetProgramBinary(const GLuint& program, const GLsizei& bufSize, GLsizei* length, GLenum* binaryFormat, void *binary)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlGetProgramBinaryCommand>(program, bufSize, length, binaryFormat, binary));
		else
			g_glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
	}

	void FunctionWrapper::glProgramParameteri(const GLuint& program, const GLenum& pname, const GLint& value)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlProgramParameteriCommand>(program, pname, value));
		else
			g_glProgramParameteri(program, pname, value);
	}

	void FunctionWrapper::glTexStorage2D(const GLenum& target, const GLsizei& levels, const GLenum& internalformat, const GLsizei& width, const GLsizei& height)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTexStorage2DCommand>(target, levels, internalformat, width, height));
		else
			g_glTexStorage2D(target, levels, internalformat, width, height);
	}

	void FunctionWrapper::glTextureStorage2D(const GLuint& texture, const GLsizei& levels, const GLenum& internalformat, const GLsizei& width, const GLsizei& height)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureStorage2DCommand>(texture, levels, internalformat, width, height));
		else
			g_glTextureStorage2D(texture, levels, internalformat, width, height);
	}

	void FunctionWrapper::glTextureSubImage2DBuffered(const GLuint& texture, const GLint& level, const GLint& xoffset, const GLint& yoffset, const GLsizei& width, const GLsizei& height, const GLenum& format, const GLenum& type, std::size_t offset)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureSubImage2DBufferedCommand>(texture, level, xoffset, yoffset, width, height, format, type, offset));
		else
			g_glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, (const GLvoid* )offset);
	}

	void FunctionWrapper::glTextureStorage2DMultisample(const GLuint& texture, const GLenum& target, const GLsizei& samples, const GLenum& internalformat, const GLsizei& width, const GLsizei& height, const GLboolean& fixedsamplelocations)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureStorage2DMultisampleCommand>(texture, target, samples, internalformat, width, height, fixedsamplelocations));
		else
			g_glTextureStorage2DMultisample(texture, target, samples, internalformat, width, height, fixedsamplelocations);
	}

	void FunctionWrapper::glTextureParameteri(const GLuint& texture, const GLenum& pname, const GLint& param)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureParameteriCommand>(texture, pname, param));
		else
			g_glTextureParameteri(texture, pname, param);
	}

	void FunctionWrapper::glTextureParameterf(const GLuint& texture, const GLenum& pname, const GLfloat& param)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlTextureParameterfCommand>(texture, pname, param));
		else
			g_glTextureParameterf(texture, pname, param);
	}

	void FunctionWrapper::glCreateTextures(const GLenum& target, const GLsizei& n, GLuint* textures)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlCreateTexturesCommand>(target, n, textures));
		else
			g_glCreateTextures(target, n, textures);
	}

	void FunctionWrapper::glCreateBuffers(const GLsizei& n, GLuint* buffers)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlCreateBuffersCommand>(n, buffers));
		else
			g_glCreateBuffers(n, buffers);
	}

	void FunctionWrapper::glCreateFramebuffers(const GLsizei& n, GLuint* framebuffers)
	{
		if(m_threaded_wrapper)
			executePriorityCommand(std::make_shared<GlCreateFramebuffersCommand>(n, framebuffers));
		else
			g_glCreateFramebuffers(n, framebuffers);
	}

	void FunctionWrapper::glNamedFramebufferTexture(const GLuint& framebuffer, const GLenum& attachment, const GLuint& texture, const GLint& level)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlNamedFramebufferTextureCommand>(framebuffer, attachment, texture, level));
		else
			g_glNamedFramebufferTexture(framebuffer, attachment, texture, level);
	}

	void FunctionWrapper::glDrawElementsBaseVertex(const GLenum& mode, const GLsizei& count, const GLenum& type, const char* indices, const GLint& basevertex)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlDrawElementsBaseVertexCommand>(mode, count, type, std::move(indices), basevertex));
		else
			g_glDrawElementsBaseVertex(mode, count, type, std::move(indices), basevertex);
	}

	void FunctionWrapper::glFlushMappedBufferRange(const GLenum& target, const GLintptr& offset, const GLsizeiptr& length)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlFlushMappedBufferRangeCommand>(target, offset, length));
		else
			g_glFlushMappedBufferRange(target, offset, length);
	}

	void FunctionWrapper::glFinish(void)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<GlFinishCommand>());
		else
			g_glFinish();
	}

#ifdef MUPENPLUSAPI

	void FunctionWrapper::CoreVideo_Init(void)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<CoreVideoInitCommand>());
		else
			std::make_shared<CoreVideoInitCommand>()->performCommandSingleThreaded();
	}

	void FunctionWrapper::CoreVideo_Quit(void)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<CoreVideoQuitCommand>());
		else
			std::make_shared<CoreVideoQuitCommand>()->performCommandSingleThreaded();

		m_shutdown = true;

		if(m_threaded_wrapper) {
			m_condition.notify_all();
			m_commandExecutionThread.join();
		}
	}

	m64p_error FunctionWrapper::CoreVideo_SetVideoMode(int screenWidth, int screenHeight, int bitsPerPixel, m64p_video_mode mode, m64p_video_flags flags)
	{
		m64p_error returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<CoreVideoSetVideoModeCommand>(screenWidth, screenHeight, bitsPerPixel, mode, flags, returnValue));
		else
			std::make_shared<CoreVideoSetVideoModeCommand>(screenWidth, screenHeight, bitsPerPixel, mode, flags, returnValue)->performCommandSingleThreaded();

		return returnValue;
	}

	void FunctionWrapper::CoreVideo_GL_SetAttribute(m64p_GLattr attribute, int value)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<CoreVideoGLSetAttributeCommand>(attribute, value));
		else
			std::make_shared<CoreVideoGLSetAttributeCommand>(attribute, value)->performCommandSingleThreaded();
	}

	void FunctionWrapper::CoreVideo_GL_GetAttribute(m64p_GLattr attribute, int *value)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<CoreVideoGLGetAttributeCommand>(attribute, value));
		else
			std::make_shared<CoreVideoGLGetAttributeCommand>(attribute, value)->performCommandSingleThreaded();
	}

	void FunctionWrapper::CoreVideo_GL_SwapBuffers(void)
	{
		++m_swapBuffersQueued;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<CoreVideoGLSwapBuffersCommand>([]{ReduceSwapBuffersQueued();}));
		else
			std::make_shared<CoreVideoGLSwapBuffersCommand>([]{ReduceSwapBuffersQueued();})->performCommandSingleThreaded();
	}
#else
	bool FunctionWrapper::windowsStart(void)
	{
		bool returnValue;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<WindowsStartCommand>(returnValue));
		else
			std::make_shared<WindowsStartCommand>(returnValue)->performCommandSingleThreaded();

		return returnValue;
	}

	void FunctionWrapper::windowsStop(void)
	{
		if(m_threaded_wrapper)
			executeCommand(std::make_shared<WindowsStopCommand>());
		else
			std::make_shared<WindowsStopCommand>()->performCommandSingleThreaded();

		m_shutdown = true;

		if (m_threaded_wrapper) {
			m_condition.notify_all();
			m_commandExecutionThread.join();
		}
	}

	void FunctionWrapper::windowsSwapBuffers(void)
	{
		++m_swapBuffersQueued;

		if(m_threaded_wrapper)
			executeCommand(std::make_shared<WindowsSwapBuffersCommand>([]{ReduceSwapBuffersQueued();}));
		else
			std::make_shared<WindowsSwapBuffersCommand>([]{ReduceSwapBuffersQueued();})->performCommandSingleThreaded();
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
