#pragma once
#include <string>
#include <Graphics/OpenGLContext/GLFunctions.h>

namespace glsl {

	struct Utils {
		static void locateAttributes(GLuint _program, bool _rect, bool _textures);
		static bool checkShaderCompileStatus(GLuint obj);
		static bool checkProgramLinkStatus(GLuint obj);
		static void logErrorShader(GLenum _shaderType, const std::string & _strShader);
	};
}
