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

void TexEnv::UpdateColors()
{
}

TexEnv::TexEnv( Combiner *color, Combiner *alpha )
{
	m_usesT0 = FALSE;
	m_usesT1 = FALSE;

	m_fragment.color = m_fragment.alpha = COMBINED;

	for (int i = 0; i < alpha->numStages; i++)
	{
		for (int j = 0; j < alpha->stage[i].numOps; j++)
		{
			switch (alpha->stage[i].op[j].op)
			{
				case LOAD:
					if ((alpha->stage[i].op[j].param1 != TEXEL0_ALPHA) && (alpha->stage[i].op[j].param1 != TEXEL1_ALPHA))
					{
						m_fragment.alpha = alpha->stage[i].op[j].param1;
						m_usesT0 = FALSE;
						m_usesT1 = FALSE;
					}
					else
					{
						m_mode = GL_REPLACE;

						m_usesT0 = alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
						m_usesT1 = alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;
					}
					break;
				case SUB:
					break;
				case MUL:
					if (((alpha->stage[i].op[j].param1 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param1 == TEXEL1_ALPHA)) &&
						((alpha->stage[i].op[j - 1].param1 != TEXEL0_ALPHA) || (alpha->stage[i].op[j - 1].param1 != TEXEL1_ALPHA)))
					{
						m_mode = GL_MODULATE;
					}
					else if (((alpha->stage[i].op[j].param1 != TEXEL0_ALPHA) || (alpha->stage[i].op[j].param1 != TEXEL1_ALPHA)) &&
						((alpha->stage[i].op[j - 1].param1 == TEXEL0_ALPHA) || (alpha->stage[i].op[j - 1].param1 == TEXEL1_ALPHA)))
					{
						m_fragment.alpha = alpha->stage[i].op[j].param1;
						m_mode = GL_MODULATE;
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
						if (m_mode == GL_MODULATE)
							m_fragment.color = ONE;

						m_usesT0 = TRUE;
						m_usesT1 = FALSE;
					}
					else if ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA))
					{
						if (m_mode == GL_MODULATE)
							m_fragment.color = ONE;

						m_usesT0 = FALSE;
						m_usesT1 = TRUE;
					}
					else
					{
						m_fragment.color = color->stage[i].op[j].param1;
						m_usesT0 = m_usesT1 = FALSE;
					}
					break;
				case SUB:
					break;
				case MUL:
					if ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA))
					{
						if (!m_usesT0 && !m_usesT1)
						{
							m_mode = GL_MODULATE;
							m_usesT0 = TRUE;
							m_usesT1 = FALSE;
						}
					}
					else if ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA))
					{
						if (!m_usesT0 && !m_usesT1)
						{
							m_mode = GL_MODULATE;
							m_usesT0 = FALSE;
							m_usesT1 = TRUE;
						}
					}
					else if (m_usesT0 || m_usesT1)
					{
						m_mode = GL_MODULATE;
						m_fragment.color = color->stage[i].op[j].param1;
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
						m_mode = GL_DECAL;
						m_fragment.color = color->stage[i].op[j].param2;
						m_usesT0 = TRUE;
						m_usesT1 = FALSE;
					}
					else if ((color->stage[i].op[j].param1 == TEXEL0) &&
					    ((color->stage[i].op[j].param2 != TEXEL0) && (color->stage[i].op[j].param2 != TEXEL0_ALPHA) &&
						 (color->stage[i].op[j].param2 != TEXEL1) && (color->stage[i].op[j].param2 != TEXEL1_ALPHA)) &&
						 (color->stage[i].op[j].param3 == TEXEL0_ALPHA))
					{
						m_mode = GL_DECAL;
						m_fragment.color = color->stage[i].op[j].param2;
						m_usesT0 = FALSE;
						m_usesT1 = TRUE;
					}
					break;
			}
		}
	}
}


void TexEnv::Set()
{
	combiner.usesT0 = m_usesT0;
	combiner.usesT1 = m_usesT1;
	combiner.usesLOD = FALSE;

	combiner.vertex.color = m_fragment.color;
	combiner.vertex.secondaryColor = COMBINED;
	combiner.vertex.alpha = m_fragment.alpha;

	glActiveTexture( GL_TEXTURE0 );

	if (m_usesT0 || m_usesT1)
		glEnable( GL_TEXTURE_2D );
	else
		glDisable( GL_TEXTURE_2D );

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, m_mode );
}
