#include <unordered_map>
#include <Graphics/Parameters.h>
#include "opengl_GLVersion.h"
#include "opengl_CachedFunctions.h"
#include "opengl_TextureManipulationObjectFactory.h"

namespace opengl {

//#define ENABLE_GL_4_5
#define ENABLE_GL_4_2

	/*---------------Create2DTexture-------------*/

	class GenTexture : public Create2DTexture
	{
	public:
		graphics::ObjectHandle createTexture(graphics::Parameter _target) override
		{
			GLuint glName;
			glGenTextures(1, &glName);
			return graphics::ObjectHandle(glName);
		}
	};

	class CreateTexture : public Create2DTexture
	{
	public:
		static bool Check(const GLVersion & _version) {
#ifdef ENABLE_GL_4_5
			return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 5);
#else
			return false;
#endif
		}

		graphics::ObjectHandle createTexture(graphics::Parameter _target) override
		{
			GLuint glName;
			glCreateTextures(GLenum(_target), 1, &glName);
			return graphics::ObjectHandle(glName);
		}
	};

	/*---------------Init2DTexture-------------*/

	class Init2DTexImage : public Init2DTexture
	{
	public:
		Init2DTexImage(CachedBindTexture* _bind) : m_bind(_bind) {}

		void init2DTexture(const graphics::Context::InitTextureParams & _params) override
		{

			if (_params.msaaLevel == 0) {
				//glBindTexture(GL_TEXTURE_2D, GLuint(_name));
				m_bind->bind(graphics::target::TEXTURE_2D, _params.handle);
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
				m_bind->bind(graphics::target::TEXTURE_2D_MULTISAMPLE, _params.handle);
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

	class Init2DTexStorage : public Init2DTexture
	{
	public:
		static bool Check(const GLVersion & _version) {
#ifdef ENABLE_GL_4_2
			return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 2);
#else
			return false;
#endif
		}

		Init2DTexStorage(CachedBindTexture* _bind) : m_bind(_bind) {}

		void init2DTexture(const graphics::Context::InitTextureParams & _params) override
		{
			if (_params.msaaLevel == 0) {
				m_bind->bind(graphics::target::TEXTURE_2D, _params.handle);
				if (m_handle != _params.handle) {
					m_handle = _params.handle;
					glTexStorage2D(GL_TEXTURE_2D,
								   _params.mipMapLevels,
								   GLenum(_params.internalFormat),
								   _params.width,
								   _params.height);
				}
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
				m_bind->bind(graphics::target::TEXTURE_2D_MULTISAMPLE, _params.handle);
				glTexStorage2DMultisample(
							GL_TEXTURE_2D_MULTISAMPLE,
							_params.msaaLevel,
							GLenum(_params.internalFormat),
							_params.width,
							_params.height,
							GL_FALSE);
			}

		}

	private:
		CachedBindTexture* m_bind;
		graphics::ObjectHandle m_handle;
	};

	class Init2DTextureStorage : public Init2DTexture
	{
	public:
		static bool Check(const GLVersion & _version) {
#ifdef ENABLE_GL_4_5
			return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 5);
#else
			return false;
#endif
		}
		void init2DTexture(const graphics::Context::InitTextureParams & _params) override
		{

			if (_params.msaaLevel == 0) {
				if (m_handle != _params.handle) {
					m_handle = _params.handle;
					glTextureStorage2D(GLuint(_params.handle),
								   _params.mipMapLevels,
								   GLenum(_params.internalFormat),
								   _params.width,
								   _params.height);
				}
				if (_params.data != nullptr)
					glTextureSubImage2D(GLuint(_params.handle),
										_params.mipMapLevel,
										0, 0,
										_params.width,
										_params.height,
										GLuint(_params.format),
										GLenum(_params.dataType),
										_params.data);
			} else {
				glTexStorage2DMultisample(GLuint(_params.handle),
										  _params.msaaLevel,
										  GLenum(_params.internalFormat),
										  _params.width,
										  _params.height,
										  GL_FALSE);
			}
		}

	private:
		graphics::ObjectHandle m_handle;
	};


	/*---------------Set2DTextureParameters-------------*/

	class SetTexParameters : public Set2DTextureParameters
	{
	public:
		SetTexParameters(CachedActiveTexture * _activeTexture, CachedBindTexture* _bind)
			: m_activeTexture(_activeTexture)
			, m_bind(_bind)	{
		}

		void setTextureParameters(const graphics::Context::TexParameters & _parameters) override
		{
			m_activeTexture->setActiveTexture(_parameters.textureUnitIndex);
			m_bind->bind(_parameters.target, _parameters.handle);
			const GLenum target(_parameters.target);
			if (_parameters.magFilter.isValid())
				glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GLint(_parameters.magFilter));
			if (_parameters.minFilter.isValid())
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GLint(_parameters.minFilter));
			if (_parameters.wrapS.isValid())
				glTexParameteri(target, GL_TEXTURE_WRAP_S, GLint(_parameters.wrapS));
			if (_parameters.wrapT.isValid())
				glTexParameteri(target, GL_TEXTURE_WRAP_T, GLint(_parameters.wrapT));
			if (_parameters.maxMipmapLevel.isValid())
				// TODO: disable for GLES2
				glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, GLint(_parameters.maxMipmapLevel));
			if (_parameters.maxAnisotropy.isValid())
				glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, GLfloat(_parameters.maxMipmapLevel));
		}

	private:
		CachedActiveTexture * m_activeTexture;
		CachedBindTexture* m_bind;
	};


	class SetTextureParameters : public Set2DTextureParameters
	{
	public:
		static bool Check(const GLVersion & _version) {
#ifdef ENABLE_GL_4_5
			return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 5);
#else
			return false;
#endif
		}

		SetTextureParameters() {}

		void setTextureParameters(const graphics::Context::TexParameters & _parameters) override
		{
			const u32 handle(_parameters.handle);
			auto it = m_parameters.find(handle);
			// TODO make cacheable
			if (it == m_parameters.end()) {
				auto res = m_parameters.emplace(handle, _parameters);
//				if (res.second)
//					return &(res.first->second);
			}

			if (_parameters.magFilter.isValid())
				glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GLint(_parameters.magFilter));
			if (_parameters.minFilter.isValid())
				glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GLint(_parameters.minFilter));
			if (_parameters.wrapS.isValid())
				glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GLint(_parameters.wrapS));
			if (_parameters.wrapT.isValid())
				glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GLint(_parameters.wrapT));
			if (_parameters.maxMipmapLevel.isValid())
				glTextureParameteri(handle, GL_TEXTURE_MAX_LEVEL, GLint(_parameters.maxMipmapLevel));
			if (_parameters.maxAnisotropy.isValid())
				glTextureParameteri(handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, GLfloat(_parameters.maxMipmapLevel));
		}

	private:
		typedef std::unordered_map<u32, graphics::Context::TexParameters> TextureParameters;
		TextureParameters m_parameters;
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

	Create2DTexture * TextureManipulationObjectFactory::getCreate2DTexture() const
	{
		if (CreateTexture::Check(m_version))
			return new CreateTexture;

		return new GenTexture;
	}

	Init2DTexture * TextureManipulationObjectFactory::getInit2DTexture() const
	{
		if (Init2DTextureStorage::Check(m_version))
			return new Init2DTextureStorage;

		if (Init2DTexStorage::Check(m_version))
			return new Init2DTexStorage(m_cachedFunctions.getCachedBindTexture());

		return new Init2DTexImage(m_cachedFunctions.getCachedBindTexture());
	}

	Set2DTextureParameters * TextureManipulationObjectFactory::getSet2DTextureParameters() const
	{
		if (SetTextureParameters::Check(m_version))
			return new SetTextureParameters;

		return new SetTexParameters(m_cachedFunctions.geCachedActiveTexture(),
			m_cachedFunctions.getCachedBindTexture());
	}

}
