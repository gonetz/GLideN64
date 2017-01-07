#pragma once
#include <Graphics/OpenGLContext/opengl_GLInfo.h>

namespace glsl {

	class ShaderStorage
	{
	public:
		ShaderStorage(const opengl::GLInfo & _glinfo);

		bool saveShadersStorage(const graphics::Combiners & _combiners) const;

		bool loadShadersStorage(graphics::Combiners & _combiners);

	private:
		const opengl::GLInfo & m_glinfo;
	};

}
