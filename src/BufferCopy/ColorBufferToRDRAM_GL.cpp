#include "ColorBufferToRDRAM.h"

#include <Textures.h>
#include <FBOTextureFormats.h>

class ColorBufferToRDRAM_GL : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAM_GL();
	~ColorBufferToRDRAM_GL() {};

private:
	void _init() override;
	void _destroy() override;
	bool _readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)  override;
	void _cleanUp()  override;
	void _initBuffers(void) override;
	static const int _numPBO = 3;
	GLuint m_PBO[_numPBO];
	u32 m_curIndex;
};

ColorBufferToRDRAM & ColorBufferToRDRAM::get()
{
	static ColorBufferToRDRAM_GL cbCopy;
	return cbCopy;
}

ColorBufferToRDRAM_GL::ColorBufferToRDRAM_GL()
	: ColorBufferToRDRAM()
	, m_curIndex(-1)
{
	for(int index = 0; index < _numPBO; ++index)
		m_PBO[index] = 0;
}

void ColorBufferToRDRAM_GL::_init()
{
	// Generate Pixel Buffer Objects
	glGenBuffers(_numPBO, m_PBO);
	m_curIndex = 0;
}

void ColorBufferToRDRAM_GL::_destroy()
{
	glDeleteBuffers(_numPBO, m_PBO);

	for(int index = 0; index < _numPBO; ++index)
		m_PBO[index] = 0;
}

void ColorBufferToRDRAM_GL::_initBuffers(void)
{
	// Initialize Pixel Buffer Objects
	for (u32 i = 0; i < _numPBO; ++i) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, nullptr, GL_DYNAMIC_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

bool ColorBufferToRDRAM_GL::_readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)
{
	GLenum colorFormat, colorType, colorFormatBytes;
	if (_size > G_IM_SIZ_8b) {
		colorFormat = fboFormats.colorFormat;
		colorType = fboFormats.colorType;
		colorFormatBytes = fboFormats.colorFormatBytes;
	} else {
		colorFormat = fboFormats.monochromeFormat;
		colorType = fboFormats.monochromeType;
		colorFormatBytes = fboFormats.monochromeFormatBytes;
	}

	// If Sync, read pixels from the buffer, copy them to RDRAM.
	// If not Sync, read pixels from the buffer, copy pixels from the previous buffer to RDRAM.
	if (!_sync) {
		m_curIndex ^= 1;
		const u32 nextIndex = m_curIndex ^ 1;
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[m_curIndex]);
		glReadPixels(_x0, _y0, m_pTexture->realWidth, _height, colorFormat, colorType, 0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[nextIndex]);
	} else {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[2]);
		glReadPixels(_x0, _y0, m_pTexture->realWidth, _height, colorFormat, colorType, 0);
	}

	GLubyte* pixelData = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_pTexture->realWidth * _height * colorFormatBytes, GL_MAP_READ_BIT);
	if (pixelData == nullptr)
		return false;

	int widthBytes = _width*colorFormatBytes;
	int strideBytes = m_pTexture->realWidth * colorFormatBytes;

	GLubyte* pixelDataAlloc = m_pixelData.data();
	for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
		memcpy(pixelDataAlloc + lnIndex*widthBytes, pixelData + (lnIndex*strideBytes), widthBytes);
	}
	return true;
}

void ColorBufferToRDRAM_GL::_cleanUp()
{
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}
