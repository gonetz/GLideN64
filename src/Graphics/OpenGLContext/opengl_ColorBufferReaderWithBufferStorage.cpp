#include <Graphics/Context.h>
#include "opengl_ColorBufferReaderWithBufferStorage.h"

using namespace graphics;
using namespace opengl;

ColorBufferReaderWithBufferStorage::ColorBufferReaderWithBufferStorage(CachedTexture * _pTexture,
	CachedBindBuffer * _bindBuffer)
	: ColorBufferReader(_pTexture), m_bindBuffer(_bindBuffer)
{
	_initBuffers();
}

ColorBufferReaderWithBufferStorage::~ColorBufferReaderWithBufferStorage()
{
	_destroyBuffers();
}

void ColorBufferReaderWithBufferStorage::_initBuffers()
{
	// Generate Pixel Buffer Objects
	glGenBuffers(_numPBO, m_PBO);
	m_curIndex = 0;

	// Initialize Pixel Buffer Objects
	for (int index = 0; index < _numPBO; ++index) {
		m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[index]));
		m_fence[index] = 0;
		glBufferStorage(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
		m_PBOData[index] = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_pTexture->textureBytes, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
	}

	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle::null);
}

void ColorBufferReaderWithBufferStorage::_destroyBuffers()
{
	glDeleteBuffers(_numPBO, m_PBO);

	for (int index = 0; index < _numPBO; ++index)
		m_PBO[index] = 0;
}

u8 * ColorBufferReaderWithBufferStorage::readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync)
{
	const FramebufferTextureFormats & fbTexFormat = gfxContext.getFramebufferTextureFormats();
	GLenum colorFormat, colorType, colorFormatBytes;
	if (_size > G_IM_SIZ_8b) {
		colorFormat = GLenum(fbTexFormat.colorFormat);
		colorType = GLenum(fbTexFormat.colorType);
		colorFormatBytes = GLenum(fbTexFormat.colorFormatBytes);
	}
	else {
		colorFormat = GLenum(fbTexFormat.monochromeFormat);
		colorType = GLenum(fbTexFormat.monochromeType);
		colorFormatBytes = GLenum(fbTexFormat.monochromeFormatBytes);
	}

	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle(m_PBO[m_curIndex]));
	glReadPixels(_x0, _y0, m_pTexture->realWidth, _height, colorFormat, colorType, 0);

	//Setup a fence sync object so that we know when glReadPixels completes
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	m_fence[m_curIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	if (!_sync) {
		m_curIndex = (m_curIndex + 1) % _numPBO;
	}

	//Wait for glReadPixels to complete for the currently selected PBO
	if (m_fence[m_curIndex] != 0) {
		glClientWaitSync(m_fence[m_curIndex], 0, 1e8);
		glDeleteSync(m_fence[m_curIndex]);
	}

	GLubyte* pixelData = reinterpret_cast<GLubyte*>(m_PBOData[m_curIndex]);
	if (pixelData == nullptr)
		return nullptr;

	int widthBytes = _width * colorFormatBytes;
	int strideBytes = m_pTexture->realWidth * colorFormatBytes;

	GLubyte* pixelDataAlloc = m_pixelData.data();
	for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
		memcpy(pixelDataAlloc + lnIndex*widthBytes, pixelData + (lnIndex*strideBytes), widthBytes);
	}

	return pixelDataAlloc;
}

void ColorBufferReaderWithBufferStorage::cleanUp()
{
	m_bindBuffer->bind(Parameter(GL_PIXEL_PACK_BUFFER), ObjectHandle::null);
}
