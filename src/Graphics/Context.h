#pragma once

#include <memory>
#include <Combiner.h>
#include "ObjectHandle.h"
#include "Parameter.h"
#include "CombinerProgram.h"
#include "ShaderProgram.h"
#include "PixelBuffer.h"

#define GRAPHICS_CONTEXT

namespace graphics {

	enum class SpecialFeatures {
		Multisampling,
		NearPlaneClipping,
		FragmentDepthWrite
	};

	class ContextImpl;

	class Context
	{
	public:
		Context();
		~Context();

		void init();

		void destroy();

		void enable(Parameter _parameter, bool _enable);

		void cullFace(Parameter _mode);

		void enableDepthWrite(bool _enable);

		void setDepthCompare(Parameter _mode);

		void setViewport(s32 _x, s32 _y, s32 _width, s32 _height);

		void setScissor(s32 _x, s32 _y, s32 _width, s32 _height);

		void setBlending(Parameter _sfactor, Parameter _dfactor);

		void setBlendColor(f32 _red, f32 _green, f32 _blue, f32 _alpha);

		ObjectHandle createTexture(Parameter _target);

		void deleteTexture(ObjectHandle _name);

		struct InitTextureParams {
			ObjectHandle handle;
			Parameter ImageUnit;
			u32 msaaLevel = 0;
			u32 width = 0;
			u32 height = 0;
			u32 mipMapLevel = 0;
			u32 mipMapLevels = 1;
			Parameter format;
			Parameter internalFormat;
			Parameter dataType;
			const void * data = nullptr;
		};

		void init2DTexture(const InitTextureParams & _params);

		struct UpdateTextureDataParams {
			ObjectHandle handle;
			Parameter ImageUnit;
			Parameter textureUnitIndex = Parameter(0U);
			u32 x = 0;
			u32 y = 0;
			u32 width = 0;
			u32 height = 0;
			u32 mipMapLevel = 0;
			Parameter format;
			Parameter internalFormat;
			Parameter dataType;
			const void * data = nullptr;
		};

		void update2DTexture(const UpdateTextureDataParams & _params);

		struct TexParameters {
			ObjectHandle handle;
			Parameter textureUnitIndex = Parameter(0U);
			Parameter target;
			Parameter magFilter;
			Parameter minFilter;
			Parameter wrapS;
			Parameter wrapT;
			Parameter maxMipmapLevel;
			Parameter maxAnisotropy;
		};

		void setTextureParameters(const TexParameters & _parameters);

		ObjectHandle createFramebuffer();

		void deleteFramebuffer(ObjectHandle _name);

		ObjectHandle createRenderbuffer();

		struct InitRenderbufferParams {
			ObjectHandle handle;
			Parameter target;
			Parameter format;
			u32 width = 0;
			u32 height = 0;
		};

		void initRenderbuffer(const InitRenderbufferParams & _params);

		struct FrameBufferRenderTarget {
			ObjectHandle bufferHandle;
			Parameter bufferTarget;
			Parameter attachment;
			Parameter textureTarget;
			ObjectHandle textureHandle;
		};

		void addFrameBufferRenderTarget(const FrameBufferRenderTarget & _params);

		PixelWriteBuffer * createPixelWriteBuffer(size_t _sizeInBytes);

		CombinerProgram * createCombinerProgram(Combiner & _color, Combiner & _alpha, const CombinerKey & _key);

		bool saveShadersStorage(const Combiners & _combiners);

		bool loadShadersStorage(Combiners & _combiners);

		ShaderProgram * createDepthFogShader();

		ShaderProgram * createMonochromeShader();

		TexDrawerShaderProgram * createTexDrawerDrawShader();

		ShaderProgram * createTexDrawerClearShader();

		ShaderProgram * createTexrectCopyShader();

		bool isSupported(SpecialFeatures _feature) const;

	private:
		std::unique_ptr<ContextImpl> m_impl;
	};

}

extern graphics::Context gfxContext;
