#pragma once

#include <memory>
#include "ObjectName.h"
#include "Parameter.h"

#define GRAPHICS_CONTEXT

namespace graphics {

	class ContextImpl;

	class Context
	{
	public:
		Context();
		~Context();

		void init();

		void destroy();

		ObjectName createTexture();

		void deleteTexture(ObjectName _name);

		void init2DTexture(ObjectName _name, u32 _msaaLevel, u32 _width, u32 _height, u32 _mipMapLevel,
			Parameter _format, Parameter _internalFormat, Parameter _dataType, const void * _data);

		bool isMultisamplingSupported() const;

	private:
		std::unique_ptr<ContextImpl> m_impl;
	};

}

extern graphics::Context gfxContext;
