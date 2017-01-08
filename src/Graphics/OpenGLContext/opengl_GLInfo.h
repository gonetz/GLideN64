#pragma once
#include "GLFunctions.h"

namespace opengl {

	struct GLInfo {
		GLint majorVersion = 0;
		GLint minorVersion = 0;
		bool isGLES2 = false;
		bool isGLESX = false;
		bool imageTextures = false;
		bool shaderStorage = false;

		void init();
	};
}
