#pragma once
#include "ObjectHandle.h"
#include "Parameter.h"

#include "Context.h"

namespace graphics {

	class ContextImpl
	{
	public:
		virtual ~ContextImpl() {}
		virtual void init() = 0;
		virtual void destroy() = 0;
		virtual void enable(Parameter _parameter, bool _enable) = 0;
		virtual void cullFace(Parameter _mode) = 0;
		virtual void enableDepthWrite(bool _enable) = 0;
		virtual void setDepthCompare(Parameter _mode) = 0;
		virtual void setViewport(s32 _x, s32 _y, s32 _width, s32 _height) = 0;
		virtual void setScissor(s32 _x, s32 _y, s32 _width, s32 _height) = 0;
		virtual void setBlending(Parameter _sfactor, Parameter _dfactor) = 0;
		virtual void setBlendColor(f32 _red, f32 _green, f32 _blue, f32 _alpha) = 0;
		virtual ObjectHandle createTexture(Parameter _target) = 0;
		virtual void deleteTexture(ObjectHandle _name) = 0;
		virtual void init2DTexture(const Context::InitTextureParams & _params) = 0;
		virtual void update2DTexture(const Context::UpdateTextureDataParams & _params) = 0;
		virtual void setTextureParameters(const Context::TexParameters & _parameters) = 0;
		virtual ObjectHandle createFramebuffer() = 0;
		virtual void deleteFramebuffer(ObjectHandle _name) = 0;
		virtual void addFrameBufferRenderTarget(const Context::FrameBufferRenderTarget & _params) = 0;
		virtual ObjectHandle createRenderbuffer() = 0;
		virtual void initRenderbuffer(const Context::InitRenderbufferParams & _params) = 0;
		virtual PixelWriteBuffer * createPixelWriteBuffer(size_t _sizeInBytes) = 0;
		virtual CombinerProgram * createCombinerProgram(Combiner & _color, Combiner & _alpha, const CombinerKey & _key) = 0;
		virtual bool saveShadersStorage(const Combiners & _combiners) = 0;
		virtual bool loadShadersStorage(Combiners & _combiners) = 0;
		virtual ShaderProgram * createDepthFogShader() = 0;
		virtual ShaderProgram * createMonochromeShader() = 0;
		virtual TexDrawerShaderProgram * createTexDrawerDrawShader() = 0;
		virtual ShaderProgram * createTexDrawerClearShader() = 0;
		virtual ShaderProgram * createTexrectCopyShader() = 0;
		virtual DrawerImpl * createDrawerImpl() = 0;
	};

}
