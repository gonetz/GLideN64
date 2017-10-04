#ifdef MINGW
#define _CRT_RAND_S
#endif

#include <thread>
#include <array>
#include <algorithm>
#include <random>
#include <functional>
#include <cstdlib>
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

#define NOISE_TEX_WIDTH 640
#define NOISE_TEX_HEIGHT 580

NoiseTexture g_noiseTexture;

NoiseTexture::NoiseTexture()
	: m_DList(0)
{
}

typedef std::array<std::vector<u8>, NOISE_TEX_NUM> NoiseTexturesData;

static
u32 Rand(u32 rand_value)
{
#ifdef MINGW
	rand_s(&rand_value);
#else
	rand_value = rand();
#endif
	return rand_value;
}

static
void FillTextureData(u32 _seed, NoiseTexturesData * _pData, u32 _start, u32 _stop)
{
	srand(_seed);
	for (u32 i = _start; i < _stop; ++i) {
		auto & vec = _pData->at(i);
		const size_t sz = vec.size();
		u32 rand_value;
		for (size_t t = 0; t < sz; ++t) {
			rand_value = Rand(rand_value);
			vec[t] = rand_value & 0xFF;
		}
	}
}

void NoiseTexture::init()
{
	if (config.generalEmulation.enableNoise == 0)
		return;

	NoiseTexturesData texData;
	for (auto& vec : texData)
		vec.resize(NOISE_TEX_WIDTH * NOISE_TEX_HEIGHT);

	const u32 concurentThreadsSupported = std::thread::hardware_concurrency();
	if (concurentThreadsSupported > 1) {
		const u32 numThreads = concurentThreadsSupported;
		u32 chunk = NOISE_TEX_NUM / numThreads;
		if (NOISE_TEX_NUM % numThreads != 0)
			chunk++;

		std::uniform_int_distribution<u32> uint_dist;
		std::mt19937 engine; // Mersenne twister MT19937
		engine.seed(std::mt19937::default_seed);
		auto generator = std::bind(uint_dist, engine);

		std::vector<std::thread> threads;
		u32 start = 0;
		do {
			threads.emplace_back(
				FillTextureData,
				generator(),
				&texData,
				start,
				std::min(start + chunk, static_cast<u32>(texData.size())));
			start += chunk;
		} while (start < NOISE_TEX_NUM - chunk);

		FillTextureData(generator(), &texData, start, texData.size());

		for (auto& t : threads)
			t.join();
	} else {
		FillTextureData(static_cast<u32>(time(nullptr)), &texData, 0, texData.size());
	}

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
		m_pTexture[i]->realWidth = NOISE_TEX_WIDTH;
		m_pTexture[i]->realHeight = NOISE_TEX_HEIGHT;
		m_pTexture[i]->textureBytes = m_pTexture[i]->realWidth * m_pTexture[i]->realHeight;
		textureCache().addFrameBufferTextureSize(m_pTexture[i]->textureBytes);

		const FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();
		{
			Context::InitTextureParams params;
			params.handle = m_pTexture[i]->name;
			params.width = m_pTexture[i]->realWidth;
			params.height = m_pTexture[i]->realHeight;
			params.internalFormat = fbTexFormats.noiseInternalFormat;
			params.format = fbTexFormats.noiseFormat;
			params.dataType = fbTexFormats.noiseType;
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
		{
			Context::UpdateTextureDataParams params;
			params.handle = m_pTexture[i]->name;
			params.textureUnitIndex = textureIndices::NoiseTex;
			params.width = m_pTexture[i]->realWidth;
			params.height = m_pTexture[i]->realHeight;
			params.format = fbTexFormats.noiseFormat;
			params.dataType = fbTexFormats.noiseType;
			params.data = texData[i].data();
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

	u32 rand_value;
	while (m_currTex == m_prevTex) {
		rand_value = Rand(rand_value);
		m_currTex = rand_value % NOISE_TEX_NUM;
	}
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
