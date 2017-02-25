#pragma once
#include <string>
#include <sstream>

namespace glsl {

	class ShaderPart
	{
	public:
		void write(std::stringstream & shader) const
		{
			shader << m_part;
		}

	protected:
		std::string m_part;
	};

}
