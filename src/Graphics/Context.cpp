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

void Context::init2DTexture(const InitTextureParams & _params)
{
	return m_impl->init2DTexture(_params);
}

bool Context::isMultisamplingSupported() const
{
	// TODO
	return true;
}
