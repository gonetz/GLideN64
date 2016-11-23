#include <OpenGL.h>
#include <Textures.h>
#include <FBOTextureFormats.h>

#if defined(EGL) || defined(GLESX)
#include <inc/ARB_buffer_storage.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#include "ColorBufferToRDRAM_BufferStorageExt.h"

ColorBufferToRDRAM_BufferStorageExt::ColorBufferToRDRAM_BufferStorageExt()
	: ColorBufferToRDRAM(), m_curIndex(0)
{
#ifdef GLESX
	glBufferStorage = (PFNGLBUFFERSTORAGEPROC)eglGetProcAddress("glBufferStorageEXT");
#endif
}

void ColorBufferToRDRAM_BufferStorageExt::_init()
{
	// Generate Pixel Buffer Objects
	glGenBuffers(_numPBO, m_PBO);
}

void ColorBufferToRDRAM_BufferStorageExt::_destroy()
{
	for (int index = 0; index < _numPBO; ++index) {
		if (m_PBOData[index] != nullptr) {
			glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[index]);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
	}

	glDeleteBuffers(_numPBO, m_PBO);

	for (int index = 0; index < _numPBO; ++index) {
		m_PBO[index] = 0;
	}
}

void ColorBufferToRDRAM_BufferStorageExt::_initBuffers(void)
{
	// Initialize Pixel Buffer Objects
	for (int index = 0; index < _numPBO; ++index) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[index]);
		m_fence[index] = 0;
		glBufferStorage(GL_PIXEL_PACK_BUFFER, m_pTexture->textureBytes, nullptr, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT);
		m_PBOData[index] = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_pTexture->textureBytes, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT );
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

bool ColorBufferToRDRAM_BufferStorageExt::_readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)
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

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBO[m_curIndex]);
	glReadPixels(_x0, _y0, m_pTexture->realWidth, _height, colorFormat, colorType, 0);

	//Setup a fence sync object so that we know when glReadPixels completes
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	m_fence[m_curIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	if (!_sync) {
		m_curIndex = (m_curIndex+1)%_numPBO;
	}

	//Wait for glReadPixels to complete for the currently selected PBO
	if (m_fence[m_curIndex] != 0) {
		glClientWaitSync(m_fence[m_curIndex], 0, 1e8);
		glDeleteSync(m_fence[m_curIndex]);
	}

	GLubyte* pixelData = reinterpret_cast<GLubyte*>(m_PBOData[m_curIndex]);
	if (pixelData == nullptr)
		return false;

	int widthBytes = _width * colorFormatBytes;
	int strideBytes = m_pTexture->realWidth * colorFormatBytes;

	GLubyte* pixelDataAlloc = m_pixelData.data();
	for (unsigned int lnIndex = 0; lnIndex < _height; ++lnIndex) {
		memcpy(pixelDataAlloc + lnIndex*widthBytes, pixelData + (lnIndex*strideBytes), widthBytes);
	}

	return true;
}

void ColorBufferToRDRAM_BufferStorageExt::_cleanUp()
{
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}
