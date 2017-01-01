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

	m_createTexture.reset(textureObjectsFactory.getCreate2DTexture());
	m_init2DTexture.reset(textureObjectsFactory.getInit2DTexture());
	m_set2DTextureParameters.reset(textureObjectsFactory.getSet2DTextureParameters());
}

void ContextImpl::destroy()
{

}

graphics::ObjectHandle ContextImpl::createTexture(graphics::Parameter _target)
{
	return m_createTexture->createTexture(_target);
}

void ContextImpl::deleteTexture(graphics::ObjectHandle _name)
{
	u32 glName(_name);
	glDeleteTextures(1, &glName);
}

void ContextImpl::init2DTexture(const graphics::Context::InitTextureParams & _params)
{
	assert(m_init2DTexture);
	m_init2DTexture->init2DTexture(_params);
}

void ContextImpl::setTextureParameters(const graphics::Context::TexParameters & _parameters)
{
	assert(m_set2DTextureParameters);
	m_set2DTextureParameters->setTextureParameters(_parameters);
}
