#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include "FrameBuffer.h"
#include "Config.h"
#include "GBI.h"
#include "VI.h"
#include "Textures.h"
#include "NoiseTexture.h"
#include "DisplayWindow.h"
#include <cstdlib>

using namespace graphics;

NoiseTexture g_noiseTexture;

NoiseTexture::NoiseTexture()
	: m_DList(0)
{
}

void NoiseTexture::init()
{
	if (config.generalEmulation.enableNoise == 0)
		return;
	for (u32 i = 0; i < NOISE_TEX_NUM; ++i) {
		m_pTexture[i] = textureCache().addFrameBufferTexture(false);
		m_pTexture[i]->format = G_IM_FMT_RGBA;
		m_pTexture[i]->clampS = 1;
		m_pTexture[i]->clampT = 1;
		m_pTexture[i]->frameBufferTexture = CachedTexture::fbOneSample;
		m_pTexture[i]->maskS = 0;
		m_pTexture[i]->maskT = 0;
		m_pTexture[i]->mirrorS = 0;
		m_pTexture[i]->mirrorT = 0;
		m_pTexture[i]->realWidth = 640;
		m_pTexture[i]->realHeight = 580;
		m_pTexture[i]->textureBytes = m_pTexture[i]->realWidth * m_pTexture[i]->realHeight;
		textureCache().addFrameBufferTextureSize(m_pTexture[i]->textureBytes);

		{
			Context::InitTextureParams params;
			params.handle = m_pTexture[i]->name;
			params.width = m_pTexture[i]->realWidth;
			params.height = m_pTexture[i]->realHeight;
			params.internalFormat = internalcolorFormat::RED;
			params.format = colorFormat::RED;
			params.dataType = datatype::UNSIGNED_BYTE;
			gfxContext.init2DTexture(params);
		}
		{
			Context::TexParameters params;
			params.handle = m_pTexture[i]->name;
			params.target = textureTarget::TEXTURE_2D;
			params.textureUnitIndex = textureIndices::NoiseTex;
			params.minFilter = textureParameters::FILTER_NEAREST;
			params.magFilter = textureParameters::FILTER_NEAREST;
			gfxContext.setTextureParameters(params);
		}
		unsigned char ptr[m_pTexture[i]->textureBytes];
		for (u32 y = 0; y < m_pTexture[i]->realHeight; ++y)     {
			for (u32 x = 0; x < m_pTexture[i]->realWidth; ++x)
				ptr[x + y*m_pTexture[i]->realWidth] = rand() & 0xFF;
		}
		{
			Context::UpdateTextureDataParams params;
			params.handle = m_pTexture[i]->name;
			params.textureUnitIndex = textureIndices::NoiseTex;
			params.width = 640;
			params.height = 580;
			params.format = colorFormat::RED;
			params.dataType = datatype::UNSIGNED_BYTE;
			params.data = ptr;
			gfxContext.update2DTexture(params);
		}
	}
}

void NoiseTexture::destroy()
{
	for (u32 i = 0; i < NOISE_TEX_NUM; ++i) {
		textureCache().removeFrameBufferTexture(m_pTexture[i]);
		m_pTexture[i] = nullptr;
	}
}

void NoiseTexture::update()
{
	if (m_DList == dwnd().getBuffersSwapCount() || config.generalEmulation.enableNoise == 0)
		return;

	while (m_currTex == m_prevTex)
		m_currTex = rand() % NOISE_TEX_NUM;
	m_prevTex = m_currTex;
	if (m_pTexture[m_currTex] == nullptr)
		return;
	{
		Context::BindTextureParameters params;
		params.texture = m_pTexture[m_currTex]->name;
		params.textureUnitIndex = textureIndices::NoiseTex;
		params.target = textureTarget::TEXTURE_2D;
		gfxContext.bindTexture(params);
	}
	m_DList = dwnd().getBuffersSwapCount();
}
