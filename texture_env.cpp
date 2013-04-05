#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif
#include "OpenGL.h"
#include "Combiner.h"
#include "texture_env.h"

void Init_texture_env()
{
}

void Uninit_texture_env()
{
}

void Update_texture_env_Colors( TexEnv *texEnv )
{
}

TexEnv *Compile_texture_env( Combiner *color, Combiner *alpha )
{
	TexEnv *texEnv = (TexEnv*)malloc( sizeof( TexEnv ) );

	texEnv->usesT0 = FALSE;
	texEnv->usesT1 = FALSE;

	texEnv->fragment.color = texEnv->fragment.alpha = COMBINED;

	for (int i = 0; i < alpha->numStages; i++)
	{
		for (int j = 0; j < alpha->stage[i].numOps; j++)
		{
			switch (alpha->stage[i].op[j].op)
			{
				case LOAD:
					if ((alpha->stage[i].op[j].param1 != TEXEL0_ALPHA) && (alpha->stage[i].op[j].param1 != TEXEL1_ALPHA))
					{
						texEnv->fragment.alpha = alpha->stage[i].op[j].param1;
						texEnv->usesT0 = FALSE;
						texEnv->usesT1 = FALSE;
					}
					else
					{
						texEnv->mode = GL_REPLACE;

						texEnv->usesT0 = alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
						texEnv->usesT1 = alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;
					}
					break;
				case SUB:
					break;
				case MUL:
					if (((alpha->stage[i].op[j].param1 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param1 == TEXEL1_ALPHA)) &&
						((alpha->stage[i].op[j - 1].param1 != TEXEL0_ALPHA) || (alpha->stage[i].op[j - 1].param1 != TEXEL1_ALPHA)))
					{
						texEnv->mode = GL_MODULATE;
					}
					else if (((alpha->stage[i].op[j].param1 != TEXEL0_ALPHA) || (alpha->stage[i].op[j].param1 != TEXEL1_ALPHA)) &&
						((alpha->stage[i].op[j - 1].param1 == TEXEL0_ALPHA) || (alpha->stage[i].op[j - 1].param1 == TEXEL1_ALPHA)))
					{
						texEnv->fragment.alpha = alpha->stage[i].op[j].param1;
						texEnv->mode = GL_MODULATE;
					}
					break;
				case ADD:
					break;
				case INTER:
					break;
			}
		}
	}

	for (int i = 0; i < color->numStages; i++)
	{
		for (int j = 0; j < color->stage[i].numOps; j++)
		{
			switch (color->stage[i].op[j].op)
			{
				case LOAD:
					if ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA))
					{
						if (texEnv->mode == GL_MODULATE)
							texEnv->fragment.color = ONE;

						texEnv->usesT0 = TRUE;
						texEnv->usesT1 = FALSE;
					}
					else if ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA))
					{
						if (texEnv->mode == GL_MODULATE)
							texEnv->fragment.color = ONE;

						texEnv->usesT0 = FALSE;
						texEnv->usesT1 = TRUE;
					}
					else
					{
						texEnv->fragment.color = color->stage[i].op[j].param1;
						texEnv->usesT0 = texEnv->usesT1 = FALSE;
					}
					break;
				case SUB:
					break;
				case MUL:
					if ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA))
					{
						if (!texEnv->usesT0 && !texEnv->usesT1)
						{
							texEnv->mode = GL_MODULATE;
							texEnv->usesT0 = TRUE;
							texEnv->usesT1 = FALSE;
						}
					}
					else if ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA))
					{
						if (!texEnv->usesT0 && !texEnv->usesT1)
						{
							texEnv->mode = GL_MODULATE;
							texEnv->usesT0 = FALSE;
							texEnv->usesT1 = TRUE;
						}
					}
					else if (texEnv->usesT0 || texEnv->usesT1)
					{
						texEnv->mode = GL_MODULATE;
						texEnv->fragment.color = color->stage[i].op[j].param1;
					}
					break;
				case ADD:
					break;
				case INTER:
					if ((color->stage[i].op[j].param1 == TEXEL0) &&
					    ((color->stage[i].op[j].param2 != TEXEL0) && (color->stage[i].op[j].param2 != TEXEL0_ALPHA) &&
						 (color->stage[i].op[j].param2 != TEXEL1) && (color->stage[i].op[j].param2 != TEXEL1_ALPHA)) &&
						 (color->stage[i].op[j].param3 == TEXEL0_ALPHA))
					{
						texEnv->mode = GL_DECAL;
						texEnv->fragment.color = color->stage[i].op[j].param2;
						texEnv->usesT0 = TRUE;
						texEnv->usesT1 = FALSE;
					}
					else if ((color->stage[i].op[j].param1 == TEXEL0) &&
					    ((color->stage[i].op[j].param2 != TEXEL0) && (color->stage[i].op[j].param2 != TEXEL0_ALPHA) &&
						 (color->stage[i].op[j].param2 != TEXEL1) && (color->stage[i].op[j].param2 != TEXEL1_ALPHA)) &&
						 (color->stage[i].op[j].param3 == TEXEL0_ALPHA))
					{
						texEnv->mode = GL_DECAL;
						texEnv->fragment.color = color->stage[i].op[j].param2;
						texEnv->usesT0 = FALSE;
						texEnv->usesT1 = TRUE;
					}
					break;
			}
		}
	}

	return texEnv;
}


void Set_texture_env( TexEnv *texEnv )
{
	combiner.usesT0 = texEnv->usesT0;
	combiner.usesT1 = texEnv->usesT1;
	combiner.usesNoise = FALSE;

	combiner.vertex.color = texEnv->fragment.color;
	combiner.vertex.secondaryColor = COMBINED;
	combiner.vertex.alpha = texEnv->fragment.alpha;

	// Shouldn't ever happen, but who knows?
	if (OGL.ARB_multitexture)
		glActiveTextureARB( GL_TEXTURE0_ARB );

	if (texEnv->usesT0 || texEnv->usesT1)
		glEnable( GL_TEXTURE_2D );
	else
		glDisable( GL_TEXTURE_2D );

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnv->mode );
}
