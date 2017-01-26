#pragma once
#include <Graphics/ColorBufferReader.h>
#include "opengl_CachedFunctions.h"

namespace opengl {

class ColorBufferReaderWithPixelBuffer :
		public graphics::ColorBufferReader
{
public:
	ColorBufferReaderWithPixelBuffer(CachedTexture * _pTexture,
			CachedBindBuffer * _bindBuffer);
	~ColorBufferReaderWithPixelBuffer();

	u8 * readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync) override;
	void cleanUp() override;

private:
	void _initBuffers();
	void _destroyBuffers();

	CachedBindBuffer * m_bindBuffer;

	static const int _numPBO = 3;
	GLuint m_PBO[_numPBO];
	u32 m_curIndex;
};

}
