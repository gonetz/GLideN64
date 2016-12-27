#pragma once
#include <Graphics/ObjectName.h>
#include <Graphics/Parameter.h>

namespace opengl {

	struct GLVersion;
	class CachedFunctions;

	class Init2DTexture {
	public:
		virtual ~Init2DTexture() {};
		virtual void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
									u32 _width, u32 _height, u32 _mipMapLevel,
									graphics::Parameter _format, graphics::Parameter _internalFormat,
									graphics::Parameter _dataType, const void * _data) = 0;
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