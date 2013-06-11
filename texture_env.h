#ifndef TEXTURE_ENV_H
#define TEXTURE_ENV_H

class TexEnv: public OGLCombiner
{
public:
	TexEnv(Combiner *_color, Combiner *_alpha);
	virtual void Set();
	virtual void UpdateColors();
	virtual void UpdateFBInfo() {};

private:
	GLint m_mode;

	struct
	{
		WORD color, alpha;
	} m_fragment;

	BOOL m_usesT0, m_usesT1;
};

void Init_texture_env();
#endif

