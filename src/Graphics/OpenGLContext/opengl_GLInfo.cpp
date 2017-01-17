#pragma once
#include <Log.h>
#include <Config.h>
#include "opengl_Utils.h"
#include "opengl_GLInfo.h"

using namespace opengl;

void GLInfo::init() {
	const char * strVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	isGLESX = strstr(strVersion, "OpenGL ES") != nullptr;
	isGLES2 = strstr(strVersion, "OpenGL ES 2") != nullptr;
	if (isGLES2) {
		majorVersion = 2;
		minorVersion = 0;
	}
	else {
		glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	}
	LOG(LOG_VERBOSE, "%s major version: %d\n", isGLESX ? "OpenGL ES" : "OpenGL", majorVersion);
	LOG(LOG_VERBOSE, "%s minor version: %d\n", isGLESX ? "OpenGL ES" : "OpenGL", minorVersion);

	if (isGLES2)
        imageTextures = false;
	else if (isGLESX)
        imageTextures = (majorVersion >= 3) && (minorVersion >= 1) && (glBindImageTexture != nullptr);
	else
		imageTextures = (((majorVersion >= 4) && (minorVersion >= 3)) || Utils::isExtensionSupported("GL_ARB_shader_image_load_store")) && (glBindImageTexture != nullptr);

#ifdef GL_NUM_PROGRAM_BINARY_FORMATS
	GLint numBinaryFormats = 0;
	glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);
	const char * strGetProgramBinary = isGLESX
		? "GL_OES_get_program_binary"
		: "GL_ARB_get_program_binary";
	shaderStorage = numBinaryFormats > 0 &&
		config.generalEmulation.enableShadersStorage != 0 &&
		Utils::isExtensionSupported(strGetProgramBinary);
#else
	shaderStorage =false;
#endif
}
