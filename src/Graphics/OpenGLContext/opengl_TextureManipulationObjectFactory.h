#pragma once
#include <Graphics/ObjectHandle.h>
#include <Graphics/Parameter.h>
#include <Graphics/Context.h>

namespace opengl {

	struct GLVersion;
	class CachedFunctions;

	class Create2DTexture
	{
	public:
		virtual ~Create2DTexture() {};
		virtual graphics::ObjectHandle createTexture(graphics::Parameter _target) = 0;
	};

	class Init2DTexture
	{
	public:
		virtual ~Init2DTexture() {};
		virtual void init2DTexture(const graphics::Context::InitTextureParams & _params) = 0;
	};

	class Set2DTextureParameters
	{
	public:
		virtual ~Set2DTextureParameters() {}
		virtual void setTextureParameters(const graphics::Context::TexParameters & _parameters) = 0;
	};

	class TextureManipulationObjectFactory
	{
	public:
		TextureManipulationObjectFactory(const GLVersion & _version, CachedFunctions & _cachedFunctions);
		~TextureManipulationObjectFactory();

		Create2DTexture * getCreate2DTexture() const;

		Init2DTexture * getInit2DTexture() const;

		Set2DTextureParameters * getSet2DTextureParameters() const;

	private:
		const GLVersion & m_version;
		CachedFunctions & m_cachedFunctions;
	};

}
