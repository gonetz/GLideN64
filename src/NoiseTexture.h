#pragma once
#include <memory>
#include "Types.h"

#define NOISE_TEX_NUM 30

struct CachedTexture;

class NoiseTexture
{
public:
	NoiseTexture();

	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture[NOISE_TEX_NUM];
	u32 m_DList;
	u32 m_currTex, m_prevTex;
};

extern NoiseTexture g_noiseTexture;
