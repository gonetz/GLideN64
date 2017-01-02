#pragma once

#include <memory>
#include "ObjectHandle.h"
#include "Parameter.h"

#define GRAPHICS_CONTEXT

namespace graphics {

	class ContextImpl;

	class Context
	{
	public:
		Context();
		~Context();

		void init();

		void destroy();

		ObjectHandle createTexture(Parameter _target);

		void deleteTexture(ObjectHandle _name);

		struct InitTextureParams {
			ObjectHandle handle;
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

		struct TexParameters {
			ObjectHandle handle;
			u32 textureUnitIndex = 0;
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

		struct FrameBufferRenderTarget {
			ObjectHandle bufferHandle;
			Parameter bufferTarget;
			Parameter attachment;
			Parameter textureTarget;
			ObjectHandle textureHandle;
		};

		void addFrameBufferRenderTarget(const FrameBufferRenderTarget & _params);

		bool isMultisamplingSupported() const;

	private:
		std::unique_ptr<ContextImpl> m_impl;
	};

}

extern graphics::Context gfxContext;
