#include <Log.h>
#include <Config.h>
#include "opengl_Utils.h"
#include "opengl_GLInfo.h"
#ifdef EGL
#include <EGL/egl.h>
#endif

using namespace opengl;

void GLInfo::init() {
	const char * strVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	isGLESX = strstr(strVersion, "OpenGL ES") != nullptr;
	isGLES2 = strstr(strVersion, "OpenGL ES 2") != nullptr;
	if (isGLES2) {
		majorVersion = 2;
		minorVersion = 0;
	} else {
		glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	}
	LOG(LOG_VERBOSE, "%s major version: %d\n", isGLESX ? "OpenGL ES" : "OpenGL", majorVersion);
	LOG(LOG_VERBOSE, "%s minor version: %d\n", isGLESX ? "OpenGL ES" : "OpenGL", minorVersion);


	LOG(LOG_VERBOSE, "OpenGL vendor: %s\n", glGetString(GL_VENDOR));
	const GLubyte * strRenderer = glGetString(GL_RENDERER);
	if (strstr((const char*)strRenderer, "Adreno") != nullptr)
		renderer = Renderer::Adreno;
	LOG(LOG_VERBOSE, "OpenGL renderer: %s\n", strRenderer);

	int numericVersion = majorVersion * 10 + minorVersion;
	if (isGLES2) {
		imageTextures = false;
		msaa = false;
	} else if (isGLESX) {
		imageTextures = (numericVersion >= 31) && (glBindImageTexture != nullptr);
		msaa = numericVersion >= 31;
	} else {
		imageTextures = ((numericVersion >= 43) || (Utils::isExtensionSupported(*this, "GL_ARB_shader_image_load_store") &&
				Utils::isExtensionSupported(*this, "GL_ARB_compute_shader"))) && (glBindImageTexture != nullptr);
		msaa = true;
	}
	bufferStorage = (!isGLESX && (numericVersion >= 44)) || Utils::isExtensionSupported(*this, "GL_ARB_buffer_storage") ||
			Utils::isExtensionSupported(*this, "GL_EXT_buffer_storage");
	copyImage = (isGLESX && (numericVersion >= 32)) || (!isGLESX && numericVersion >= 43) ||
			Utils::isExtensionSupported(*this, "GL_ARB_copy_image") || Utils::isExtensionSupported(*this, "GL_EXT_copy_image");
#ifdef EGL
	if (isGLESX && bufferStorage)
		g_glBufferStorage = (PFNGLBUFFERSTORAGEPROC) eglGetProcAddress("glBufferStorageEXT");
	if (isGLEX && numericVersion < 32)
		g_glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC) eglGetProcAddress("glCopyImageSubDataEXT");
#endif
	texStorage = (isGLESX && (numericVersion >= 30)) || (!isGLESX && numericVersion >= 42) ||
			Utils::isExtensionSupported(*this, "GL_ARB_texture_storage");

	shaderStorage = false;
	if (config.generalEmulation.enableShadersStorage != 0) {
		const char * strGetProgramBinary = isGLESX
			? "GL_OES_get_program_binary"
			: "GL_ARB_get_program_binary";
		if (Utils::isExtensionSupported(*this, strGetProgramBinary)) {
			GLint numBinaryFormats = 0;
			glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);
			shaderStorage = numBinaryFormats > 0;
		}
	}
}
