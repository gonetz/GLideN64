#pragma once
#include <array>
#include <Graphics/DrawerImpl.h>

namespace opengl {
	class CachedVertexAttribArray;

	class UnbufferedDrawer : public graphics::DrawerImpl
	{
	public:
		UnbufferedDrawer(CachedVertexAttribArray * _cachedAttribArray);
		~UnbufferedDrawer();

		void drawTriangles(const DrawTriangleParameters & _params) override;

		void drawRects(const DrawRectParameters & _params) override;

	private:
		bool _updateAttribPointer(u32 _index, const void * _ptr);

		CachedVertexAttribArray * m_cachedAttribArray;
		std::array<const void*, MaxAttribIndex> m_attribsData;
	};

}
