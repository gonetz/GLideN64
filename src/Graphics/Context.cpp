#include "Context.h"
#include "OpenGLContext/opengl_ContextImpl.h"

using namespace graphics;

Context gfxContext;

Context::Context() {}

Context::~Context() {
	m_impl.reset();
}

void Context::init()
{
	m_impl.reset(new opengl::ContextImpl);
	m_impl->init();
}

void Context::destroy()
{
	m_impl->destroy();
	m_impl.reset();
}

void Context::enable(Parameter _parameter, bool _enable)
{
	m_impl->enable(_parameter, _enable);
}

void Context::cullFace(Parameter _parameter)
{
	m_impl->cullFace(_parameter);
}

void Context::enableDepthWrite(bool _enable)
{
	m_impl->enableDepthWrite(_enable);
}

void Context::setDepthCompare(Parameter _mode)
{
	m_impl->setDepthCompare(_mode);
}

void Context::setViewport(s32 _x, s32 _y, s32 _width, s32 _height)
{
	m_impl->setViewport(_x, _y, _width, _height);
}

void Context::setScissor(s32 _x, s32 _y, s32 _width, s32 _height)
{
	m_impl->setScissor(_x, _y, _width, _height);
}

void Context::setBlending(Parameter _sfactor, Parameter _dfactor)
{
	m_impl->setBlending(_sfactor, _dfactor);
}

void Context::setBlendColor(f32 _red, f32 _green, f32 _blue, f32 _alpha)
{
	m_impl->setBlendColor(_red, _green, _blue, _alpha);
}

void Context::clearColorBuffer(f32 _red, f32 _green, f32 _blue, f32 _alpha)
{
	m_impl->clearColorBuffer(_red, _green, _blue, _alpha);
}

void Context::clearDepthBuffer()
{
	m_impl->clearDepthBuffer();
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

TexDrawerShaderProgram * Context::createTexDrawerDrawShader()
{
	return m_impl->createTexDrawerDrawShader();
}

ShaderProgram * Context::createTexDrawerClearShader()
{
	return m_impl->createTexDrawerClearShader();
}

ShaderProgram * Context::createTexrectCopyShader()
{
	return m_impl->createTexrectCopyShader();
}

DrawerImpl * Context::createDrawerImpl()
{
	return m_impl->createDrawerImpl();
}

TextDrawer * Context::createTextDrawer()
{
	return m_impl->createTextDrawer();
}

f32 Context::getMaxLineWidth()
{
	return m_impl->getMaxLineWidth();
}

bool Context::isSupported(SpecialFeatures _feature) const
{
	// TODO
	return true;
}