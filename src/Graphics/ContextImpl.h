#pragma once
#include "ObjectName.h"
#include "Parameter.h"

namespace graphics {

	class ContextImpl
	{
	public:
		virtual ~ContextImpl() {}
		virtual void init() = 0;
		virtual void destroy() = 0;
		virtual ObjectName createTexture() = 0;
		virtual void deleteTexture(ObjectName _name) = 0;
		virtual void init2DTexture(ObjectName _name, u32 _msaaLevel, u32 _width, u32 _height, u32 _mipMapLevel,
			Parameter _format, Parameter _internalFormat, Parameter _dataType, const void * _data) = 0;
	};

}
