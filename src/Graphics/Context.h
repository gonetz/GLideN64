#pragma once

#include <memory>
#include "ObjectName.h"

namespace graphics {

	class ContextImpl;

	class Context
	{
	public:
		Context();
		~Context();
		void init();
		void destroy();

		ObjectName createTexture() const;
		void deleteTexture(ObjectName _name) const;

	private:
		std::unique_ptr<ContextImpl> m_impl;
	};

}

extern graphics::Context gfxContext;
