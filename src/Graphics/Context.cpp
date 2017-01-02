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

ObjectHandle Context::createTexture(Parameter _target)
{
	return m_impl->createTexture(_target);
}

void Context::deleteTexture(ObjectHandle _name)
{
	m_impl->deleteTexture(_name);
}

void Context::init2DTexture(const InitTextureParams & _params)
{
	m_impl->init2DTexture(_params);
}

void Context::setTextureParameters(const TexParameters & _parameters)
{
	m_impl->setTextureParameters(_parameters);
}

ObjectHandle Context::createFramebuffer()
{
	return m_impl->createFramebuffer();
}

void Context::deleteFramebuffer(ObjectHandle _name)
{
	m_impl->deleteFramebuffer(_name);
}

void Context::addFrameBufferRenderTarget(const FrameBufferRenderTarget & _params)
{
	m_impl->addFrameBufferRenderTarget(_params);
}

bool Context::isMultisamplingSupported() const
{
	// TODO
	return true;
}
