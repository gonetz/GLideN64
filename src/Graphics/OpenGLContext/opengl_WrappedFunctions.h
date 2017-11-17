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
		static std::shared_ptr<OpenGlCommand> get(GLenum sfactor, GLenum dfactor)
		{
			auto ptr = std::shared_ptr<GlBlendFuncCommand>(new GlBlendFuncCommand);
			ptr->set(sfactor, dfactor);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBlendFunc(m_sfactor, m_dfactor);
		}
	private:
		GlBlendFuncCommand(void) :
			OpenGlCommand(false, false, "glBlendFunc")
		{
		}

		void set(GLenum sfactor, GLenum dfactor)
		{
			m_sfactor = sfactor;
			m_dfactor = dfactor;
		}

		GLenum m_sfactor;
		GLenum m_dfactor;
	};

	class GlPixelStoreiCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum pname, GLint param)
		{
			auto ptr = std::shared_ptr<GlPixelStoreiCommand>(new GlPixelStoreiCommand);
			ptr->set(pname, param);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glPixelStorei(m_pname, m_param);
		}

	private:
		GlPixelStoreiCommand(void) :
			OpenGlCommand(false, false, "glPixelStorei")
		{
		}

		void set(GLenum pname, GLint param)
		{
			m_pname = pname;
			m_param = param;
		}

		GLenum m_pname;
		GLint m_param;
	};

	class GlClearColorCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
		{
			auto ptr = std::shared_ptr<GlClearColorCommand>(new GlClearColorCommand);
			ptr->set(red, green, blue, alpha);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glClearColor(m_red, m_green, m_blue, m_alpha);
		}
	private:
		GlClearColorCommand(void) :
			OpenGlCommand(false, false, "glClearColor")
		{

		}

		void set(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
		{
			m_red = red;
			m_green = green;
			m_blue = blue;
			m_alpha = alpha;
		}

		GLfloat m_red;
		GLfloat m_green;
		GLfloat m_blue;
		GLfloat m_alpha;
	};

	class GlCullFaceCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum mode)
		{
			auto ptr = std::shared_ptr<GlCullFaceCommand>(new GlCullFaceCommand);
			ptr->set(mode);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glCullFace(m_mode);
		}

	private:
		GlCullFaceCommand(void) :
			OpenGlCommand(false, false, "glCullFace")
		{
		}

		void set(GLenum mode)
		{
			m_mode = mode;
		}

		GLenum m_mode;
	};

	class GlDepthFuncCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum func)
		{
			auto ptr = std::shared_ptr<GlDepthFuncCommand>(new GlDepthFuncCommand);
			ptr->set(func);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDepthFunc(m_func);
		}
	private:
		GlDepthFuncCommand(void) :
			OpenGlCommand(false, false, "glDepthFunc")
		{
		}

		void set(GLenum func)
		{
			m_func = func;
		}

		GLenum m_func;
	};

	class GlDepthMaskCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLboolean flag)
		{
			auto ptr = std::shared_ptr<GlDepthMaskCommand>(new GlDepthMaskCommand);
			ptr->set(flag);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDepthMask(m_flag);
		}
	private:
		GlDepthMaskCommand(void) :
			OpenGlCommand(false, false, "glDepthMask")
		{

		}

		void set(GLboolean flag)
		{
			m_flag = flag;
		}

		GLboolean m_flag;
	};

	class GlDisableCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum cap)
		{
			auto ptr = std::shared_ptr<GlDisableCommand>(new GlDisableCommand);
			ptr->set(cap);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDisable(m_cap);
		}
	private:
		GlDisableCommand(void) :
			OpenGlCommand(false, false, "glDisable")
		{
		}

		void set(GLenum cap)
		{
			m_cap = cap;
		}

		GLenum m_cap;
	};

	class GlEnableCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum cap)
		{
			auto ptr = std::shared_ptr<GlEnableCommand>(new GlEnableCommand);
			ptr->set(cap);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glEnable(m_cap);
		}
	private:
		GlEnableCommand(void) :
			OpenGlCommand(false, false, "glEnable")
		{
		}

		void set(GLenum cap)
		{
			m_cap = cap;
		}

		GLenum m_cap;
	};

	class GlPolygonOffsetCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLfloat factor, GLfloat units)
		{
			auto ptr = std::shared_ptr<GlPolygonOffsetCommand>(new GlPolygonOffsetCommand);
			ptr->set(factor, units);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glPolygonOffset(m_factor, m_units);
		}
	private:
		GlPolygonOffsetCommand(void) :
			OpenGlCommand(false, false, "glPolygonOffset")
		{
		}

		void set(GLfloat factor, GLfloat units)
		{
			m_factor = factor;
			m_units = units;
		}

		GLfloat m_factor;
		GLfloat m_units;
	};

	class GlScissorCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint x, GLint y, GLsizei width, GLsizei height)
		{
			auto ptr = std::shared_ptr<GlScissorCommand>(new GlScissorCommand);
			ptr->set(x, y, width, height);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glScissor(m_x, m_y, m_width, m_height);
		}
	private:
		GlScissorCommand(void) :
			OpenGlCommand(false, false, "glScissor")
		{
		}

		void set(GLint x, GLint y, GLsizei width, GLsizei height)
		{
			m_x = x;
			m_y = y;
			m_width = width;
			m_height = height;
		}

		GLint m_x;
		GLint m_y;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlViewportCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint x, GLint y, GLsizei width, GLsizei height)
		{
			auto ptr = std::shared_ptr<GlViewportCommand>(new GlViewportCommand);
			ptr->set(x, y, width, height);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glViewport(m_x, m_y, m_width, m_height);
		}
	private:
		GlViewportCommand(void) :
			OpenGlCommand(false, false, "glViewport")
		{
		}

		void set(GLint x, GLint y, GLsizei width, GLsizei height)
		{
			m_x = x;
			m_y = y;
			m_width = width;
			m_height = height;
		}

		GLint m_x;
		GLint m_y;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlBindTextureCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint texture)
		{
			auto ptr = std::shared_ptr<GlBindTextureCommand>(new GlBindTextureCommand);
			ptr->set(target, texture);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindTexture(m_target, m_texture);
		}
	private:
		GlBindTextureCommand(void) :
			OpenGlCommand(false, false, "glBindTexture")
		{
		}

		void set(GLenum target, GLuint texture)
		{
			m_target = target;
			m_texture = texture;
		}

		GLenum m_target;
		GLuint m_texture;
	};

	template <class pixelType>
	class GlTexImage2DCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
			GLint border, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
		{
			auto ptr = std::shared_ptr<GlTexImage2DCommand>(new GlTexImage2DCommand);
			ptr->set(target, level, internalformat, width, height, border, format, type, std::move(pixels));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexImage2D(m_target, m_level, m_internalformat, m_width, m_height, m_border, m_format, m_type,
				m_pixels.get());
		}
	private:
		GlTexImage2DCommand(void) :
			OpenGlCommand(false, false, "glTexImage2D")
		{
		}

		void set(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
			GLint border, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
		{
			m_target = target;
			m_level = level;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
			m_border = border;
			m_format = format;
			m_type = type;
			m_pixels = std::move(pixels);
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum pname, GLint param)
		{
			auto ptr = std::shared_ptr<GlTexParameteriCommand>(new GlTexParameteriCommand);
			ptr->set(target, pname, param);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexParameteri(m_target, m_pname, m_param);
		}
	private:
		GlTexParameteriCommand(void) :
			OpenGlCommand(false, false, "glTexParameteri")
		{
		}

		void set(GLenum target, GLenum pname, GLint param)
		{
			m_target = target;
			m_pname = pname;
			m_param = param;
		}

		GLenum m_target;
		GLenum m_pname;
		GLint m_param;
	};

	class GlGetIntegervCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum pname, GLint* data)
		{
			auto ptr = std::shared_ptr<GlGetIntegervCommand>(new GlGetIntegervCommand);
			ptr->set(pname, data);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetIntegerv(m_pname, m_data);
		}
	private:
		GlGetIntegervCommand(void) :
			OpenGlCommand(true, false, "glGetIntegerv")
		{
		}

		void set(GLenum pname, GLint* data)
		{
			m_pname = pname;
			m_data = data;
		}

		GLenum m_pname;
		GLint* m_data;
	};

	class GlGetStringCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum name, const GLubyte*& returnValue)
		{
			auto ptr = std::shared_ptr<GlGetStringCommand>(new GlGetStringCommand);
			ptr->set(name, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glGetString(m_name);
		}
	private:
		GlGetStringCommand(void) :
			OpenGlCommand(true, false, "glGetString")
		{
		}

		void set(GLenum name, const GLubyte*& returnValue)
		{
			m_name = name;
			m_returnValue = &returnValue;
		}

		GLenum m_name;
		const GLubyte** m_returnValue;
	};

	class GlReadPixelsCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
		{
			auto ptr = std::shared_ptr<GlReadPixelsCommand>(new GlReadPixelsCommand);
			ptr->set(x, y, width, height, format, type, pixels);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glReadPixels(m_x, m_y, m_width, m_height, m_format, m_type, m_pixels);
		}
	private:
		GlReadPixelsCommand(void) :
			OpenGlCommand(true, true, "glReadPixels")
		{
		}

		void set(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
		{
			m_x = x;
			m_y = y;
			m_width = width;
			m_height = height;
			m_format = format;
			m_type = type;
			m_pixels = pixels;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type)
		{
			auto ptr = std::shared_ptr<GlReadPixelsAsyncCommand>(new GlReadPixelsAsyncCommand);
			ptr->set(x, y, width, height, format, type);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glReadPixels(m_x, m_y, m_width, m_height, m_format, m_type, nullptr);
		}
	private:
		GlReadPixelsAsyncCommand(void) :
			OpenGlCommand(false, false, "GlReadPixelsAync")
		{
		}

		void set(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type)
		{
			m_x = x;
			m_y = y;
			m_width = width;
			m_height = height;
			m_format = format;
			m_type = type;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
		{
			auto ptr = std::shared_ptr<GlTexSubImage2DUnbufferedCommand>(new GlTexSubImage2DUnbufferedCommand);
			ptr->set(target, level, xoffset, yoffset, width, height, format, type, std::move(pixels));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexSubImage2D(m_target, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type, m_pixels.get());
		}
	private:
		GlTexSubImage2DUnbufferedCommand(void) :
			OpenGlCommand(false, false, "glTexSubImage2D")
		{
		}

		void set(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
		{
			m_target = target;
			m_level = level;
			m_xoffset = xoffset;
			m_yoffset = yoffset;
			m_width = width;
			m_height = height;
			m_format = format;
			m_type = type;
			m_pixels = std::move(pixels);
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLenum type, std::size_t offset)
		{
			auto ptr = std::shared_ptr<GlTexSubImage2DBufferedCommand>(new GlTexSubImage2DBufferedCommand);
			ptr->set(target, level, xoffset, yoffset, width, height, format, type, offset);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexSubImage2D(m_target, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type, (const GLvoid *)m_offset);
		}
	private:
		GlTexSubImage2DBufferedCommand(void) :
			OpenGlCommand(false, false, "glTexSubImage2D")
		{
		}

		void set(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLenum type, std::size_t offset)
		{
			m_target = target;
			m_level = level;
			m_xoffset = xoffset;
			m_yoffset = yoffset;
			m_width = width;
			m_height = height;
			m_format = format;
			m_type = type;
			m_offset = offset;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum mode, GLint first, GLsizei count)
		{
			auto ptr = std::shared_ptr<GlDrawArraysCommand>(new GlDrawArraysCommand);
			ptr->set(mode, first, count);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDrawArrays(m_mode, m_first, m_count);
		}
	private:
		GlDrawArraysCommand(void) :
			OpenGlCommand(false, false, "glDrawArrays")
		{
		}

		void set(GLenum mode, GLint first, GLsizei count)
		{
			m_mode = mode;
			m_first = first;
			m_count = count;
		}

		GLenum m_mode;
		GLint m_first;
		GLsizei m_count;
	};

	class GlVertexAttribPointerUnbufferedCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
			std::size_t offset)
		{
			auto ptr = std::shared_ptr<GlVertexAttribPointerUnbufferedCommand>(new GlVertexAttribPointerUnbufferedCommand);
			ptr->set(index, size, type, normalized, stride, offset);
			return ptr;
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
		GlVertexAttribPointerUnbufferedCommand(void) :
			OpenGlCommand(false, false, "glVertexAttribPointer")
		{
		}

		void set(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
			std::size_t offset)
		{
			m_index = index;
			m_size = size;
			m_type = type;
			m_normalized = normalized;
			m_stride = stride;
			m_offset = offset;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum mode, GLint first, GLsizei count, std::unique_ptr<std::vector<char>> data)
		{
			auto ptr = std::shared_ptr<GlDrawArraysUnbufferedCommand>(new GlDrawArraysUnbufferedCommand);
			ptr->set(mode, first, count, std::move(data));
			return ptr;
		}

		void commandToExecute(void) override
		{
			char* data = GlVertexAttribPointerUnbufferedCommand::getDrawingData();
			std::copy_n(m_data->data(), m_data->size(), data);
			g_glDrawArrays(m_mode, m_first, m_count);
		}
	private:
		GlDrawArraysUnbufferedCommand(void) :
			OpenGlCommand(false, false, "glDrawArrays")
		{
		}

		void set(GLenum mode, GLint first, GLsizei count, std::unique_ptr<std::vector<char>> data)
		{
			m_mode = mode;
			m_first = first;
			m_count = count;
			m_data = std::move(data);
		}

		GLenum m_mode;
		GLint m_first;
		GLsizei m_count;
		std::unique_ptr<std::vector<char>> m_data;
	};

	class GlGetErrorCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum& returnValue)
		{
			auto ptr = std::shared_ptr<GlGetErrorCommand>(new GlGetErrorCommand);
			ptr->set(returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glGetError();
		}
	private:
		GlGetErrorCommand(void) :
			OpenGlCommand(true, true, "glGetError")
		{
		}

		void set(GLenum& returnValue)
		{
			m_returnValue = &returnValue;
		}

		GLenum* m_returnValue;
	};

	template <class indiceType>
	class GlDrawElementsUnbufferedCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum mode, GLsizei count, GLenum type, std::unique_ptr<indiceType[]> indices,
			std::unique_ptr<std::vector<char>> data)
		{
			auto ptr = std::shared_ptr<GlDrawElementsUnbufferedCommand>(new GlDrawElementsUnbufferedCommand);
			ptr->set(mode, count, type, std::move(indices), std::move(data));
			return ptr;
		}

		void commandToExecute(void) override
		{
			char* data = GlVertexAttribPointerUnbufferedCommand::getDrawingData();
			std::copy_n(m_data->data(), m_data->size(), data);
			g_glDrawElements(m_mode, m_count, m_type, m_indices.get());
		}
	private:
		GlDrawElementsUnbufferedCommand(void) :
			OpenGlCommand(false, false, "glDrawElementsUnbuffered")
		{
		}

		void set(GLenum mode, GLsizei count, GLenum type, std::unique_ptr<indiceType[]> indices,
			std::unique_ptr<std::vector<char>> data)
		{
			m_mode = mode;
			m_count = count;
			m_type = type;
			m_indices = std::move(indices);
			m_data = std::move(data);
		}

		GLenum m_mode;
		GLsizei m_count;
		GLenum m_type;
		std::unique_ptr<indiceType[]> m_indices;
		std::unique_ptr<std::vector<char>> m_data;
	};

	class GlLineWidthCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLfloat width)
		{
			auto ptr = std::shared_ptr<GlLineWidthCommand>(new GlLineWidthCommand);
			ptr->set(width);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glLineWidth(m_width);
		}
	private:
		GlLineWidthCommand(void) :
			OpenGlCommand(false, false, "glLineWidth")
		{
		}

		void set(GLfloat width)
		{
			m_width = width;
		}

		GLfloat m_width;
	};

	class GlClearCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLbitfield mask)
		{
			auto ptr = std::shared_ptr<GlClearCommand>(new GlClearCommand);
			ptr->set(mask);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glClear(m_mask);
		}
	private:
		GlClearCommand(void) :
			OpenGlCommand(false, false, "glClear")
		{
		}

		void set(GLbitfield mask)
		{
			m_mask = mask;
		}

		GLbitfield m_mask;
	};

	class GlGetFloatvCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum pname, GLfloat* data)
		{
			auto ptr = std::shared_ptr<GlGetFloatvCommand>(new GlGetFloatvCommand);
			ptr->set(pname, data);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetFloatv(m_pname, m_data);
		}
	private:
		GlGetFloatvCommand(void) :
			OpenGlCommand(true, false, "glGetFloatv")
		{
		}

		void set(GLenum pname, GLfloat* data)
		{
			m_pname = pname;
			m_data = data;
		}

		GLenum m_pname;
		GLfloat* m_data;
	};

	class GlDeleteTexturesCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, std::unique_ptr<GLuint[]> textures)
		{
			auto ptr = std::shared_ptr<GlDeleteTexturesCommand>(new GlDeleteTexturesCommand);
			ptr->set(n, std::move(textures));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteTextures(m_n, m_textures.get());
		}
	private:
		GlDeleteTexturesCommand(void) :
			OpenGlCommand(false, false, "glDeleteTextures")
		{
		}

		void set(GLsizei n, std::unique_ptr<GLuint[]> textures)
		{
			m_n = n;
			m_textures = std::move(textures);
		}

		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_textures;
	};

	class GlGenTexturesCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* textures)
		{
			auto ptr = std::shared_ptr<GlGenTexturesCommand>(new GlGenTexturesCommand);
			ptr->set(n, textures);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGenTextures(m_n, m_textures);
		}
	private:
		GlGenTexturesCommand(void) :
			OpenGlCommand(true, false, "glGenTextures")
		{
		}

		void set(GLsizei n, GLuint* textures)
		{
			m_n = n;
			m_textures = textures;
		}

		GLsizei m_n;
		GLuint* m_textures;
	};

	class GlTexParameterfCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum pname, GLfloat param)
		{
			auto ptr = std::shared_ptr<GlTexParameterfCommand>(new GlTexParameterfCommand);
			ptr->set(target, pname, param);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexParameterf(m_target, m_pname, m_param);
		}
	private:
		GlTexParameterfCommand(void) :
			OpenGlCommand(false, false, "glTexParameterf")
		{
		}

		void set(GLenum target, GLenum pname, GLfloat param)
		{
			m_target = target;
			m_pname = pname;
			m_param = param;
		}

		GLenum m_target;
		GLenum m_pname;
		GLfloat m_param;
	};

	class GlActiveTextureCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum texture)
		{
			auto ptr = std::shared_ptr<GlActiveTextureCommand>(new GlActiveTextureCommand);
			ptr->set(texture);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glActiveTexture(m_texture);
		}
	private:
		GlActiveTextureCommand(void) :
			OpenGlCommand(false, false, "glActiveTexture")
		{
		}

		void set(GLenum texture)
		{
			m_texture = texture;
		}

		GLenum m_texture;
	};

	class GlBlendColorCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
		{
			auto ptr = std::shared_ptr<GlBlendColorCommand>(new GlBlendColorCommand);
			ptr->set(red, green, blue, alpha);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBlendColor(m_red, m_green, m_blue, m_alpha);
		}
	private:
		GlBlendColorCommand(void) :
			OpenGlCommand(false, false, "glBlendColor")
		{
		}

		void set(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
		{
			m_red = red;
			m_green = green;
			m_blue = blue;
			m_alpha = alpha;
		}

		GLfloat m_red;
		GLfloat m_green;
		GLfloat m_blue;
		GLfloat m_alpha;
	};

	class GlReadBufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum src)
		{
			auto ptr = std::shared_ptr<GlReadBufferCommand>(new GlReadBufferCommand);
			ptr->set(src);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glReadBuffer(m_src);
		}
	private:
		GlReadBufferCommand(void) :
			OpenGlCommand(false, false, "glReadBuffer")
		{
		}

		void set(GLenum src)
		{
			m_src = src;
		}

		GLenum m_src;
	};

	class GlCreateShaderCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum type, GLuint& returnValue)
		{
			auto ptr = std::shared_ptr<GlCreateShaderCommand>(new GlCreateShaderCommand);
			ptr->set(type, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glCreateShader(m_type);
		}
	private:
		GlCreateShaderCommand(void) :
			OpenGlCommand(true, true, "glCreateShader")
		{
		}

		void set(GLenum type, GLuint& returnValue)
		{
			m_type = type;
			m_returnValue = &returnValue;
		}

		GLenum m_type;
		GLuint* m_returnValue;
	};

	class GlCompileShaderCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint shader)
		{
			auto ptr = std::shared_ptr<GlCompileShaderCommand>(new GlCompileShaderCommand);
			ptr->set(shader);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glCompileShader(m_shader);
		}
	private:
		GlCompileShaderCommand(void) :
			OpenGlCommand(false, false, "glCompileShader")
		{
		}

		void set(GLuint shader)
		{
			m_shader = shader;
		}

		GLuint m_shader;
	};

	class GlShaderSourceCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint shader, const std::string& string)
		{
			auto ptr = std::shared_ptr<GlShaderSourceCommand>(new GlShaderSourceCommand);
			ptr->set(shader, string);
			return ptr;
		}

		void commandToExecute(void) override
		{
			const GLchar* strShaderData = m_string.data();
			g_glShaderSource(m_shader, 1, &strShaderData, nullptr);
		}
	private:
		GlShaderSourceCommand(void) :
			OpenGlCommand(false, false, "glShaderSource")
		{
		}

		void set(GLuint shader, const std::string& string)
		{
			m_shader = shader;
			m_string = std::move(string);
		}

		GLuint m_shader;
		std::string m_string;
	};

	class GlCreateProgramCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint& returnValue)
		{
			auto ptr = std::shared_ptr<GlCreateProgramCommand>(new GlCreateProgramCommand);
			ptr->set(returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glCreateProgram();
		}
	private:
		GlCreateProgramCommand(void) :
			OpenGlCommand(true, true, "glCreateProgram")
		{
		}

		void set(GLuint& returnValue)
		{
			m_returnValue = &returnValue;
		}

		GLuint* m_returnValue;
	};

	class GlAttachShaderCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLuint shader)
		{
			auto ptr = std::shared_ptr<GlAttachShaderCommand>(new GlAttachShaderCommand);
			ptr->set(program, shader);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glAttachShader(m_program, m_shader);
		}
	private:
		GlAttachShaderCommand(void) :
			OpenGlCommand(false, false, "glAttachShader")
		{
		}

		void set(GLuint program, GLuint shader)
		{
			m_program = program;
			m_shader = shader;
		}

		GLuint m_program;
		GLuint m_shader;
	};

	class GlLinkProgramCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program)
		{
			auto ptr = std::shared_ptr<GlLinkProgramCommand>(new GlLinkProgramCommand);
			ptr->set(program);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glLinkProgram(m_program);
		}
	private:
		GlLinkProgramCommand(void) :
			OpenGlCommand(false, false, "glLinkProgram")
		{
		}

		void set(GLuint program)
		{
			m_program = program;
		}

		GLuint m_program;
	};

	class GlUseProgramCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program)
		{
			auto ptr = std::shared_ptr<GlUseProgramCommand>(new GlUseProgramCommand);
			ptr->set(program);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUseProgram(m_program);
		}
	private:
		GlUseProgramCommand(void) :
			OpenGlCommand(false, false, "glUseProgram")
		{
		}

		void set(GLuint program)
		{
			m_program = program;
		}

		GLuint m_program;
	};

	class GlGetUniformLocationCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, const GLchar* name, GLint& returnValue)
		{
			auto ptr = std::shared_ptr<GlGetUniformLocationCommand>(new GlGetUniformLocationCommand);
			ptr->set(program, name, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glGetUniformLocation(m_program, m_name);
		}
	private:
		GlGetUniformLocationCommand(void) :
			OpenGlCommand(true, true, "glGetUniformLocation")
		{
		}

		void set(GLuint program, const GLchar* name, GLint& returnValue)
		{
			m_program = program;
			m_name = name;
			m_returnValue = &returnValue;
		}

		GLint* m_returnValue;
		GLuint m_program;
		const GLchar* m_name;
	};

	class GlUniform1iCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLint v0)
		{
			auto ptr = std::shared_ptr<GlUniform1iCommand>(new GlUniform1iCommand);
			ptr->set(location, v0);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform1i(m_location, m_v0);
		}
	private:
		GlUniform1iCommand(void) :
			OpenGlCommand(false, false, "glUniform1i")
		{
		}

		void set(GLint location, GLint v0)
		{
			m_location = location;
			m_v0 = v0;
		}

		GLint m_location;
		GLint m_v0;
	};

	class GlUniform1fCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLfloat v0)
		{
			auto ptr = std::shared_ptr<GlUniform1fCommand>(new GlUniform1fCommand);
			ptr->set(location, v0);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform1f(m_location, m_v0);
		}
	private:
		GlUniform1fCommand(void) :
			OpenGlCommand(false, false, "glUniform1f")
		{
		}

		void set(GLint location, GLfloat v0)
		{
			m_location = location;
			m_v0 = v0;
		}

		GLint m_location;
		GLfloat m_v0;
	};

	class GlUniform2fCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLfloat v0, GLfloat v1)
		{
			auto ptr = std::shared_ptr<GlUniform2fCommand>(new GlUniform2fCommand);
			ptr->set(location, v0, v1);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform2f(m_location, m_v0, m_v1);
		}
	private:
		GlUniform2fCommand(void) :
			OpenGlCommand(false, false, "glUniform2f")
		{
		}

		void set(GLint location, GLfloat v0, GLfloat v1)
		{
			m_location = location;
			m_v0 = v0;
			m_v1 = v1;
		}

		GLint m_location;
		GLfloat m_v0;
		GLfloat m_v1;
	};

	class GlUniform2iCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLint v0, GLint v1)
		{
			auto ptr = std::shared_ptr<GlUniform2iCommand>(new GlUniform2iCommand);
			ptr->set(location, v0, v1);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform2i(m_location, m_v0, m_v1);
		}
	private:
		GlUniform2iCommand(void) :
			OpenGlCommand(false, false, "glUniform2i")
		{
		}

		void set(GLint location, GLint v0, GLint v1)
		{
			m_location = location;
			m_v0 = v0;
			m_v1 = v1;
		}

		GLint m_location;
		GLint m_v0;
		GLint m_v1;
	};

	class GlUniform4iCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
		{
			auto ptr = std::shared_ptr<GlUniform4iCommand>(new GlUniform4iCommand);
			ptr->set(location, v0, v1, v2, v3);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform4i(m_location, m_v0, m_v1, m_v2, m_v3);
		}
	private:
		GlUniform4iCommand(void) :
			OpenGlCommand(false, false, "glUniform4i")
		{
		}

		void set(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
		{
			m_location = location;
			m_v0 = v0;
			m_v1 = v1;
			m_v2 = v2;
			m_v3 = v3;
		}

		GLint m_location;
		GLint m_v0;
		GLint m_v1;
		GLint m_v2;
		GLint m_v3;
	};

	class GlUniform4fCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
		{
			auto ptr = std::shared_ptr<GlUniform4fCommand>(new GlUniform4fCommand);
			ptr->set(location, v0, v1, v2, v3);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform4f(m_location, m_v0, m_v1, m_v2, m_v3);
		}
	private:
		GlUniform4fCommand(void) :
			OpenGlCommand(false, false, "glUniform4f")
		{
		}

		void set(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
		{
			m_location = location;
			m_v0 = v0;
			m_v1 = v1;
			m_v2 = v2;
			m_v3 = v3;
		}

		GLint m_location;
		GLfloat m_v0;
		GLfloat m_v1;
		GLfloat m_v2;
		GLfloat m_v3;
	};

	class GlUniform3fvCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value)
		{
			auto ptr = std::shared_ptr<GlUniform3fvCommand>(new GlUniform3fvCommand);
			ptr->set(location, count, std::move(value));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform3fv(m_location, m_count, m_value.get());
		}
	private:
		GlUniform3fvCommand(void) :
			OpenGlCommand(false, false, "glUniform3fv")
		{
		}

		void set(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value)
		{
			m_location = location;
			m_count = count;
			m_value = std::move(value);
		}

		GLint m_location;
		GLsizei m_count;
		std::unique_ptr<GLfloat[]> m_value;
	};

	class GlUniform4fvCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value)
		{
			auto ptr = std::shared_ptr<GlUniform4fvCommand>(new GlUniform4fvCommand);
			ptr->set(location, count, std::move(value));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniform4fv(m_location, m_count, m_value.get());
		}
	private:
		GlUniform4fvCommand(void) :
			OpenGlCommand(false, false, "glUniform4fv")
		{
		}

		void set(GLint location, GLsizei count, std::unique_ptr<GLfloat[]> value)
		{
			m_location = location;
			m_count = count;
			m_value = std::move(value);
		}

		GLint m_location;
		GLsizei m_count;
		std::unique_ptr<GLfloat[]> m_value;
	};

	class GlDetachShaderCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLuint shader)
		{
			auto ptr = std::shared_ptr<GlDetachShaderCommand>(new GlDetachShaderCommand);
			ptr->set(program, shader);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDetachShader(m_program, m_shader);
		}
	private:
		GlDetachShaderCommand(void) :
			OpenGlCommand(false, false, "glDetachShader")
		{
		}

		void set(GLuint program, GLuint shader)
		{
			m_program = program;
			m_shader = shader;
		}

		GLuint m_program;
		GLuint m_shader;
	};

	class GlDeleteShaderCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint shader)
		{
			auto ptr = std::shared_ptr<GlDeleteShaderCommand>(new GlDeleteShaderCommand);
			ptr->set(shader);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteShader(m_shader);
		}
	private:
		GlDeleteShaderCommand(void) :
			OpenGlCommand(false, false, "glDeleteShader")
		{
		}

		void set(GLuint shader)
		{
			m_shader = shader;
		}

		GLuint m_shader;
	};

	class GlDeleteProgramCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program)
		{
			auto ptr = std::shared_ptr<GlDeleteProgramCommand>(new GlDeleteProgramCommand);
			ptr->set(program);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteProgram(m_program);
		}
	private:
		GlDeleteProgramCommand(void) :
			OpenGlCommand(false, false, "glDeleteProgram")
		{
		}

		void set(GLuint program)
		{
			m_program = program;
		}

		GLuint m_program;
	};

	class GlGetProgramInfoLogCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
		{
			auto ptr = std::shared_ptr<GlGetProgramInfoLogCommand>(new GlGetProgramInfoLogCommand);
			ptr->set(program, bufSize, length, infoLog);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetProgramInfoLog(m_program, m_bufSize, m_length, m_infoLog);
		}
	private:
		GlGetProgramInfoLogCommand(void) :
			OpenGlCommand(true, true, "glGetProgramInfoLog")
		{
		}

		void set(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
		{
			m_program = program;
			m_bufSize = bufSize;
			m_length = length;
			m_infoLog = infoLog;
		}

		GLuint m_program;
		GLsizei m_bufSize;
		GLsizei* m_length;
		GLchar* m_infoLog;
	};

	class GlGetShaderInfoLogCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
		{
			auto ptr = std::shared_ptr<GlGetShaderInfoLogCommand>(new GlGetShaderInfoLogCommand);
			ptr->set(shader, bufSize, length, infoLog);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetShaderInfoLog(m_shader, m_bufSize, m_length, m_infoLog);
		}
	private:
		GlGetShaderInfoLogCommand(void) :
			OpenGlCommand(true, true, "glGetShaderInfoLog")
		{
		}

		void set(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
		{
			m_shader = shader;
			m_bufSize = bufSize;
			m_length = length;
			m_infoLog = infoLog;
		}

		GLuint m_shader;
		GLsizei m_bufSize;
		GLsizei* m_length;
		GLchar* m_infoLog;
	};

	class GlGetShaderivCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint shader, GLenum pname, GLint* params)
		{
			auto ptr = std::shared_ptr<GlGetShaderivCommand>(new GlGetShaderivCommand);
			ptr->set(shader, pname, params);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetShaderiv(m_shader, m_pname, m_params);
		}
	private:
		GlGetShaderivCommand(void) :
			OpenGlCommand(true, true, "glGetShaderiv")
		{
		}

		void set(GLuint shader, GLenum pname, GLint* params)
		{
			m_shader = shader;
			m_pname = pname;
			m_params = params;
		}

		GLuint m_shader;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlGetProgramivCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLenum pname, GLint* params)
		{
			auto ptr = std::shared_ptr<GlGetProgramivCommand>(new GlGetProgramivCommand);
			ptr->set(program, pname, params);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetProgramiv(m_program, m_pname, m_params);
		}
	private:
		GlGetProgramivCommand(void) :
			OpenGlCommand(true, true, "glGetProgramiv")
		{
		}

		void set(GLuint program, GLenum pname, GLint* params)
		{
			m_program = program;
			m_pname = pname;
			m_params = params;
		}

		GLuint m_program;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlEnableVertexAttribArrayCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index)
		{
			auto ptr = std::shared_ptr<GlEnableVertexAttribArrayCommand>(new GlEnableVertexAttribArrayCommand);
			ptr->set(index);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glEnableVertexAttribArray(m_index);
		}
	private:
		GlEnableVertexAttribArrayCommand(void) :
			OpenGlCommand(false, false, "glEnableVertexAttribArray")
		{
		}

		void set(GLuint index)
		{
			m_index = index;
		}

		GLuint m_index;
	};

	class GlDisableVertexAttribArrayCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index)
		{
			auto ptr = std::shared_ptr<GlDisableVertexAttribArrayCommand>(new GlDisableVertexAttribArrayCommand);
			ptr->set(index);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDisableVertexAttribArray(m_index);
		}
	private:
		GlDisableVertexAttribArrayCommand(void) :
			OpenGlCommand(false, false, "glDisableVertexAttribArray")
		{
		}

		void set(GLuint index)
		{
			m_index = index;
		}

		GLuint m_index;
	};

	class GlVertexAttribPointerBufferedCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
			std::size_t offset)
		{
			auto ptr = std::shared_ptr<GlVertexAttribPointerBufferedCommand>(new GlVertexAttribPointerBufferedCommand);
			ptr->set(index, size, type, normalized, stride, offset);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glVertexAttribPointer(m_index, m_size, m_type, m_normalized, m_stride, (const GLvoid *)(m_offset));
		}
	private:
		GlVertexAttribPointerBufferedCommand(void) :
			OpenGlCommand(false, false, "glVertexAttribPointer")
		{
		}

		void set(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
			std::size_t offset)
		{
			m_index = index;
			m_size = size;
			m_type = type;
			m_normalized = normalized;
			m_stride = stride;
			m_offset = offset;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLuint index, const std::string& name)
		{
			auto ptr = std::shared_ptr<GlBindAttribLocationCommand>(new GlBindAttribLocationCommand);
			ptr->set(program, index, name);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindAttribLocation(m_program, m_index, m_name.data());
		}
	private:
		GlBindAttribLocationCommand(void) :
			OpenGlCommand(false, false, "glBindAttribLocation")
		{
		}

		void set(GLuint program, GLuint index, const std::string& name)
		{
			m_program = program;
			m_index = index;
			m_name = std::move(name);
		}

		GLuint m_program;
		GLuint m_index;
		std::string m_name;
	};

	class GlVertexAttrib1fCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index, GLfloat x)
		{
			auto ptr = std::shared_ptr<GlVertexAttrib1fCommand>(new GlVertexAttrib1fCommand);
			ptr->set(index, x);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glVertexAttrib1f(m_index, m_x);
		}
	private:
		GlVertexAttrib1fCommand(void) :
			OpenGlCommand(false, false, "glVertexAttrib1f")
		{
		}

		void set(GLuint index, GLfloat x)
		{
			m_index = index;
			m_x = x;
		}

		GLuint m_index;
		GLfloat m_x;
	};

	class GlVertexAttrib4fCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
		{
			auto ptr = std::shared_ptr<GlVertexAttrib4fCommand>(new GlVertexAttrib4fCommand);
			ptr->set(index, x, y, z, w);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glVertexAttrib4f(m_index, m_x, m_y, m_z, m_w);
		}
	private:
		GlVertexAttrib4fCommand(void) :
			OpenGlCommand(false, false, "glVertexAttrib4f")
		{
		}

		void set(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
		{
			m_index = index;
			m_x = x;
			m_y = y;
			m_z = z;
			m_w = w;
		}

		GLuint m_index;
		GLfloat m_x;
		GLfloat m_y;
		GLfloat m_z;
		GLfloat m_w;
	};

	class GlVertexAttrib4fvCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint index, std::unique_ptr<GLfloat[]> v)
		{
			auto ptr = std::shared_ptr<GlVertexAttrib4fvCommand>(new GlVertexAttrib4fvCommand);
			ptr->set(index, std::move(v));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glVertexAttrib4fv(m_index, m_v.get());
		}
	private:
		GlVertexAttrib4fvCommand(void) :
			OpenGlCommand(false, false, "glVertexAttrib4fv")
		{
		}

		void set(GLuint index, std::unique_ptr<GLfloat[]> v)
		{
			m_index = index;
			m_v = std::move(v);
		}

		GLuint m_index;
		std::unique_ptr<GLfloat[]> m_v;
	};

	class GlDepthRangefCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLfloat n, GLfloat f)
		{
			auto ptr = std::shared_ptr<GlDepthRangefCommand>(new GlDepthRangefCommand);
			ptr->set(n, f);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDepthRangef(m_n,m_f);
		}
	private:
		GlDepthRangefCommand(void) :
			OpenGlCommand(false, false, "glDepthRangef")
		{
		}

		void set(GLfloat n, GLfloat f)
		{
			m_n = n;
			m_f = f;
		}

		GLfloat m_n;
		GLfloat m_f;
	};

	class GlClearDepthfCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLfloat d)
		{
			auto ptr = std::shared_ptr<GlClearDepthfCommand>(new GlClearDepthfCommand);
			ptr->set(d);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glClearDepthf(m_d);
		}
	private:
		GlClearDepthfCommand(void) :
			OpenGlCommand(false, false, "glClearDepthf")
		{
		}

		void set(GLfloat d)
		{
			m_d = d;
		}

		GLfloat m_d;
	};

	class GlDrawBuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, std::unique_ptr<GLenum[]> bufs)
		{
			auto ptr = std::shared_ptr<GlDrawBuffersCommand>(new GlDrawBuffersCommand);
			ptr->set(n, std::move(bufs));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDrawBuffers(m_n, m_bufs.get());
		}
	private:
		GlDrawBuffersCommand(void) :
			OpenGlCommand(false, false, "glDrawBuffers")
		{
		}

		void set(GLsizei n, std::unique_ptr<GLenum[]> bufs)
		{
			m_n = n;
			m_bufs = std::move(bufs);
		}

		GLsizei m_n;
		std::unique_ptr<GLenum[]> m_bufs;
	};

	class GlGenFramebuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* framebuffers)
		{
			auto ptr = std::shared_ptr<GlGenFramebuffersCommand>(new GlGenFramebuffersCommand);
			ptr->set(n, framebuffers);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGenFramebuffers(m_n, m_framebuffers);
		}
	private:
		GlGenFramebuffersCommand(void) :
			OpenGlCommand(true, false, "glGenFramebuffers")
		{
		}

		void set(GLsizei n, GLuint* framebuffers)
		{
			m_n = n;
			m_framebuffers = framebuffers;
		}

		GLsizei m_n;
		GLuint* m_framebuffers;
	};

	class GlBindFramebufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint framebuffer)
		{
			auto ptr = std::shared_ptr<GlBindFramebufferCommand>(new GlBindFramebufferCommand);
			ptr->set(target, framebuffer);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindFramebuffer(m_target, m_framebuffer);
		}
	private:
		GlBindFramebufferCommand(void) :
			OpenGlCommand(false, false, "glBindFramebuffer")
		{
		}

		void set(GLenum target, GLuint framebuffer)
		{
			m_target = target;
			m_framebuffer = framebuffer;
		}

		GLenum m_target;
		GLuint m_framebuffer;
	};

	class GlDeleteFramebuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, std::unique_ptr<GLuint[]> framebuffers)
		{
			auto ptr = std::shared_ptr<GlDeleteFramebuffersCommand>(new GlDeleteFramebuffersCommand);
			ptr->set(n, std::move(framebuffers));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteFramebuffers(m_n, m_framebuffers.get());
		}
	private:
		GlDeleteFramebuffersCommand(void) :
			OpenGlCommand(false, false, "glDeleteFramebuffers")
		{
		}

		void set(GLsizei n, std::unique_ptr<GLuint[]> framebuffers)
		{
			m_n = n;
			m_framebuffers = std::move(framebuffers);
		}

		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_framebuffers;
	};

	class GlFramebufferTexture2DCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
		{
			auto ptr = std::shared_ptr<GlFramebufferTexture2DCommand>(new GlFramebufferTexture2DCommand);
			ptr->set(target, attachment, textarget, texture, level);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glFramebufferTexture2D(m_target, m_attachment, m_textarget, m_texture, m_level);
		}
	private:
		GlFramebufferTexture2DCommand(void) :
			OpenGlCommand(false, false, "glFramebufferTexture2D")
		{
		}

		void set(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
		{
			m_target = target;
			m_attachment = attachment;
			m_textarget = textarget;
			m_texture = texture;
			m_level = level;
		}

		GLenum m_target;
		GLenum m_attachment;
		GLenum m_textarget;
		GLuint m_texture;
		GLint m_level;
	};

	class GlTexImage2DMultisampleCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
			GLsizei height, GLboolean fixedsamplelocations)
		{
			auto ptr = std::shared_ptr<GlTexImage2DMultisampleCommand>(new GlTexImage2DMultisampleCommand);
			ptr->set(target, samples, internalformat, width, height, fixedsamplelocations);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexImage2DMultisample(m_target, m_samples, m_internalformat, m_width, m_height, m_fixedsamplelocations);
		}
	private:
		GlTexImage2DMultisampleCommand(void) :
			OpenGlCommand(false, false, "glTexImage2DMultisample")
		{
		}

		void set(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
			GLsizei height, GLboolean fixedsamplelocations)
		{
			m_target = target;
			m_samples = samples;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
			m_fixedsamplelocations = fixedsamplelocations;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
			GLsizei height, GLboolean fixedsamplelocations)
		{
			auto ptr = std::shared_ptr<GlTexStorage2DMultisampleCommand>(new GlTexStorage2DMultisampleCommand);
			ptr->set(target, samples, internalformat, width, height, fixedsamplelocations);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexStorage2DMultisample(m_target, m_samples, m_internalformat, m_width, m_height, m_fixedsamplelocations);
		}
	private:
		GlTexStorage2DMultisampleCommand(void) :
			OpenGlCommand(false, false, "glTexStorage2DMultisample")
		{
		}

		void set(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width,
			GLsizei height, GLboolean fixedsamplelocations)
		{
			m_target = target;
			m_samples = samples;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
			m_fixedsamplelocations = fixedsamplelocations;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* renderbuffers)
		{
			auto ptr = std::shared_ptr<GlGenRenderbuffersCommand>(new GlGenRenderbuffersCommand);
			ptr->set(n, renderbuffers);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGenRenderbuffers(m_n, m_renderbuffers);
		}
	private:
		GlGenRenderbuffersCommand(void) :
			OpenGlCommand(true, false, "glGenRenderbuffers")
		{
		}

		void set(GLsizei n, GLuint* renderbuffers)
		{
			m_n = n;
			m_renderbuffers = renderbuffers;
		}

		GLsizei m_n;
		GLuint* m_renderbuffers;
	};

	class GlBindRenderbufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint renderbuffer)
		{
			auto ptr = std::shared_ptr<GlBindRenderbufferCommand>(new GlBindRenderbufferCommand);
			ptr->set(target, renderbuffer);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindRenderbuffer(m_target, m_renderbuffer);
		}
	private:
		GlBindRenderbufferCommand(void) :
			OpenGlCommand(false, false, "glBindRenderbuffer")
		{
		}

		void set(GLenum target, GLuint renderbuffer)
		{
			m_target = target;
			m_renderbuffer = renderbuffer;
		}

		GLenum m_target;
		GLuint m_renderbuffer;
	};

	class GlRenderbufferStorageCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
		{
			auto ptr = std::shared_ptr<GlRenderbufferStorageCommand>(new GlRenderbufferStorageCommand);
			ptr->set(target, internalformat, width, height);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glRenderbufferStorage(m_target, m_internalformat, m_width, m_height);
		}
	private:
		GlRenderbufferStorageCommand(void) :
			OpenGlCommand(false, false, "glRenderbufferStorage")
		{
		}

		void set(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
		{
			m_target = target;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
		}

		GLenum m_target;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlDeleteRenderbuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, std::unique_ptr<GLuint[]> renderbuffers)
		{
			auto ptr = std::shared_ptr<GlDeleteRenderbuffersCommand>(new GlDeleteRenderbuffersCommand);
			ptr->set(n, std::move(renderbuffers));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteRenderbuffers(m_n, m_renderbuffers.get());
		}
	private:
		GlDeleteRenderbuffersCommand(void) :
			OpenGlCommand(false, false, "glDeleteRenderbuffers")
		{
		}

		void set(GLsizei n, std::unique_ptr<GLuint[]> renderbuffers)
		{
			m_n = n;
			m_renderbuffers = std::move(renderbuffers);
		}

		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_renderbuffers;
	};

	class GlFramebufferRenderbufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
		{
			auto ptr = std::shared_ptr<GlFramebufferRenderbufferCommand>(new GlFramebufferRenderbufferCommand);
			ptr->set(target, attachment, renderbuffertarget, renderbuffer);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glFramebufferRenderbuffer(m_target, m_attachment, m_renderbuffertarget, m_renderbuffer);
		}
	private:
		GlFramebufferRenderbufferCommand(void) :
			OpenGlCommand(false, false, "glFramebufferRenderbuffer")
		{
		}

		void set(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
		{
			m_target = target;
			m_attachment = attachment;
			m_renderbuffertarget = renderbuffertarget;
			m_renderbuffer = renderbuffer;
		}

		GLenum m_target;
		GLenum m_attachment;
		GLenum m_renderbuffertarget;
		GLuint m_renderbuffer;
	};

	class GlCheckFramebufferStatusCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum& returnValue)
		{
			auto ptr = std::shared_ptr<GlCheckFramebufferStatusCommand>(new GlCheckFramebufferStatusCommand);
			ptr->set(target, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glCheckFramebufferStatus(m_target);
		}
	private:
		GlCheckFramebufferStatusCommand(void) :
			OpenGlCommand(true, true, "glCheckFramebufferStatus")
		{
		}

		void set(GLenum target, GLenum& returnValue)
		{
			m_target = target;
			m_returnValue = &returnValue;
		}

		GLenum m_target;
		GLenum* m_returnValue;
	};

	class GlBlitFramebufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0,
			GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
		{
			auto ptr = std::shared_ptr<GlBlitFramebufferCommand>(new GlBlitFramebufferCommand);
			ptr->set(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBlitFramebuffer(m_srcX0, m_srcY0, m_srcX1, m_srcY1, m_dstX0, m_dstY0, m_dstX1, m_dstY1, m_mask,
				m_filter);
		}
	private:
		GlBlitFramebufferCommand(void) :
			OpenGlCommand(false, false, "glBlitFramebuffer")
		{
		}

		void set(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0,
			GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
		{
			m_srcX0 = srcX0;
			m_srcY0 = srcY0;
			m_srcX1 = srcX1;
			m_srcY1 = srcY1;
			m_dstX0 = dstX0;
			m_dstY0 = dstY0;
			m_dstX1 = dstX1;
			m_dstY1 = dstY1;
			m_mask = mask;
			m_filter = filter;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* arrays)
		{
			auto ptr = std::shared_ptr<GlGenVertexArraysCommand>(new GlGenVertexArraysCommand);
			ptr->set(n, arrays);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGenVertexArrays(m_n, m_arrays);
		}
	private:
		GlGenVertexArraysCommand(void) :
			OpenGlCommand(true, false, "glGenVertexArrays")
		{
		}

		void set(GLsizei n, GLuint* arrays)
		{
			m_n = n;
			m_arrays = arrays;
		}

		GLsizei m_n;
		GLuint* m_arrays;
	};

	class GlBindVertexArrayCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint array)
		{
			auto ptr = std::shared_ptr<GlBindVertexArrayCommand>(new GlBindVertexArrayCommand);
			ptr->set(array);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindVertexArray(m_array);
		}
	private:
		GlBindVertexArrayCommand(void) :
			OpenGlCommand(false, false, "glBindVertexArray")
		{
		}

		void set(GLuint array)
		{
			m_array = array;
		}

		GLuint m_array;
	};

	class GlDeleteVertexArraysCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, std::unique_ptr<GLuint[]> arrays)
		{
			auto ptr = std::shared_ptr<GlDeleteVertexArraysCommand>(new GlDeleteVertexArraysCommand);
			ptr->set(n, std::move(arrays));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteVertexArrays(m_n, m_arrays.get());
		}
	private:
		GlDeleteVertexArraysCommand(void) :
			OpenGlCommand(false, false, "glDeleteVertexArrays")
		{
		}

		void set(GLsizei n, std::unique_ptr<GLuint[]> arrays)
		{
			m_n = n;
			m_arrays = std::move(arrays);
		}

		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_arrays;
	};

	class GlGenBuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* buffers)
		{
			auto ptr = std::shared_ptr<GlGenBuffersCommand>(new GlGenBuffersCommand);
			ptr->set(n, buffers);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGenBuffers(m_n, m_buffers);
		}
	private:
		GlGenBuffersCommand(void) :
			OpenGlCommand(true, false, "glGenBuffers")
		{
		}

		void set(GLsizei n, GLuint* buffers)
		{
			m_n = n;
			m_buffers = buffers;
		}

		GLsizei m_n;
		GLuint* m_buffers;
	};

	class GlBindBufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint buffer)
		{
			auto ptr = std::shared_ptr<GlBindBufferCommand>(new GlBindBufferCommand);
			ptr->set(target, buffer);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindBuffer(m_target, m_buffer);
		}
	private:
		GlBindBufferCommand(void) :
			OpenGlCommand(false, false, "glBindBuffer")
		{
		}

		void set(GLenum target, GLuint buffer)
		{
			m_target = target;
			m_buffer = buffer;
		}

		GLenum m_target;
		GLuint m_buffer;
	};

	template <class dataType>
	class GlBufferDataCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLenum usage)
		{
			auto ptr = std::shared_ptr<GlBufferDataCommand>(new GlBufferDataCommand);
			ptr->set(target, size, std::move(data), usage);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBufferData(m_target, m_size, m_data.get(), m_usage);
		}
	private:
		GlBufferDataCommand(void) :
			OpenGlCommand(false, false, "glBufferData")
		{
		}

		void set(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLenum usage)
		{
			m_target = target;
			m_size = size;
			m_data = std::move(data);
			m_usage = usage;
		}

		GLenum m_target;
		GLsizeiptr m_size;
		std::unique_ptr<dataType[]> m_data;
		GLenum m_usage;
	};

	class GlMapBufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLenum access)
		{
			auto ptr = std::shared_ptr<GlMapBufferCommand>(new GlMapBufferCommand);
			ptr->set(target, access);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glMapBuffer(m_target, m_access);
		}
	private:
		GlMapBufferCommand(void) :
			OpenGlCommand(false, false, "glMapBuffer")
		{
		}

		void set(GLenum target, GLenum access)
		{
			m_target = target;
			m_access = access;
		}

		GLenum m_target;
		GLenum m_access;
	};

	class GlMapBufferRangeCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access,
			GLubyte*& returnValue)
		{
			auto ptr = std::shared_ptr<GlMapBufferRangeCommand>(new GlMapBufferRangeCommand);
			ptr->set(target, offset, length, access, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = reinterpret_cast<GLubyte*>(g_glMapBufferRange(m_target, m_offset, m_length, m_access));
		}
	private:
		GlMapBufferRangeCommand(void) :
			OpenGlCommand(true, true, "glMapBufferRange")
		{
		}

		void set(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access,
			GLubyte*& returnValue)
		{
			m_target = target;
			m_offset = offset;
			m_length = length;
			m_access = access;
			m_returnValue = &returnValue;
		}

		GLenum m_target;
		GLintptr m_offset;
		GLsizeiptr m_length;
		GLbitfield m_access;
		GLubyte** m_returnValue;
	};

	class GlMapBufferRangeWriteAsyncCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint buffer, GLintptr offset, u32 length,
			GLbitfield access, std::unique_ptr<u8[]> data)
		{
			auto ptr = std::shared_ptr<GlMapBufferRangeWriteAsyncCommand>(new GlMapBufferRangeWriteAsyncCommand);
			ptr->set(target, buffer, offset, length, access, std::move(data));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindBuffer(m_target, m_buffer);
			void* buffer_pointer = g_glMapBufferRange(m_target, m_offset, m_length, m_access);
			memcpy(buffer_pointer, m_data.get(), m_length);
			g_glUnmapBuffer(m_target);
		}
	private:
		GlMapBufferRangeWriteAsyncCommand(void) :
			OpenGlCommand(false, false, "GlMapBufferRangeWriteAsyncCommand")
		{
		}

		void set(GLenum target, GLuint buffer, GLintptr offset, u32 length,
			GLbitfield access, std::unique_ptr<u8[]> data)
		{
			m_target = target;
			m_buffer = buffer;
			m_offset = offset;
			m_length = length;
			m_access = access;
			m_data = std::move(data);
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint buffer, GLintptr offset, u32 length,
			GLbitfield access)
		{
			auto ptr = std::shared_ptr<GlMapBufferRangeReadAsyncCommand>(new GlMapBufferRangeReadAsyncCommand);
			ptr->set(target, buffer, offset, length, access);
			return ptr;
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
		GlMapBufferRangeReadAsyncCommand(void) :
			OpenGlCommand(false, false, "GlMapBufferRangeReadAsyncCommand")
		{
		}

		void set(GLenum target, GLuint buffer, GLintptr offset, u32 length,
			GLbitfield access)
		{
			m_target = target;
			m_buffer = buffer;
			m_offset = offset;
			m_length = length;
			m_access = access;
		}
	
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
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLboolean& returnValue)
		{
			auto ptr = std::shared_ptr<GlUnmapBufferCommand>(new GlUnmapBufferCommand);
			ptr->set(target, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glUnmapBuffer(m_target);
		}
	private:
		GlUnmapBufferCommand(void) :
			OpenGlCommand(true, true, "glUnmapBuffer")
		{
		}

		void set(GLenum target, GLboolean& returnValue)
		{
			m_target = target;
			m_returnValue = &returnValue;
		}

		GLenum m_target;
		GLboolean* m_returnValue;
	};

	class GlUnmapBufferAsyncCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target)
		{
			auto ptr = std::shared_ptr<GlUnmapBufferAsyncCommand>(new GlUnmapBufferAsyncCommand);
			ptr->set(target);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUnmapBuffer(m_target);
		}
	private:
		GlUnmapBufferAsyncCommand(void) :
			OpenGlCommand(false, false, "glUnmapBuffer")
		{
		}

		void set(GLenum target)
		{
			m_target = target;
		}

		GLenum m_target;
	};

	class GlDeleteBuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, std::unique_ptr<GLuint[]> buffers)
		{
			auto ptr = std::shared_ptr<GlDeleteBuffersCommand>(new GlDeleteBuffersCommand);
			ptr->set(n, std::move(buffers));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteBuffers(m_n, m_buffers.get());
		}
	private:
		GlDeleteBuffersCommand(void) :
			OpenGlCommand(false, false, "glDeleteBuffers")
		{
		}

		void set(GLsizei n, std::unique_ptr<GLuint[]> buffers)
		{
			m_n = n;
			m_buffers = std::move(buffers);
		}

		GLsizei m_n;
		std::unique_ptr<GLuint[]> m_buffers;
	};

	class GlBindImageTextureCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
			GLenum access, GLenum format)
		{
			auto ptr = std::shared_ptr<GlBindImageTextureCommand>(new GlBindImageTextureCommand);
			ptr->set(unit, texture, level, layered, layer, access, format);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindImageTexture(m_unit, m_texture, m_level, m_layered, m_layer, m_access, m_format);
		}
	private:
		GlBindImageTextureCommand(void) :
			OpenGlCommand(false, false, "glBindImageTexture")
		{
		}

		void set(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
			GLenum access, GLenum format)
		{
			m_unit = unit;
			m_texture = texture;
			m_level = level;
			m_layered = layered;
			m_layer = layer;
			m_access = access;
			m_format = format;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLbitfield barriers)
		{
			auto ptr = std::shared_ptr<GlMemoryBarrierCommand>(new GlMemoryBarrierCommand);
			ptr->set(barriers);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glMemoryBarrier(m_barriers);
		}
	private:
		GlMemoryBarrierCommand(void) :
			OpenGlCommand(false, false, "glMemoryBarrier")
		{
		}

		void set(GLbitfield barriers)
		{
			m_barriers = barriers;
		}

		GLbitfield m_barriers;
	};

	class GlGetStringiCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum name, GLuint index, const GLubyte*& returnValue)
		{
			auto ptr = std::shared_ptr<GlGetStringiCommand>(new GlGetStringiCommand);
			ptr->set(name, index, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glGetStringi(m_name, m_index);
		}
	private:
		GlGetStringiCommand(void) :
			OpenGlCommand(true, false, "glGetStringi")
		{
		}

		void set(GLenum name, GLuint index, const GLubyte*& returnValue)
		{
			m_name = name;
			m_index = index;
			m_returnValue = &returnValue;
		}

		GLenum m_name;
		GLuint m_index;
		const GLubyte** m_returnValue;
	};

	class GlInvalidateFramebufferCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizei numAttachments, std::unique_ptr<GLenum[]> attachments)
		{
			auto ptr = std::shared_ptr<GlInvalidateFramebufferCommand>(new GlInvalidateFramebufferCommand);
			ptr->set(target, numAttachments, std::move(attachments));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glInvalidateFramebuffer(m_target, m_numAttachments, m_attachments.get());
		}
	private:
		GlInvalidateFramebufferCommand(void) :
			OpenGlCommand(false, false, "glInvalidateFramebuffer")
		{
		}

		void set(GLenum target, GLsizei numAttachments, std::unique_ptr<GLenum[]> attachments)
		{
			m_target = target;
			m_numAttachments = numAttachments;
			m_attachments = std::move(attachments);
		}

		GLenum m_target;
		GLsizei m_numAttachments;
		std::unique_ptr<GLenum[]> m_attachments;
	};

	template <class dataType>
	class GlBufferStorageCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLbitfield flags)
		{
			auto ptr = std::shared_ptr<GlBufferStorageCommand>(new GlBufferStorageCommand);
			ptr->set(target, size, std::move(data), flags);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBufferStorage(m_target, m_size, m_data.get(), m_flags);
		}
	private:
		GlBufferStorageCommand(void) :
			OpenGlCommand(false, false, "glBufferStorage")
		{
		}

		void set(GLenum target, GLsizeiptr size, std::unique_ptr<dataType[]> data, GLbitfield flags)
		{
			m_target = target;
			m_size = size;
			m_data = std::move(data);
			m_flags = flags;
		}

		GLenum m_target;
		GLsizeiptr m_size;
		std::unique_ptr<dataType[]> m_data;
		GLbitfield m_flags;
	};

	class GlFenceSyncCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum condition, GLbitfield flags, GLsync& returnValue)
		{
			auto ptr = std::shared_ptr<GlFenceSyncCommand>(new GlFenceSyncCommand);
			ptr->set(condition, flags, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glFenceSync(m_condition, m_flags);
		}
	private:
		GlFenceSyncCommand(void) :
			OpenGlCommand(true, false, "glFenceSync")
		{
		}

		void set(GLenum condition, GLbitfield flags, GLsync& returnValue)
		{
			m_condition = condition;
			m_flags = flags;
			m_returnValue = &returnValue;
		}

		GLenum m_condition;
		GLbitfield m_flags;
		GLsync* m_returnValue;
	};

	class GlClientWaitSyncCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsync sync, GLbitfield flags, GLuint64 timeout)
		{
			auto ptr = std::shared_ptr<GlClientWaitSyncCommand>(new GlClientWaitSyncCommand);
			ptr->set(sync, flags, timeout);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glClientWaitSync(m_sync, m_flags, m_timeout);
		}
	private:
		GlClientWaitSyncCommand(void) :
			OpenGlCommand(true, false, "glClientWaitSync")
		{
		}

		void set(GLsync sync, GLbitfield flags, GLuint64 timeout)
		{
			m_sync = sync;
			m_flags = flags;
			m_timeout = timeout;
		}

		GLsync m_sync;
		GLbitfield m_flags;
		GLuint64 m_timeout;
	};

	class GlDeleteSyncCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsync sync)
		{
			auto ptr = std::shared_ptr<GlDeleteSyncCommand>(new GlDeleteSyncCommand);
			ptr->set(sync);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDeleteSync(m_sync);
		}
	private:
		GlDeleteSyncCommand(void) :
			OpenGlCommand(true, false, "glDeleteSync")
		{
		}

		void set(GLsync sync)
		{
			m_sync = sync;
		}

		GLsync m_sync;
	};

	class GlGetUniformBlockIndexCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, const GLchar* uniformBlockName, GLuint& returnValue)
		{
			auto ptr = std::shared_ptr<GlGetUniformBlockIndexCommand>(new GlGetUniformBlockIndexCommand);
			ptr->set(program, uniformBlockName, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = g_glGetUniformBlockIndex(m_program, m_uniformBlockName);
		}
	private:
		GlGetUniformBlockIndexCommand(void) :
			OpenGlCommand(true, true, "glGetUniformBlockIndex")
		{
		}

		void set(GLuint program, const GLchar* uniformBlockName, GLuint& returnValue)
		{
			m_program = program;
			m_uniformBlockName = uniformBlockName;
			m_returnValue = &returnValue;
		}

		GLuint m_program;
		const GLchar* m_uniformBlockName;
		GLuint* m_returnValue;
	};

	class GlUniformBlockBindingCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
		{
			auto ptr = std::shared_ptr<GlUniformBlockBindingCommand>(new GlUniformBlockBindingCommand);
			ptr->set(program, uniformBlockIndex, uniformBlockBinding);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glUniformBlockBinding(m_program, m_uniformBlockIndex, m_uniformBlockBinding);
		}
	private:
		GlUniformBlockBindingCommand(void) :
			OpenGlCommand(false, false, "glUniformBlockBinding")
		{
		}

		void set(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
		{
			m_program = program;
			m_uniformBlockIndex = uniformBlockIndex;
			m_uniformBlockBinding = uniformBlockBinding;
		}

		GLuint m_program;
		GLuint m_uniformBlockIndex;
		GLuint m_uniformBlockBinding;
	};

	class GlGetActiveUniformBlockivCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)
		{
			auto ptr = std::shared_ptr<GlGetActiveUniformBlockivCommand>(new GlGetActiveUniformBlockivCommand);
			ptr->set(program, uniformBlockIndex, pname, params);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetActiveUniformBlockiv(m_program, m_uniformBlockIndex, m_pname, m_params);
		}
	private:
		GlGetActiveUniformBlockivCommand(void) :
			OpenGlCommand(true, true, "glGetActiveUniformBlockiv")
		{
		}

		void set(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)
		{
			m_program = program;
			m_uniformBlockIndex = uniformBlockIndex;
			m_pname = pname;
			m_params = params;
		}

		GLuint m_program;
		GLuint m_uniformBlockIndex;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlGetUniformIndicesCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames,
			GLuint* uniformIndices)
		{
			auto ptr = std::shared_ptr<GlGetUniformIndicesCommand>(new GlGetUniformIndicesCommand);
			ptr->set(program, uniformCount, uniformNames, uniformIndices);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetUniformIndices(m_program, m_uniformCount, m_uniformNames, m_uniformIndices);
		}
	private:
		GlGetUniformIndicesCommand(void) :
			OpenGlCommand(true, true, "glGetUniformIndices")
		{
		}

		void set(GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames,
			GLuint* uniformIndices)
		{
			m_program = program;
			m_uniformCount = uniformCount;
			m_uniformNames = uniformNames;
			m_uniformIndices = uniformIndices;
		}

		GLuint m_program;
		GLsizei m_uniformCount;
		const GLchar* const* m_uniformNames;
		GLuint* m_uniformIndices;
	};

	class GlGetActiveUniformsivCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname,
			GLint* params)
		{
			auto ptr = std::shared_ptr<GlGetActiveUniformsivCommand>(new GlGetActiveUniformsivCommand);
			ptr->set(program, uniformCount, uniformIndices, pname, params);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetActiveUniformsiv(m_program, m_uniformCount, m_uniformIndices, m_pname, m_params);
		}
	private:
		GlGetActiveUniformsivCommand(void) :
			OpenGlCommand(true, true, "glGetActiveUniformsiv")
		{
		}

		void set(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname,
			GLint* params)
		{
			m_program = program;
			m_uniformCount = uniformCount;
			m_uniformIndices = uniformIndices;
			m_pname = pname;
			m_params = params;
		}

		GLuint m_program;
		GLsizei m_uniformCount;
		const GLuint* m_uniformIndices;
		GLenum m_pname;
		GLint* m_params;
	};

	class GlBindBufferBaseCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLuint index, GLuint buffer)
		{
			auto ptr = std::shared_ptr<GlBindBufferBaseCommand>(new GlBindBufferBaseCommand);
			ptr->set(target, index, buffer);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBindBufferBase(m_target, m_index, m_buffer);
		}
	private:
		GlBindBufferBaseCommand(void) :
			OpenGlCommand(false, false, "glBindBufferBase")
		{
		}

		void set(GLenum target, GLuint index, GLuint buffer)
		{
			m_target = target;
			m_index = index;
			m_buffer = buffer;
		}

		GLenum m_target;
		GLuint m_index;
		GLuint m_buffer;
	};

	template <class dataType>
	class GlBufferSubDataCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLintptr offset, GLsizeiptr size, std::unique_ptr<dataType[]> data)
		{
			auto ptr = std::shared_ptr<GlBufferSubDataCommand>(new GlBufferSubDataCommand);
			ptr->set(target, offset, size, std::move(data));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glBufferSubData(m_target, m_offset, m_size, m_data.get());
		}
	private:
		GlBufferSubDataCommand(void) :
			OpenGlCommand(false, false, "glBufferSubData")
		{
		}

		void set(GLenum target, GLintptr offset, GLsizeiptr size, std::unique_ptr<dataType[]> data)
		{
			m_target = target;
			m_offset = offset;
			m_size = size;
			m_data = std::move(data);
		}

		GLenum m_target;
		GLintptr m_offset;
		GLsizeiptr m_size;
		std::unique_ptr<dataType[]> m_data;
	};

	class GlGetProgramBinaryCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary)
		{
			auto ptr = std::shared_ptr<GlGetProgramBinaryCommand>(new GlGetProgramBinaryCommand);
			ptr->set(program, bufSize, length, binaryFormat, binary);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glGetProgramBinary(m_program, m_bufSize, m_length, m_binaryFormat, m_binary);
		}
	private:
		GlGetProgramBinaryCommand(void) :
			OpenGlCommand(true, true, "glGetProgramBinary")
		{
		}

		void set(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary)
		{
			m_program = program;
			m_bufSize = bufSize;
			m_length = length;
			m_binaryFormat = binaryFormat;
			m_binary = binary;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLenum binaryFormat, std::unique_ptr<dataType[]> binary, GLsizei length)
		{
			auto ptr = std::shared_ptr<GlProgramBinaryCommand>(new GlProgramBinaryCommand);
			ptr->set(program, binaryFormat, std::move(binary), length);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glProgramBinary(m_program, m_binaryFormat, m_binary.get(), m_length);
		}
	private:
		GlProgramBinaryCommand(void) :
			OpenGlCommand(false, false, "glProgramBinary")
		{
		}

		void set(GLuint program, GLenum binaryFormat, std::unique_ptr<dataType[]> binary, GLsizei length)
		{
			m_program = program;
			m_binaryFormat = binaryFormat;
			m_binary = std::move(binary);
			m_length = length;
		}

		GLuint m_program;
		GLenum m_binaryFormat;
		std::unique_ptr<dataType[]> m_binary;
		GLsizei m_length;
	};

	class GlProgramParameteriCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint program, GLenum pname, GLint value)
		{
			auto ptr = std::shared_ptr<GlProgramParameteriCommand>(new GlProgramParameteriCommand);
			ptr->set(program, pname, value);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glProgramParameteri(m_program, m_pname, m_value);
		}
	private:
		GlProgramParameteriCommand(void) :
			OpenGlCommand(false, false, "glProgramParameteri")
		{
		}

		void set(GLuint program, GLenum pname, GLint value)
		{
			m_program = program;
			m_pname = pname;
			m_value = value;
		}

		GLuint m_program;
		GLenum m_pname;
		GLint m_value;
	};

	class GlTexStorage2DCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
		{
			auto ptr = std::shared_ptr<GlTexStorage2DCommand>(new GlTexStorage2DCommand);
			ptr->set(target, levels, internalformat, width, height);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTexStorage2D(m_target, m_levels, m_internalformat, m_width, m_height);
		}
	private:
		GlTexStorage2DCommand(void) :
			OpenGlCommand(false, false, "glTexStorage2D")
		{
		}

		void set(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
		{
			m_target = target;
			m_levels = levels;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
		}

		GLenum m_target;
		GLsizei m_levels;
		GLenum m_internalformat;
		GLsizei m_width;
		GLsizei m_height;
	};

	class GlTextureStorage2DCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
		{
			auto ptr = std::shared_ptr<GlTextureStorage2DCommand>(new GlTextureStorage2DCommand);
			ptr->set(texture, levels, internalformat, width, height);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTextureStorage2D(m_texture, m_levels, m_internalformat, m_width, m_height);
		}
	private:
		GlTextureStorage2DCommand(void) :
			OpenGlCommand(false, false, "glTextureStorage2D")
		{
		}

		void set(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
		{
			m_texture = texture;
			m_levels = levels;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
			GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
		{
			auto ptr = std::shared_ptr<GlTextureSubImage2DUnbufferedCommand>(new GlTextureSubImage2DUnbufferedCommand);
			ptr->set(texture, level, xoffset, yoffset, width, height, format, type, std::move(pixels));
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTextureSubImage2D(m_texture, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type,
								  m_pixels.get());
		}
	private:
		GlTextureSubImage2DUnbufferedCommand(void) :
			OpenGlCommand(false, false, "glTextureSubImage2D")
		{
		}

		void set(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
			GLsizei height, GLenum format, GLenum type, std::unique_ptr<pixelType[]> pixels)
		{
			m_texture = texture;
			m_level = level;
			m_xoffset = xoffset;
			m_yoffset = yoffset;
			m_width = width;
			m_height = height;
			m_format = format;
			m_type = type;
			m_pixels = std::move(pixels);
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
			GLsizei height, GLenum format, GLenum type, std::size_t offset)
		{
			auto ptr = std::shared_ptr<GlTextureSubImage2DBufferedCommand>(new GlTextureSubImage2DBufferedCommand);
			ptr->set(texture, level, xoffset, yoffset, width, height, format, type, offset);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTextureSubImage2D(m_texture, m_level, m_xoffset, m_yoffset, m_width, m_height, m_format, m_type,
								  (const GLvoid* )m_offset);
		}
	private:
		GlTextureSubImage2DBufferedCommand(void) :
			OpenGlCommand(false, false, "glTextureSubImage2D")
		{
		}

		void set(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
			GLsizei height, GLenum format, GLenum type, std::size_t offset)
		{
			m_texture = texture;
			m_level = level;
			m_xoffset = xoffset;
			m_yoffset = yoffset;
			m_width = width;
			m_height = height;
			m_format = format;
			m_type = type;
			m_offset = offset;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
			GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
		{
			auto ptr = std::shared_ptr<GlTextureStorage2DMultisampleCommand>(new GlTextureStorage2DMultisampleCommand);
			ptr->set(texture, target, samples, internalformat, width, height, fixedsamplelocations);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTextureStorage2DMultisample(m_texture, m_target, m_samples, m_internalformat,m_width, m_height,
				m_fixedsamplelocations);
		}
	private:
		GlTextureStorage2DMultisampleCommand(void) :
			OpenGlCommand(false, false, "glTextureStorage2DMultisample")
		{
		}

		void set(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
			GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
		{
			m_texture = texture;
			m_target = target;
			m_samples = samples;
			m_internalformat = internalformat;
			m_width = width;
			m_height = height;
			m_fixedsamplelocations = fixedsamplelocations;
		}

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
		static std::shared_ptr<OpenGlCommand> get(GLuint texture, GLenum pname, GLint param)
		{
			auto ptr = std::shared_ptr<GlTextureParameteriCommand>(new GlTextureParameteriCommand);
			ptr->set(texture, pname, param);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTextureParameteri(m_texture, m_pname, m_param);
		}
	private:
		GlTextureParameteriCommand(void) :
			OpenGlCommand(false, false, "glTextureParameteri")
		{
		}

		void set(GLuint texture, GLenum pname, GLint param)
		{
			m_texture = texture;
			m_pname = pname;
			m_param = param;
		}

		GLuint m_texture;
		GLenum m_pname;
		GLint m_param;
	};

	class GlTextureParameterfCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint texture, GLenum pname, GLfloat param)
		{
			auto ptr = std::shared_ptr<GlTextureParameterfCommand>(new GlTextureParameterfCommand);
			ptr->set(texture, pname, param);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glTextureParameterf(m_texture, m_pname, m_param);
		}
	private:
		GlTextureParameterfCommand(void) :
			OpenGlCommand(false, false, "glTextureParameterf")
		{
		}

		void set(GLuint texture, GLenum pname, GLfloat param)
		{
			m_texture = texture;
			m_pname = pname;
			m_param = param;
		}

		GLuint m_texture;
		GLenum m_pname;
		GLfloat m_param;
	};

	class GlCreateTexturesCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLsizei n, GLuint* textures)
		{
			auto ptr = std::shared_ptr<GlCreateTexturesCommand>(new GlCreateTexturesCommand);
			ptr->set(target, n, textures);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glCreateTextures(m_target, m_n, m_textures);
		}
	private:
		GlCreateTexturesCommand(void) :
			OpenGlCommand(true, false, "glCreateTextures")
		{
		}

		void set(GLenum target, GLsizei n, GLuint* textures)
		{
			m_target = target;
			m_n = n;
			m_textures = textures;
		}

		GLenum m_target;
		GLsizei m_n;
		GLuint* m_textures;
	};

	class GlCreateBuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* buffers)
		{
			auto ptr = std::shared_ptr<GlCreateBuffersCommand>(new GlCreateBuffersCommand);
			ptr->set(n, buffers);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glCreateBuffers(m_n, m_buffers);
		}
	private:
		GlCreateBuffersCommand(void) :
			OpenGlCommand(true, false, "glCreateBuffers")
		{
		}

		void set(GLsizei n, GLuint* buffers)
		{
			m_n = n;
			m_buffers = buffers;
		}

		GLsizei m_n;
		GLuint* m_buffers;
	};

	class GlCreateFramebuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLsizei n, GLuint* framebuffers)
		{
			auto ptr = std::shared_ptr<GlCreateFramebuffersCommand>(new GlCreateFramebuffersCommand);
			ptr->set(n, framebuffers);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glCreateFramebuffers(m_n, m_framebuffers);
		}
	private:
		GlCreateFramebuffersCommand(void) :
			OpenGlCommand(true, false, "glCreateFramebuffers")
		{
		}

		void set(GLsizei n, GLuint* framebuffers)
		{
			m_n = n;
			m_framebuffers = framebuffers;
		}

		GLsizei m_n;
		GLuint* m_framebuffers;
	};

	class GlNamedFramebufferTextureCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level)
		{
			auto ptr = std::shared_ptr<GlNamedFramebufferTextureCommand>(new GlNamedFramebufferTextureCommand);
			ptr->set(framebuffer, attachment, texture, level);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glNamedFramebufferTexture(m_framebuffer, m_attachment, m_texture, m_level);
		}
	private:
		GlNamedFramebufferTextureCommand(void) :
			OpenGlCommand(false, false, "glNamedFramebufferTexture")
		{
		}

		void set(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level)
		{
			m_framebuffer = framebuffer;
			m_attachment = attachment;
			m_texture = texture;
			m_level = level;
		}

		GLuint m_framebuffer;
		GLenum m_attachment;
		GLuint m_texture;
		GLint m_level;
	};

	class GlDrawElementsBaseVertexCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum mode, GLsizei count, GLenum type, const char* indices,
			GLint basevertex)
		{
			auto ptr = std::shared_ptr<GlDrawElementsBaseVertexCommand>(new GlDrawElementsBaseVertexCommand);
			ptr->set(mode, count, type, indices, basevertex);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glDrawElementsBaseVertex(m_mode, m_count, m_type, m_indices, m_basevertex);
		}
	private:
		GlDrawElementsBaseVertexCommand(void) :
			OpenGlCommand(false, false, "glDrawElementsBaseVertex")
		{
		}

		void set(GLenum mode, GLsizei count, GLenum type, const char* indices,
			GLint basevertex)
		{
			m_mode = mode;
			m_count = count;
			m_type = type;
			m_indices = indices;
			m_basevertex = basevertex;
		}

		GLenum m_mode;
		GLsizei m_count;
		GLenum m_type;
		const char*  m_indices;
		GLint m_basevertex;
	};

	class GlFlushMappedBufferRangeCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(GLenum target, GLintptr offset, GLsizeiptr length)
		{
			auto ptr = std::shared_ptr<GlFlushMappedBufferRangeCommand>(new GlFlushMappedBufferRangeCommand);
			ptr->set(target, offset, length);
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glFlushMappedBufferRange(m_target, m_offset, m_length);
		}
	private:
		GlFlushMappedBufferRangeCommand(void) :
			OpenGlCommand(false, false, "glFlushMappedBufferRange")
		{
		}

		void set(GLenum target, GLintptr offset, GLsizeiptr length)
		{
			m_target = target;
			m_offset = offset;
			m_length = length;
		}

		GLenum m_target;
		GLintptr m_offset;
		GLsizeiptr m_length;
	};

	class GlFinishCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(void)
		{
			auto ptr = std::shared_ptr<GlFinishCommand>(new GlFinishCommand);
			ptr->set();
			return ptr;
		}

		void commandToExecute(void) override
		{
			g_glFinish();
		}
	private:
		GlFinishCommand(void) :
			OpenGlCommand(true, true, "glFinish")
		{
		}

		void set(void)
		{

		}
	};
#ifdef MUPENPLUSAPI
	//Vid ext functions
	class CoreVideoInitCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(void)
		{
			auto ptr = std::shared_ptr<CoreVideoInitCommand>(new CoreVideoInitCommand);
			ptr->set();
			return ptr;
		}

		void commandToExecute(void) override
		{
			::CoreVideo_Init();
		}
	private:
		CoreVideoInitCommand(void) :
			OpenGlCommand(true, false, "CoreVideo_Init", false)
		{
		}

		void set(void)
		{
		}
	};

	class CoreVideoQuitCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(void)
		{
			auto ptr = std::shared_ptr<CoreVideoQuitCommand>(new CoreVideoQuitCommand);
			ptr->set();
			return ptr;
		}

		void commandToExecute(void) override
		{
			::CoreVideo_Quit();
		}
	private:
		CoreVideoQuitCommand(void) :
			OpenGlCommand(true, false, "CoreVideo_Quit", false)
		{
		}

		void set(void)
		{
		}
	};

	class CoreVideoSetVideoModeCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(int screenWidth, int screenHeight, int bitsPerPixel, m64p_video_mode mode,
			m64p_video_flags flags, m64p_error& returnValue)
		{
			auto ptr = std::shared_ptr<CoreVideoSetVideoModeCommand>(new CoreVideoSetVideoModeCommand);
			ptr->set(screenWidth, screenHeight, bitsPerPixel, mode, flags, returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = ::CoreVideo_SetVideoMode(m_screenWidth, m_screenHeight, m_bitsPerPixel, m_mode, m_flags);

			initGLFunctions();
		}
	private:
		CoreVideoSetVideoModeCommand(void) :
			OpenGlCommand(true, false, "CoreVideo_SetVideoMode", false)
		{
		}

		void set(int screenWidth, int screenHeight, int bitsPerPixel, m64p_video_mode mode,
			m64p_video_flags flags, m64p_error& returnValue)
		{
			m_screenWidth = screenWidth;
			m_screenHeight = screenHeight;
			m_bitsPerPixel = bitsPerPixel;
			m_mode = mode;
			m_flags = flags;
			m_returnValue = &returnValue;
		}

		int m_screenWidth;
		int m_screenHeight;
		int m_bitsPerPixel;
		m64p_video_mode m_mode;
		m64p_video_flags m_flags;
		m64p_error* m_returnValue;
	};

	class CoreVideoGLSetAttributeCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(m64p_GLattr attribute, int value)
		{
			auto ptr = std::shared_ptr<CoreVideoGLSetAttributeCommand>(new CoreVideoGLSetAttributeCommand);
			ptr->set(attribute, value);
			return ptr;
		}

		void commandToExecute(void) override
		{
			::CoreVideo_GL_SetAttribute(m_attribute, m_value);
		}
	private:
		CoreVideoGLSetAttributeCommand(void) :
			OpenGlCommand(true, false, "CoreVideo_GL_SetAttribute", false)
		{
		}

		void set(m64p_GLattr attribute, int value)
		{
			m_attribute = attribute;
			m_value = value;
		}

		m64p_GLattr m_attribute;
		int m_value;
	};

	class CoreVideoGLGetAttributeCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(m64p_GLattr attribute, int* value)
		{
			auto ptr = std::shared_ptr<CoreVideoGLGetAttributeCommand>(new CoreVideoGLGetAttributeCommand);
			ptr->set(attribute, value);
			return ptr;
		}

		void commandToExecute(void) override
		{
			::CoreVideo_GL_GetAttribute(m_attribute, m_value);
		}
	private:
		CoreVideoGLGetAttributeCommand(void) :
			OpenGlCommand(true, false, "CoreVideo_GL_GetAttribute", false)
		{
		}

		void set(m64p_GLattr attribute, int* value)
		{
			m_attribute = attribute;
			m_value = value;
		}

		m64p_GLattr m_attribute;
		int* m_value;
	};

	class CoreVideoGLSwapBuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(std::function<void(void)> swapBuffersCallback)
		{
			auto ptr = std::shared_ptr<CoreVideoGLSwapBuffersCommand>(new CoreVideoGLSwapBuffersCommand);
			ptr->set(swapBuffersCallback);
			return ptr;
		}

		void commandToExecute(void) override
		{
			::CoreVideo_GL_SwapBuffers();
			m_swapBuffersCallback();
		}
	private:
		CoreVideoGLSwapBuffersCommand(void) :
			OpenGlCommand(false, false, "CoreVideo_GL_SwapBuffers", false)
		{
		}

		void set(std::function<void(void)> swapBuffersCallback)
		{
			m_swapBuffersCallback = swapBuffersCallback;
		}

		std::function<void(void)> m_swapBuffersCallback;
	};
#else
	//Zilmar API functions
	class WindowsStartCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(bool& returnValue)
		{
			auto ptr = std::shared_ptr<WindowsStartCommand>(new WindowsStartCommand);
			ptr->set(returnValue);
			return ptr;
		}

		void commandToExecute(void) override
		{
			*m_returnValue = WindowsWGL::start();
		}

	private:
		WindowsStartCommand(void) :
			OpenGlCommand(true, false, "WindowsStartCommand", false)
		{
		}

		void set(bool& returnValue)
		{
			m_returnValue = &returnValue;
		}

		bool* m_returnValue;
	};

	class WindowsStopCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(void)
		{
			auto ptr = std::shared_ptr<WindowsStopCommand>(new WindowsStopCommand);
			ptr->set();
			return ptr;
		}

		void commandToExecute(void) override
		{
			WindowsWGL::stop();
		}

	private:
		WindowsStopCommand(void) :
			OpenGlCommand(true, false, "WindowsStopCommand", false)
		{
		}

		void set(void)
		{
		}
	};

	class WindowsSwapBuffersCommand : public OpenGlCommand
	{
	public:
		static std::shared_ptr<OpenGlCommand> get(std::function<void(void)> swapBuffersCallback)
		{
			auto ptr = std::shared_ptr<WindowsSwapBuffersCommand>(new WindowsSwapBuffersCommand);
			ptr->set(swapBuffersCallback);
			return ptr;
		}

		void commandToExecute(void) override
		{
			WindowsWGL::swapBuffers();
			m_swapBuffersCallback();
		}
	private:
		WindowsSwapBuffersCommand(void) :
			OpenGlCommand(false, false, "WindowsSwapBuffersCommand", false)
		{
		}

		void set(std::function<void(void)> swapBuffersCallback)
		{
			m_swapBuffersCallback = swapBuffersCallback;
		}

		std::function<void(void)> m_swapBuffersCallback;
	};

#endif
}
