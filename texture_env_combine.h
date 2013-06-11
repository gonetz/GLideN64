#ifndef TEXTURE_ENV_COMBINE_H
#define TEXTURE_ENV_COMBINE_H

struct TexEnvCombinerArg
{
	GLenum source, operand;
};

struct TexEnvCombinerStage
{
	WORD constant;
	BOOL used;
	GLenum combine;
	TexEnvCombinerArg arg0, arg1, arg2;
	WORD outputTexture;
};

class TexEnvCombiner : public OGLCombiner
{
public:
	TexEnvCombiner(Combiner *_color, Combiner *_alpha);
	virtual void Set();
	virtual void UpdateColors();
	virtual void UpdateFBInfo() {};

private:
	BOOL m_usesT0, m_usesT1, m_usesNoise;

	WORD m_usedUnits;
	
	struct
	{
		WORD color, secondaryColor, alpha;
	} m_vertex;

	TexEnvCombinerStage m_color[8];
	TexEnvCombinerStage m_alpha[8];
};

void Init_texture_env_combine();
void BeginTextureUpdate_texture_env_combine();
#endif

