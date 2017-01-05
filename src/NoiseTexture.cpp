#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include "FrameBuffer.h"
#include "Config.h""
#include "GBI.h"
#include "VI.h""
#include "Textures.h"
#include "NoiseTexture.h"

NoiseTexture g_noiseTexture;

NoiseTexture::NoiseTexture()
	: m_pTexture(nullptr)
	, m_PBO(0)
	, m_DList(0)
{
}

void NoiseTexture::init()
{
	if (config.generalEmulation.enableNoise == 0)
		return;
	m_pTexture = textureCache().addFrameBufferTexture(false);
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 640;
	m_pTexture->realHeight = 580;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);

	{
		graphics::Context::InitTextureParams params;
		params.handle = graphics::ObjectHandle(m_pTexture->glName);
		params.width = m_pTexture->realWidth;
		params.height = m_pTexture->realHeight;
		params.internalFormat = graphics::internalcolor::RED;
		params.format = graphics::color::RED;
		params.dataType = graphics::datatype::UNSIGNED_BYTE;
		gfxContext.init2DTexture(params);
	}
	{
		graphics::Context::TexParameters params;
		params.handle = graphics::ObjectHandle(m_pTexture->glName);
		params.target = graphics::target::TEXTURE_2D;
		params.textureUnitIndex = 0;
		params.minFilter = graphics::textureParameters::FILTER_NEAREST;
		params.magFilter = graphics::textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(params);
	}

	// TODO rewrite in GL independent way
	// Generate Pixel Buffer Object. Initialize it with max buffer size.
	glGenBuffers(1, &m_PBO);
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, 640 * 580, nullptr, GL_DYNAMIC_DRAW);
}

void NoiseTexture::destroy()
{
	if (m_pTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = nullptr;
	}
	glDeleteBuffers(1, &m_PBO);
	m_PBO = 0;
}

void NoiseTexture::update()
{
	if (m_PBO == 0 || m_pTexture == nullptr)
		return;
	if (m_DList == video().getBuffersSwapCount() || config.generalEmulation.enableNoise == 0)
		return;
	const u32 dataSize = VI.width*VI.height;
	if (dataSize == 0)
		return;
	PBOBinder binder(GL_PIXEL_UNPACK_BUFFER, m_PBO);
	GLubyte* ptr = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
	if (ptr == nullptr)
		return;
	for (u32 y = 0; y < VI.height; ++y)	{
		for (u32 x = 0; x < VI.width; ++x)
			ptr[x + y*VI.width] = rand() & 0xFF;
	}
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer

	glActiveTexture(GL_TEXTURE0 + g_noiseTexIndex);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VI.width, VI.height, GL_RED, GL_UNSIGNED_BYTE, 0);
	m_DList = video().getBuffersSwapCount();
}
