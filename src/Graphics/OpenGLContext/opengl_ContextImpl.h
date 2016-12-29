#pragma once
#include <memory>
#include <Graphics/ContextImpl.h>
#include "opengl_TextureManipulationObjectFactory.h"
#include "opengl_GLVersion.h"
#include "opengl_CachedFunctions.h"

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

		void init2DTexture(const graphics::Context::InitTextureParams & _params) override;

	private:
		std::unique_ptr<Init2DTexture> m_init2DTexture;
		GLVersion m_version;
		CachedFunctions m_cachedFunctions;
	};

}
