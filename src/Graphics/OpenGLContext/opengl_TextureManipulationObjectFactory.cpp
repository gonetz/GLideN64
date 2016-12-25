#include "opengl_GLVersion.h"
#include "opengl_TextureManipulationObjectFactory.h"

namespace opengl {

	TextureManipulationObjectFactory::TextureManipulationObjectFactory(const GLVersion & _version)
		: m_version(_version)
	{
	}

	TextureManipulationObjectFactory::~TextureManipulationObjectFactory()
	{
	}

	/*---------------Init2DTexture-------------*/

	class Init2DTexImage : public Init2DTexture {
	public:
		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
			u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat,
			graphics::Parameter _dataType, const void * _data) override {

		}
	};

	class Init2DTexStorage : public Init2DTexture {
	public:
		static bool Check(const GLVersion & _version) {
			return false;
		}

		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
			u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat,
			graphics::Parameter _dataType, const void * _data) override {

		}
	};

	class Init2DTextureStorage : public Init2DTexture {
	public:
		static bool Check(const GLVersion & _version) {
			return false;
		}

		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
			u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat,
			graphics::Parameter _dataType, const void * _data) override {

		}
	};


	Init2DTexture * TextureManipulationObjectFactory::getInit2DTexture() const
	{
		if (Init2DTextureStorage::Check(m_version))
			return new Init2DTextureStorage;

		if (Init2DTexStorage::Check(m_version))
			return new Init2DTexStorage;

		return new Init2DTexImage;
	}

}