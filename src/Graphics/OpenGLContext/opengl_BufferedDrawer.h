#pragma once
#include <array>
#include "opengl_GraphicsDrawer.h"
#include "opengl_GLInfo.h"

namespace opengl {
	class CachedVertexAttribArray;

	class BufferedDrawer : public GraphicsDrawer
	{
	public:
		BufferedDrawer(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray,
			CachedBindBuffer * _bindBuffer);
		~BufferedDrawer();

		void drawTriangles(const graphics::Context::DrawTriangleParameters & _params) override;

		void drawRects(const graphics::Context::DrawRectParameters & _params) override;

		void drawLine(f32 _width, SPVertex * _vertices) override;

	private:
		bool _updateAttribPointer(u32 _type, u32 _index, const void * _ptr);
		void _updateBuffer(u32 _type, u32 _length, const void * _data);

		CachedBindBuffer * m_bindBuffer;
		const GLInfo & m_glInfo;
		GLuint m_vao;
		enum {
			TRI_VBO = 0,
			RECT_VBO,
			IBO,
			BO_COUNT
		};
		GLuint m_bufObj[BO_COUNT];
		GLenum m_bufType[BO_COUNT];
		char* m_bufData[BO_COUNT];
		u32 m_bufOffset[BO_COUNT];
		u32 m_bufFormatSize[BO_COUNT];
		u32 m_bufMaxSize;
		CachedVertexAttribArray * m_cachedAttribArray;
		std::array<const void*, MaxAttribIndex> m_attribsData;
	};

}
