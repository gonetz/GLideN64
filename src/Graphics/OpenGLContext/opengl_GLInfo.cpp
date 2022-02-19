#include <Log.h>
#include <Config.h>
#include "opengl_Utils.h"
#include "opengl_GLInfo.h"
#include <regex>
#include <Graphics/Parameters.h>

#ifdef EGL
#include <EGL/egl.h>
#endif

#ifdef OS_ANDROID
#include <Graphics/OpenGLContext/GraphicBuffer/GraphicBufferWrapper.h>
#endif

using namespace opengl;

static
void APIENTRY on_gl_error(GLenum source,
						GLenum type,
						GLuint id,
						GLenum severity,
						GLsizei length,
						const char* message,
						const void *userParam)
{
	LOG(LOG_ERROR, "%s", message);
}

void GLInfo::init() {
	const char * strDriverVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	isGLESX = strstr(strDriverVersion, "OpenGL ES") != nullptr;
	isGLES2 = strstr(strDriverVersion, "OpenGL ES 2") != nullptr;
	if (isGLES2) {
		majorVersion = 2;
		minorVersion = 0;
	} else {
		glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
		glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	}
	LOG(LOG_VERBOSE, "%s major version: %d", isGLESX ? "OpenGL ES" : "OpenGL", majorVersion);
	LOG(LOG_VERBOSE, "%s minor version: %d", isGLESX ? "OpenGL ES" : "OpenGL", minorVersion);


	LOG(LOG_VERBOSE, "OpenGL vendor: %s", glGetString(GL_VENDOR));
	const char * strRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	bool isAnyAdreno = strstr(strRenderer, "Adreno") != nullptr;

	if (std::regex_match(std::string(strRenderer), std::regex("Adreno.*530")))
		renderer = Renderer::Adreno530;
	else if (std::regex_match(std::string(strRenderer), std::regex("Adreno.*540")) ||
		std::regex_match(std::string(strRenderer), std::regex("Adreno.*6\\d\\d")))
		renderer = Renderer::Adreno_no_bugs;
	else if (strstr(strRenderer, "Adreno") != nullptr)
		renderer = Renderer::Adreno;
	else if (strstr(strRenderer, "VideoCore IV") != nullptr)
		renderer = Renderer::VideoCore;
	else if (strstr(strRenderer, "Intel") != nullptr)
		renderer = Renderer::Intel;
	else if (strstr(strRenderer, "PowerVR") != nullptr)
		renderer = Renderer::PowerVR;
	else if (strstr(strRenderer, "NVIDIA Tegra") != nullptr)
		renderer = Renderer::Tegra;
	LOG(LOG_VERBOSE, "OpenGL renderer: %s", strRenderer);

	if (strstr(strDriverVersion, "ANGLE") != nullptr)
		renderer = Renderer::Angle;

	int numericVersion = majorVersion * 10 + minorVersion;
	if (isGLES2) {
		imageTextures = false;
		msaa = false;
	} else if (isGLESX) {
		imageTextures = (numericVersion >= 31);
		msaa = numericVersion >= 31;
	} else {
		imageTextures = (numericVersion >= 42) || Utils::isExtensionSupported(*this, "GL_ARB_shader_image_load_store");
		msaa = true;
	}

	//Tegra has a buggy implementation of fragment_shader_interlock that causes graphics lockups on drivers below 390.00
	bool hasBuggyFragmentShaderInterlock = false;

	if (renderer == Renderer::Tegra) {
		std::string strDriverVersionString(strDriverVersion);
		std::string nvidiaText = "NVIDIA";
		std::size_t versionPosition = strDriverVersionString.find(nvidiaText);

		if (versionPosition == std::string::npos) {
			hasBuggyFragmentShaderInterlock = true;
		} else {
			std::string strDriverVersionNumber = strDriverVersionString.substr(versionPosition + nvidiaText.length() + 1);
			float versionNumber = std::stof(strDriverVersionNumber);
			hasBuggyFragmentShaderInterlock = versionNumber < 390.0;
		}
	}

	fragment_interlock = Utils::isExtensionSupported(*this, "GL_ARB_fragment_shader_interlock") && !hasBuggyFragmentShaderInterlock;
	fragment_interlockNV = Utils::isExtensionSupported(*this, "GL_NV_fragment_shader_interlock") && !fragment_interlock && !hasBuggyFragmentShaderInterlock;
	fragment_ordering = Utils::isExtensionSupported(*this, "GL_INTEL_fragment_shader_ordering") && !fragment_interlock && !fragment_interlockNV;

	const bool imageTexturesInterlock = imageTextures && (fragment_interlock || fragment_interlockNV || fragment_ordering);

	if (isGLES2) {
		config.generalEmulation.enableFragmentDepthWrite = 0;
		config.generalEmulation.enableHybridFilter = 0;
	}

	// Force software vertex clipping to enabled for GLES
	if (isGLESX) {
		config.generalEmulation.enableClipping = 1;
	}

	drawElementsBaseVertex = !isGLESX ||
		(Utils::isExtensionSupported(*this, "GL_EXT_draw_elements_base_vertex") || numericVersion >= 32);
#ifdef EGL
	if (isGLESX && Utils::isExtensionSupported(*this, "GL_EXT_draw_elements_base_vertex") && numericVersion < 32) {
		ptrDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) eglGetProcAddress("glDrawRangeElementsBaseVertexEXT");
	}
#endif

	bufferStorage = (!isGLESX && (numericVersion >= 44)) || Utils::isExtensionSupported(*this, "GL_ARB_buffer_storage") ||
			Utils::isExtensionSupported(*this, "GL_EXT_buffer_storage");

	texStorage = (isGLESX && (numericVersion >= 30)) || (!isGLESX && numericVersion >= 42) ||
			Utils::isExtensionSupported(*this, "GL_ARB_texture_storage");

	shaderStorage = false;
	if (config.generalEmulation.enableShadersStorage != 0) {
		const char * strGetProgramBinary = isGLESX
			? "GL_OES_get_program_binary"
			: "GL_ARB_get_program_binary";
		if ((isGLESX && numericVersion >= 30) || (!isGLESX && numericVersion >= 41) || Utils::isExtensionSupported(*this, strGetProgramBinary)) {
			GLint numBinaryFormats = 0;
			glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);
			shaderStorage = numBinaryFormats > 0;
		}
	}

	bool ext_draw_buffers_indexed = isGLESX && (Utils::isExtensionSupported(*this, "GL_EXT_draw_buffers_indexed") || numericVersion >= 32);
#ifdef EGL
	if (isGLESX && bufferStorage)
		ptrBufferStorage = (PFNGLBUFFERSTORAGEPROC) eglGetProcAddress("glBufferStorageEXT");
	if (isGLESX && numericVersion < 32) {
		if (ext_draw_buffers_indexed) {
			ptrEnablei = (PFNGLENABLEIPROC) eglGetProcAddress("glEnableiEXT");
			ptrDisablei = (PFNGLDISABLEIPROC) eglGetProcAddress("glDisableiEXT");
		} else {
			ptrEnablei = nullptr;
			ptrDisablei = nullptr;
		}
	}
	if (isGLES2 && shaderStorage) {
		ptrProgramBinary = (PFNGLPROGRAMBINARYPROC) eglGetProcAddress("glProgramBinaryOES");
		ptrGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC) eglGetProcAddress("glGetProgramBinaryOES");
		ptrProgramParameteri = nullptr;
	}
#endif
#ifndef OS_ANDROID
	if (isGLES2 && config.frameBufferEmulation.copyToRDRAM > Config::ctSync) {
		config.frameBufferEmulation.copyToRDRAM = Config::ctDisable;
		LOG(LOG_WARNING, "Async color buffer copies are not supported on GLES2");
	}
#endif
	if (isGLES2 && config.generalEmulation.enableLOD) {
		if (!Utils::isExtensionSupported(*this, "GL_EXT_shader_texture_lod") || !Utils::isExtensionSupported(*this, "GL_OES_standard_derivatives")) {
			config.generalEmulation.enableLOD = 0;
			LOG(LOG_WARNING, "LOD emulation not possible on this device");
		}
	}

	if (renderer == Renderer::PowerVR) {
		config.frameBufferEmulation.forceDepthBufferClear = 1;
		config.generalEmulation.enableFragmentDepthWrite = 0;
	}

#ifdef OS_ANDROID
	if (renderer == Renderer::Angle) {
        config.generalEmulation.enableFragmentDepthWrite = 0;
    }
#endif

	depthTexture = !isGLES2 || Utils::isExtensionSupported(*this, "GL_OES_depth_texture");
	noPerspective = Utils::isExtensionSupported(*this, "GL_NV_shader_noperspective_interpolation");

	fetch_depth = Utils::isExtensionSupported(*this, "GL_ARM_shader_framebuffer_fetch_depth_stencil");
	texture_barrier = !isGLESX && (numericVersion >= 45 || Utils::isExtensionSupported(*this, "GL_ARB_texture_barrier"));
	texture_barrierNV = Utils::isExtensionSupported(*this, "GL_NV_texture_barrier");

	ext_fetch = Utils::isExtensionSupported(*this, "GL_EXT_shader_framebuffer_fetch") && (!isGLESX || ext_draw_buffers_indexed);
	n64DepthWithFbFetch = ext_fetch && !imageTexturesInterlock;
	eglImage = (Utils::isEGLExtensionSupported("EGL_KHR_image_base") || Utils::isEGLExtensionSupported("EGL_KHR_image"));
	ext_fetch_arm =  Utils::isExtensionSupported(*this, "GL_ARM_shader_framebuffer_fetch") && !ext_fetch;

	dual_source_blending = !isGLESX || (Utils::isExtensionSupported(*this, "GL_EXT_blend_func_extended") && !isAnyAdreno);
	anisotropic_filtering = Utils::isExtensionSupported(*this, "GL_EXT_texture_filter_anisotropic");

#ifdef OS_ANDROID
	eglImage = eglImage &&
	        ( (isGLES2 && GraphicBufferWrapper::isSupportAvailable()) || (isGLESX && GraphicBufferWrapper::isPublicSupportAvailable()) ) &&
		    (renderer != Renderer::PowerVR) && (renderer != Renderer::Tegra) && (renderer != Renderer::Angle);
#endif

	if (renderer == Renderer::Intel) {
		graphics::textureTarget::TEXTURE_EXTERNAL = GL_TEXTURE_2D;
	}

	eglImageFramebuffer = eglImage && !isGLES2;

	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
		if (config.frameBufferEmulation.N64DepthCompare == Config::dcFast) {
			if (!imageTexturesInterlock && !n64DepthWithFbFetch) {
				config.frameBufferEmulation.N64DepthCompare = Config::dcDisable;
				LOG(LOG_WARNING, "Your GPU does not support the extensions needed for fast N64 Depth Compare.");
			}
		} else {
			// Compatible
			if (!imageTextures) {
				config.frameBufferEmulation.N64DepthCompare = Config::dcDisable;
				LOG(LOG_WARNING, "Your GPU does not support the extensions needed for N64 Depth Compare.");
			}
		}
	}

	coverage = dual_source_blending || ext_fetch || ext_fetch_arm;
	if (coverage) {
		GLint maxVertexAttribs = 0;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
		coverage = maxVertexAttribs >= 10;
	}

#ifdef EGL
	if (isGLESX)
	{
		ptrDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) eglGetProcAddress("glDebugMessageCallbackKHR");
		ptrDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC) eglGetProcAddress("glDebugMessageControlKHR");
	}
#endif



#ifdef GL_DEBUG
	glDebugMessageCallback(on_gl_error, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
}
