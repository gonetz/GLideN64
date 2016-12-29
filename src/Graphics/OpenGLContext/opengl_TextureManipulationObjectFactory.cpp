#include <Graphics/Parameters.h>
#include "opengl_GLVersion.h"
#include "opengl_CachedFunctions.h"
#include "opengl_TextureManipulationObjectFactory.h"

namespace opengl {

	/*---------------Init2DTexture-------------*/

	class Init2DTexImage : public Init2DTexture {
	public:
		Init2DTexImage(CachedBindTexture* _bind) : m_bind(_bind) {}

		void init2DTexture(const graphics::Context::InitTextureParams & _params) override
		{

			if (_params.msaaLevel == 0) {
				//glBindTexture(GL_TEXTURE_2D, GLuint(_name));
				m_bind->bind(graphics::target::TEXTURE_2D, _params.name);
				glTexImage2D(GL_TEXTURE_2D,
							 _params.mipMapLevel,
							 GLuint(_params.internalFormat),
							 _params.width,
							 _params.height,
							 0,
							 GLenum(_params.format),
							 GLenum(_params.dataType),
							 _params.data);
			} else {
				//glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, GLuint(_name));
				m_bind->bind(graphics::target::TEXTURE_2D_MULTISAMPLE, _params.name);
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
										_params.msaaLevel,
										GLenum(_params.internalFormat),
										_params.width,
										_params.height,
										false);
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

		void init2DTexture(const graphics::Context::InitTextureParams & _params) override
		{
			if (_params.msaaLevel == 0) {
				m_bind->bind(graphics::target::TEXTURE_2D, _params.name);
				glTexStorage2D(GL_TEXTURE_2D,
							   _params.mipMapLevel,
							   GLenum(_params.internalFormat),
							   _params.width,
							   _params.height);
				if (_params.data != nullptr)
					glTexSubImage2D(GL_TEXTURE_2D,
									_params.mipMapLevel,
									0, 0,
									_params.width,
									_params.height,
									GLuint(_params.format),
									GLenum(_params.dataType),
									_params.data);
			} else {
				m_bind->bind(graphics::target::TEXTURE_2D_MULTISAMPLE, _params.name);
				glTexStorage2DMultisample(
							GL_TEXTURE_2D_MULTISAMPLE,
							_params.msaaLevel,
							GLenum(_params.internalFormat),
							_params.width,
							_params.height,
							false);
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
		void init2DTexture(const graphics::Context::InitTextureParams & _params) override
		{

			if (_params.msaaLevel == 0) {
				glTextureStorage2D(GLuint(_params.name),
								   _params.mipMapLevel,
								   GLenum(_params.internalFormat),
								   _params.width,
								   _params.height);
				if (_params.data != nullptr)
					glTextureSubImage2D(GLuint(_params.name),
										_params.mipMapLevel,
										0, 0,
										_params.width,
										_params.height,
										GLuint(_params.format),
										GLenum(_params.dataType),
										_params.data);
			} else {
				glTexStorage2DMultisample(GLuint(_params.name),
										  _params.msaaLevel,
										  GLenum(_params.internalFormat),
										  _params.width,
										  _params.height,
										  false);
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
