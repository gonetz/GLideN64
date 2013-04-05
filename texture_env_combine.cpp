#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
# include <stdlib.h> // malloc()
#endif
#include "OpenGL.h"
#include "Combiner.h"
#include "texture_env_combine.h"

/*	if ((i == COMBINED) && (OGL.ATIX_texture_env_route)) \
	{ \
		envCombiner->color[combinedUnit].outputTexture = GL_TEXTURE0_ARB + n; \
		envCombiner->color[n].a.source = GL_PRIMARY_COLOR_NV; \
		envCombiner->color[n].a.operand = GL_SRC_COLOR; \
	} \
	else if ((i == LOD_FRACTION) && (envCombiner->vertex.secondaryColor == COMBINED) && (OGL.ATIX_texture_env_route)) \
	{ \
		envCombiner->vertex.secondaryColor = LOD_FRACTION; \
		envCombiner->color[n].a.source = GL_SECONDARY_COLOR_ATIX; \
		envCombiner->color[n].a.operand = GL_SRC_COLOR; \
	} \*/

#define SetColorCombinerArg( n, a, i ) \
 if (TexEnvArgs[i].source == GL_CONSTANT_ARB) \
	{ \
		if ((i > 5) && ((envCombiner->alpha[n].constant == COMBINED) || (envCombiner->alpha[n].constant == i))) \
		{ \
			envCombiner->alpha[n].constant = i; \
			envCombiner->color[n].a.source = GL_CONSTANT_ARB; \
			envCombiner->color[n].a.operand = GL_SRC_ALPHA; \
		} \
		else if ((i > 5) && ((envCombiner->vertex.alpha == COMBINED) || (envCombiner->vertex.alpha == i))) \
		{ \
			envCombiner->vertex.alpha = i; \
			envCombiner->color[n].a.source = GL_PRIMARY_COLOR_ARB; \
			envCombiner->color[n].a.operand = GL_SRC_ALPHA; \
		} \
		else if ((envCombiner->color[n].constant == COMBINED) || (envCombiner->color[n].constant == i)) \
		{ \
			envCombiner->color[n].constant = i; \
			envCombiner->color[n].a.source = GL_CONSTANT_ARB; \
			envCombiner->color[n].a.operand = GL_SRC_COLOR; \
		} \
		else if (OGL.ATIX_texture_env_route && ((envCombiner->vertex.secondaryColor == COMBINED) || (envCombiner->vertex.secondaryColor == i))) \
		{ \
			envCombiner->vertex.secondaryColor = i; \
			envCombiner->color[n].a.source = GL_SECONDARY_COLOR_ATIX; \
			envCombiner->color[n].a.operand = GL_SRC_COLOR; \
		} \
		else if ((envCombiner->vertex.color == COMBINED) || (envCombiner->vertex.color == i))\
		{ \
			envCombiner->vertex.color = i; \
			envCombiner->color[n].a.source = GL_PRIMARY_COLOR_ARB; \
			envCombiner->color[n].a.operand = GL_SRC_COLOR; \
		} \
	} \
	else \
	{ \
		envCombiner->color[n].a.source = TexEnvArgs[i].source; \
		envCombiner->color[n].a.operand = TexEnvArgs[i].operand; \
	}

#define SetColorCombinerValues( n, a, s, o ) \
	envCombiner->color[n].a.source = s; \
	envCombiner->color[n].a.operand = o

/*	if ((TexEnvArgs[i].source == GL_PREVIOUS_ARB) && (OGL.ATIX_texture_env_route)) \
	{ \
		envCombiner->alpha[combinedUnit].outputTexture = GL_TEXTURE0_ARB + n; \
		envCombiner->alpha[n].a.source = GL_TEXTURE0_ARB + n; \
		envCombiner->alpha[n].a.operand = GL_SRC_ALPHA; \
	} \
	else*/
#define SetAlphaCombinerArg( n, a, i ) \
	if (TexEnvArgs[i].source == GL_CONSTANT_ARB) \
	{ \
		if ((envCombiner->alpha[n].constant == COMBINED) || (envCombiner->alpha[n].constant == i)) \
		{ \
			envCombiner->alpha[n].constant = i; \
			envCombiner->alpha[n].a.source = GL_CONSTANT_ARB; \
			envCombiner->alpha[n].a.operand = GL_SRC_ALPHA; \
		} \
		else if ((envCombiner->vertex.alpha == COMBINED) || (envCombiner->vertex.alpha == i)) \
		{ \
			envCombiner->vertex.alpha = i; \
			envCombiner->alpha[n].a.source = GL_PRIMARY_COLOR_ARB; \
			envCombiner->alpha[n].a.operand = GL_SRC_ALPHA; \
		} \
	} \
	else \
	{ \
		envCombiner->alpha[n].a.source = TexEnvArgs[i].source; \
		envCombiner->alpha[n].a.operand = GL_SRC_ALPHA; \
	}

#define SetAlphaCombinerValues( n, a, s, o ) \
	envCombiner->alpha[n].a.source = s; \
	envCombiner->alpha[n].a.operand = o

void Init_texture_env_combine()
{
	DWORD tex[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		TextureCache_ActivateDummy( i );
		//glActiveTextureARB( GL_TEXTURE0_ARB + i );
		//glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
		//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex );
		//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	if ((OGL.ARB_texture_env_crossbar) || (OGL.NV_texture_env_combine4) || (OGL.ATIX_texture_env_route))
	{
		TexEnvArgs[TEXEL0].source = GL_TEXTURE0_ARB;
		TexEnvArgs[TEXEL0_ALPHA].source = GL_TEXTURE0_ARB;

		TexEnvArgs[TEXEL1].source = GL_TEXTURE1_ARB;
		TexEnvArgs[TEXEL1_ALPHA].source = GL_TEXTURE1_ARB;
	}

	if (OGL.ATI_texture_env_combine3)
	{
		TexEnvArgs[ONE].source = GL_ONE;
		TexEnvArgs[ZERO].source = GL_ZERO;
	}
}

void Uninit_texture_env_combine()
{
	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	}
}

void Update_texture_env_combine_Colors( TexEnvCombiner *envCombiner )
{
	GLcolor color;

	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		SetConstant( color, envCombiner->color[i].constant, envCombiner->alpha[i].constant );

		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &color.r );
	}
}

TexEnvCombiner *Compile_texture_env_combine( Combiner *color, Combiner *alpha )
{
	TexEnvCombiner *envCombiner = (TexEnvCombiner*)malloc( sizeof( TexEnvCombiner ) );

	int curUnit, combinedUnit;

	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		envCombiner->color[i].combine = GL_REPLACE;
		envCombiner->alpha[i].combine = GL_REPLACE;

		SetColorCombinerValues( i, arg0, GL_PREVIOUS_ARB, GL_SRC_COLOR );
		SetColorCombinerValues( i, arg1, GL_PREVIOUS_ARB, GL_SRC_COLOR );
		SetColorCombinerValues( i, arg2, GL_PREVIOUS_ARB, GL_SRC_COLOR );
		envCombiner->color[i].constant = COMBINED;
		envCombiner->color[i].outputTexture = GL_TEXTURE0_ARB + i;

		SetAlphaCombinerValues( i, arg0, GL_PREVIOUS_ARB, GL_SRC_ALPHA );
		SetAlphaCombinerValues( i, arg1, GL_PREVIOUS_ARB, GL_SRC_ALPHA );
		SetAlphaCombinerValues( i, arg2, GL_PREVIOUS_ARB, GL_SRC_ALPHA );
		envCombiner->alpha[i].constant = COMBINED;
		envCombiner->alpha[i].outputTexture = GL_TEXTURE0_ARB + i;
	}

	envCombiner->usesT0 = FALSE;
	envCombiner->usesT1 = FALSE;

	envCombiner->vertex.color = COMBINED;
	envCombiner->vertex.secondaryColor = COMBINED;
	envCombiner->vertex.alpha = COMBINED;

	curUnit = 0;

	for (int i = 0; i < alpha->numStages; i++)
	{
		for (int j = 0; j < alpha->stage[i].numOps; j++)
		{
			float sb = 0.0f;

			if (alpha->stage[i].op[j].param1 == PRIMITIVE_ALPHA)
				sb = gDP.primColor.a;
			else if (alpha->stage[i].op[j].param1 == ENV_ALPHA)
				sb = gDP.envColor.a;
			else if (alpha->stage[i].op[j].param1 == ONE)
				sb = 1.0f;

			if (((alpha->stage[i].numOps - j) >= 3) &&
				(alpha->stage[i].op[j].op == SUB) &&
				(alpha->stage[i].op[j+1].op == MUL) &&
				(alpha->stage[i].op[j+2].op == ADD) &&
				(sb > 0.5f) && 
				(OGL.ARB_texture_env_combine))
			{
				envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				if (alpha->stage[i].op[j].param1 == ONE)
				{
					SetAlphaCombinerValues( curUnit, arg0, envCombiner->alpha[curUnit].arg0.source, GL_ONE_MINUS_SRC_ALPHA );
				}
				else
				{
					envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
					SetAlphaCombinerValues( curUnit, arg1, envCombiner->alpha[curUnit].arg0.source, GL_SRC_ALPHA );
					SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );

					curUnit++;
				}

				j++;

				envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				envCombiner->alpha[curUnit].combine = GL_MODULATE;
				SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );

				curUnit++;
				j++;

				envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
				SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );

				curUnit++;
			}
			else
			{
				envCombiner->usesT0 |= alpha->stage[i].op[j].param1 == TEXEL0_ALPHA;
				envCombiner->usesT1 |= alpha->stage[i].op[j].param1 == TEXEL1_ALPHA;

				switch (alpha->stage[i].op[j].op)
				{
					case LOAD:
						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						envCombiner->alpha[curUnit].combine = GL_REPLACE;

						SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );
						break;
					case SUB:
						if (!OGL.ARB_texture_env_combine)
							break;

						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						if ((j > 0) && (alpha->stage[i].op[j-1].op == LOAD) && (alpha->stage[i].op[j-1].param1 == ONE))
						{
							SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );
							envCombiner->alpha[curUnit].arg0.operand = GL_ONE_MINUS_SRC_ALPHA;
						}
						else if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->alpha[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetAlphaCombinerValues( curUnit, arg2, envCombiner->alpha[curUnit].arg1.source, envCombiner->alpha[curUnit].arg1.operand );
							envCombiner->alpha[curUnit].combine = GL_MODULATE_SUBTRACT_ATI;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
							curUnit++;
						}
						else
						{
							envCombiner->alpha[curUnit].combine = GL_SUBTRACT_ARB;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
							curUnit++;
						}
						break;
					case MUL:
						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						envCombiner->alpha[curUnit].combine = GL_MODULATE;

						SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
						curUnit++;
						break;
					case ADD:
						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							(alpha->stage[i].op[j].param1 == TEXEL1_ALPHA) && (curUnit == 0))
							curUnit++;

						if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->alpha[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetAlphaCombinerValues( curUnit, arg2, envCombiner->alpha[curUnit].arg1.source, envCombiner->alpha[curUnit].arg1.operand );
							envCombiner->alpha[curUnit].combine = GL_MODULATE_ADD_ATI;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
						}
						else
						{
							envCombiner->alpha[curUnit].combine = GL_ADD;
							SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param1 );
						}
						curUnit++;
						break;
					case INTER:
						envCombiner->usesT0 |= (alpha->stage[i].op[j].param2 == TEXEL0_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL0_ALPHA);
						envCombiner->usesT1 |= (alpha->stage[i].op[j].param2 == TEXEL1_ALPHA) || (alpha->stage[i].op[j].param3 == TEXEL1_ALPHA);

						envCombiner->alpha[curUnit].combine = GL_INTERPOLATE_ARB;

						SetAlphaCombinerArg( curUnit, arg0, alpha->stage[i].op[j].param1 );
						SetAlphaCombinerArg( curUnit, arg1, alpha->stage[i].op[j].param2 );
						SetAlphaCombinerArg( curUnit, arg2, alpha->stage[i].op[j].param3 );

						curUnit++;
						break;
				}
			}
		}
		combinedUnit = max( curUnit - 1, 0 );
	}

	envCombiner->usedUnits = max( curUnit, 1 );

	curUnit = 0;
	for (int i = 0; i < color->numStages; i++)
	{
		for (int j = 0; j < color->stage[i].numOps; j++)
		{
			float sb = 0.0f;

			if (color->stage[i].op[j].param1 == PRIMITIVE)
				sb = (gDP.primColor.r + gDP.primColor.b + gDP.primColor.g) / 3.0f;
			else if (color->stage[i].op[j].param1 == ENVIRONMENT)
				sb = (gDP.envColor.r + gDP.envColor.b + gDP.envColor.g) / 3.0f;

			// This helps with problems caused by not using signed values between texture units
			if (((color->stage[i].numOps - j) >= 3) &&
				(color->stage[i].op[j].op == SUB) &&
				(color->stage[i].op[j+1].op == MUL) &&
				(color->stage[i].op[j+2].op == ADD) &&
				(sb > 0.5f) && 
				(OGL.ARB_texture_env_combine))
			{
				envCombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				envCombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
				SetColorCombinerValues( curUnit, arg1, envCombiner->color[curUnit].arg0.source, envCombiner->color[curUnit].arg0.operand );
				SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );

				curUnit++;
				j++;

				envCombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				envCombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				envCombiner->color[curUnit].combine = GL_MODULATE;
				SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );

				curUnit++;
				j++;

				envCombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				envCombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
				SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );

				curUnit++;
			}
			else
			{
				envCombiner->usesT0 |= ((color->stage[i].op[j].param1 == TEXEL0) || (color->stage[i].op[j].param1 == TEXEL0_ALPHA));
				envCombiner->usesT1 |= ((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA));

				switch (color->stage[i].op[j].op)
				{
					case LOAD:
						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						envCombiner->color[curUnit].combine = GL_REPLACE;

						SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
						break;
					case SUB:
						if (!OGL.ARB_texture_env_combine)
							break;

						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						if ((j > 0) && (color->stage[i].op[j-1].op == LOAD) && (color->stage[i].op[j-1].param1 == ONE))
						{
							SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
							envCombiner->color[curUnit].arg0.operand = GL_ONE_MINUS_SRC_COLOR;
						}
						else if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->color[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetColorCombinerValues( curUnit, arg2, envCombiner->color[curUnit].arg1.source, envCombiner->color[curUnit].arg1.operand );
							envCombiner->color[curUnit].combine = GL_MODULATE_SUBTRACT_ATI;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
							curUnit++;
						}
						else
						{
							envCombiner->color[curUnit].combine = GL_SUBTRACT_ARB;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
							curUnit++;
						}
						break;
					case MUL:
						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						envCombiner->color[curUnit].combine = GL_MODULATE;

						SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
						curUnit++;
						break;
					case ADD:
						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param1 == TEXEL1_ALPHA)) && (curUnit == 0))
							curUnit++;

						// ATI_texture_env_combine3 adds GL_MODULATE_ADD_ATI; saves texture units
						if ((OGL.ATI_texture_env_combine3) && (curUnit > 0) && (envCombiner->color[curUnit - 1].combine == GL_MODULATE))
						{
							curUnit--;
							SetColorCombinerValues( curUnit, arg2, envCombiner->color[curUnit].arg1.source, envCombiner->color[curUnit].arg1.operand );
							envCombiner->color[curUnit].combine = GL_MODULATE_ADD_ATI;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
						}
						else
						{
							envCombiner->color[curUnit].combine = GL_ADD;
							SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param1 );
						}
						curUnit++;
						break;
					case INTER:
						envCombiner->usesT0 |= (color->stage[i].op[j].param2 == TEXEL0) || (color->stage[i].op[j].param3 == TEXEL0) || (color->stage[i].op[j].param3 == TEXEL0_ALPHA);
						envCombiner->usesT1 |= (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA);

						if (!(OGL.ARB_texture_env_crossbar || OGL.NV_texture_env_combine4) &&
							((color->stage[i].op[j].param1 == TEXEL1) || (color->stage[i].op[j].param2 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1) || (color->stage[i].op[j].param3 == TEXEL1_ALPHA)) && (curUnit == 0))
						{
							if (color->stage[i].op[j].param1 == TEXEL0)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
								color->stage[i].op[j].param1 = COMBINED;
							}
							if (color->stage[i].op[j].param2 == TEXEL0)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param2 )

								color->stage[i].op[j].param2 = COMBINED;
							}
							if (color->stage[i].op[j].param3 == TEXEL0)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param3 );
								color->stage[i].op[j].param3 = COMBINED;
							}
							if (color->stage[i].op[j].param3 == TEXEL0_ALPHA)
							{
								envCombiner->color[curUnit].combine = GL_REPLACE;
								SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param3 );
								color->stage[i].op[j].param3 = COMBINED_ALPHA;
							}

							curUnit++;
						}

						envCombiner->color[curUnit].combine = GL_INTERPOLATE_ARB;

						SetColorCombinerArg( curUnit, arg0, color->stage[i].op[j].param1 );
						SetColorCombinerArg( curUnit, arg1, color->stage[i].op[j].param2 );
						SetColorCombinerArg( curUnit, arg2, color->stage[i].op[j].param3 );

						curUnit++;
						break;
				}
			}
		}
		combinedUnit = max( curUnit - 1, 0 );
	}

	envCombiner->usedUnits = max( curUnit, envCombiner->usedUnits );

	return envCombiner;
}

void BeginTextureUpdate_texture_env_combine()
{
	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glDisable( GL_TEXTURE_2D );
	}
}

void EndTextureUpdate_texture_env_combine()
{
	for (int i = 0; i < ((TexEnvCombiner*)combiner.current->compiled)->usedUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );
		glEnable( GL_TEXTURE_2D );
	}
}

void Set_texture_env_combine( TexEnvCombiner *envCombiner )
{
	combiner.usesT0 = envCombiner->usesT0;
	combiner.usesT1 = envCombiner->usesT1;
	combiner.usesNoise = FALSE;

	combiner.vertex.color = envCombiner->vertex.color;
	combiner.vertex.secondaryColor = envCombiner->vertex.secondaryColor;
	combiner.vertex.alpha = envCombiner->vertex.alpha;

	for (int i = 0; i < OGL.maxTextureUnits; i++)
	{
		glActiveTextureARB( GL_TEXTURE0_ARB + i );

		if ((i < envCombiner->usedUnits ) || ((i < 2) && envCombiner->usesT1))
		{
			glEnable( GL_TEXTURE_2D );

			glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, envCombiner->color[i].combine );

			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,  envCombiner->color[i].arg0.source );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, envCombiner->color[i].arg0.operand );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,  envCombiner->color[i].arg1.source );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, envCombiner->color[i].arg1.operand );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,  envCombiner->color[i].arg2.source );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, envCombiner->color[i].arg2.operand );
//			if (OGL.ATIX_texture_env_route)
//				glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_OUTPUT_RGB_ATIX, envCombiner->color[i].outputTexture );

			glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, envCombiner->alpha[i].combine );

			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,  envCombiner->alpha[i].arg0.source );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, envCombiner->alpha[i].arg0.operand );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,  envCombiner->alpha[i].arg1.source );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, envCombiner->alpha[i].arg1.operand );
			glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB,  envCombiner->alpha[i].arg2.source );
			glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, envCombiner->alpha[i].arg2.operand );
//			if (OGL.ATIX_texture_env_route)
//				glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_OUTPUT_ALPHA_ATIX, envCombiner->alpha[i].outputTexture );
		}
		else
		{
			glDisable( GL_TEXTURE_2D );
		}			
	}
}
