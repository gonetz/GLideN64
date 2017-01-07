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

void Context::update2DTexture(const UpdateTextureDataParams & _params)
{
	m_impl->update2DTexture(_params);
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

ObjectHandle Context::createRenderbuffer()
{
	return m_impl->createRenderbuffer();
}

void Context::initRenderbuffer(const InitRenderbufferParams & _params)
{
	m_impl->initRenderbuffer(_params);
}

void Context::addFrameBufferRenderTarget(const FrameBufferRenderTarget & _params)
{
	m_impl->addFrameBufferRenderTarget(_params);
}

PixelWriteBuffer * Context::createPixelWriteBuffer(size_t _sizeInBytes)
{
	return m_impl->createPixelWriteBuffer(_sizeInBytes);
}

CombinerProgram * Context::createCombinerProgram(Combiner & _color, Combiner & _alpha, const CombinerKey & _key)
{
	return m_impl->createCombinerProgram(_color, _alpha, _key);
}

bool Context::saveShadersStorage(const Combiners & _combiners)
{
	return m_impl->saveShadersStorage(_combiners);
}

bool Context::loadShadersStorage(Combiners & _combiners)
{
	return m_impl->loadShadersStorage(_combiners);
}

ShaderProgram * Context::createDepthFogShader()
{
	return m_impl->createDepthFogShader();
}

ShaderProgram * Context::createMonochromeShader()
{
	return m_impl->createMonochromeShader();
}

bool Context::isMultisamplingSupported() const
{
	// TODO
	return true;
}
