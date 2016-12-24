#pragma once
#include "ObjectName.h"

namespace graphics {

	class ContextImpl
	{
	public:
		virtual ~ContextImpl() {}
		virtual void init() = 0;
		virtual void destroy() = 0;
		virtual ObjectName createTexture() const = 0;
		virtual void deleteTexture(ObjectName _name) const = 0;
	};

}
