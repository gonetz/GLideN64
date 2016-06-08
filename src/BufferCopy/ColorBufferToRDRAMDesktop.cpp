#include "ColorBufferToRDRAM.h"

#include <Textures.h>

class ColorBufferToRDRAMDesktop : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAMDesktop();
	~ColorBufferToRDRAMDesktop() {};

private:
	void _init() override;
	void _destroy() override;
	GLubyte* _getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)  override;
	void _cleanUpPixels(GLubyte* pixelData)  override;
	void _initBuffers(void) override;
	void _destroyBuffers(void) override;
	static const int _numPBO = 3;
	GLuint m_PBO[_numPBO];
	u32 m_curIndex;
};

ColorBufferToRDRAM & ColorBufferToRDRAM::get()
{
	static ColorBufferToRDRAMDesktop cbCopy;
	return cbCopy;
}

ColorBufferToRDRAMDesktop::ColorBufferToRDRAMDesktop() 
	: ColorBufferToRDRAM()
	, m_curIndex(-1)
{
	for(int index = 0; index < _numPBO; ++index)
	{
		m_PBO[index] = 0;
	}
}

void ColorBufferToRDRAMDesktop::_init()
{
	// Generate Pixel Buffer Objects
	glGenBuffers(_numPBO, m_PBO);
	m_curIndex = 0;
}

void ColorBufferToRDRAMDesktop::_destroy()
{
	glDeleteBuffers(_numPBO, m_PBO);

	for(int index = 0; index < _numPBO; ++index)
	{
		m_PBO[index] = 0;
	}
}

void ColorBufferToRDRAMDesktop::_initBuffers(void)
{
	// Initialize Pixel Buffer Objects
	for (u32 i = 0; i < _numPBO; ++i) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, NULL, GL_DYNAMIC_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void ColorBufferToRDRAMDesktop::_destroyBuffers(void)
{

}

GLubyte* ColorBufferToRDRAMDesktop::_getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)
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
	if (pixelData == NULL)
		return NULL;

	int widthBytes = _width*colorFormatBytes;
	int strideBytes = m_pTexture->realWidth * colorFormatBytes;
	GLubyte* pixelDataAlloc = (GLubyte*)malloc(_width * _height * colorFormatBytes);
	for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
		memcpy(pixelDataAlloc + lnIndex*widthBytes, pixelData + (lnIndex*strideBytes), widthBytes);
	}
	return pixelDataAlloc;
}

void ColorBufferToRDRAMDesktop::_cleanUpPixels(GLubyte* pixelData)
{
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	free(pixelData);
}
