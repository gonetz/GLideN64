#ifndef GRAPHICS_FRAMEBUFFER_TEXTUREFORMATS_H
#define GRAPHICS_FRAMEBUFFER_TEXTUREFORMATS_H
#include "Parameter.h"

namespace graphics {

	struct FramebufferTextureFormats
	{
		Parameter colorInternalFormat;
		Parameter colorFormat;
		Parameter colorType;
		u32 colorFormatBytes;

		Parameter monochromeInternalFormat;
		Parameter monochromeFormat;
		Parameter monochromeType;
		u32 monochromeFormatBytes;

		Parameter depthInternalFormat;
		Parameter depthFormat;
		Parameter depthType;
		u32 depthFormatBytes;

		Parameter depthImageInternalFormat;
		Parameter depthImageFormat;
		Parameter depthImageType;
		u32 depthImageFormatBytes;

		Parameter lutInternalFormat;
		Parameter lutFormat;
		Parameter lutType;
		u32 lutFormatBytes;

		virtual ~FramebufferTextureFormats() {}

	protected:
		virtual void init() = 0;
	};

}
#endif // GRAPHICS_FRAMEBUFFER_TEXTUREFORMATS_H
