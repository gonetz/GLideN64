#pragma once

namespace graphics {

	class ShaderProgram
	{
	public:
		virtual ~ShaderProgram() {};
		virtual void activate() = 0;
	};

}

