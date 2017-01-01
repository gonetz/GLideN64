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

		graphics::ObjectHandle createTexture(graphics::Parameter _target) override;

		void deleteTexture(graphics::ObjectHandle _name) override;

		void init2DTexture(const graphics::Context::InitTextureParams & _params) override;

		void setTextureParameters(const graphics::Context::TexParameters & _parameters) override;

	private:
		std::unique_ptr<Create2DTexture> m_createTexture;
		std::unique_ptr<Init2DTexture> m_init2DTexture;
		std::unique_ptr<Set2DTextureParameters> m_set2DTextureParameters;
		GLVersion m_version;
		CachedFunctions m_cachedFunctions;
	};

}
