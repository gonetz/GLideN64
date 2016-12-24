#pragma once
#include <Types.h>

namespace graphics {

	class ObjectName
	{
	public:
		explicit ObjectName(u32 _name) : m_name(_name) {}
		explicit operator u32() const { return m_name; }

	private:
		u32 m_name;
	};

}
