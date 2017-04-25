#pragma once
#include <array>
#include "opengl_GraphicsDrawer.h"
#include "opengl_GLInfo.h"

namespace opengl {
	class CachedVertexAttribArray;

	class UnbufferedDrawerThreadSafe : public GraphicsDrawer
	{
	public:
		UnbufferedDrawerThreadSafe(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray);
		~UnbufferedDrawerThreadSafe();

		void drawTriangles(const graphics::Context::DrawTriangleParameters & _params) override;

		void drawRects(const graphics::Context::DrawRectParameters & _params) override;

		void drawLine(f32 _width, SPVertex * _vertices) override;

	private:
		bool _updateAttribPointer(u32 _index, const void * _ptr);

		const GLInfo & m_glInfo;
		CachedVertexAttribArray * m_cachedAttribArray;
		std::array<const void*, MaxAttribIndex> m_attribsData;
	};

}
