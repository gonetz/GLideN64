#include <assert.h>
#include <Log.h>
#include "opengl_ContextImpl.h"
#include "opengl_GLVersion.h"
#include "opengl_CachedFunctions.h"

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

	GLVersion version;
	glGetIntegerv(GL_MAJOR_VERSION, &version.majorVersion);
	LOG(LOG_VERBOSE, "OpenGL major version: %d\n", version.majorVersion);
	assert(version.majorVersion >= 3 && "Plugin requires GL version 3 or higher.");
	GLint minorVersion = 0;
	glGetIntegerv(GL_MINOR_VERSION, &version.minorVersion);
	LOG(LOG_VERBOSE, "OpenGL minor version: %d\n", version.minorVersion);

	CachedFunctions cachedFunctions;

	TextureManipulationObjectFactory textureObjectsFactory(version, cachedFunctions);
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

void ContextImpl::init2DTexture(graphics::ObjectName _name, u32 _msaaLevel, u32 _width, u32 _height, u32 _mipMapLevel,
	graphics::Parameter _format, graphics::Parameter _internalFormat, graphics::Parameter _dataType,
	const void * _data)
{
	assert(m_init2DTexture);
	m_init2DTexture->init2DTexture(_name, _msaaLevel, _width, _height,
		_mipMapLevel, _format, _internalFormat, _dataType, _data);
}
