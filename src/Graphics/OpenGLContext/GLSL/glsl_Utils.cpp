#include <assert.h>
#include <Log.h>
#include <Graphics/OpenGLContext/opengl_Attributes.h>
#include <Graphics/OpenGLContext/opengl_Wrapper.h>
#include "glsl_Utils.h"

using namespace glsl;
using namespace opengl;

void Utils::locateAttributes(GLuint _program, bool _rect, bool _textures)
{
	if (_rect) {
		FunctionWrapper::glBindAttribLocation(_program, opengl::rectAttrib::position, "aRectPosition");
		if (_textures) {
			FunctionWrapper::glBindAttribLocation(_program, opengl::rectAttrib::texcoord0, "aTexCoord0");
			FunctionWrapper::glBindAttribLocation(_program, opengl::rectAttrib::texcoord1, "aTexCoord1");
		}
		return;
	}

	FunctionWrapper::glBindAttribLocation(_program, opengl::triangleAttrib::position, "aPosition");
	FunctionWrapper::glBindAttribLocation(_program, opengl::triangleAttrib::color, "aColor");
	FunctionWrapper::glBindAttribLocation(_program, opengl::triangleAttrib::numlights, "aNumLights");
	FunctionWrapper::glBindAttribLocation(_program, opengl::triangleAttrib::modify, "aModify");
	if (_textures)
		FunctionWrapper::glBindAttribLocation(_program, opengl::triangleAttrib::texcoord, "aTexCoord");
}


static const GLsizei nShaderLogSize = 1024;

bool Utils::checkShaderCompileStatus(GLuint obj)
{
#ifdef GL_DEBUG
	GLint status;
	FunctionWrapper::glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLchar shader_log[nShaderLogSize];
		GLsizei nLogSize = nShaderLogSize;
		FunctionWrapper::glGetShaderInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		shader_log[nLogSize] = 0;
		LOG(LOG_ERROR, "shader_compile error: %s\n", shader_log);
		return false;
	}
#endif
	return true;
}

bool Utils::checkProgramLinkStatus(GLuint obj)
{
#ifdef GL_DEBUG
	GLint status;
	FunctionWrapper::glGetProgramiv(obj, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLsizei nLogSize = nShaderLogSize;
		GLchar shader_log[nShaderLogSize];
		FunctionWrapper::glGetProgramInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		LOG(LOG_ERROR, "shader_link error: %s\n", shader_log);
		return false;
	}
#endif
	return true;
}

void Utils::logErrorShader(GLenum _shaderType, const std::string & _strShader)
{
	LOG(LOG_ERROR, "Error in %s shader", _shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment");

	const u32 max = 800;
	u32 pos = 0;

	while (pos < _strShader.length()) {

		if (_strShader.length() - pos < max) {
			LOG(LOG_ERROR, "%s", _strShader.substr(pos).data());
		}
		else {
			LOG(LOG_ERROR, "%s", _strShader.substr(pos, max).data());
		}
		pos += max;
	}
}

GLuint Utils::createRectShaderProgram(const std::string& _strVertex, const std::string& _strFragment)
{
	GLuint vertex_shader_object = FunctionWrapper::glCreateShader(GL_VERTEX_SHADER);
	FunctionWrapper::glShaderSource(vertex_shader_object, _strVertex);
	FunctionWrapper::glCompileShader(vertex_shader_object);
	assert(checkShaderCompileStatus(vertex_shader_object));

	if (!checkShaderCompileStatus(vertex_shader_object))
		logErrorShader(GL_VERTEX_SHADER, _strVertex);

	GLuint fragment_shader_object = FunctionWrapper::glCreateShader(GL_FRAGMENT_SHADER);
	FunctionWrapper::glShaderSource(fragment_shader_object, _strFragment);
	FunctionWrapper::glCompileShader(fragment_shader_object);
	assert(checkShaderCompileStatus(fragment_shader_object));

	if (!checkShaderCompileStatus(fragment_shader_object))
		logErrorShader(GL_VERTEX_SHADER, _strFragment);

	GLuint program = FunctionWrapper::glCreateProgram();
	locateAttributes(program, true, true);
	FunctionWrapper::glAttachShader(program, vertex_shader_object);
	FunctionWrapper::glAttachShader(program, fragment_shader_object);
	FunctionWrapper::glLinkProgram(program);
	FunctionWrapper::glDeleteShader(vertex_shader_object);
	FunctionWrapper::glDeleteShader(fragment_shader_object);
	assert(checkProgramLinkStatus(program));
	return program;
}
