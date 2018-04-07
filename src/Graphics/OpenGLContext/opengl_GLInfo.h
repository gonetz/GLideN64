#pragma once
#include "GLFunctions.h"

namespace opengl {

enum class Renderer {
	Adreno530,
	Adreno_no_bugs,
	Adreno,
	VideoCore,
	Intel,
	PowerVR,
	Other
};

struct GLInfo {
	GLint majorVersion = 0;
	GLint minorVersion = 0;
	bool isGLES2 = false;
	bool isGLESX = false;
	bool imageTextures = false;
	bool bufferStorage = false;
	bool texStorage    = false;
	bool shaderStorage = false;
	bool msaa = false;
	bool depthTexture = false;
	bool noPerspective = false;
	bool fetch_depth = false;
	bool texture_barrier = false;
	bool texture_barrierNV = false;
	Renderer renderer = Renderer::Other;

	void init();
};
}
