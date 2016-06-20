#ifndef FBOTEXTUREFORMATS_H
#define FBOTEXTUREFORMATS_H

#include "OpenGL.h"

struct FBOTextureFormats
{
	GLint colorInternalFormat;
	GLenum colorFormat;
	GLenum colorType;
	u32 colorFormatBytes;

	GLint monochromeInternalFormat;
	GLenum monochromeFormat;
	GLenum monochromeType;
	u32 monochromeFormatBytes;

	GLint depthInternalFormat;
	GLenum depthFormat;
	GLenum depthType;
	u32 depthFormatBytes;

	GLint depthImageInternalFormat;
	GLenum depthImageFormat;
	GLenum depthImageType;
	u32 depthImageFormatBytes;

	GLint lutInternalFormat;
	GLenum lutFormat;
	GLenum lutType;
	u32 lutFormatBytes;

	void init();
};

extern FBOTextureFormats fboFormats;

#endif // FBOTEXTUREFORMATS_H
