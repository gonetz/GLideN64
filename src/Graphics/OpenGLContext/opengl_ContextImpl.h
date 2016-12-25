#pragma once
#include <memory>
#include <Graphics/ContextImpl.h>
#include "opengl_TextureManipulationObjectFactory.h"

namespace opengl {

	class ContextImpl : public graphics::ContextImpl
	{
	public:
		ContextImpl();
		~ContextImpl();

		void init() override;

		void destroy() override;

		graphics::ObjectName createTexture() override;

		void deleteTexture(graphics::ObjectName _name) override;

		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel, u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat, graphics::Parameter _dataType,
			const void * _data) override;

	private:
		std::unique_ptr<Init2DTexture> m_init2DTexture;
	};

}
