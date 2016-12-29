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

		struct InitTextureParams {
			ObjectName name;
			u32 msaaLevel = 0;
			u32 width = 0;
			u32 height = 0;
			u32 mipMapLevel = 0;
			Parameter format;
			Parameter internalFormat;
			Parameter dataType;
			const void * data;
		};

		void init2DTexture(const InitTextureParams & _params);

		bool isMultisamplingSupported() const;

	private:
		std::unique_ptr<ContextImpl> m_impl;
	};

}

extern graphics::Context gfxContext;
