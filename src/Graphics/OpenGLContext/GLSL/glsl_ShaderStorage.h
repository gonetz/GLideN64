#pragma once
#include <Graphics/OpenGLContext/opengl_GLInfo.h>

namespace opengl {
	class CachedUseProgram;
}

namespace glsl {

	class ShaderStorage
	{
	public:
		ShaderStorage(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram);

		bool saveShadersStorage(const graphics::Combiners & _combiners) const;

		bool loadShadersStorage(graphics::Combiners & _combiners);

	private:
		const opengl::GLInfo & m_glinfo;
		opengl::CachedUseProgram * m_useProgram;
	};

}
