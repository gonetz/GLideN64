#pragma once
#include "CombinerKey.h"

namespace graphics {

	class CombinerProgram
	{
	public:
		virtual ~CombinerProgram() {};
		virtual void activate() = 0;
		virtual void update(bool _force) = 0;
		virtual CombinerKey getKey() const = 0;
	};

}
