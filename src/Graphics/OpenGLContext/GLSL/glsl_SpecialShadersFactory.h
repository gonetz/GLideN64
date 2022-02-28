#pragma once
#include <Graphics/OpenGLContext/opengl_GLInfo.h>

namespace graphics {
	class ShaderProgram;
}

namespace opengl {
	class CachedUseProgram;
}

namespace glsl {

	class ShaderPart;

	class SpecialShadersFactory
	{
	public:
		SpecialShadersFactory(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd);

		graphics::ShaderProgram * createShadowMapShader() const;

		graphics::TexrectDrawerShaderProgram * createTexrectDrawerDrawShader() const;

		graphics::ShaderProgram * createTexrectDrawerClearShader() const;

		graphics::ShaderProgram * createTexrectUpscaleCopyShader() const;

		graphics::ShaderProgram * createTexrectColorAndDepthUpscaleCopyShader() const;

		graphics::ShaderProgram * createTexrectDownscaleCopyShader() const;

		graphics::ShaderProgram * createTexrectColorAndDepthDownscaleCopyShader() const;

		graphics::ShaderProgram * createGammaCorrectionShader() const;

		graphics::ShaderProgram * createFXAAShader() const;

		graphics::TextDrawerShaderProgram * createTextDrawerShader() const;

	private:
		const opengl::GLInfo & m_glinfo;
		const ShaderPart * m_vertexHeader;
		const ShaderPart * m_fragmentHeader;
		const ShaderPart * m_fragmentEnd;
		opengl::CachedUseProgram * m_useProgram;
	};

}
