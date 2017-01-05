#pragma once
#include <Graphics/OpenGLContext/opengl_GLInfo.h>
#include "glsl_CombinerProgramImpl.h"

namespace glsl {

	class CombinerProgramUniformFactory
	{
	public:
		CombinerProgramUniformFactory(const opengl::GLInfo & _glInfo);
		~CombinerProgramUniformFactory();

		void buildUniforms(GLuint _program, const CombinerInputs & _inputs, bool _rect, UniformGroups & _uniforms);

	private:
		const opengl::GLInfo & m_glInfo;
	};

}
