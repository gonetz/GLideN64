#include <Graphics/Parameters.h>
#include "opengl_GLVersion.h"
#include "opengl_CachedFunctions.h"
#include "opengl_BufferManipulationObjectFactory.h"

//#define ENABLE_GL_4_5

using namespace opengl;

/*---------------CreateFramebufferObject-------------*/

class GenFramebuffer : public CreateFramebufferObject
{
public:
	graphics::ObjectHandle createFramebuffer() override
	{
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		return graphics::ObjectHandle(fbo);
	}
};

class CreateFramebuffer : public CreateFramebufferObject
{
public:
	static bool Check(const GLVersion & _version) {
#ifdef ENABLE_GL_4_5
		return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 5);
#else
		return false;
#endif
	}

	graphics::ObjectHandle createFramebuffer() override
	{
		GLuint fbo;
		glCreateFramebuffers(1, &fbo);
		return graphics::ObjectHandle(fbo);
	}
};

/*---------------AddFramebufferTarget-------------*/

class AddFramebufferTexture2D : public AddFramebufferRenderTarget
{
public:
	AddFramebufferTexture2D(CachedBindFramebuffer * _bind) : m_bind(_bind) {}

	void addFrameBufferRenderTarget(const graphics::Context::FrameBufferRenderTarget & _params) override
	{
		m_bind->bind(_params.bufferTarget, _params.bufferHandle);
		glFramebufferTexture2D(GLenum(_params.bufferTarget),
			GLenum(_params.attachment),
			GLenum(_params.textureTarget),
			GLuint(_params.textureHandle),
			0);
	}

private:
	CachedBindFramebuffer * m_bind;
};

class AddNamedFramebufferTexture : public AddFramebufferRenderTarget
{
public:
	static bool Check(const GLVersion & _version) {
#ifdef ENABLE_GL_4_5
		return (_version.majorVersion > 4) || (_version.majorVersion == 4 && _version.minorVersion >= 5);
#else
		return false;
#endif
	}

	void addFrameBufferRenderTarget(const graphics::Context::FrameBufferRenderTarget & _params) override
	{
		glNamedFramebufferTexture(GLuint(_params.bufferHandle),
			GLenum(_params.attachment),
			GLuint(_params.textureHandle),
			0);
	}
};

/*---------------BufferManipulationObjectFactory-------------*/

BufferManipulationObjectFactory::BufferManipulationObjectFactory(const GLVersion & _version,
	CachedFunctions & _cachedFunctions)
	: m_version(_version)
	, m_cachedFunctions(_cachedFunctions)
{
}


BufferManipulationObjectFactory::~BufferManipulationObjectFactory()
{
}

CreateFramebufferObject * BufferManipulationObjectFactory::getCreateFramebufferObject() const
{
	if (CreateFramebuffer::Check(m_version))
		return new CreateFramebuffer;

	return new GenFramebuffer;
}

AddFramebufferRenderTarget * BufferManipulationObjectFactory::getAddFramebufferRenderTarget() const
{
	if (AddNamedFramebufferTexture::Check(m_version))
		return new AddNamedFramebufferTexture;

	return new AddFramebufferTexture2D(m_cachedFunctions.geCachedBindFramebuffer());
}
