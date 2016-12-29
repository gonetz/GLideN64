#pragma once
#include <Types.h>

namespace graphics {

	class Parameter
	{
	public:
		Parameter() : m_parameter(0U) {}
		Parameter(u32 _parameter) : m_parameter(_parameter) {}
		explicit operator u32() const { return m_parameter; }
		explicit operator s32() const { return static_cast<s32>(m_parameter); }

	private:
		u32 m_parameter;
	};

}
