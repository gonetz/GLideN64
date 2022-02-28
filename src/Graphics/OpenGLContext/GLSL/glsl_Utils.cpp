#include <assert.h>
#include <Log.h>
#include <Graphics/OpenGLContext/opengl_Attributes.h>
#include "glsl_Utils.h"

using namespace glsl;

void Utils::locateAttributes(GLuint _program, bool _rect, bool _textures)
{
	static GLint maxVertexAttribs = 0;
	if (maxVertexAttribs == 0)
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);

	if (_rect) {
		glBindAttribLocation(_program, opengl::rectAttrib::position, "aRectPosition");
		if (opengl::rectAttrib::barycoords < static_cast<u32>(maxVertexAttribs))
			glBindAttribLocation(_program, opengl::rectAttrib::barycoords, "aBaryCoords");
		if (_textures) {
			glBindAttribLocation(_program, opengl::rectAttrib::texcoord0, "aTexCoord0");
			glBindAttribLocation(_program, opengl::rectAttrib::texcoord1, "aTexCoord1");
		}
		return;
	}

	glBindAttribLocation(_program, opengl::triangleAttrib::position, "aPosition");
	glBindAttribLocation(_program, opengl::triangleAttrib::color, "aColor");
	glBindAttribLocation(_program, opengl::triangleAttrib::numlights, "aNumLights");
	glBindAttribLocation(_program, opengl::triangleAttrib::modify, "aModify");
	if (opengl::triangleAttrib::barycoords < static_cast<u32>(maxVertexAttribs))
		glBindAttribLocation(_program, opengl::triangleAttrib::barycoords, "aBaryCoords");
	if (_textures)
		glBindAttribLocation(_program, opengl::triangleAttrib::texcoord, "aTexCoord");
}


static const GLsizei nShaderLogSize = 1024;

bool Utils::checkShaderCompileStatus(GLuint obj)
{
#ifdef GL_DEBUG
	GLint status;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLchar shader_log[nShaderLogSize];
		GLsizei nLogSize = nShaderLogSize;
		glGetShaderInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		shader_log[nLogSize] = 0;
		LOG(LOG_ERROR, "shader_compile error: %s", shader_log);
		return false;
	}
#endif
	return true;
}

static
bool _checkProgramLinkStatus(GLuint obj)
{
	GLint status;
	glGetProgramiv(obj, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLsizei nLogSize = nShaderLogSize;
		GLchar shader_log[nShaderLogSize];
		glGetProgramInfoLog(obj, nShaderLogSize, &nLogSize, shader_log);
		LOG(LOG_ERROR, "shader_link error: %s", shader_log);
		return false;
	}
	return true;
}

bool Utils::checkProgramLinkStatus(GLuint obj, bool _force)
{
#ifdef GL_DEBUG
	return _checkProgramLinkStatus(obj);
#else
	if (_force)
		return _checkProgramLinkStatus(obj);
	return true;
#endif
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

GLuint Utils::createRectShaderProgram(const char * _strVertex, const char * _strFragment)
{
	GLuint vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_object, 1, &_strVertex, nullptr);
	glCompileShader(vertex_shader_object);
	assert(checkShaderCompileStatus(vertex_shader_object));

	if (!checkShaderCompileStatus(vertex_shader_object))
		logErrorShader(GL_VERTEX_SHADER, _strVertex);

	GLuint fragment_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_object, 1, &_strFragment, nullptr);
	glCompileShader(fragment_shader_object);
	assert(checkShaderCompileStatus(fragment_shader_object));

	if (!checkShaderCompileStatus(fragment_shader_object))
		logErrorShader(GL_FRAGMENT_SHADER, _strFragment);

	GLuint program = glCreateProgram();
	locateAttributes(program, true, true);
	glAttachShader(program, vertex_shader_object);
	glAttachShader(program, fragment_shader_object);
	glLinkProgram(program);
	glDeleteShader(vertex_shader_object);
	glDeleteShader(fragment_shader_object);
	assert(checkProgramLinkStatus(program));
	return program;
}
