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

#define SetColorCombinerInput( n, v, p, s ) \
	if (CombinerInputs[p].input == GL_CONSTANT_COLOR0_NV) \
	{ \
		if ((p > 5) && ((regCombiners->constant[0].alpha == COMBINED) || (regCombiners->constant[0].alpha == p))) \
		{ \
			regCombiners->constant[0].alpha = p; \
			regCombiners->color[n].v.input = GL_CONSTANT_COLOR0_NV; \
			regCombiners->color[n].v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((regCombiners->constant[1].alpha == COMBINED) || (regCombiners->constant[1].alpha == p))) \
		{ \
			regCombiners->constant[1].alpha = p; \
			regCombiners->color[n].v.input = GL_CONSTANT_COLOR1_NV; \
			regCombiners->color[n].v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((regCombiners->vertex.alpha == COMBINED) || (regCombiners->vertex.alpha == p))) \
		{ \
			regCombiners->vertex.alpha = p; \
			regCombiners->color[n].v.input = GL_PRIMARY_COLOR_NV; \
			regCombiners->color[n].v.usage = GL_ALPHA; \
		} \
		else if ((regCombiners->constant[0].color == COMBINED) || (regCombiners->constant[0].color == p)) \
		{ \
			regCombiners->constant[0].color = p; \
			regCombiners->color[n].v.input = GL_CONSTANT_COLOR0_NV; \
			regCombiners->color[n].v.usage = GL_RGB; \
		} \
		else if ((regCombiners->constant[1].color == COMBINED) || (regCombiners->constant[1].color == p)) \
		{ \
			regCombiners->constant[1].color = p; \
			regCombiners->color[n].v.input = GL_CONSTANT_COLOR1_NV; \
			regCombiners->color[n].v.usage = GL_RGB; \
		} \
		else if ((regCombiners->vertex.secondaryColor == COMBINED) || (regCombiners->vertex.secondaryColor == p)) \
		{ \
			regCombiners->vertex.secondaryColor = p; \
			regCombiners->color[n].v.input = GL_SECONDARY_COLOR_NV; \
			regCombiners->color[n].v.usage = GL_RGB; \
		} \
		else if ((regCombiners->vertex.color == COMBINED) || (regCombiners->vertex.color == p)) \
		{ \
			regCombiners->vertex.color = p; \
			regCombiners->color[n].v.input = GL_PRIMARY_COLOR_NV; \
			regCombiners->color[n].v.usage = GL_RGB; \
		} \
	} \
	else \
	{ \
			regCombiners->color[n].v.input = CombinerInputs[p].input; \
			regCombiners->color[n].v.usage = CombinerInputs[p].usage; \
	} \
	regCombiners->color[n].v.mapping = CombinerInputs[p].mapping; \
	regCombiners->color[n].v.used = s

#define SetColorCombinerVariable( n, v, i, m, u, s ) \
	regCombiners->color[n].v.input = i; \
	regCombiners->color[n].v.mapping = m; \
	regCombiners->color[n].v.usage = u; \
	regCombiners->color[n].v.used = s

#define SetAlphaCombinerInput( n, v, p, use ) \
	if (CombinerInputs[p].input == GL_CONSTANT_COLOR0_NV) \
	{ \
		if ((regCombiners->constant[0].alpha == COMBINED) || (regCombiners->constant[0].alpha == p)) \
		{ \
			regCombiners->constant[0].alpha = p; \
			regCombiners->alpha[n].v.input = GL_CONSTANT_COLOR0_NV; \
			regCombiners->alpha[n].v.usage = GL_ALPHA; \
		} \
		else if ((regCombiners->constant[1].alpha == COMBINED) || (regCombiners->constant[1].alpha == p)) \
		{ \
			regCombiners->constant[1].alpha = p; \
			regCombiners->alpha[n].v.input = GL_CONSTANT_COLOR1_NV; \
			regCombiners->alpha[n].v.usage = GL_ALPHA; \
		} \
		else if ((regCombiners->vertex.alpha == COMBINED) || (regCombiners->vertex.alpha == p)) \
		{ \
			regCombiners->vertex.alpha = p; \
			regCombiners->alpha[n].v.input = GL_PRIMARY_COLOR_NV; \
			regCombiners->alpha[n].v.usage = GL_ALPHA; \
		} \
	} \
	else \
	{ \
			regCombiners->alpha[n].v.input = CombinerInputs[p].input; \
			regCombiners->alpha[n].v.usage = CombinerInputs[p].usage; \
	} \
	regCombiners->alpha[n].v.mapping = CombinerInputs[p].mapping; \
	regCombiners->alpha[n].v.used = use

#define SetAlphaCombinerVariable( n, v, i, m, u, use ) \
	regCombiners->alpha[n].v.input = i; \
	regCombiners->alpha[n].v.mapping = m; \
	regCombiners->alpha[n].v.usage = u; \
	regCombiners->alpha[n].v.used = use

#define SetFinalCombinerInput( v, p, use ) \
	if (CombinerInputs[p].input == GL_CONSTANT_COLOR0_NV) \
	{ \
		if ((p > 5) && ((regCombiners->constant[0].alpha == COMBINED) || (regCombiners->constant[0].alpha == p))) \
		{ \
			regCombiners->constant[0].alpha = p; \
			regCombiners->final.v.input = GL_CONSTANT_COLOR0_NV; \
			regCombiners->final.v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((regCombiners->constant[1].alpha == COMBINED) || (regCombiners->constant[1].alpha == p))) \
		{ \
			regCombiners->constant[1].alpha = p; \
			regCombiners->final.v.input = GL_CONSTANT_COLOR1_NV; \
			regCombiners->final.v.usage = GL_ALPHA; \
		} \
		else if ((p > 5) && ((regCombiners->vertex.alpha == COMBINED) || (regCombiners->vertex.alpha == p))) \
		{ \
			regCombiners->vertex.alpha = p; \
			regCombiners->final.v.input = GL_PRIMARY_COLOR_NV; \
			regCombiners->final.v.usage = GL_ALPHA; \
		} \
		else if ((regCombiners->constant[0].color == COMBINED) || (regCombiners->constant[0].color == p)) \
		{ \
			regCombiners->constant[0].color = p; \
			regCombiners->final.v.input = GL_CONSTANT_COLOR0_NV; \
			regCombiners->final.v.usage = GL_RGB; \
		} \
		else if ((regCombiners->constant[1].color == COMBINED) || (regCombiners->constant[1].color == p)) \
		{ \
			regCombiners->constant[1].color = p; \
			regCombiners->final.v.input = GL_CONSTANT_COLOR1_NV; \
			regCombiners->final.v.usage = GL_RGB; \
		} \
		else if ((regCombiners->vertex.secondaryColor == COMBINED) || (regCombiners->vertex.secondaryColor == p)) \
		{ \
			regCombiners->vertex.secondaryColor = p; \
			regCombiners->final.v.input = GL_SECONDARY_COLOR_NV; \
			regCombiners->final.v.usage = GL_RGB; \
		} \
		else if ((regCombiners->vertex.color == COMBINED) || (regCombiners->vertex.color == p)) \
		{ \
			regCombiners->vertex.color = p; \
			regCombiners->final.v.input = GL_PRIMARY_COLOR_NV; \
			regCombiners->final.v.usage = GL_RGB; \
		} \
	} \
	else \
	{ \
			regCombiners->final.v.input = CombinerInputs[p].input; \
			regCombiners->final.v.usage = CombinerInputs[p].usage; \
	} \
	regCombiners->final.v.mapping = CombinerInputs[p].mapping; \
	regCombiners->final.v.used = use

#define SetFinalCombinerVariable( v, i, m, u, use ) \
	regCombiners->final.v.input = i; \
	regCombiners->final.v.mapping = m; \
	regCombiners->final.v.usage = u; \
	regCombiners->final.v.used = use

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

void Uninit_NV_register_combiners()
{
	glDisable( GL_REGISTER_COMBINERS_NV );
}

void Update_NV_register_combiners_Colors( RegisterCombiners *regCombiners)
{
	GLcolor color;

	for (int i = 0; i < 2; i++)
	{
		SetConstant( color, regCombiners->constant[i].color, regCombiners->constant[i].alpha );

	  	glCombinerParameterfvNV( GL_CONSTANT_COLOR0_NV + i, (GLfloat*)&color );
	}

	SetConstant( color, regCombiners->vertex.secondaryColor, ZERO );
	glSecondaryColor3fvEXT( (GLfloat*)&color );
}

RegisterCombiners *Compile_NV_register_combiners( Combiner *color, Combiner *alpha )
{
	int curCombiner, numCombiners;

	RegisterCombiners *regCombiners = (RegisterCombiners*)malloc( sizeof( RegisterCombiners ) );

	for (int i = 0; i < OGL.maxGeneralCombiners; i++)
	{
		SetColorCombinerInput( i, A, ZERO, FALSE );
		SetColorCombinerInput( i, B, ZERO, FALSE );
		SetColorCombinerInput( i, C, ZERO, FALSE );
		SetColorCombinerInput( i, D, ZERO, FALSE );
		regCombiners->color[i].output.ab = GL_DISCARD_NV;
		regCombiners->color[i].output.cd = GL_DISCARD_NV;
		regCombiners->color[i].output.sum = GL_DISCARD_NV;

		SetAlphaCombinerInput( i, A, ZERO, FALSE );
		SetAlphaCombinerInput( i, B, ZERO, FALSE );
		SetAlphaCombinerInput( i, C, ZERO, FALSE );
		SetAlphaCombinerInput( i, D, ZERO, FALSE );
		regCombiners->alpha[i].output.ab = GL_DISCARD_NV;
		regCombiners->alpha[i].output.cd = GL_DISCARD_NV;
		regCombiners->alpha[i].output.sum = GL_DISCARD_NV;
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

	regCombiners->usesT0 = FALSE;
	regCombiners->usesT1 = FALSE;
	regCombiners->usesNoise = FALSE;
	regCombiners->constant[0].color = COMBINED;
	regCombiners->constant[0].alpha = COMBINED;
	regCombiners->constant[1].color = COMBINED;
	regCombiners->constant[1].alpha = COMBINED;
	regCombiners->vertex.color = COMBINED;
	regCombiners->vertex.secondaryColor = COMBINED;
	regCombiners->vertex.alpha = COMBINED;
	
	curCombiner = 0;
	for (int i = 0; i < alpha->numStages; i++)
	{
		for (int j = 0; j < alpha->stage[i].numOps; j++)
		{
			regCombiners->usesT0 |= (alpha->stage[i].op[j].param1 == TEXEL0_ALPHA);
			regCombiners->usesT1 |= (alpha->stage[i].op[j].param1 == TEXEL1_ALPHA);
			regCombiners->usesNoise |= (alpha->stage[i].op[j].param1 == NOISE);

			switch (alpha->stage[i].op[j].op)
			{
				case LOAD:
					if (regCombiners->alpha[curCombiner].A.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;
					}

					SetAlphaCombinerInput( curCombiner, A, alpha->stage[i].op[j].param1, TRUE );
					SetAlphaCombinerInput( curCombiner, B, ONE, FALSE );
					regCombiners->alpha[curCombiner].output.sum = GL_SPARE0_NV;
					break;

				case SUB:
					if (regCombiners->alpha[curCombiner].C.used || regCombiners->alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetAlphaCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_ALPHA, TRUE );
						SetAlphaCombinerInput( curCombiner, B, ONE, FALSE );
						regCombiners->alpha[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetAlphaCombinerInput( curCombiner, C, alpha->stage[i].op[j].param1, TRUE );

					if (regCombiners->alpha[curCombiner].C.mapping == GL_UNSIGNED_INVERT_NV)
						regCombiners->alpha[curCombiner].C.mapping = GL_EXPAND_NORMAL_NV;
					else
						regCombiners->alpha[curCombiner].C.mapping = GL_SIGNED_NEGATE_NV;

					SetAlphaCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case MUL:
					if (regCombiners->alpha[curCombiner].B.used || regCombiners->alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetAlphaCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_ALPHA, TRUE );
						regCombiners->alpha[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetAlphaCombinerInput( curCombiner, B, alpha->stage[i].op[j].param1, TRUE );

					if (regCombiners->alpha[curCombiner].C.used)
					{
						SetAlphaCombinerInput( curCombiner, D, alpha->stage[i].op[j].param1, TRUE );
					}
					break;
				case ADD:
					if (regCombiners->alpha[curCombiner].C.used || regCombiners->alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetAlphaCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_ALPHA, TRUE );
						SetAlphaCombinerInput( curCombiner, B, ONE, FALSE );
						regCombiners->alpha[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetAlphaCombinerInput( curCombiner, C, alpha->stage[i].op[j].param1, TRUE );
					SetAlphaCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case INTER:
					if (regCombiners->alpha[curCombiner].A.used ||
						regCombiners->alpha[curCombiner].B.used ||
						regCombiners->alpha[curCombiner].C.used ||
						regCombiners->alpha[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;
					}
					regCombiners->usesT0 |= (alpha->stage[i].op[j].param2 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL0_ALPHA);
					regCombiners->usesT1 |= (alpha->stage[i].op[j].param2 == TEXEL1_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL1_ALPHA);
					regCombiners->usesNoise |= (alpha->stage[i].op[j].param2 == NOISE) || (alpha->stage[i].op[j].param3 == NOISE);

					SetAlphaCombinerInput( curCombiner, A, alpha->stage[i].op[j].param1, TRUE );
					SetAlphaCombinerInput( curCombiner, B, alpha->stage[i].op[j].param3, TRUE );
					SetAlphaCombinerInput( curCombiner, C, alpha->stage[i].op[j].param2, TRUE );
					SetAlphaCombinerVariable( curCombiner, D, regCombiners->alpha[curCombiner].B.input, GL_UNSIGNED_INVERT_NV, GL_ALPHA, TRUE );

					regCombiners->alpha[curCombiner].output.sum = GL_SPARE0_NV;
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
			regCombiners->usesT0 |= (color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA);
			regCombiners->usesT1 |= (color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA);
			regCombiners->usesNoise |= (color->stage[i].op[j].param1 == NOISE);
			
			switch (color->stage[i].op[j].op)
			{
				case LOAD:
					if (regCombiners->color[curCombiner].A.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;
					}

					SetColorCombinerInput( curCombiner, A, color->stage[i].op[j].param1, TRUE );
					SetColorCombinerInput( curCombiner, B, ONE, FALSE );
					regCombiners->color[curCombiner].output.sum = GL_SPARE0_NV;
					break;

				case SUB:
					if (regCombiners->color[curCombiner].C.used || regCombiners->color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
							break;

						SetColorCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB, TRUE );
						SetColorCombinerInput( curCombiner, B, ONE, FALSE );
						regCombiners->color[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetColorCombinerInput( curCombiner, C, color->stage[i].op[j].param1, TRUE );
					regCombiners->color[curCombiner].C.mapping = GL_SIGNED_NEGATE_NV;
					SetColorCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case MUL:
					if (regCombiners->color[curCombiner].B.used || regCombiners->color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
						{
							if ((!regCombiners->final.B.used) &&
								(!regCombiners->final.E.used) &&
								(!regCombiners->final.F.used))
							{
								SetFinalCombinerVariable( B, GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB, TRUE );
								SetFinalCombinerInput( E, COMBINED, TRUE );
								SetFinalCombinerInput( F, color->stage[i].op[j].param1, TRUE );
							}
							break;
						}

						SetColorCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB, TRUE );
						regCombiners->color[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetColorCombinerInput( curCombiner, B, color->stage[i].op[j].param1, TRUE );

					if (regCombiners->color[curCombiner].C.used)
					{
						SetColorCombinerInput( curCombiner, D, color->stage[i].op[j].param1, TRUE );
					}
					break;
				case ADD:
					if (regCombiners->color[curCombiner].C.used || regCombiners->color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
						{
							if (!regCombiners->final.D.used)
							{
								SetFinalCombinerInput( D, color->stage[i].op[j].param1, TRUE );
							}

							break;
						}

						SetColorCombinerVariable( curCombiner, A, GL_SPARE0_NV, GL_SIGNED_IDENTITY_NV, GL_RGB, TRUE );
						SetColorCombinerInput( curCombiner, B, ONE, FALSE );
						regCombiners->color[curCombiner].output.sum = GL_SPARE0_NV;
					}

					SetColorCombinerInput( curCombiner, C, color->stage[i].op[j].param1, TRUE );
					SetColorCombinerInput( curCombiner, D, ONE, FALSE );
					break;

				case INTER:
					regCombiners->usesT0 |= (color->stage[i].op[j].param2 == TEXEL0) || (color->stage[i].op[j].param3 == TEXEL0) || (color->stage[i].op[j].param2 == TEXEL0_ALPHA) || (color->stage[i].op[j].param3 == TEXEL0_ALPHA);
					regCombiners->usesT1 |= (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param2 == TEXEL1_ALPHA) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA);
					regCombiners->usesNoise |= (color->stage[i].op[j].param2 == NOISE) || (color->stage[i].op[j].param3 == NOISE);

					if (regCombiners->color[curCombiner].A.used ||
						regCombiners->color[curCombiner].B.used ||
						regCombiners->color[curCombiner].C.used ||
						regCombiners->color[curCombiner].D.used)
					{
						curCombiner++;

						if (curCombiner == OGL.maxGeneralCombiners)
						{
							if (!regCombiners->final.A.used &&
								!regCombiners->final.B.used &&
								!regCombiners->final.C.used)
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
					SetColorCombinerVariable( curCombiner, D, regCombiners->color[curCombiner].B.input, GL_UNSIGNED_INVERT_NV, regCombiners->color[curCombiner].B.usage, TRUE );

					regCombiners->color[curCombiner].output.sum = GL_SPARE0_NV;
					break;
			}
		}
	}

	regCombiners->numCombiners = max( min( curCombiner + 1, OGL.maxGeneralCombiners ), numCombiners );

	return regCombiners;
}

void Set_NV_register_combiners( RegisterCombiners *regCombiners )
{
	combiner.usesT0 = regCombiners->usesT0;
	combiner.usesT1 = regCombiners->usesT1;
	combiner.usesNoise = FALSE;

	combiner.vertex.color = regCombiners->vertex.color;
	combiner.vertex.secondaryColor = regCombiners->vertex.secondaryColor;
	combiner.vertex.alpha = regCombiners->vertex.alpha;

	glActiveTextureARB( GL_TEXTURE0_ARB );
	if (combiner.usesT0) glEnable( GL_TEXTURE_2D ); else glDisable( GL_TEXTURE_2D );

	glActiveTextureARB( GL_TEXTURE1_ARB );
	if (combiner.usesT1) glEnable( GL_TEXTURE_2D ); else glDisable( GL_TEXTURE_2D );

	glCombinerParameteriNV( GL_NUM_GENERAL_COMBINERS_NV, regCombiners->numCombiners );

	for (int i = 0; i < regCombiners->numCombiners; i++)
	{
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_A_NV, regCombiners->color[i].A.input, regCombiners->color[i].A.mapping, regCombiners->color[i].A.usage );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_B_NV, regCombiners->color[i].B.input, regCombiners->color[i].B.mapping, regCombiners->color[i].B.usage );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_C_NV, regCombiners->color[i].C.input, regCombiners->color[i].C.mapping, regCombiners->color[i].C.usage );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_RGB, GL_VARIABLE_D_NV, regCombiners->color[i].D.input, regCombiners->color[i].D.mapping, regCombiners->color[i].D.usage );
		glCombinerOutputNV( GL_COMBINER0_NV + i, GL_RGB, regCombiners->color[i].output.ab, regCombiners->color[i].output.cd, regCombiners->color[i].output.sum, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_A_NV, regCombiners->alpha[i].A.input, regCombiners->alpha[i].A.mapping, GL_ALPHA );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_B_NV, regCombiners->alpha[i].B.input, regCombiners->alpha[i].B.mapping, GL_ALPHA );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_C_NV, regCombiners->alpha[i].C.input, regCombiners->alpha[i].C.mapping, GL_ALPHA );
		glCombinerInputNV( GL_COMBINER0_NV + i, GL_ALPHA, GL_VARIABLE_D_NV, regCombiners->alpha[i].D.input, regCombiners->alpha[i].D.mapping, GL_ALPHA );
		glCombinerOutputNV( GL_COMBINER0_NV + i, GL_ALPHA, regCombiners->alpha[i].output.ab, regCombiners->alpha[i].output.cd, regCombiners->alpha[i].output.sum, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );
	}

	glFinalCombinerInputNV( GL_VARIABLE_A_NV, regCombiners->final.A.input, regCombiners->final.A.mapping, regCombiners->final.A.usage );
	glFinalCombinerInputNV( GL_VARIABLE_B_NV, regCombiners->final.B.input, regCombiners->final.B.mapping, regCombiners->final.B.usage );
	glFinalCombinerInputNV( GL_VARIABLE_C_NV, regCombiners->final.C.input, regCombiners->final.C.mapping, regCombiners->final.C.usage );
	glFinalCombinerInputNV( GL_VARIABLE_D_NV, regCombiners->final.D.input, regCombiners->final.D.mapping, regCombiners->final.D.usage );
	glFinalCombinerInputNV( GL_VARIABLE_E_NV, regCombiners->final.E.input, regCombiners->final.E.mapping, regCombiners->final.E.usage );
	glFinalCombinerInputNV( GL_VARIABLE_F_NV, regCombiners->final.F.input, regCombiners->final.F.mapping, regCombiners->final.F.usage );
	glFinalCombinerInputNV( GL_VARIABLE_G_NV, regCombiners->final.G.input, regCombiners->final.G.mapping, GL_ALPHA );
}
