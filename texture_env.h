#ifndef TEXTURE_ENV_H
#define TEXTURE_ENV_H

struct TexEnv
{
	GLint mode;

	struct
	{
		WORD color, alpha;
	} fragment;

	BOOL usesT0, usesT1;
};

void Init_texture_env();
TexEnv *Compile_texture_env( Combiner *color, Combiner *alpha );
void Set_texture_env( TexEnv *texEnv );
void Update_texture_env_Colors( TexEnv *texEnv );
void Uninit_texture_env();

#endif

