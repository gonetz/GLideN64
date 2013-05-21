#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif // __LINUX__
#include "OpenGL.h"
#include "Combiner.h"
#include "NV_register_combiners.h"
#include "Debug.h"
#include "gDP.h"
#include "gSP.h"

static CombinerInput CombinerInputs[] =
{
	// CMB
	{ GL_SPARE0_NV,				GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// T0
	{ GL_TEXTURE0_ARB,			GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// T1
	{ GL_TEXTURE1_ARB,			GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// PRIM
	{ GL_CONSTANT_COLOR0_NV,	GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// SHADE
	{ GL_PRIMARY_COLOR_NV,		GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// ENV
	{ GL_CONSTANT_COLOR0_NV,	GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// CENTER
	{ GL_ZERO,					GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// SCALE
	{ GL_ZERO,					GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// CMBALPHA
	{ GL_SPARE0_NV,				GL_UNSIGNED_IDENTITY_NV,	GL_ALPHA },
	// T0ALPHA
	{ GL_TEXTURE0_ARB,			GL_UNSIGNED_IDENTITY_NV,	GL_ALPHA },
	// T1ALPHA
	{ GL_TEXTURE1_ARB,			GL_UNSIGNED_IDENTITY_NV,	GL_ALPHA },
	// PRIMALPHA
	{ GL_CONSTANT_COLOR0_NV,	GL_UNSIGNED_IDENTITY_NV,	GL_ALPHA },
	// SHADEALPHA
	{ GL_PRIMARY_COLOR_NV,		GL_UNSIGNED_IDENTITY_NV,	GL_ALPHA },
	// ENVALPHA
	{ GL_CONSTANT_COLOR0_NV,	GL_UNSIGNED_IDENTITY_NV,	GL_ALPHA },
	// LODFRAC
	{ GL_ZERO,					GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// PRIMLODFRAC
	{ GL_CONSTANT_COLOR0_NV,	GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// NOISE
	{ GL_TEXTURE1_ARB,			GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// K4
	{ GL_ZERO,					GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// K5
	{ GL_ZERO,					GL_UNSIGNED_IDENTITY_NV,	GL_RGB },
	// ONE
	{ GL_ZERO,					GL_UNSIGNED_INVERT_NV,		GL_RGB },
	// ZERO
	{ GL_ZERO,					GL_UNSIGNED_IDENTITY_NV,	GL_RGB }
};

#define SetColorCombinerInput( n, v, p, s ) \
	if (CombinerInputs[p].input == GL_CONSTANT_COLOR0_NV) \
	{ \
		if ((p > 5) && ((m_constant[0].alpha == COMBINED) || (m_constant[0].alpha == p))) \
		{ \
			m_constant[0].alpha = p; \
			m_color[n].v.input = GL_CONSTANT_COLOR0_NV; \
			m_color[n].v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((m_constant[1].alpha == COMBINED) || (m_constant[1].alpha == p))) \
		{ \
			m_constant[1].alpha = p; \
			m_color[n].v.input = GL_CONSTANT_COLOR1_NV; \
			m_color[n].v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((m_vertex.alpha == COMBINED) || (m_vertex.alpha == p))) \
		{ \
			m_vertex.alpha = p; \
			m_color[n].v.input = GL_PRIMARY_COLOR_NV; \
			m_color[n].v.usage = GL_ALPHA; \
		} \
		else if ((m_constant[0].color == COMBINED) || (m_constant[0].color == p)) \
		{ \
			m_constant[0].color = p; \
			m_color[n].v.input = GL_CONSTANT_COLOR0_NV; \
			m_color[n].v.usage = GL_RGB; \
		} \
		else if ((m_constant[1].color == COMBINED) || (m_constant[1].color == p)) \
		{ \
			m_constant[1].color = p; \
			m_color[n].v.input = GL_CONSTANT_COLOR1_NV; \
			m_color[n].v.usage = GL_RGB; \
		} \
		else if ((m_vertex.secondaryColor == COMBINED) || (m_vertex.secondaryColor == p)) \
		{ \
			m_vertex.secondaryColor = p; \
			m_color[n].v.input = GL_SECONDARY_COLOR_NV; \
			m_color[n].v.usage = GL_RGB; \
		} \
		else if ((m_vertex.color == COMBINED) || (m_vertex.color == p)) \
		{ \
			m_vertex.color = p; \
			m_color[n].v.input = GL_PRIMARY_COLOR_NV; \
			m_color[n].v.usage = GL_RGB; \
		} \
	} \
	else \
	{ \
			m_color[n].v.input = CombinerInputs[p].input; \
			m_color[n].v.usage = CombinerInputs[p].usage; \
	} \
	m_color[n].v.mapping = CombinerInputs[p].mapping; \
	m_color[n].v.used = s

#define SetColorCombinerVariable( n, v, i, m, u, s ) \
	m_color[n].v.input = i; \
	m_color[n].v.mapping = m; \
	m_color[n].v.usage = u; \
	m_color[n].v.used = s

#define SetAlphaCombinerInput( n, v, p, use ) \
	if (CombinerInputs[p].input == GL_CONSTANT_COLOR0_NV) \
	{ \
		if ((m_constant[0].alpha == COMBINED) || (m_constant[0].alpha == p)) \
		{ \
			m_constant[0].alpha = p; \
			m_alpha[n].v.input = GL_CONSTANT_COLOR0_NV; \
			m_alpha[n].v.usage = GL_ALPHA; \
		} \
		else if ((m_constant[1].alpha == COMBINED) || (m_constant[1].alpha == p)) \
		{ \
			m_constant[1].alpha = p; \
			m_alpha[n].v.input = GL_CONSTANT_COLOR1_NV; \
			m_alpha[n].v.usage = GL_ALPHA; \
		} \
		else if ((m_vertex.alpha == COMBINED) || (m_vertex.alpha == p)) \
		{ \
			m_vertex.alpha = p; \
			m_alpha[n].v.input = GL_PRIMARY_COLOR_NV; \
			m_alpha[n].v.usage = GL_ALPHA; \
		} \
	} \
	else \
	{ \
			m_alpha[n].v.input = CombinerInputs[p].input; \
			m_alpha[n].v.usage = CombinerInputs[p].usage; \
	} \
	m_alpha[n].v.mapping = CombinerInputs[p].mapping; \
	m_alpha[n].v.used = use

#define SetAlphaCombinerVariable( n, v, i, m, u, use ) \
	m_alpha[n].v.input = i; \
	m_alpha[n].v.mapping = m; \
	m_alpha[n].v.usage = u; \
	m_alpha[n].v.used = use

#define SetFinalCombinerInput( v, p, use ) \
	if (CombinerInputs[p].input == GL_CONSTANT_COLOR0_NV) \
	{ \
		if ((p > 5) && ((m_constant[0].alpha == COMBINED) || (m_constant[0].alpha == p))) \
		{ \
			m_constant[0].alpha = p; \
			m_final.v.input = GL_CONSTANT_COLOR0_NV; \
			m_final.v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((m_constant[1].alpha == COMBINED) || (m_constant[1].alpha == p))) \
		{ \
			m_constant[1].alpha = p; \
			m_final.v.input = GL_CONSTANT_COLOR1_NV; \
			m_final.v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((m_vertex.alpha == COMBINED) || (m_vertex.alpha == p))) \
		{ \
			m_vertex.alpha = p; \
			m_final.v.input = GL_PRIMARY_COLOR_NV; \
			m_final.v.usage = GL_ALPHA; \
		} \
		else if ((m_constant[0].color == COMBINED) || (m_constant[0].color == p)) \
		{ \
			m_constant[0].color = p; \
			m_final.v.input = GL_CONSTANT_COLOR0_NV; \
			m_final.v.usage = GL_RGB; \
		} \
		else if ((m_constant[1].color == COMBINED) || (m_constant[1].color == p)) \
		{ \
			m_constant[1].color = p; \
			m_final.v.input = GL_CONSTANT_COLOR1_NV; \
			m_final.v.usage = GL_RGB; \
		} \
		else if ((m_vertex.secondaryColor == COMBINED) || (m_vertex.secondaryColor == p)) \
		{ \
			m_vertex.secondaryColor = p; \
			m_final.v.input = GL_SECONDARY_COLOR_NV; \
			m_final.v.usage = GL_RGB; \
		} \
		else if ((m_vertex.color == COMBINED) || (m_vertex.color == p)) \
		{ \
			m_vertex.color = p; \
			m_final.v.input = GL_PRIMARY_COLOR_NV; \
			m_final.v.usage = GL_RGB; \
		} \
	} \
	else \
	{ \
			m_final.v.input = CombinerInputs[p].input; \
			m_final.v.usage = CombinerInputs[p].usage; \
	} \
	m_final.v.mapping = CombinerInputs[p].mapping; \
	m_final.v.used = use

#define SetFinalCombinerVariable( v, i, m, u, use ) \
	m_final.v.input = i; \
	m_final.v.mapping = m; \
	m_final.v.usage = u; \
	m_final.v.used = use

void Init_NV_register_combiners()
{
	glCombinerParameteriNV( GL_COLOR_SUM_CLAMP_NV, GL_TRUE );
	glEnable( GL_REGISTER_COMBINERS_NV );

	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glDisable( GL_TEXTURE_2D );
	}
}

void RegisterCombiners::UpdateColors()
{
	GLcolor color;

	for (int i = 0; i < 2; i++)
	{
		SetConstant( color, m_constant[i].color, m_constant[i].alpha );

	  	glCombinerParameterfvNV( GL_CONSTANT_COLOR0_NV + i, (GLfloat*)&color );
	}

	SetConstant( color, m_vertex.secondaryColor, ZERO );
	glSecondaryColor3fvEXT( (GLfloat*)&color );
}

RegisterCombiners::RegisterCombiners( Combiner *color, Combiner *alpha )
{
	int curCombiner, numCombiners;

	for (int i = 0; i < OGL.maxGeneralCombiners; i++)
	{
		SetColorCombinerInput( i, A, ZERO, FALSE );
		SetColorCombinerInput( i, B, ZERO, FALSE );
		SetColorCombinerInput( i, C, ZERO, FALSE );
		SetColorCombinerInput( i, D, ZERO, FALSE );
		m_color[i].output.ab = GL_DISCARD_NV;
		m_color[i].output.cd = GL_DISCARD_NV;
		m_color[i].output.sum = GL_DISCARD_NV;

		SetAlphaCombinerInput( i, A, ZERO, FALSE );
		SetAlphaCombinerInput( i, B, ZERO, FALSE );
		SetAlphaCombinerInput( i, C, ZERO, FALSE );
		SetAlphaCombinerInput( i, D, ZERO, FALSE );
		m_alpha[i].output.ab = GL_DISCARD_NV;
		m_alpha[i].output.cd = GL_DISCARD_NV;
		m_alpha[i].output.sum = GL_DISCARD_NV;
	}

	SetFinalCombinerInput( A, ONE, FALSE );
	SetFinalCombinerInput( B, COMBINED, FALSE );
	SetFinalCombinerInput( C, ZERO, FALSE );
	SetFinalCombinerInput( D, ZERO, FALSE );
	SetFinalCombinerInput( E, ZERO, FALSE );
	SetFinalCombinerInput( F, ZERO, FALSE );
	SetFinalCombinerInput( G, COMBINED, FALSE );

	if ((gSP.geometryMode & G_FOG) &&
	   ((gDP.otherMode.cycleType == G_CYC_1CYCLE) ||
	    (gDP.otherMode.cycleType == G_CYC_2CYCLE)) && OGL.fog)
	{
		SetFinalCombinerVariable( A, GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA, FALSE );
		SetFinalCombinerVariable( C, GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_RGB, FALSE );
	}

	m_usesT0 = FALSE;
	m_usesT1 = FALSE;
	m_usesNoise = FALSE;
	m_constant[0].color = COMBINED;
	m_constant[0].alpha = COMBINED;
	m_constant[1].color = COMBINED;
	m_constant[1].alpha = COMBINED;
	m_vertex.color = COMBINED;
	m_vertex.secondaryColor = COMBINED;
	m_vertex.alpha = COMBINED;
	
	curCombiner = 0;
	for (int i = 0; i < alpha->numStages; i++)
	{
		for (int j = 0; j < alpha->stage[i].numOps; j++)
		{
			m_usesT0 |= (alpha->stage[i].op[j].param1 == TEXEL0_ALPHA);
			m_usesT1 |= (alpha->stage[i].op[j].param1 == TEXEL1_ALPHA);
			m_usesNoise |= (alpha->stage[i].op[j].param1 == NOISE);

			switch (alpha->stage[i].op[j].op)
			{
				case LOAD:
					if (m_alpha[curCombiner].A.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;
					}

					SetAlphaCombinerInput( curCombiner, A, alpha->stage[i].op[j].param1, TRUE );
					SetAlphaCombinerInput( curCombiner, B, ONE, FALSE );
					m_alpha[curCombiner].output.sum = GL_SPARE0_NV;
					break;

				case SUB:
					if (m_alpha[curCombiner].C.used || m_alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetAlphaCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_ALPHA, TRUE );
						SetAlphaCombinerInput( curCombiner, B, ONE, FALSE );
						m_alpha[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetAlphaCombinerInput( curCombiner, C, alpha->stage[i].op[j].param1, TRUE );

					if (m_alpha[curCombiner].C.mapping == GL_UNSIGNED_INVERT_NV)
						m_alpha[curCombiner].C.mapping = GL_EXPAND_NORMAL_NV;
					else
						m_alpha[curCombiner].C.mapping = GL_SIGNED_NEGATE_NV;

					SetAlphaCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case MUL:
					if (m_alpha[curCombiner].B.used || m_alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetAlphaCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_ALPHA, TRUE );
						m_alpha[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetAlphaCombinerInput( curCombiner, B, alpha->stage[i].op[j].param1, TRUE );

					if (m_alpha[curCombiner].C.used)
					{
						SetAlphaCombinerInput( curCombiner, D, alpha->stage[i].op[j].param1, TRUE );
					}
					break;
				case ADD:
					if (m_alpha[curCombiner].C.used || m_alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetAlphaCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_ALPHA, TRUE );
						SetAlphaCombinerInput( curCombiner, B, ONE, FALSE );
						m_alpha[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetAlphaCombinerInput( curCombiner, C, alpha->stage[i].op[j].param1, TRUE );
					SetAlphaCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case INTER:
					if (m_alpha[curCombiner].A.used ||
						m_alpha[curCombiner].B.used ||
						m_alpha[curCombiner].C.used ||
						m_alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;
					}
					m_usesT0 |= (alpha->stage[i].op[j].param2 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL0_ALPHA);
					m_usesT1 |= (alpha->stage[i].op[j].param2 == TEXEL1_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL1_ALPHA);
					m_usesNoise |= (alpha->stage[i].op[j].param2 == NOISE) || (alpha->stage[i].op[j].param3 == NOISE);

					SetAlphaCombinerInput( curCombiner, A, alpha->stage[i].op[j].param1, TRUE );
					SetAlphaCombinerInput( curCombiner, B, alpha->stage[i].op[j].param3, TRUE );
					SetAlphaCombinerInput( curCombiner, C, alpha->stage[i].op[j].param2, TRUE );
					SetAlphaCombinerVariable( curCombiner, D, m_alpha[curCombiner].B.input, GL_UNSIGNED_INVERT_NV, GL_ALPHA, TRUE );

					m_alpha[curCombiner].output.sum = GL_SPARE0_NV;
					break;
			}

			if (curCombiner == OGL.maxGeneralCombiners)
				break; // Get out if the combiners are full
		}
		if (curCombiner == OGL.maxGeneralCombiners)
			break; // Get out if the combiners are full
	}

	numCombiners = min( curCombiner + 1, OGL.maxGeneralCombiners );

	curCombiner = 0;
	for (int i = 0; i < (color->numStages) && (curCombiner < OGL.maxGeneralCombiners); i++)
	{
		for (int j = 0; (j < color->stage[i].numOps) && (curCombiner < OGL.maxGeneralCombiners); j++)
		{
			m_usesT0 |= (color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA);
			m_usesT1 |= (color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA);
			m_usesNoise |= (color->stage[i].op[j].param1 == NOISE);
			
			switch (color->stage[i].op[j].op)
			{
				case LOAD:
					if (m_color[curCombiner].A.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;
					}

					SetColorCombinerInput( curCombiner, A, color->stage[i].op[j].param1, TRUE );
					SetColorCombinerInput( curCombiner, B, ONE, FALSE );
					m_color[curCombiner].output.sum = GL_SPARE0_NV;
					break;

				case SUB:
					if (m_color[curCombiner].C.used || m_color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetColorCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB, TRUE );
						SetColorCombinerInput( curCombiner, B, ONE, FALSE );
						m_color[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetColorCombinerInput( curCombiner, C, color->stage[i].op[j].param1, TRUE );
					m_color[curCombiner].C.mapping = GL_SIGNED_NEGATE_NV;
					SetColorCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case MUL:
					if (m_color[curCombiner].B.used || m_color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
						{
							if ((!m_final.B.used) &&
								(!m_final.E.used) &&
								(!m_final.F.used))
							{
								SetFinalCombinerVariable( B, GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB, TRUE );
								SetFinalCombinerInput( E, COMBINED, TRUE );
								SetFinalCombinerInput( F, color->stage[i].op[j].param1, TRUE );
							}
							break;
						}

						SetColorCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB, TRUE );
						m_color[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetColorCombinerInput( curCombiner, B, color->stage[i].op[j].param1, TRUE );

					if (m_color[curCombiner].C.used)
					{
						SetColorCombinerInput( curCombiner, D, color->stage[i].op[j].param1, TRUE );
					}
					break;
				case ADD:
					if (m_color[curCombiner].C.used || m_color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
						{
							if (!m_final.D.used)
							{
								SetFinalCombinerInput( D, color->stage[i].op[j].param1, TRUE );
							}

							break;
						}

						SetColorCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB, TRUE );
						SetColorCombinerInput( curCombiner, B, ONE, FALSE );
						m_color[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetColorCombinerInput( curCombiner, C, color->stage[i].op[j].param1, TRUE );
					SetColorCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case INTER:
					m_usesT0 |= (color->stage[i].op[j].param2 == TEXEL0) || (color->stage[i].op[j].param3 == TEXEL0) || (color->stage[i].op[j].param2 == TEXEL0_ALPHA) || (color->stage[i].op[j].param3 == TEXEL0_ALPHA);
					m_usesT1 |= (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param2 == TEXEL1_ALPHA) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA);
					m_usesNoise |= (color->stage[i].op[j].param2 == NOISE) || (color->stage[i].op[j].param3 == NOISE);

					if (m_color[curCombiner].A.used ||
						m_color[curCombiner].B.used ||
						m_color[curCombiner].C.used ||
						m_color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
						{
							if (!m_final.A.used &&
								!m_final.B.used &&
								!m_final.C.used)
							{
								SetFinalCombinerInput( A, color->stage[i].op[j].param3, TRUE );
								SetFinalCombinerInput( B, color->stage[i].op[j].param1, TRUE );
								SetFinalCombinerInput( C, color->stage[i].op[j].param2, TRUE );
							}
							break;
						}
					}

					SetColorCombinerInput( curCombiner, A, color->stage[i].op[j].param1, TRUE );
					SetColorCombinerInput( curCombiner, B, color->stage[i].op[j].param3, TRUE );
					SetColorCombinerInput( curCombiner, C, color->stage[i].op[j].param2, TRUE );
					SetColorCombinerVariable( curCombiner, D, m_color[curCombiner].B.input, GL_UNSIGNED_INVERT_NV, m_color[curCombiner].B.usage, TRUE );

					m_color[curCombiner].output.sum = GL_SPARE0_NV;
					break;
			}
		}
	}

	m_numCombiners = max( min( curCombiner + 1, OGL.maxGeneralCombiners ), numCombiners );
}

void RegisterCombiners::Set()
{
	combiner.usesT0 = m_usesT0;
	combiner.usesT1 = m_usesT1;
	combiner.usesLOD = FALSE;

	combiner.vertex.color = m_vertex.color;
	combiner.vertex.secondaryColor = m_vertex.secondaryColor;
	combiner.vertex.alpha = m_vertex.alpha;

	glActiveTextureARB( GL_TEXTURE0_ARB );
	if (combiner.usesT0) glEnable( GL_TEXTURE_2D ); else glDisable( GL_TEXTURE_2D );

	glActiveTextureARB( GL_TEXTURE1_ARB );
	if (combiner.usesT1) glEnable( GL_TEXTURE_2D ); else glDisable( GL_TEXTURE_2D );

	glCombinerParameteriNV( GL_NUM_GENERAL_COMBINERS_NV, m_numCombiners );

	for (int i = 0; i < m_numCombiners; i++)
	{
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_A_NV, m_color[i].A.input, m_color[i].A.mapping, m_color[i].A.usage );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_B_NV, m_color[i].B.input, m_color[i].B.mapping, m_color[i].B.usage );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_C_NV, m_color[i].C.input, m_color[i].C.mapping, m_color[i].C.usage );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_D_NV, m_color[i].D.input, m_color[i].D.mapping, m_color[i].D.usage );
		glCombinerOutputNV( GL_COMBINER0_NV + i, GL_RGB, m_color[i].output.ab, m_color[i].output.cd, m_color[i].output.sum, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_A_NV, m_alpha[i].A.input, m_alpha[i].A.mapping, GL_ALPHA );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_B_NV, m_alpha[i].B.input, m_alpha[i].B.mapping, GL_ALPHA );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_C_NV, m_alpha[i].C.input, m_alpha[i].C.mapping, GL_ALPHA );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_D_NV, m_alpha[i].D.input, m_alpha[i].D.mapping, GL_ALPHA );
		glCombinerOutputNV( GL_COMBINER0_NV + i, GL_ALPHA, m_alpha[i].output.ab, m_alpha[i].output.cd, m_alpha[i].output.sum, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );
	}

	glFinalCombinerInputNV( GL_VARIABLE_A_NV, m_final.A.input, m_final.A.mapping, m_final.A.usage );
	glFinalCombinerInputNV( GL_VARIABLE_B_NV, m_final.B.input, m_final.B.mapping, m_final.B.usage );
	glFinalCombinerInputNV( GL_VARIABLE_C_NV, m_final.C.input, m_final.C.mapping, m_final.C.usage );
	glFinalCombinerInputNV( GL_VARIABLE_D_NV, m_final.D.input, m_final.D.mapping, m_final.D.usage );
	glFinalCombinerInputNV( GL_VARIABLE_E_NV, m_final.E.input, m_final.E.mapping, m_final.E.usage );
	glFinalCombinerInputNV( GL_VARIABLE_F_NV, m_final.F.input, m_final.F.mapping, m_final.F.usage );
	glFinalCombinerInputNV( GL_VARIABLE_G_NV, m_final.G.input, m_final.G.mapping, GL_ALPHA );
}
