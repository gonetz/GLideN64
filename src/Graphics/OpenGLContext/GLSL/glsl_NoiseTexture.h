#pragma once
#include <Types.h>
#include <Graphics/OpenGLContext/GLFunctions.h>

struct CachedTexture;

namespace glsl {

	class NoiseTexture
	{
	public:
		NoiseTexture();
		~NoiseTexture();

		void init();
		void destroy();
		void update();

	private:
		CachedTexture * m_pTexture;
		GLuint m_PBO;
		u32 m_DList;
	};
}
