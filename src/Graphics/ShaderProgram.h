#pragma once
#include <Types.h>

namespace graphics {

	class ShaderProgram
	{
	public:
		virtual ~ShaderProgram() {}
		virtual void activate() = 0;
	};

	class TexDrawerShaderProgram : public ShaderProgram
	{
	public:
		virtual void setTextureSize(u32 _width, u32 _height) = 0;
		virtual void setTextureBounds(float _texBounds[4]) = 0;
		virtual void setEnableAlphaTest(int _enable) = 0;
	};

}
