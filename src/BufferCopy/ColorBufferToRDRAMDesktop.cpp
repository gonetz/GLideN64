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

	GLuint m_PBO[3];
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
	m_PBO[0] = m_PBO[1] = m_PBO[2] = 0;
}

void ColorBufferToRDRAMDesktop::_init()
{
	// Generate and initialize Pixel Buffer Objects
	glGenBuffers(3, m_PBO);
	for (u32 i = 0; i < 3; ++i) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, NULL, GL_DYNAMIC_READ);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	m_curIndex = 0;
}

void ColorBufferToRDRAMDesktop::_destroy()
{
	glDeleteBuffers(3, m_PBO);
	m_PBO[0] = m_PBO[1] = m_PBO[2] = 0;
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
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, 0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[nextIndex]);
	} else {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[2]);
		glReadPixels(_x0, _y0, _width, _height, colorFormat, colorType, 0);
	}

	GLubyte* pixelData = (GLubyte*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _width * _height * colorFormatBytes, GL_MAP_READ_BIT);
	if (pixelData == NULL)
		return NULL;

	return pixelData;
}

void ColorBufferToRDRAMDesktop::_cleanUpPixels(GLubyte* pixelData)
{
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}
