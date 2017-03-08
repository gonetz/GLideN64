#pragma once
#include <Graphics/ColorBufferReader.h>
#include "opengl_CachedFunctions.h"

namespace opengl {

class ColorBufferReaderWithReadPixels :
		public graphics::ColorBufferReader
{
public:
	ColorBufferReaderWithReadPixels(CachedTexture * _pTexture);
	~ColorBufferReaderWithReadPixels() = default;

	u8 * readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync) override;
	void cleanUp() override;
};

}
