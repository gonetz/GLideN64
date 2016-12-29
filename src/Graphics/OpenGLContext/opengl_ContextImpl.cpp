#include <assert.h>
#include <Log.h>
#include "opengl_ContextImpl.h"

using namespace opengl;

ContextImpl::ContextImpl()
: m_init2DTexture(nullptr)
{
}


ContextImpl::~ContextImpl()
{
	m_init2DTexture.reset(nullptr);
}

void ContextImpl::init()
{
	initGLFunctions();

	glGetIntegerv(GL_MAJOR_VERSION, &m_version.majorVersion);
	LOG(LOG_VERBOSE, "OpenGL major version: %d\n", m_version.majorVersion);
	assert(m_version.majorVersion >= 3 && "Plugin requires GL version 3 or higher.");
	glGetIntegerv(GL_MINOR_VERSION, &m_version.minorVersion);
	LOG(LOG_VERBOSE, "OpenGL minor version: %d\n", m_version.minorVersion);

	TextureManipulationObjectFactory textureObjectsFactory(m_version, m_cachedFunctions);
	m_init2DTexture.reset(textureObjectsFactory.getInit2DTexture());

}

void ContextImpl::destroy()
{

}

graphics::ObjectName ContextImpl::createTexture()
{
	GLuint glName;
	glGenTextures(1, &glName);
	return graphics::ObjectName(static_cast<u32>(glName));
}

void ContextImpl::deleteTexture(graphics::ObjectName _name)
{
	u32 glName(_name);
	glDeleteTextures(1, &glName);
}

void ContextImpl::init2DTexture(const graphics::Context::InitTextureParams & _params)
{
	assert(m_init2DTexture);
	m_init2DTexture->init2DTexture(_params);
}
