#include "Context.h"
#include "OpenGLContext/opengl_ContextImpl.h"

using namespace graphics;

Context gfxContext;

Context::Context() : m_impl(nullptr) {}

Context::~Context() {
	m_impl.reset(nullptr);
}

void Context::init()
{
	m_impl.reset(new opengl::ContextImpl);
	m_impl->init();
}

void Context::destroy()
{
	m_impl->destroy();
	m_impl.reset(nullptr);
}

ObjectName Context::createTexture()
{
	return m_impl->createTexture();
}

void Context::deleteTexture(ObjectName _name)
{
	return m_impl->deleteTexture(_name);
}

void Context::init2DTexture(ObjectName _name, u32 _msaaLevel, u32 _width, u32 _height, u32 _mipMapLevel,
	Parameter _format, Parameter _internalFormat, Parameter _dataType, const void * _data)
{
	return m_impl->init2DTexture(_name, _msaaLevel, _width, _height,
		_mipMapLevel, _format, _internalFormat, _dataType, _data);
}

bool Context::isMultisamplingSupported() const
{
	// TODO
	return true;
}
