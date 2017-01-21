#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include "FrameBuffer.h"
#include "Config.h"
#include "GBI.h"
#include "VI.h"
#include "Textures.h"
#include "NoiseTexture.h"
#include "DisplayWindow.h"

using namespace graphics;

NoiseTexture g_noiseTexture;

NoiseTexture::NoiseTexture()
	: m_pTexture(nullptr)
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
		Context::InitTextureParams params;
		params.handle = m_pTexture->name;
		params.width = m_pTexture->realWidth;
		params.height = m_pTexture->realHeight;
		params.internalFormat = internalcolor::RED;
		params.format = color::RED;
		params.dataType = datatype::UNSIGNED_BYTE;
		gfxContext.init2DTexture(params);
	}
	{
		Context::TexParameters params;
		params.handle = m_pTexture->name;
		params.target = target::TEXTURE_2D;
		params.textureUnitIndex = textureIndices::NoiseTex;
		params.minFilter = textureParameters::FILTER_NEAREST;
		params.magFilter = textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(params);
	}

	// Generate Pixel Buffer Object. Initialize it with max buffer size.
	m_pbuf.reset(gfxContext.createPixelWriteBuffer(m_pTexture->textureBytes));
}

void NoiseTexture::destroy()
{
	textureCache().removeFrameBufferTexture(m_pTexture);
	m_pTexture = nullptr;
	m_pbuf.reset();
}

void NoiseTexture::update()
{
	if (!m_pbuf || m_pTexture == nullptr)
		return;
	if (m_DList == dwnd().getBuffersSwapCount() || config.generalEmulation.enableNoise == 0)
		return;
	const u32 dataSize = VI.width*VI.height;
	if (dataSize == 0)
		return;

	PixelBufferBinder<PixelWriteBuffer> binder(m_pbuf.get());
	GLubyte* ptr = (GLubyte*)m_pbuf->getWriteBuffer(dataSize);
	if (ptr == nullptr) {
		return;
	}
	for (u32 y = 0; y < VI.height; ++y)	{
		for (u32 x = 0; x < VI.width; ++x)
			ptr[x + y*VI.width] = rand() & 0xFF;
	}
	m_pbuf->closeWriteBuffer();

	Context::UpdateTextureDataParams params;
	params.handle = m_pTexture->name;
	params.textureUnitIndex = textureIndices::NoiseTex;
	params.width = VI.width;
	params.height = VI.height;
	params.format = color::RED;
	params.dataType = datatype::UNSIGNED_BYTE;
	params.data = m_pbuf->getData();
	gfxContext.update2DTexture(params);

	m_DList = dwnd().getBuffersSwapCount();
}
