#include "Context.h"
#include "OpenGLContext/OpenGLContextImpl.h"

using namespace graphics;

Context gfxContext;

Context::Context() : m_impl(nullptr) {}

Context::~Context() {
	m_impl.reset(nullptr);
}

void Context::init()
{
	m_impl.reset(new opengl::OpenGLContextImpl);
	m_impl->init();
}

void Context::destroy()
{
	m_impl->destroy();
	m_impl.reset(nullptr);
}

ObjectName Context::createTexture() const
{
	return m_impl->createTexture();
}

void Context::deleteTexture(ObjectName _name) const
{
	return m_impl->deleteTexture(_name);
}
