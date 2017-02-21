#pragma once

#include <Graphics/ColorBufferReader.h>
#include "opengl_CachedFunctions.h"

namespace opengl {

	class ColorBufferReaderWithBufferStorage :
		public graphics::ColorBufferReader
	{
	public:
		ColorBufferReaderWithBufferStorage(CachedTexture * _pTexture,
			CachedBindBuffer * _bindBuffer);
		virtual ~ColorBufferReaderWithBufferStorage();

		u8 * readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync) override;
		void cleanUp() override;

	private:
		void _initBuffers();
		void _destroyBuffers();

		CachedBindBuffer * m_bindBuffer;

		static const int _numPBO = 2;
		GLuint m_PBO[_numPBO];
		void* m_PBOData[_numPBO];
		u32 m_curIndex;
		GLsync m_fence[_numPBO];
	};

}
