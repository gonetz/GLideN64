#pragma once
#include "Types.h"

struct CachedTexture;

class NoiseTexture
{
public:
	NoiseTexture();

	void init();
	void destroy();
	void update();

private:
	CachedTexture * m_pTexture;
	GLuint m_PBO;
	u32 m_DList;
};

extern NoiseTexture g_noiseTexture;