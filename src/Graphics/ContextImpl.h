#pragma once
#include "ObjectName.h"
#include "Parameter.h"

#include "Context.h"

namespace graphics {

	class ContextImpl
	{
	public:
		virtual ~ContextImpl() {}
		virtual void init() = 0;
		virtual void destroy() = 0;
		virtual ObjectName createTexture() = 0;
		virtual void deleteTexture(ObjectName _name) = 0;
		virtual void init2DTexture(const Context::InitTextureParams & _params) = 0;
	};

}
