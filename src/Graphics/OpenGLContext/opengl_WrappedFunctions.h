#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <vector>
#include "BlockingQueue.h"
#include "GLFunctions.h"
#include "opengl_Attributes.h"
#include <algorithm>
#include <functional>
#include <unordered_map>


#ifdef MUPENPLUSAPI
#include <mupenplus/GLideN64_mupenplus.h>
#else
#include <Graphics/OpenGLContext/windows/WindowsWGL.h>
#endif

namespace opengl {

	class OpenGlCommand
	{
	public:
		void performCommandSingleThreaded(void)
		{
			commandToExecute();
#ifdef GL_DEBUG
			if (m_isGlCommand) {
				auto error = g_glGetError();
				if (error != GL_NO_ERROR) {
					std::stringstream errorString;
					errorString << " OpenGL error: 0x" << std::hex << error << ", on function: " << m_functionName;
					LOG(LOG_VERBOSE, errorString.str().c_str());
					throw std::runtime_error(errorString.str().c_str());
				}
			}
#endif
		}

		void performCommand(void) {
			std::unique_lock<std::mutex> lock(m_condvarMutex);
			performCommandSingleThreaded();
			if (m_synced) {
#ifdef GL_DEBUG
				if (m_logIfSynced) {
					std::stringstream errorString;
					errorString << " Executing synced: " << m_functionName;
					LOG(LOG_VERBOSE, errorString.str().c_str());
				}
#endif
				m_executed = true;
				m_condition.notify_all();
			}
		}

		void waitOnCommand(void) {
			std::unique_lock<std::mutex> lock(m_condvarMutex);

			if (m_synced && !m_executed) {
				m_condition.wait(lock, [this]{return m_executed;});
			}
		}

	protected:
		OpenGlCommand (bool _synced, bool _logIfSynced, const std::string& _functionName,
			bool _isGlCommand = true):
			m_synced(_synced)
			, m_executed(false)
#ifdef GL_DEBUG
			, m_logIfSynced(_logIfSynced)
			, m_functionName(std::move(_functionName))
			, m_isGlCommand(_isGlCommand)
#endif
		{
		}

		virtual void commandToExecute(void) = 0;

	protected:
#ifdef GL_DEBUG
		const bool m_logIfSynced;
		const std::string m_functionName;
		const bool m_isGlCommand;
#endif

	private:
		std::atomic<bool> m_synced;
		bool m_executed;
		std::mutex m_condvarMutex;
		std::condition_variable m_condition;
	};

	class GlBlendFuncCommand : public OpenGlCommand
	{
	public:
		GlBlendFuncCommand(GLenum sfactor, GLenum dfactor):
			OpenGlCommand(false, false, "glBlendFunc"), m_sfactor(sfactor), m_dfactor(dfactor)
		{
		}

		void commandToExecute(void) override
		{
			g_glBlendFunc(m_sfactor, m_dfactor);
		}
	private:
		GLenum m_sfactor;
		GLenum m_dfactor;
	};

	class GlPixelStoreiCommand : public OpenGlCommand
	{
	public:
		GlPixelStoreiCommand(GLenum pname, GLint param):
			OpenGlCommand(false, false, "glPixelStorei"), m_pname(pname), m_param(param)
		{
		}

		void commandToExecute(void) override
		{
			g_glPixelStorei(m_pname, m_param);
		}
	private:
		GLenum m_pname;
		GLint m_param;
	};

	class GlClearColorCommand : public OpenGlCommand
	{
	public:
		GlClearColorCommand(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha):
			OpenGlCommand(false, false, "glClearColor"), m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
		{
		}

		void commandToExecute(void) override
		{
			g_glClearColor(m_red, m_green, m_blue, m_alpha);
		}
	private:
		GLfloat m_red;
		GLfloat m_green;
		GLfloat m_blue;
		GLfloat m_alpha;
	};

	class GlCullFaceCommand : public OpenGlCommand
	{
	public:
		GlCullFaceCommand(GLenum mode):
			OpenGlCommand(false, false, "glCullFace"), m_mode(mode)
		{
		}

		void commandToExecute(void) override
		{
			g_glCullFace(m_mode);
		}
	private:
		GLenum m_mode;
	};

	class GlDepthFuncCommand : public OpenGlCommand
	{
	public:
		GlDepthFuncCommand(GLenum func):
			OpenGlCommand(false, false, "glDepthFunc"), m_func(func)
		{
		}

		void commandToExecute(void) override
		{
			g_glDepthFunc(m_func);
		}
	private:
		GLenum m_func;
	};

	class GlDepthMaskCommand : public OpenGlCommand
	{
	public:
		GlDepthMaskCommand(GLboolean flag):
			OpenGlCommand(false, false, "glDepthMask"), m_flag(flag)
		{
		}

		void commandToExecute(void) override
		{
			g_glDepthMask(m_flag);
		}
	private:
		GLboolean m_flag;
	};

	class GlDisableCommand : public OpenGlCommand
	{
	public:
		GlDisableCommand(GLenum cap):
			OpenGlCommand(false, false, "glDisable"), m_cap(cap)
		{
		}

		void commandToExecute(void) override
		{
			g_glDisable(m_cap);
		}
	private:
		GLenum m_cap;
	};

	class GlEnableCommand : public OpenGlCommand
	{
	public:
		GlEnableCommand(GLenum cap):
			OpenGlCommand(false, false, "glEnable"), m_cap(cap)
		{
		}

		void commandToExecute(void) override
		{
			g_glEnable(m_cap);
		}
	private:
		GLenum m_cap;
	};

	class GlPolygonOffsetCommand : public OpenGlCommand
	{
	public:
		GlPolygonOffsetCommand(GLfloat factor, GLfloat units):
			OpenGlCommand(false, false, "glPolygonOffset"), m_factor(factor), m_units(units)
		{
		}

		void commandToExecute(void) override
		{
			g_glPolygonOffset(m_factor, m_units);
		}
	private:
		GLfloat m_factor;
		GLfloat m_units;
	};

	class GlScissorCommand : public OpenGlCommand
	{
	public:
		GlScissorCommand(GLint x, GLint y, GLsizei width, GLsizei height):
			OpenGlCommand(false, false, "glScissor"), m_x(x), m_y(y), m_width(width), m_height(height)
		{
		}

		void commandToExecute(void) override
		{
			g_glScissor(m_x, m_y, m_width, m_height);
		}
	private:
		GLint m_x;
		GLint m_y;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlViewportCommand : public OpenGlCommand
	{
	public:
		GlViewportCommand(GLint x, GLint y, GLsizei width, GLsizei height):
				OpenGlCommand(false, false, "glViewport"), m_x(x), m_y(y), m_width(width), m_height(height)
		{
		}

		void commandToExecute(void) override
		{
			g_glViewport(m_x, m_y, m_width, m_height);
		}
	private:
		GLint m_x;
		GLint m_y;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlBindTextureCommand : public OpenGlCommand
	{
	public:
		GlBindTextureCommand(GLenum target, GLuint texture):
			OpenGlCommand(false, false, "glBindTexture"), m_target(target), m_texture(texture)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindTexture(m_target, m_texture);
		}
	private:
		const GLenum m_target;
		const GLuint m_texture;
	};

	template <class pixelType>
	class GlTexImage2DCommand : public OpenGlCommand
	{
	public:
		GlTexImage2DCommand(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
			GLint border, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels):
			OpenGlCommand(false, false, "glTexImage2D"), m_target(target), m_level(level), m_internalformat(internalformat),
			m_width(width), m_height(height), m_border(border), m_format(format), m_type(type),
			m_pixels(std::move(pixels))
		{
		}

		void commandToExecute(void) override
		{
			g_glTexImage2D(m_target, m_level, m_internalformat, m_width, m_height, m_border, m_format, m_type,
				m_pixels.get());
		}
	private:
		GLenum m_target;
		GLint m_level;
		GLint m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
		GLint m_border;
		GLenum m_format;
		GLenum m_type;
		std::unique_ptr<pixelType[]> m_pixels;
	};

	class GlTexParameteriCommand : public OpenGlCommand
	{
	public:
		GlTexParameteriCommand(GLenum target, GLenum pname, GLint param):
			OpenGlCommand(false, false, "glTexParameteri"), m_target(target), m_pname(pname), m_param(param)
		{
		}

		void commandToExecute(void) override
		{
			g_glTexParameteri(m_target, m_pname, m_param);
		}
	private:
		GLenum m_target;
		GLenum m_pname;
		GLint m_param;
	};

	class GlGetIntegervCommand : public OpenGlCommand
	{
	public:
		GlGetIntegervCommand(GLenum pname, GLint* data):
			OpenGlCommand(true, false, "glGetIntegerv"), m_pname(pname), m_data(data)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetIntegerv(m_pname, m_data);
		}
	private:
		GLenum m_pname;
		GLint* m_data;
	};

	class GlGetStringCommand : public OpenGlCommand
	{
	public:
		GlGetStringCommand(GLenum name, const GLubyte*& returnValue):
			OpenGlCommand(true, false, "glGetString"), m_name(name), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glGetString(m_name);
		}
	private:
		GLenum m_name;
		const GLubyte*& m_returnValue;
	};

	class GlReadPixelsCommand : public OpenGlCommand
	{
	public:
		GlReadPixelsCommand(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels):
			OpenGlCommand(true, true, "glReadPixels"), m_x(x), m_y(y), m_width(width), m_height(height), m_format(format), m_type(type), m_pixels(pixels)
		{
		}

		void commandToExecute(void) override
		{
			g_glReadPixels(m_x, m_y, m_width, m_height, m_format, m_type, m_pixels);
		}
	private:
		GLint m_x;
		GLint m_y;
		GLsizei m_width;
		GLsizei m_height;
		GLenum m_format;
		GLenum m_type;
		void* m_pixels;
	};

	class GlReadPixelsAsyncCommand : public OpenGlCommand
	{
	public:
		GlReadPixelsAsyncCommand(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type):
			OpenGlCommand(false, false, "GlReadPixelsAync"), m_x(x), m_y(y), m_width(width), m_height(height), m_format(format), m_type(type)
		{
		}

		void commandToExecute(void) override
		{
			g_glReadPixels(m_x, m_y, m_width, m_height, m_format, m_type, nullptr);
		}
	private:
		GLint m_x;
		GLint m_y;
		GLsizei m_width;
		GLsizei m_height;
		GLenum m_format;
		GLenum m_type;
	};

	template <class pixelType>
	class GlTexSubImage2DUnbufferedCommand : public OpenGlCommand
	{
	public:
		GlTexSubImage2DUnbufferedCommand(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels):
			OpenGlCommand(false, false, "glTexSubImage2D"), m_target(target), m_level(level), m_xoffset(xoffset), m_yoffset(yoffset),
			m_width(width), m_height(height), m_format(format), m_type(type), m_pixels(std::move(pixels))
		{
		}

		void commandToExecute(void) override
		{
			g_glTexSubImage2D(m_target, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type, m_pixels.get());
		}
	private:
		GLenum m_target;
		GLint m_level;
		GLint m_xoffset;
		GLint m_yoffset;
		GLsizei m_width;
		GLsizei m_height;
		GLenum m_format;
		GLenum m_type;
		std::unique_ptr<pixelType[]> m_pixels;
	};

	class GlTexSubImage2DBufferedCommand : public OpenGlCommand
	{
	public:
		GlTexSubImage2DBufferedCommand(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
										 GLenum format, GLenum type, std::size_t offset):
				OpenGlCommand(false, false, "glTexSubImage2D"), m_target(target), m_level(level), m_xoffset(xoffset), m_yoffset(yoffset),
				m_width(width), m_height(height), m_format(format), m_type(type), m_offset(offset)
		{
		}

		void commandToExecute(void) override
		{
			g_glTexSubImage2D(m_target, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type, (const GLvoid *)m_offset);
		}
	private:
		GLenum m_target;
		GLint m_level;
		GLint m_xoffset;
		GLint m_yoffset;
		GLsizei m_width;
		GLsizei m_height;
		GLenum m_format;
		GLenum m_type;
		std::size_t m_offset;
	};

	class GlDrawArraysCommand : public OpenGlCommand
	{
	public:
		GlDrawArraysCommand(GLenum mode, GLint first, GLsizei count):
			OpenGlCommand(false, false, "glDrawArrays"), m_mode(mode), m_first(first), m_count(count)
		{
		}

		void commandToExecute(void) override
		{
			g_glDrawArrays(m_mode, m_first, m_count);
		}
	private:
		GLenum m_mode;
		GLint m_first;
		GLsizei m_count;
	};

	class GlVertexAttribPointerUnbufferedCommand : public OpenGlCommand
	{
	public:
		GlVertexAttribPointerUnbufferedCommand(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
			std::size_t offset):
			OpenGlCommand(false, false, "glVertexAttribPointer"), m_index(index), m_size(size), m_type(type), m_normalized(normalized),
			m_stride(stride), m_offset(offset)
		{
		}

		void commandToExecute(void) override
		{
			if (m_attribsData == nullptr) {
				m_attribsData = std::unique_ptr<char[]>(new char[2*1024*1024]);
			}

			g_glVertexAttribPointer(m_index, m_size, m_type, m_normalized, m_stride, (const GLvoid *)(m_attribsData.get()+m_offset));
		}

		static char* getDrawingData()
		{
			return m_attribsData.get();
		}
	private:

		GLuint m_index;
		GLint m_size;
		GLenum m_type;
		GLboolean m_normalized;
		GLsizei m_stride;
		std::size_t m_offset;

		static std::unique_ptr<char[]> m_attribsData;
	};

	class GlDrawArraysUnbufferedCommand : public OpenGlCommand
	{
	public:
		GlDrawArraysUnbufferedCommand(GLenum mode, GLint first, GLsizei count, std::unique_ptr<std::vector<char>> data):
			OpenGlCommand(false, false, "glDrawArrays"), m_mode(mode), m_first(first), m_count(count),
			m_data(std::move(data))
		{
		}

		void commandToExecute(void) override
		{
			char* data = GlVertexAttribPointerUnbufferedCommand::getDrawingData();
			std::copy_n(m_data->data(), m_data->size(), data);
			g_glDrawArrays(m_mode, m_first, m_count);
		}
	private:
		GLenum m_mode;
		GLint m_first;
		GLsizei m_count;
		std::unique_ptr<std::vector<char>> m_data;
	};

	class GlGetErrorCommand : public OpenGlCommand
	{
	public:
		GlGetErrorCommand(GLenum& returnValue):
			OpenGlCommand(true, true, "glGetError"), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glGetError();
		}
	private:
		GLenum& m_returnValue;
	};

	template <class indiceType>
	class GlDrawElementsUnbufferedCommand : public OpenGlCommand
	{
	public:
		GlDrawElementsUnbufferedCommand(GLenum mode, GLsizei count, GLenum type, std::unique_ptr<indiceType[]> indices,
				std::unique_ptr<std::vector<char>> data):
				OpenGlCommand(false, false, "glDrawElementsUnbuffered"), m_mode(mode), m_count(count), m_type(type),
				m_indices(std::move(indices)), m_data(std::move(data))
		{
		}

		void commandToExecute(void) override
		{
			char* data = GlVertexAttribPointerUnbufferedCommand::getDrawingData();
			std::copy_n(m_data->data(), m_data->size(), data);
			g_glDrawElements(m_mode, m_count, m_type, m_indices.get());
		}
	private:
		GLenum m_mode;
		GLsizei m_count;
		GLenum m_type;
		std::unique_ptr<indiceType[]> m_indices;
		std::unique_ptr<std::vector<char>> m_data;
	};

	class GlLineWidthCommand : public OpenGlCommand
	{
	public:
		GlLineWidthCommand(GLfloat width):
			OpenGlCommand(false, false, "glLineWidth"), m_width(width)
		{
		}

		void commandToExecute(void) override
		{
			g_glLineWidth(m_width);
		}
	private:
		GLfloat m_width;
	};

	class GlClearCommand : public OpenGlCommand
	{
	public:
		GlClearCommand(GLbitfield mask):
			OpenGlCommand(false, false, "glClear"), m_mask(mask)
		{
		}

		void commandToExecute(void) override
		{
			g_glClear(m_mask);
		}
	private:
		GLbitfield m_mask;
	};

	class GlGetFloatvCommand : public OpenGlCommand
	{
	public:
		GlGetFloatvCommand(GLenum pname, GLfloat* data):
			OpenGlCommand(true, false, "glGetFloatv"), m_pname(pname), m_data(data)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetFloatv(m_pname, m_data);
		}
	private:
		GLenum m_pname;
		GLfloat* m_data;
	};

	class GlDeleteTexturesCommand : public OpenGlCommand
	{
	public:
		GlDeleteTexturesCommand(GLsizei n, std::unique_ptr<GLuint[]> textures):
			OpenGlCommand(false, false, "glDeleteTextures"), m_n(n), m_textures(std::move(textures))
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteTextures(m_n, m_textures.get());
		}
	private:
		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_textures;
	};

	class GlGenTexturesCommand : public OpenGlCommand
	{
	public:
		GlGenTexturesCommand(GLsizei n, GLuint* textures):
			OpenGlCommand(true, false, "glGenTextures"), m_n(n), m_textures(textures)
		{
		}

		void commandToExecute(void) override
		{
			g_glGenTextures(m_n, m_textures);
		}
	private:
		GLsizei m_n;
		GLuint* m_textures;
	};

	class GlTexParameterfCommand : public OpenGlCommand
	{
	public:
		GlTexParameterfCommand(GLenum target, GLenum pname, GLfloat param):
			OpenGlCommand(false, false, "glTexParameterf"), m_target(target), m_pname(pname), m_param(param)
		{
		}

		void commandToExecute(void) override
		{
			g_glTexParameterf(m_target, m_pname, m_param);
		}
	private:
		GLenum m_target;
		GLenum m_pname;
		GLfloat m_param;
	};

	class GlActiveTextureCommand : public OpenGlCommand
	{
	public:
		GlActiveTextureCommand(GLenum texture):
			OpenGlCommand(false, false, "glActiveTexture"), m_texture(texture)
		{
		}

		void commandToExecute(void) override
		{
			g_glActiveTexture(m_texture);
		}
	private:
		const GLenum m_texture;
	};

	class GlBlendColorCommand : public OpenGlCommand
	{
	public:
		GlBlendColorCommand(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha):
			OpenGlCommand(false, false, "glBlendColor"), m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
		{
		}

		void commandToExecute(void) override
		{
			g_glBlendColor(m_red, m_green, m_blue, m_alpha);
		}
	private:
		GLfloat m_red;
		GLfloat m_green;
		GLfloat m_blue;
		GLfloat m_alpha;
	};

	class GlReadBufferCommand : public OpenGlCommand
	{
	public:
		GlReadBufferCommand(GLenum src):
			OpenGlCommand(false, false, "glReadBuffer"), m_src(src)
		{
		}

		void commandToExecute(void) override
		{
			g_glReadBuffer(m_src);
		}
	private:
		GLenum m_src;
	};

	class GlCreateShaderCommand : public OpenGlCommand
	{
	public:
		GlCreateShaderCommand(GLenum type, GLuint& returnValue):
			OpenGlCommand(true, true, "glCreateShader"), m_type(type), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glCreateShader(m_type);
		}
	private:
		GLenum m_type;
		GLuint& m_returnValue;
	};

	class GlCompileShaderCommand : public OpenGlCommand
	{
	public:
		GlCompileShaderCommand(GLuint shader):
			OpenGlCommand(false, false, "glCompileShader"), m_shader(shader)
		{
		}

		void commandToExecute(void) override
		{
			g_glCompileShader(m_shader);
		}
	private:
		GLuint m_shader;
	};

	class GlShaderSourceCommand : public OpenGlCommand
	{
	public:
		GlShaderSourceCommand(GLuint shader, const std::string& string):
			OpenGlCommand(false, false, "glShaderSource"), m_shader(shader), m_string(std::move(string))
		{
		}

		void commandToExecute(void) override
		{
			const GLchar* strShaderData = m_string.data();
			g_glShaderSource(m_shader, 1, &strShaderData, nullptr);
		}
	private:
		GLuint m_shader;
		const std::string m_string;
	};

	class GlCreateProgramCommand : public OpenGlCommand
	{
	public:
		GlCreateProgramCommand(GLuint& returnValue):
			OpenGlCommand(true, true, "glCreateProgram"), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glCreateProgram();
		}
	private:
		GLuint& m_returnValue;
	};

	class GlAttachShaderCommand : public OpenGlCommand
	{
	public:
		GlAttachShaderCommand(GLuint program, GLuint shader):
			OpenGlCommand(false, false, "glAttachShader"), m_program(program), m_shader(shader)
		{
		}

		void commandToExecute(void) override
		{
			g_glAttachShader(m_program, m_shader);
		}
	private:
		GLuint m_program;
		GLuint m_shader;
	};

	class GlLinkProgramCommand : public OpenGlCommand
	{
	public:
		GlLinkProgramCommand(GLuint program):
			OpenGlCommand(false, false, "glLinkProgram"), m_program(program)
		{
		}

		void commandToExecute(void) override
		{
			g_glLinkProgram(m_program);
		}
	private:
		GLuint m_program;
	};

	class GlUseProgramCommand : public OpenGlCommand
	{
	public:
		GlUseProgramCommand(GLuint program):
				OpenGlCommand(false, false, "glUseProgram"), m_program(program)
		{
		}

		void commandToExecute(void) override
		{
			g_glUseProgram(m_program);
		}
	private:
		GLuint m_program;
	};

	class GlGetUniformLocationCommand : public OpenGlCommand
	{
	public:
		GlGetUniformLocationCommand(GLuint program, const GLchar* name, GLint& returnValue):
			OpenGlCommand(true, true, "glGetUniformLocation"), m_program(program), m_name(name), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glGetUniformLocation(m_program, m_name);
		}
	private:
		GLint& m_returnValue;
		GLuint m_program;
		const GLchar* m_name;
	};

	class GlUniform1iCommand : public OpenGlCommand
	{
	public:
		GlUniform1iCommand(GLint location, GLint v0):
			OpenGlCommand(false, false, "glUniform1i"), m_location(location), m_v0(v0)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform1i(m_location, m_v0);
		}
	private:
		GLint m_location;
		GLint m_v0;
	};

	class GlUniform1fCommand : public OpenGlCommand
	{
	public:
		GlUniform1fCommand(GLint location, GLfloat v0):
				OpenGlCommand(false, false, "glUniform1f"), m_location(location), m_v0(v0)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform1f(m_location, m_v0);
		}
	private:
		GLint m_location;
		GLfloat m_v0;
	};

	class GlUniform2fCommand : public OpenGlCommand
	{
	public:
		GlUniform2fCommand(GLint location, GLfloat v0, GLfloat v1):
				OpenGlCommand(false, false, "glUniform2f"), m_location(location), m_v0(v0), m_v1(v1)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform2f(m_location, m_v0, m_v1);
		}
	private:
		GLint m_location;
		GLfloat m_v0;
		GLfloat m_v1;
	};

	class GlUniform2iCommand : public OpenGlCommand
	{
	public:
		GlUniform2iCommand(GLint location, GLint v0, GLint v1):
			OpenGlCommand(false, false, "glUniform2i"), m_location(location), m_v0(v0), m_v1(v1)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform2i(m_location, m_v0, m_v1);
		}
	private:
		GLint m_location;
		GLint m_v0;
		GLint m_v1;
	};

	class GlUniform4iCommand : public OpenGlCommand
	{
	public:
		GlUniform4iCommand(GLint location, GLint v0, GLint v1, GLint v2, GLint v3):
			OpenGlCommand(false, false, "glUniform4i"), m_location(location), m_v0(v0), m_v1(v1), m_v2(v2), m_v3(v3)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform4i(m_location, m_v0, m_v1, m_v2, m_v3);
		}
	private:
		GLint m_location;
		GLint m_v0;
		GLint m_v1;
		GLint m_v2;
		GLint m_v3;
	};

	class GlUniform4fCommand : public OpenGlCommand
	{
	public:
		GlUniform4fCommand(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3):
			OpenGlCommand(false, false, "glUniform4f"), m_location(location), m_v0(v0), m_v1(v1), m_v2(v2), m_v3(v3)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform4f(m_location, m_v0, m_v1, m_v2, m_v3);
		}
	private:
		GLint m_location;
		GLfloat m_v0;
		GLfloat m_v1;
		GLfloat m_v2;
		GLfloat m_v3;
	};

	class GlUniform3fvCommand : public OpenGlCommand
	{
	public:
		GlUniform3fvCommand(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value):
			OpenGlCommand(false, false, "glUniform3fv"), m_location(location), m_count(count), m_value(std::move(value))
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform3fv(m_location, m_count, m_value.get());
		}
	private:
		GLint m_location;
		GLsizei m_count;
		std::unique_ptr<GLfloat[]> m_value;
	};

	class GlUniform4fvCommand : public OpenGlCommand
	{
	public:
		GlUniform4fvCommand(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value):
			OpenGlCommand(false, false, "glUniform4fv"), m_location(location), m_count(count), m_value(std::move(value))
		{
		}

		void commandToExecute(void) override
		{
			g_glUniform4fv(m_location, m_count, m_value.get());
		}
	private:
		GLint m_location;
		GLsizei m_count;
		std::unique_ptr<GLfloat[]> m_value;
	};

	class GlDetachShaderCommand : public OpenGlCommand
	{
	public:
		GlDetachShaderCommand(GLuint program, GLuint shader):
			OpenGlCommand(false, false, "glDetachShader"), m_program(program), m_shader(shader)
		{
		}

		void commandToExecute(void) override
		{
			g_glDetachShader(m_program, m_shader);
		}
	private:
		GLuint m_program;
		GLuint m_shader;
	};

	class GlDeleteShaderCommand : public OpenGlCommand
	{
	public:
		GlDeleteShaderCommand(GLuint shader):
			OpenGlCommand(false, false, "glDeleteShader"), m_shader(shader)
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteShader(m_shader);
		}
	private:
		GLuint m_shader;
	};

	class GlDeleteProgramCommand : public OpenGlCommand
	{
	public:
		GlDeleteProgramCommand(GLuint program):
			OpenGlCommand(false, false, "glDeleteProgram"), m_program(program)
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteProgram(m_program);
		}
	private:
		GLuint m_program;
	};

	class GlGetProgramInfoLogCommand : public OpenGlCommand
	{
	public:
		GlGetProgramInfoLogCommand(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog):
			OpenGlCommand(true, true, "glGetProgramInfoLog"), m_program(program), m_bufSize(bufSize), m_length(length), m_infoLog(infoLog)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetProgramInfoLog(m_program, m_bufSize, m_length, m_infoLog);
		}
	private:
		GLuint m_program;
		GLsizei m_bufSize;
		GLsizei* m_length;
		GLchar* m_infoLog;
	};

	class GlGetShaderInfoLogCommand : public OpenGlCommand
	{
	public:
		GlGetShaderInfoLogCommand(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog):
			OpenGlCommand(true, true, "glGetShaderInfoLog"), m_shader(shader), m_bufSize(bufSize), m_length(length), m_infoLog(infoLog)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetShaderInfoLog(m_shader, m_bufSize, m_length, m_infoLog);
		}
	private:
		GLuint m_shader;
		GLsizei m_bufSize;
		GLsizei* m_length;
		GLchar* m_infoLog;
	};

	class GlGetShaderivCommand : public OpenGlCommand
	{
	public:
		GlGetShaderivCommand(GLuint shader, GLenum pname, GLint* params):
			OpenGlCommand(true, true, "glGetShaderiv"), m_shader(shader), m_pname(pname), m_params(params)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetShaderiv(m_shader, m_pname, m_params);
		}
	private:
		GLuint m_shader;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlGetProgramivCommand : public OpenGlCommand
	{
	public:
		GlGetProgramivCommand(GLuint program, GLenum pname, GLint* params):
			OpenGlCommand(true, true, "glGetProgramiv"), m_program(program), m_pname(pname), m_params(params)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetProgramiv(m_program, m_pname, m_params);
		}
	private:
		GLuint m_program;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlEnableVertexAttribArrayCommand : public OpenGlCommand
	{
	public:
		GlEnableVertexAttribArrayCommand(GLuint index):
			OpenGlCommand(false, false, "glEnableVertexAttribArray"), m_index(index)
		{
		}

		void commandToExecute(void) override
		{
			g_glEnableVertexAttribArray(m_index);
		}
	private:
		GLuint m_index;
	};

	class GlDisableVertexAttribArrayCommand : public OpenGlCommand
	{
	public:
		GlDisableVertexAttribArrayCommand(GLuint index):
			OpenGlCommand(false, false, "glDisableVertexAttribArray"), m_index(index)
		{
		}

		void commandToExecute(void) override
		{
			g_glDisableVertexAttribArray(m_index);
		}
	private:
		GLuint m_index;
	};

	class GlVertexAttribPointerBufferedCommand : public OpenGlCommand
	{
	public:
		GlVertexAttribPointerBufferedCommand(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
			std::size_t offset):
			OpenGlCommand(false, false, "glVertexAttribPointer"), m_index(index), m_size(size), m_type(type), m_normalized(normalized),
			m_stride(stride), m_offset(offset)
		{
		}

		void commandToExecute(void) override
		{
			g_glVertexAttribPointer(m_index, m_size, m_type, m_normalized, m_stride, (const GLvoid *)(m_offset));
		}
	private:
		GLuint m_index;
		GLint m_size;
		GLenum m_type;
		GLboolean m_normalized;
		GLsizei m_stride;
		std::size_t m_offset;
	};

	class GlBindAttribLocationCommand : public OpenGlCommand
	{
	public:
		GlBindAttribLocationCommand(GLuint program, GLuint index, const std::string& name):
			OpenGlCommand(false, false, "glBindAttribLocation"), m_program(program), m_index(index), m_name(std::move(name))
		{
		}

		void commandToExecute(void) override
		{
			g_glBindAttribLocation(m_program, m_index, m_name.data());
		}
	private:
		GLuint m_program;
		GLuint m_index;
		const std::string m_name;
	};

	class GlVertexAttrib1fCommand : public OpenGlCommand
	{
	public:
		GlVertexAttrib1fCommand(GLuint index, GLfloat x):
			OpenGlCommand(false, false, "glVertexAttrib1f"), m_index(index), m_x(x)
		{
		}

		void commandToExecute(void) override
		{
			g_glVertexAttrib1f(m_index, m_x);
		}
	private:
		GLuint m_index;
		GLfloat m_x;
	};

	class GlVertexAttrib4fCommand : public OpenGlCommand
	{
	public:
		GlVertexAttrib4fCommand(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w):
			OpenGlCommand(false, false, "glVertexAttrib4f"), m_index(index), m_x(x), m_y(y), m_z(z), m_w(w)
		{
		}

		void commandToExecute(void) override
		{
			g_glVertexAttrib4f(m_index, m_x, m_y, m_z, m_w);
		}
	private:
		GLuint m_index;
		GLfloat m_x;
		GLfloat m_y;
		GLfloat m_z;
		GLfloat m_w;
	};

	class GlVertexAttrib4fvCommand : public OpenGlCommand
	{
	public:
		GlVertexAttrib4fvCommand(GLuint index, std::unique_ptr<GLfloat[]> v):
			OpenGlCommand(false, false, "glVertexAttrib4fv"), m_index(index), m_v(std::move(v))
		{
		}

		void commandToExecute(void) override
		{
			g_glVertexAttrib4fv(m_index, m_v.get());
		}
	private:
		GLuint m_index;
		std::unique_ptr<GLfloat[]> m_v;
	};

	class GlDepthRangefCommand : public OpenGlCommand
	{
	public:
		GlDepthRangefCommand(GLfloat n, GLfloat f):
			OpenGlCommand(false, false, "glDepthRangef"), m_n(n), m_f(f)
		{
		}

		void commandToExecute(void) override
		{
			g_glDepthRangef(m_n,m_f);
		}
	private:
		GLfloat m_n;
		GLfloat m_f;
	};

	class GlClearDepthfCommand : public OpenGlCommand
	{
	public:
		GlClearDepthfCommand(GLfloat d):
			OpenGlCommand(false, false, "glClearDepthf"), m_d(d)
		{
		}

		void commandToExecute(void) override
		{
			g_glClearDepthf(m_d);
		}
	private:
		GLfloat m_d;
	};

	class GlDrawBuffersCommand : public OpenGlCommand
	{
	public:
		GlDrawBuffersCommand(GLsizei n, std::unique_ptr<GLenum[]> bufs):
			OpenGlCommand(false, false, "glDrawBuffers"), m_n(n), m_bufs(std::move(bufs))
		{
		}

		void commandToExecute(void) override
		{
			g_glDrawBuffers(m_n, m_bufs.get());
		}
	private:
		GLsizei m_n;
		std::unique_ptr<GLenum[]> m_bufs;
	};

	class GlGenFramebuffersCommand : public OpenGlCommand
	{
	public:
		GlGenFramebuffersCommand(GLsizei n, GLuint* framebuffers):
			OpenGlCommand(true, false, "glGenFramebuffers"), m_n(n), m_framebuffers(framebuffers)
		{
		}

		void commandToExecute(void) override
		{
			g_glGenFramebuffers(m_n, m_framebuffers);
		}
	private:
		GLsizei m_n;
		GLuint* m_framebuffers;
	};

	class GlBindFramebufferCommand : public OpenGlCommand
	{
	public:
		GlBindFramebufferCommand(GLenum target, GLuint framebuffer):
			OpenGlCommand(false, false, "glBindFramebuffer"), m_target(target), m_framebuffer(framebuffer)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindFramebuffer(m_target, m_framebuffer);
		}
	private:
		GLenum m_target;
		GLuint m_framebuffer;
	};

	class GlDeleteFramebuffersCommand : public OpenGlCommand
	{
	public:
		GlDeleteFramebuffersCommand(GLsizei n, std::unique_ptr<GLuint[]> framebuffers):
			OpenGlCommand(false, false, "glDeleteFramebuffers"), m_n(n), m_framebuffers(std::move(framebuffers))
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteFramebuffers(m_n, m_framebuffers.get());
		}
	private:
		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_framebuffers;
	};

	class GlFramebufferTexture2DCommand : public OpenGlCommand
	{
	public:
		GlFramebufferTexture2DCommand(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level):
			OpenGlCommand(false, false, "glFramebufferTexture2D"), m_target(target), m_attachment(attachment), m_textarget(textarget),
			m_texture(texture), m_level(level)
		{
		}

		void commandToExecute(void) override
		{
			g_glFramebufferTexture2D(m_target, m_attachment, m_textarget, m_texture, m_level);
		}
	private:
		GLenum m_target;
		GLenum m_attachment;
		GLenum m_textarget;
		GLuint m_texture;
		GLint m_level;
	};

	class GlTexImage2DMultisampleCommand : public OpenGlCommand
	{
	public:
		GlTexImage2DMultisampleCommand(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
			GLsizei height, GLboolean fixedsamplelocations):
			OpenGlCommand(false, false, "glTexImage2DMultisample"), m_target(target), m_samples(samples), m_internalformat(internalformat),
			m_width(width), m_height(height), m_fixedsamplelocations(fixedsamplelocations)
		{
		}

		void commandToExecute(void) override
		{
			g_glTexImage2DMultisample(m_target, m_samples, m_internalformat, m_width, m_height, m_fixedsamplelocations);
		}
	private:
		GLenum m_target;
		GLsizei m_samples;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
		GLboolean m_fixedsamplelocations;
	};

	class GlTexStorage2DMultisampleCommand : public OpenGlCommand
	{
	public:
		GlTexStorage2DMultisampleCommand(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
			GLsizei height, GLboolean fixedsamplelocations):
			OpenGlCommand(false, false, "glTexStorage2DMultisample"), m_target(target), m_samples(samples), m_internalformat(internalformat),
			m_width(width), m_height(height), m_fixedsamplelocations(fixedsamplelocations)
		{
		}

		void commandToExecute(void) override
		{
			g_glTexStorage2DMultisample(m_target, m_samples, m_internalformat, m_width, m_height, m_fixedsamplelocations);
		}
	private:
		GLenum m_target;
		GLsizei m_samples;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
		GLboolean m_fixedsamplelocations;
	};

	class GlGenRenderbuffersCommand : public OpenGlCommand
	{
	public:
		GlGenRenderbuffersCommand(GLsizei n, GLuint* renderbuffers):
			OpenGlCommand(true, false, "glGenRenderbuffers"), m_n(n), m_renderbuffers(renderbuffers)
		{
		}

		void commandToExecute(void) override
		{
			g_glGenRenderbuffers(m_n, m_renderbuffers);
		}
	private:
		GLsizei m_n;
		GLuint* m_renderbuffers;
	};

	class GlBindRenderbufferCommand : public OpenGlCommand
	{
	public:
		GlBindRenderbufferCommand(GLenum target, GLuint renderbuffer):
			OpenGlCommand(false, false, "glBindRenderbuffer"), m_target(target), m_renderbuffer(renderbuffer)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindRenderbuffer(m_target, m_renderbuffer);
		}
	private:
		GLenum m_target;
		GLuint m_renderbuffer;
	};

	class GlRenderbufferStorageCommand : public OpenGlCommand
	{
	public:
		GlRenderbufferStorageCommand(GLenum target, GLenum internalformat, GLsizei width, GLsizei height):
			OpenGlCommand(false, false, "glRenderbufferStorage"), m_target(target), m_internalformat(internalformat), m_width(width), m_height(height)
		{
		}

		void commandToExecute(void) override
		{
			g_glRenderbufferStorage(m_target, m_internalformat, m_width, m_height);
		}
	private:
		GLenum m_target;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlDeleteRenderbuffersCommand : public OpenGlCommand
	{
	public:
		GlDeleteRenderbuffersCommand(GLsizei n, std::unique_ptr<GLuint[]> renderbuffers):
			OpenGlCommand(false, false, "glDeleteRenderbuffers"), m_n(n), m_renderbuffers(std::move(renderbuffers))
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteRenderbuffers(m_n, m_renderbuffers.get());
		}
	private:
		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_renderbuffers;
	};

	class GlFramebufferRenderbufferCommand : public OpenGlCommand
	{
	public:
		GlFramebufferRenderbufferCommand(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer):
			OpenGlCommand(false, false, "glFramebufferRenderbuffer"), m_target(target), m_attachment(attachment), m_renderbuffertarget(renderbuffertarget),
			m_renderbuffer(renderbuffer)
		{
		}

		void commandToExecute(void) override
		{
			g_glFramebufferRenderbuffer(m_target, m_attachment, m_renderbuffertarget, m_renderbuffer);
		}
	private:
		GLenum m_target;
		GLenum m_attachment;
		GLenum m_renderbuffertarget;
		GLuint m_renderbuffer;
	};

	class GlCheckFramebufferStatusCommand : public OpenGlCommand
	{
	public:
		GlCheckFramebufferStatusCommand(GLenum target, GLenum& returnValue):
			OpenGlCommand(true, true, "glCheckFramebufferStatus"), m_target(target), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glCheckFramebufferStatus(m_target);
		}
	private:
		GLenum m_target;
		GLenum& m_returnValue;
	};

	class GlBlitFramebufferCommand : public OpenGlCommand
	{
	public:
		GlBlitFramebufferCommand(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0,
			GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter):
			OpenGlCommand(false, false, "glBlitFramebuffer"), m_srcX0(srcX0), m_srcY0(srcY0), m_srcX1(srcX1), m_srcY1(srcY1), m_dstX0(dstX0),
			m_dstY0(dstY0), m_dstX1(dstX1), m_dstY1(dstY1), m_mask(mask), m_filter(filter)
		{
		}

		void commandToExecute(void) override
		{
			g_glBlitFramebuffer(m_srcX0, m_srcY0, m_srcX1, m_srcY1, m_dstX0, m_dstY0, m_dstX1, m_dstY1, m_mask,
				m_filter);
		}
	private:
		GLint m_srcX0;
		GLint m_srcY0;
		GLint m_srcX1;
		GLint m_srcY1;
		GLint m_dstX0;
		GLint m_dstY0;
		GLint m_dstX1;
		GLint m_dstY1;
		GLbitfield m_mask;
		GLenum m_filter;
	};

	class GlGenVertexArraysCommand : public OpenGlCommand
	{
	public:
		GlGenVertexArraysCommand(GLsizei n, GLuint* arrays):
			OpenGlCommand(true, false, "glGenVertexArrays"), m_n(n), m_arrays(arrays)
		{
		}

		void commandToExecute(void) override
		{
			g_glGenVertexArrays(m_n, m_arrays);
		}
	private:
		GLsizei m_n;
		GLuint* m_arrays;
	};

	class GlBindVertexArrayCommand : public OpenGlCommand
	{
	public:
		GlBindVertexArrayCommand(GLuint array):
			OpenGlCommand(false, false, "glBindVertexArray"), m_array(array)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindVertexArray(m_array);
		}
	private:
		GLuint m_array;
	};

	class GlDeleteVertexArraysCommand : public OpenGlCommand
	{
	public:
		GlDeleteVertexArraysCommand(GLsizei n, std::unique_ptr<GLuint[]> arrays):
			OpenGlCommand(false, false, "glDeleteVertexArrays"), m_n(n), m_arrays(std::move(arrays))
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteVertexArrays(m_n, m_arrays.get());
		}
	private:
		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_arrays;
	};

	class GlGenBuffersCommand : public OpenGlCommand
	{
	public:
		GlGenBuffersCommand(GLsizei n, GLuint* buffers):
			OpenGlCommand(true, false, "glGenBuffers"), m_n(n), m_buffers(buffers)
		{
		}

		void commandToExecute(void) override
		{
			g_glGenBuffers(m_n, m_buffers);
		}
	private:
		GLsizei m_n;
		GLuint* m_buffers;
	};

	class GlBindBufferCommand : public OpenGlCommand
	{
	public:
		GlBindBufferCommand(GLenum target, GLuint buffer):
			OpenGlCommand(false, false, "glBindBuffer"), m_target(target), m_buffer(buffer)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindBuffer(m_target, m_buffer);
		}
	private:
		GLenum m_target;
		GLuint m_buffer;
	};

	template <class dataType>
	class GlBufferDataCommand : public OpenGlCommand
	{
	public:
		GlBufferDataCommand(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLenum usage):
			OpenGlCommand(false, false, "glBufferData"), m_target(target), m_size(size), m_data(std::move(data)), m_usage(usage)
		{
		}

		void commandToExecute(void) override
		{
			g_glBufferData(m_target, m_size, m_data.get(), m_usage);
		}
	private:
		GLenum m_target;
		GLsizeiptr m_size;
		std::unique_ptr<dataType[]> m_data;
		GLenum m_usage;
	};

	class GlMapBufferCommand : public OpenGlCommand
	{
	public:
		GlMapBufferCommand(GLenum target, GLenum access):
			OpenGlCommand(false, false, "glMapBuffer"), m_target(target), m_access(access)
		{
		}

		void commandToExecute(void) override
		{
			g_glMapBuffer(m_target, m_access);
		}
	private:
		GLenum m_target;
		GLenum m_access;
	};

	class GlMapBufferRangeCommand : public OpenGlCommand
	{
	public:
		GlMapBufferRangeCommand(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access,
			GLubyte*& returnValue):
			OpenGlCommand(true, true, "glMapBufferRange"), m_target(target), m_offset(offset), m_length(length), m_access(access),
			m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = reinterpret_cast<GLubyte*>(g_glMapBufferRange(m_target, m_offset, m_length, m_access));
		}
	private:
		GLenum m_target;
		GLintptr m_offset;
		GLsizeiptr m_length;
		GLbitfield m_access;
		GLubyte*& m_returnValue;
	};

	class GlMapBufferRangeWriteAsyncCommand : public OpenGlCommand
	{
	public:
		GlMapBufferRangeWriteAsyncCommand(GLenum target, GLuint buffer, GLintptr offset, u32 length,
			GLbitfield access, std::unique_ptr<u8[]> data):
			OpenGlCommand(false, false, "GlMapBufferRangeWriteAsyncCommand"), m_target(target), m_buffer(buffer),
			m_offset(offset), m_length(length), m_access(access), m_data(std::move(data))
		{
		}

		void commandToExecute(void) override
		{
			g_glBindBuffer(m_target, m_buffer);
			void* buffer_pointer = g_glMapBufferRange(m_target, m_offset, m_length, m_access);
			memcpy(buffer_pointer, m_data.get(), m_length);
			g_glUnmapBuffer(m_target);
		}
	private:
		GLenum m_target;
		GLuint m_buffer;
		GLintptr m_offset;
		u32 m_length;
		GLbitfield m_access;
		std::unique_ptr<u8[]> m_data;
	};

	class GlMapBufferRangeReadAsyncCommand : public OpenGlCommand
	{
	public:
		GlMapBufferRangeReadAsyncCommand(GLenum target, GLuint buffer, GLintptr offset, u32 length,
				GLbitfield access):
				OpenGlCommand(false, false, "GlMapBufferRangeReadAsyncCommand"), m_target(target), m_buffer(buffer),
				m_offset(offset), m_length(length), m_access(access)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindBuffer(m_target, m_buffer);
			void* buffer_pointer = g_glMapBufferRange(m_target, m_offset, m_length, m_access);

			if (buffer_pointer != nullptr) {
				std::unique_lock<std::mutex> lock(m_mapMutex);
				verifyBuffer(m_buffer, m_length);
				auto data = m_data[m_buffer];
				memcpy(data->data(), buffer_pointer, m_length);
			}
		}

		static std::shared_ptr<std::vector<u8>> getData(GLuint buffer, u32 length)
		{
			std::unique_lock<std::mutex> lock(m_mapMutex);
			verifyBuffer(buffer, length);
			return m_data[buffer];
		}
		
	private:
	
		static void verifyBuffer(GLuint buffer, u32 length)
		{
			if (m_data[buffer] == nullptr || m_data[buffer]->size() < length) {
				m_data[buffer] = std::make_shared<std::vector<u8>>(length);
			}
		}
		
		GLenum m_target;
		GLuint m_buffer;
		GLintptr m_offset;
		u32 m_length;
		GLbitfield m_access;
		static std::unordered_map<int, std::shared_ptr<std::vector<u8>>> m_data;
		static std::unordered_map<int, u32> m_sizes;
		static std::mutex m_mapMutex;
	};

	class GlUnmapBufferCommand : public OpenGlCommand
	{
	public:
		GlUnmapBufferCommand(GLenum target, GLboolean& returnValue):
			OpenGlCommand(true, true, "glUnmapBuffer"), m_target(target), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glUnmapBuffer(m_target);
		}
	private:
		GLenum m_target;
		GLboolean& m_returnValue;
	};

	class GlUnmapBufferAsyncCommand : public OpenGlCommand
	{
	public:
		GlUnmapBufferAsyncCommand(GLenum target):
			OpenGlCommand(false, false, "glUnmapBuffer"), m_target(target)
		{
		}

		void commandToExecute(void) override
		{
			g_glUnmapBuffer(m_target);
		}
	private:
		GLenum m_target;
	};

	class GlDeleteBuffersCommand : public OpenGlCommand
	{
	public:
		GlDeleteBuffersCommand(GLsizei n, std::unique_ptr<GLuint[]> buffers):
			OpenGlCommand(false, false, "glDeleteBuffers"), m_n(n), m_buffers(std::move(buffers))
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteBuffers(m_n, m_buffers.get());
		}
	private:
		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_buffers;
	};

	class GlBindImageTextureCommand : public OpenGlCommand
	{
	public:
		GlBindImageTextureCommand(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
			GLenum access, GLenum format):
			OpenGlCommand(false, false, "glBindImageTexture"), m_unit(unit), m_texture(texture), m_level(level), m_layered(layered), m_layer(layer),
			m_access(access), m_format(format)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindImageTexture(m_unit, m_texture, m_level, m_layered, m_layer, m_access, m_format);
		}
	private:
		GLuint m_unit;
		GLuint m_texture;
		GLint m_level;
		GLboolean m_layered;
		GLint m_layer;
		GLenum m_access;
		GLenum m_format;
	};

	class GlMemoryBarrierCommand : public OpenGlCommand
	{
	public:
		GlMemoryBarrierCommand(GLbitfield barriers):
			OpenGlCommand(false, false, "glMemoryBarrier"), m_barriers(barriers)
		{
		}

		void commandToExecute(void) override
		{
			g_glMemoryBarrier(m_barriers);
		}
	private:
		GLbitfield m_barriers;
	};

	class GlGetStringiCommand : public OpenGlCommand
	{
	public:
		GlGetStringiCommand(GLenum name, GLuint index, const GLubyte*& returnValue):
			OpenGlCommand(true, false, "glGetStringi"), m_name(name), m_index(index), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glGetStringi(m_name, m_index);
		}
	private:
		GLenum m_name;
		GLuint m_index;
		const GLubyte*& m_returnValue;
	};

	class GlInvalidateFramebufferCommand : public OpenGlCommand
	{
	public:
		GlInvalidateFramebufferCommand(GLenum target, GLsizei numAttachments, std::unique_ptr<GLenum[]> attachments):
			OpenGlCommand(false, false, "glInvalidateFramebuffer"), m_target(target), m_numAttachments(numAttachments),
			m_attachments(std::move(attachments))
		{
		}

		void commandToExecute(void) override
		{
			g_glInvalidateFramebuffer(m_target, m_numAttachments, m_attachments.get());
		}
	private:
		GLenum m_target;
		GLsizei m_numAttachments;
		std::unique_ptr<GLenum[]> m_attachments;
	};

	template <class dataType>
	class GlBufferStorageCommand : public OpenGlCommand
	{
	public:
		GlBufferStorageCommand(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLbitfield flags):
			OpenGlCommand(false, false, "glBufferStorage"), m_target(target), m_size(size), m_data(std::move(data)), m_flags(flags)
		{
		}

		void commandToExecute(void) override
		{
			g_glBufferStorage(m_target, m_size, m_data.get(), m_flags);
		}
	private:
		GLenum m_target;
		GLsizeiptr m_size;
		std::unique_ptr<dataType[]> m_data;
		GLbitfield m_flags;
	};

	class GlFenceSyncCommand : public OpenGlCommand
	{
	public:
		GlFenceSyncCommand(GLenum condition, GLbitfield flags, GLsync& returnValue):
			OpenGlCommand(true, false, "glFenceSync"), m_condition(condition), m_flags(flags), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glFenceSync(m_condition, m_flags);
		}
	private:
		GLenum m_condition;
		GLbitfield m_flags;
		GLsync& m_returnValue;
	};

	class GlClientWaitSyncCommand : public OpenGlCommand
	{
	public:
		GlClientWaitSyncCommand(GLsync sync, GLbitfield flags, GLuint64 timeout):
			OpenGlCommand(true, false, "glClientWaitSync"), m_sync(sync), m_flags(flags), m_timeout(timeout)
		{
		}

		void commandToExecute(void) override
		{
			g_glClientWaitSync(m_sync, m_flags, m_timeout);
		}
	private:
		GLsync m_sync;
		GLbitfield m_flags;
		GLuint64 m_timeout;
	};

	class GlDeleteSyncCommand : public OpenGlCommand
	{
	public:
		GlDeleteSyncCommand(GLsync sync):
			OpenGlCommand(true, false, "glDeleteSync"), m_sync(sync)
		{
		}

		void commandToExecute(void) override
		{
			g_glDeleteSync(m_sync);
		}
	private:
		GLsync m_sync;
	};

	class GlGetUniformBlockIndexCommand : public OpenGlCommand
	{
	public:
		GlGetUniformBlockIndexCommand(GLuint program, const GLchar* uniformBlockName, GLuint& returnValue):
			OpenGlCommand(true, true, "glGetUniformBlockIndex"), m_program(program), m_uniformBlockName(uniformBlockName), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = g_glGetUniformBlockIndex(m_program, m_uniformBlockName);
		}
	private:
		GLuint m_program;
		const GLchar* m_uniformBlockName;
		GLuint& m_returnValue;
	};

	class GlUniformBlockBindingCommand : public OpenGlCommand
	{
	public:
		GlUniformBlockBindingCommand(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding):
			OpenGlCommand(false, false, "glUniformBlockBinding"), m_program(program), m_uniformBlockIndex(uniformBlockIndex),
			m_uniformBlockBinding(uniformBlockBinding)
		{
		}

		void commandToExecute(void) override
		{
			g_glUniformBlockBinding(m_program, m_uniformBlockIndex, m_uniformBlockBinding);
		}
	private:
		GLuint m_program;
		GLuint m_uniformBlockIndex;
		GLuint m_uniformBlockBinding;
	};

	class GlGetActiveUniformBlockivCommand : public OpenGlCommand
	{
	public:
		GlGetActiveUniformBlockivCommand(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params):
			OpenGlCommand(true, true, "glGetActiveUniformBlockiv"), m_program(program), m_uniformBlockIndex(uniformBlockIndex), m_pname(pname),
			m_params(params)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetActiveUniformBlockiv(m_program, m_uniformBlockIndex, m_pname, m_params);
		}
	private:
		GLuint m_program;
		GLuint m_uniformBlockIndex;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlGetUniformIndicesCommand : public OpenGlCommand
	{
	public:
		GlGetUniformIndicesCommand(GLuint program, GLsizei uniformCount, const GLchar* const*uniformNames,
			GLuint* uniformIndices):
			OpenGlCommand(true, true, "glGetUniformIndices"), m_program(program), m_uniformCount(uniformCount), m_uniformNames(uniformNames),
			m_uniformIndices(uniformIndices)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetUniformIndices(m_program, m_uniformCount, m_uniformNames, m_uniformIndices);
		}
	private:
		GLuint m_program;
		GLsizei m_uniformCount;
		const GLchar* const* m_uniformNames;
		GLuint* m_uniformIndices;
	};

	class GlGetActiveUniformsivCommand : public OpenGlCommand
	{
	public:
		GlGetActiveUniformsivCommand(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname,
			GLint* params):
			OpenGlCommand(true, true, "glGetActiveUniformsiv"), m_program(program), m_uniformCount(uniformCount), m_uniformIndices(uniformIndices),
			m_pname(pname), m_params(params)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetActiveUniformsiv(m_program, m_uniformCount, m_uniformIndices, m_pname, m_params);
		}
	private:
		GLuint m_program;
		GLsizei m_uniformCount;
		const GLuint* m_uniformIndices;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlBindBufferBaseCommand : public OpenGlCommand
	{
	public:
		GlBindBufferBaseCommand(GLenum target, GLuint index, GLuint buffer):
			OpenGlCommand(false, false, "glBindBufferBase"), m_target(target), m_index(index), m_buffer(buffer)
		{
		}

		void commandToExecute(void) override
		{
			g_glBindBufferBase(m_target, m_index, m_buffer);
		}
	private:
		GLenum m_target;
		GLuint m_index;
		GLuint m_buffer;
	};

	template <class dataType>
	class GlBufferSubDataCommand : public OpenGlCommand
	{
	public:
		GlBufferSubDataCommand(GLenum target, GLintptr offset, GLsizeiptr size, std::unique_ptr<dataType[]> data):
			OpenGlCommand(false, false, "glBufferSubData"), m_target(target), m_offset(offset), m_size(size), m_data(std::move(data))
		{
		}

		void commandToExecute(void) override
		{
			g_glBufferSubData(m_target, m_offset, m_size, m_data.get());
		}
	private:
		GLenum m_target;
		GLintptr m_offset;
		GLsizeiptr m_size;
		std::unique_ptr<dataType[]> m_data;
	};

	class GlGetProgramBinaryCommand : public OpenGlCommand
	{
	public:
		GlGetProgramBinaryCommand(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary):
			OpenGlCommand(true, true, "glGetProgramBinary"), m_program(program), m_bufSize(bufSize), m_length(length), m_binaryFormat(binaryFormat),
			m_binary(binary)
		{
		}

		void commandToExecute(void) override
		{
			g_glGetProgramBinary(m_program, m_bufSize, m_length, m_binaryFormat, m_binary);
		}
	private:
		GLuint m_program;
		GLsizei m_bufSize;
		GLsizei* m_length;
		GLenum* m_binaryFormat;
		void* m_binary;
	};

	template <class dataType>
	class GlProgramBinaryCommand : public OpenGlCommand
	{
	public:
		GlProgramBinaryCommand(GLuint program, GLenum binaryFormat, std::unique_ptr<dataType[]> binary, GLsizei length):
			OpenGlCommand(false, false, "glProgramBinary"), m_program(program), m_binaryFormat(binaryFormat), m_binary(std::move(binary)),
			m_length(length)
		{
		}

		void commandToExecute(void) override
		{
			g_glProgramBinary(m_program, m_binaryFormat, m_binary.get(), m_length);
		}
	private:
		GLuint m_program;
		GLenum m_binaryFormat;
		std::unique_ptr<dataType[]> m_binary;
		GLsizei m_length;
	};

	class GlProgramParameteriCommand : public OpenGlCommand
	{
	public:
		GlProgramParameteriCommand(GLuint program, GLenum pname, GLint value):
			OpenGlCommand(false, false, "glProgramParameteri"), m_program(program), m_pname(pname), m_value(value)
		{
		}

		void commandToExecute(void) override
		{
			g_glProgramParameteri(m_program, m_pname, m_value);
		}
	private:
		GLuint m_program;
		GLenum m_pname;
		GLint m_value;
	};

	class GlTexStorage2DCommand : public OpenGlCommand
	{
	public:
		GlTexStorage2DCommand(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height):
			OpenGlCommand(false, false, "glTexStorage2D"), m_target(target), m_levels(levels), m_internalformat(internalformat),
			m_width(width), m_height(height)
		{
		}

		void commandToExecute(void) override
		{
			g_glTexStorage2D(m_target, m_levels, m_internalformat, m_width, m_height);
		}
	private:
		GLenum m_target;
		GLsizei m_levels;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlTextureStorage2DCommand : public OpenGlCommand
	{
	public:
		GlTextureStorage2DCommand(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height):
			OpenGlCommand(false, false, "glTextureStorage2D"), m_texture(texture), m_levels(levels), m_internalformat(internalformat),
			m_width(width), m_height(height)
		{
		}

		void commandToExecute(void) override
		{
			g_glTextureStorage2D(m_texture, m_levels, m_internalformat, m_width, m_height);
		}
	private:
		GLuint m_texture;
		GLsizei m_levels;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
	};

	template <class pixelType>
	class GlTextureSubImage2DUnbufferedCommand : public OpenGlCommand
	{
	public:
		GlTextureSubImage2DUnbufferedCommand(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
			GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels):
			OpenGlCommand(false, false, "glTextureSubImage2D"), m_texture(texture), m_level(level), m_xoffset(xoffset), m_yoffset(yoffset),
			m_width(width), m_height(height), m_format(format), m_type(type), m_pixels(std::move(pixels))
		{
		}

		void commandToExecute(void) override
		{
			g_glTextureSubImage2D(m_texture, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type,
								  m_pixels.get());
		}
	private:
		GLuint m_texture;
		GLint m_level;
		GLint m_xoffset;
		GLint m_yoffset;
		GLsizei m_width;
		GLsizei m_height;
		GLenum m_format;
		GLenum m_type;
		std::unique_ptr<pixelType[]> m_pixels;
	};

	class GlTextureSubImage2DBufferedCommand : public OpenGlCommand
	{
	public:
		GlTextureSubImage2DBufferedCommand(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
			GLsizei height, GLenum format, GLenum type, std::size_t offset):
			OpenGlCommand(false, false, "glTextureSubImage2D"), m_texture(texture), m_level(level), m_xoffset(xoffset), m_yoffset(yoffset),
			m_width(width), m_height(height), m_format(format), m_type(type), m_offset(offset)
		{
		}

		void commandToExecute(void) override
		{
			g_glTextureSubImage2D(m_texture, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type,
								  (const GLvoid* )m_offset);
		}
	private:
		GLuint m_texture;
		GLint m_level;
		GLint m_xoffset;
		GLint m_yoffset;
		GLsizei m_width;
		GLsizei m_height;
		GLenum m_format;
		GLenum m_type;
		std::size_t m_offset;
	};

	class GlTextureStorage2DMultisampleCommand : public OpenGlCommand
	{
	public:
		GlTextureStorage2DMultisampleCommand(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
			GLsizei width, GLsizei height, GLboolean fixedsamplelocations):
			OpenGlCommand(false, false, "glTextureStorage2DMultisample"), m_texture(texture), m_target(target), m_samples(samples),
			m_internalformat(internalformat), m_width(width), m_height(height),
			m_fixedsamplelocations(fixedsamplelocations)
		{
		}

		void commandToExecute(void) override
		{
			g_glTextureStorage2DMultisample(m_texture, m_target, m_samples, m_internalformat,m_width, m_height,
				m_fixedsamplelocations);
		}
	private:
		GLuint m_texture;
		GLenum m_target;
		GLsizei m_samples;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
		GLboolean m_fixedsamplelocations;
	};

	class GlTextureParameteriCommand : public OpenGlCommand
	{
	public:
		GlTextureParameteriCommand(GLuint texture, GLenum pname, GLint param):
			OpenGlCommand(false, false, "glTextureParameteri"), m_texture(texture), m_pname(pname), m_param(param)
		{
		}

		void commandToExecute(void) override
		{
			g_glTextureParameteri(m_texture, m_pname, m_param);
		}
	private:
		GLuint m_texture;
		GLenum m_pname;
		GLint m_param;
	};

	class GlTextureParameterfCommand : public OpenGlCommand
	{
	public:
		GlTextureParameterfCommand(GLuint texture, GLenum pname, GLfloat param):
			OpenGlCommand(false, false, "glTextureParameterf"), m_texture(texture), m_pname(pname), m_param(param)
		{
		}

		void commandToExecute(void) override
		{
			g_glTextureParameterf(m_texture, m_pname, m_param);
		}
	private:
		GLuint m_texture;
		GLenum m_pname;
		GLfloat m_param;
	};

	class GlCreateTexturesCommand : public OpenGlCommand
	{
	public:
		GlCreateTexturesCommand(GLenum target, GLsizei n, GLuint* textures):
			OpenGlCommand(true, false, "glCreateTextures"), m_target(target), m_n(n), m_textures(textures)
		{
		}

		void commandToExecute(void) override
		{
			g_glCreateTextures(m_target, m_n, m_textures);
		}
	private:
		GLenum m_target;
		GLsizei m_n;
		GLuint* m_textures;
	};

	class GlCreateBuffersCommand : public OpenGlCommand
	{
	public:
		GlCreateBuffersCommand(GLsizei n, GLuint* buffers):
			OpenGlCommand(true, false, "glCreateBuffers"), m_n(n), m_buffers(buffers)
		{
		}

		void commandToExecute(void) override
		{
			g_glCreateBuffers(m_n, m_buffers);
		}
	private:
		GLsizei m_n;
		GLuint* m_buffers;
	};

	class GlCreateFramebuffersCommand : public OpenGlCommand
	{
	public:
		GlCreateFramebuffersCommand(GLsizei n, GLuint* framebuffers):
			OpenGlCommand(true, false, "glCreateFramebuffers"), m_n(n), m_framebuffers(framebuffers)
		{
		}

		void commandToExecute(void) override
		{
			g_glCreateFramebuffers(m_n, m_framebuffers);
		}
	private:
		GLsizei m_n;
		GLuint* m_framebuffers;
	};

	class GlNamedFramebufferTextureCommand : public OpenGlCommand
	{
	public:
		GlNamedFramebufferTextureCommand(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level):
			OpenGlCommand(false, false, "glNamedFramebufferTexture"), m_framebuffer(framebuffer), m_attachment(attachment), m_texture(texture),
			m_level(level)
		{
		}

		void commandToExecute(void) override
		{
			g_glNamedFramebufferTexture(m_framebuffer, m_attachment, m_texture, m_level);
		}
	private:
		GLuint m_framebuffer;
		GLenum m_attachment;
		GLuint m_texture;
		GLint m_level;
	};

	class GlDrawElementsBaseVertexCommand : public OpenGlCommand
	{
	public:
		GlDrawElementsBaseVertexCommand(GLenum mode, GLsizei count, GLenum type, const char* indices,
										GLint basevertex):
			OpenGlCommand(false, false, "glDrawElementsBaseVertex"), m_mode(mode), m_count(count), m_type(type), m_indices(indices),
			m_basevertex(basevertex)
		{
		}

		void commandToExecute(void) override
		{
			g_glDrawElementsBaseVertex(m_mode, m_count, m_type, m_indices, m_basevertex);
		}
	private:
		GLenum m_mode;
		GLsizei m_count;
		GLenum m_type;
		const char*  m_indices;
		GLint m_basevertex;
	};

	class GlFlushMappedBufferRangeCommand : public OpenGlCommand
	{
	public:
		GlFlushMappedBufferRangeCommand(GLenum target, GLintptr offset, GLsizeiptr length):
			OpenGlCommand(false, false, "glFlushMappedBufferRange"), m_target(target), m_offset(offset), m_length(length)
		{
		}

		void commandToExecute(void) override
		{
			g_glFlushMappedBufferRange(m_target, m_offset, m_length);
		}
	private:
		GLenum m_target;
		GLintptr m_offset;
		GLsizeiptr m_length;
	};

	class GlFinishCommand : public OpenGlCommand
	{
	public:
		GlFinishCommand(void):
			OpenGlCommand(true, true, "glFinish")
		{
		}

		void commandToExecute(void) override
		{
			g_glFinish();
		}
	};
#ifdef MUPENPLUSAPI
	//Vid ext functions
	class CoreVideoInitCommand : public OpenGlCommand
	{
	public:
		CoreVideoInitCommand(void):
			OpenGlCommand(true, false, "CoreVideo_Init", false)
		{
		}

		void commandToExecute(void) override
		{
			::CoreVideo_Init();
		}
	};

	class CoreVideoQuitCommand : public OpenGlCommand
	{
	public:
		CoreVideoQuitCommand(void):
			OpenGlCommand(true, false, "CoreVideo_Quit", false)
		{
		}

		void commandToExecute(void) override
		{
			::CoreVideo_Quit();
		}
	};

	class CoreVideoSetVideoModeCommand : public OpenGlCommand
	{
	public:
		CoreVideoSetVideoModeCommand(int screenWidth, int screenHeight, int bitsPerPixel, m64p_video_mode mode,
			m64p_video_flags flags, m64p_error& returnValue):
			OpenGlCommand(true, false, "CoreVideo_SetVideoMode", false), m_screenWidth(screenWidth), m_screenHeight(screenHeight), m_bitsPerPixel(bitsPerPixel),
			m_mode(mode), m_flags(flags), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = ::CoreVideo_SetVideoMode(m_screenWidth, m_screenHeight, m_bitsPerPixel, m_mode, m_flags);

			initGLFunctions();
		}

		int m_screenWidth;
		int m_screenHeight;
		int m_bitsPerPixel;
		m64p_video_mode m_mode;
		m64p_video_flags m_flags;
		m64p_error& m_returnValue;
	};

	class CoreVideoGLSetAttributeCommand : public OpenGlCommand
	{
	public:
		CoreVideoGLSetAttributeCommand(m64p_GLattr attribute, int value):
			OpenGlCommand(true, false, "CoreVideo_GL_SetAttribute", false), m_attribute(attribute), m_value(value)
		{
		}

		void commandToExecute(void) override
		{
			::CoreVideo_GL_SetAttribute(m_attribute, m_value);
		}

		m64p_GLattr m_attribute;
		int m_value;
	};

	class CoreVideoGLGetAttributeCommand : public OpenGlCommand
	{
	public:
		CoreVideoGLGetAttributeCommand(m64p_GLattr attribute, int* value):
			OpenGlCommand(true, false, "CoreVideo_GL_GetAttribute", false), m_attribute(attribute), m_value(value)
		{
		}

		void commandToExecute(void) override
		{
			::CoreVideo_GL_GetAttribute(m_attribute, m_value);
		}

		m64p_GLattr m_attribute;
		int* m_value;
	};

	class CoreVideoGLSwapBuffersCommand : public OpenGlCommand
	{
	public:
		CoreVideoGLSwapBuffersCommand(std::function<void(void)> swapBuffersCallback):
			OpenGlCommand(false, false, "CoreVideo_GL_SwapBuffers", false), m_swapBuffersCallback(swapBuffersCallback)
		{
		}

		void commandToExecute(void) override
		{
			::CoreVideo_GL_SwapBuffers();
			m_swapBuffersCallback();
		}

		std::function<void(void)> m_swapBuffersCallback;
	};
#else
	//Zilmar API functions
	class WindowsStartCommand : public OpenGlCommand
	{
	public:
		WindowsStartCommand(bool& returnValue) :
			OpenGlCommand(true, false, "WindowsStartCommand", false), m_returnValue(returnValue)
		{
		}

		void commandToExecute(void) override
		{
			m_returnValue = WindowsWGL::start();
		}

		bool& m_returnValue;
	};

	class WindowsStopCommand : public OpenGlCommand
	{
	public:
		WindowsStopCommand(void) :
			OpenGlCommand(true, false, "WindowsStopCommand", false)
		{
		}

		void commandToExecute(void) override
		{
			WindowsWGL::stop();
		}
	};

	class WindowsSwapBuffersCommand : public OpenGlCommand
	{
	public:
		WindowsSwapBuffersCommand(std::function<void(void)> swapBuffersCallback) :
			OpenGlCommand(false, false, "WindowsSwapBuffersCommand", false), m_swapBuffersCallback(swapBuffersCallback)
		{
		}

		void commandToExecute(void) override
		{
			WindowsWGL::swapBuffers();
			m_swapBuffersCallback();
		}
	private:
		std::function<void(void)> m_swapBuffersCallback;
	};

#endif
}
