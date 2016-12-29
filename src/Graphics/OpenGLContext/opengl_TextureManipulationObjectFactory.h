#pragma once
#include <Graphics/ObjectName.h>
#include <Graphics/Parameter.h>
#include <Graphics/Context.h>

namespace opengl {

	struct GLVersion;
	class CachedFunctions;

	class Init2DTexture {
	public:
		virtual ~Init2DTexture() {};
		virtual void init2DTexture(const graphics::Context::InitTextureParams & _params) = 0;
	};

	class TextureManipulationObjectFactory
	{
	public:
		TextureManipulationObjectFactory(const GLVersion & _version, CachedFunctions & _cachedFunctions);
		~TextureManipulationObjectFactory();

		Init2DTexture * getInit2DTexture() const;

	private:
		const GLVersion & m_version;
		CachedFunctions & m_cachedFunctions;
	};

}
