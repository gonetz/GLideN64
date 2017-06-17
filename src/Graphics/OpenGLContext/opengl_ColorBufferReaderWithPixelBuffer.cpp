#include <Config.h>
#include <Graphics/Context.h>
#include "opengl_ColorBufferReaderWithPixelBuffer.h"
#include "opengl_Wrapper.h"

using namespace graphics;
using namespace opengl;

ColorBufferReaderWithPixelBuffer::ColorBufferReaderWithPixelBuffer(CachedTexture *_pTexture,
																   CachedBindBuffer *_bindBuffer)
	: ColorBufferReader(_pTexture), m_bindBuffer(_bindBuffer)
{
	_initBuffers();
}


ColorBufferReaderWithPixelBuffer::~ColorBufferReaderWithPixelBuffer()
{
	_destroyBuffers();
}

void ColorBufferReaderWithPixelBuffer::_destroyBuffers()
{
	auto buffers = std::unique_ptr<GLuint[]>(new GLuint[m_numPBO]);

	for(unsigned int index = 0; index < m_numPBO; ++index) {
		buffers[index] = m_PBO[index];
	}

	FunctionWrapper::glDeleteBuffers(m_numPBO, std::move(buffers));

	for (u32 index = 0; index < m_numPBO; ++index)
		m_PBO[index] = 0;
}

void ColorBufferReaderWithPixelBuffer::_initBuffers()
{
	m_numPBO = config.frameBufferEmulation.copyToRDRAM;
	if (m_numPBO > _maxPBO)
		m_numPBO = _maxPBO;

	// Generate Pixel Buffer Objects
	FunctionWrapper::glGenBuffers(m_numPBO, m_PBO);
	m_curIndex = 0;

	// Initialize Pixel Buffer Objects
	for (u32 i = 0; i < m_numPBO; ++i) {
		m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[i]));
		FunctionWrapper::glBufferData(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, std::move(std::unique_ptr<u8[]>(nullptr)), GL_DYNAMIC_READ);
	}
	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle::null);
}

const u8 * ColorBufferReaderWithPixelBuffer::_readPixels(const ReadColorBufferParams& _params, u32& _heightOffset,
	u32& _stride)
{
	GLenum format = GLenum(_params.colorFormat);
	GLenum type = GLenum(_params.colorType);

	_heightOffset = 0;
	_stride = m_pTexture->realWidth;

	// If Sync, read pixels from the buffer, copy them to RDRAM.
	// If not Sync, read pixels from the buffer, copy pixels from the previous buffer to RDRAM.
	if (!_params.sync) {
		m_curIndex = (m_curIndex + 1) % m_numPBO;
		m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[m_curIndex]));
		FunctionWrapper::glReadPixelsAsync(_params.x0, _params.y0, m_pTexture->realWidth, _params.height, format, type);
		m_localData = FunctionWrapper::glMapBufferRangeReadAsync(GL_PIXEL_PACK_BUFFER, m_PBO[m_curIndex], 0,
			m_pTexture->realWidth * _params.height * _params.colorFormatBytes, GL_MAP_READ_BIT);
		return m_localData->data();
	} else {
		GLubyte* pixelData = nullptr;
		m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[m_numPBO-1]));
		FunctionWrapper::glReadPixels(_params.x0, _params.y0, m_pTexture->realWidth, _params.height, format, type, 0);
		pixelData = (GLubyte*)FunctionWrapper::glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0,
			m_pTexture->realWidth * _params.height * _params.colorFormatBytes, GL_MAP_READ_BIT);
		return reinterpret_cast<u8*>(pixelData);
	}

}

void ColorBufferReaderWithPixelBuffer::cleanUp()
{
	FunctionWrapper::glUnmapBufferAsync(GL_PIXEL_PACK_BUFFER);
	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle::null);
}
