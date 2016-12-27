#include <Graphics/Parameters.h>
#include "opengl_GLVersion.h"
#include "opengl_CachedFunctions.h"
#include "opengl_TextureManipulationObjectFactory.h"

namespace opengl {

	/*---------------Init2DTexture-------------*/

	class Init2DTexImage : public Init2DTexture {
	public:
		Init2DTexImage(CachedBindTexture* _bind) : m_bind(_bind) {}

		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
			u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat,
			graphics::Parameter _dataType, const void * _data) override {

			if (_msaaLevel == 0) {
				//glBindTexture(GL_TEXTURE_2D, GLuint(_name));
				m_bind->bind(graphics::target::TEXTURE_2D, _name);
				glTexImage2D(GL_TEXTURE_2D, _mipMapLevel, GLuint(_internalFormat), _width, _height, 0, GLenum(_format), GLenum(_dataType), _data);
			} else {
				//glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, GLuint(_name));
				m_bind->bind(graphics::target::TEXTURE_2D_MULTISAMPLE, _name);
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _msaaLevel, GLenum(_internalFormat), _width, _height, false);
			}
		}

	private:
		CachedBindTexture* m_bind;
	};

	class Init2DTexStorage : public Init2DTexture {
	public:
		static bool Check(const GLVersion & _version) {
//			return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 2);
			return false;
		}

		Init2DTexStorage(CachedBindTexture* _bind) : m_bind(_bind) {}

		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
			u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat,
			graphics::Parameter _dataType, const void * _data) override {

			if (_msaaLevel == 0) {
				m_bind->bind(graphics::target::TEXTURE_2D, _name);
				glTexStorage2D(GL_TEXTURE_2D, _mipMapLevel, GLenum(_internalFormat), _width, _height);
				if (_data != nullptr)
					glTexSubImage2D(GL_TEXTURE_2D, _mipMapLevel, 0, 0, _width, _height, GLuint(_format), GLenum(_dataType), _data);
			} else {
				m_bind->bind(graphics::target::TEXTURE_2D_MULTISAMPLE, _name);
				glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _msaaLevel, GLenum(_internalFormat), _width, _height, false);
			}

		}

	private:
		CachedBindTexture* m_bind;
	};

	class Init2DTextureStorage : public Init2DTexture {
	public:
		static bool Check(const GLVersion & _version) {
//			return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 5);
			return false;
		}
		void init2DTexture(graphics::ObjectName _name, u32 _msaaLevel,
			u32 _width, u32 _height, u32 _mipMapLevel,
			graphics::Parameter _format, graphics::Parameter _internalFormat,
			graphics::Parameter _dataType, const void * _data) override {

			if (_msaaLevel == 0) {
				glTextureStorage2D(GLuint(_name), _mipMapLevel, GLenum(_internalFormat), _width, _height);
				if (_data != nullptr)
					glTextureSubImage2D(GLuint(_name), _mipMapLevel, 0, 0, _width, _height, GLuint(_format), GLenum(_dataType), _data);
			} else {
				glTexStorage2DMultisample(GLuint(_name), _msaaLevel, GLenum(_internalFormat), _width, _height, false);
			}
		}
	};


	/*---------------TextureManipulationObjectFactory-------------*/

	TextureManipulationObjectFactory::TextureManipulationObjectFactory(const GLVersion & _version,
		CachedFunctions & _cachedFunctions)
		: m_version(_version)
		, m_cachedFunctions(_cachedFunctions)
	{
	}

	TextureManipulationObjectFactory::~TextureManipulationObjectFactory()
	{
	}

	Init2DTexture * TextureManipulationObjectFactory::getInit2DTexture() const
	{
		if (Init2DTextureStorage::Check(m_version))
			return new Init2DTextureStorage;

		if (Init2DTexStorage::Check(m_version))
			return new Init2DTexStorage(m_cachedFunctions.getCachedBindTexture());

		return new Init2DTexImage(m_cachedFunctions.getCachedBindTexture());
	}

}