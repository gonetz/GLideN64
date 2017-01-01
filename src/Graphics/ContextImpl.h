#pragma once
#include "ObjectHandle.h"
#include "Parameter.h"

#include "Context.h"

namespace graphics {

	class ContextImpl
	{
	public:
		virtual ~ContextImpl() {}
		virtual void init() = 0;
		virtual void destroy() = 0;
		virtual ObjectHandle createTexture(Parameter _target) = 0;
		virtual void deleteTexture(ObjectHandle _name) = 0;
		virtual void init2DTexture(const Context::InitTextureParams & _params) = 0;
		virtual void setTextureParameters(const Context::TexParameters & _parameters) = 0;
	};

}
