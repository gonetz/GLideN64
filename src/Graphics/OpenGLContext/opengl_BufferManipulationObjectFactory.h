#pragma once
#include <Graphics/ObjectHandle.h>
#include <Graphics/Parameter.h>
#include <Graphics/Context.h>

namespace opengl {

	struct GLVersion;
	class CachedFunctions;

	class CreateFramebufferObject
	{
	public:
		virtual ~CreateFramebufferObject() {};
		virtual graphics::ObjectHandle createFramebuffer() = 0;
	};

	class CreateRenderbuffer
	{
	public:
		virtual ~CreateRenderbuffer() {};
		virtual graphics::ObjectHandle createRenderbuffer() = 0;
	};

	class InitRenderbuffer
	{
	public:
		virtual ~InitRenderbuffer() {};
		virtual void initRenderbuffer(const graphics::Context::InitRenderbufferParams & _params) = 0;
	};

	class AddFramebufferRenderTarget
	{
	public:
		virtual ~AddFramebufferRenderTarget() {};
		virtual void addFrameBufferRenderTarget(const graphics::Context::FrameBufferRenderTarget & _params) = 0;
	};


	class BufferManipulationObjectFactory
	{
	public:
		BufferManipulationObjectFactory(const GLVersion & _version, CachedFunctions & _cachedFunctions);
		~BufferManipulationObjectFactory();

		CreateFramebufferObject * getCreateFramebufferObject() const;

		CreateRenderbuffer * getCreateRenderbuffer() const;

		InitRenderbuffer * getInitRenderbuffer() const;

		AddFramebufferRenderTarget * getAddFramebufferRenderTarget() const;

	private:
		const GLVersion & m_version;
		CachedFunctions & m_cachedFunctions;
	};

}