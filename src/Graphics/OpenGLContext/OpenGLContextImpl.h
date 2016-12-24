#pragma once
#include <Graphics/ContextImpl.h>

namespace opengl {

	class OpenGLContextImpl : public graphics::ContextImpl
	{
	public:
		OpenGLContextImpl();
		virtual ~OpenGLContextImpl();
		void init() override;
		void destroy() override;
		graphics::ObjectName createTexture() const override;
		void deleteTexture(graphics::ObjectName _name) const override;
	};

}
