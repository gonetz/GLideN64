#pragma once
#include <string>
#include <sstream>
#include <CombinerKey.h>

namespace glsl {

	class ShaderPart
	{
	public:
		virtual void write(std::stringstream & shader, CombinerKey _key) const
		{
			shader << m_part;
		}

	protected:
		std::string m_part;
	};

}
