#include "FBOTextureFormats.h"

FBOTextureFormats fboFormats;

void FBOTextureFormats::init()
{
#ifdef GLES2
	monochromeInternalFormat = GL_RGB;
	monochromeFormat = GL_RGB;
	monochromeType = GL_UNSIGNED_SHORT_5_6_5;
	monochromeFormatBytes = 2;

#ifndef USE_DEPTH_RENDERBUFFER
	depthInternalFormat = GL_DEPTH_COMPONENT;
	depthFormatBytes = 4;
#else
	depthInternalFormat = GL_DEPTH_COMPONENT16;
	depthFormatBytes = 2;
#endif

	depthFormat = GL_DEPTH_COMPONENT;
	depthType = GL_UNSIGNED_INT;

	if (OGLVideo::isExtensionSupported("GL_OES_rgb8_rgba8")) {
		colorInternalFormat = GL_RGBA;
		colorFormat = GL_RGBA;
		colorType = GL_UNSIGNED_BYTE;
		colorFormatBytes = 4;
	} else {
		colorInternalFormat = GL_RGB;
		colorFormat = GL_RGB;
		colorType = GL_UNSIGNED_SHORT_5_6_5;
		colorFormatBytes = 2;
	}
#elif defined(GLES3) || defined (GLES3_1)
	colorInternalFormat = GL_RGBA8;
	colorFormat = GL_RGBA;
	colorType = GL_UNSIGNED_BYTE;
	colorFormatBytes = 4;

	monochromeInternalFormat = GL_R8;
	monochromeFormat = GL_RED;
	monochromeType = GL_UNSIGNED_BYTE;
	monochromeFormatBytes = 1;

	depthInternalFormat = GL_DEPTH_COMPONENT24;
	depthFormat = GL_DEPTH_COMPONENT;
	depthType = GL_UNSIGNED_INT;
	depthFormatBytes = 4;

	depthImageInternalFormat = GL_RGBA32F;
	depthImageFormat = GL_RGBA;
	depthImageType = GL_FLOAT;
	depthImageFormatBytes = 16;

	lutInternalFormat = GL_R32UI;
	lutFormat = GL_RED;
	lutType = GL_UNSIGNED_INT;
	lutFormatBytes = 4;
#else
	colorInternalFormat = GL_RGBA;
	colorFormat = GL_RGBA;
	colorType = GL_UNSIGNED_BYTE;
	colorFormatBytes = 4;

	monochromeInternalFormat = GL_RED;
	monochromeFormat = GL_RED;
	monochromeType = GL_UNSIGNED_BYTE;
	monochromeFormatBytes = 1;

	depthInternalFormat = GL_DEPTH_COMPONENT;
	depthFormat = GL_DEPTH_COMPONENT;
	depthType = GL_FLOAT;
	depthFormatBytes = 4;

	depthImageInternalFormat = GL_RG32F;
	depthImageFormat = GL_RG;
	depthImageType = GL_FLOAT;
	depthImageFormatBytes = 8;

	lutInternalFormat = GL_R16;
	lutFormat = GL_RED;
	lutType = GL_UNSIGNED_SHORT;
	lutFormatBytes = 2;

#endif
}
