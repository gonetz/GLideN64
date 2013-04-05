#include "glN64.h"
#include "Debug.h"
#include "F3D.h"
#include "N64.h"
#include "RSP.h"	
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

void F3D_SPNoOp( u32 w0, u32 w1 )
{
	gSPNoOp();
}

void F3D_Mtx( u32 w0, u32 w1 )
{
	if (_SHIFTR( w0, 0, 16 ) != 64)
	{
//		GBI_DetectUCode(); // Something's wrong
#ifdef DEBUG
	DebugMsg( DEBUG_MEDIUM | DEBUG_HIGH | DEBUG_ERROR, "G_MTX: address = 0x%08X    length = %i    params = 0x%02X\n", w1, _SHIFTR( w0, 0, 16 ), _SHIFTR( w0, 16, 8 ) );
#endif
		return;
	}

	gSPMatrix( w1, _SHIFTR( w0, 16, 8 ) );
}

void F3D_Reserved0( u32 w0, u32 w1 )
{
#ifdef DEBUG
	DebugMsg( DEBUG_MEDIUM | DEBUG_IGNORED | DEBUG_UNKNOWN, "G_RESERVED0: w0=0x%08lX w1=0x%08lX\n", w0, w1 );
#endif
}

void F3D_MoveMem( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 16, 8 ))
	{
		case F3D_MV_VIEWPORT://G_MV_VIEWPORT:
			gSPViewport( w1 );
			break;
		case G_MV_MATRIX_1:
			gSPForceMatrix( w1 );

			// force matrix takes four commands
			RSP.PC[RSP.PCi] += 24;
			break;
		case G_MV_L0:
			gSPLight( w1, LIGHT_1 );
			break;
		case G_MV_L1:
			gSPLight( w1, LIGHT_2 );
			break;
		case G_MV_L2:
			gSPLight( w1, LIGHT_3 );
			break;
		case G_MV_L3:
			gSPLight( w1, LIGHT_4 );
			break;
		case G_MV_L4:
			gSPLight( w1, LIGHT_5 );
			break;
		case G_MV_L5:
			gSPLight( w1, LIGHT_6 );
			break;
		case G_MV_L6:
			gSPLight( w1, LIGHT_7 );
			break;
		case G_MV_L7:
			gSPLight( w1, LIGHT_8 );
			break;
		case G_MV_LOOKATX:
			break;
		case G_MV_LOOKATY:
			break;
	}
}

void F3D_Vtx( u32 w0, u32 w1 )
{
	gSPVertex( w1, _SHIFTR( w0, 20, 4 ) + 1, _SHIFTR( w0, 16, 4 ) );
}

void F3D_Reserved1( u32 w0, u32 w1 )
{
}

void F3D_DList( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 16, 8 ))
	{
		case G_DL_PUSH:
			gSPDisplayList( w1 );
			break;
		case G_DL_NOPUSH:
			gSPBranchList( w1 );
			break;
	}
}

void F3D_Reserved2( u32 w0, u32 w1 )
{
}

void F3D_Reserved3( u32 w0, u32 w1 )
{
}

void F3D_Sprite2D_Base( u32 w0, u32 w1 )
{
	//gSPSprite2DBase( w1 );
	RSP.PC[RSP.PCi] += 8;
}

void F3D_Tri1( u32 w0, u32 w1 )
{
	gSP1Triangle( _SHIFTR( w1, 16, 8 ) / 10, 
		          _SHIFTR( w1, 8, 8 ) / 10, 
				  _SHIFTR( w1, 0, 8 ) / 10, 
				  _SHIFTR( w1, 24, 8 ) );
}

void F3D_CullDL( u32 w0, u32 w1 )
{
	gSPCullDisplayList( _SHIFTR( w0, 0, 24 ) / 40, (w1 / 40) - 1 );
}

void F3D_PopMtx( u32 w0, u32 w1 )
{
	gSPPopMatrix( w1 );
}

void F3D_MoveWord( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 0, 8 ))
	{
		case G_MW_MATRIX:
			gSPInsertMatrix( _SHIFTR( w0, 8, 16 ), w1 );
			break;
		case G_MW_NUMLIGHT:
			gSPNumLights( ((w1 - 0x80000000) >> 5) - 1 );
			break;
		case G_MW_CLIP:
			gSPClipRatio( w1 );
			break;
		case G_MW_SEGMENT:
			gSPSegment( _SHIFTR( w0, 8, 16 ) >> 2, w1 & 0x00FFFFFF );
			break;
		case G_MW_FOG:
/*			u32 fm, fo, min, max;

			fm = _SHIFTR( w1, 16, 16 );
			fo = _SHIFTR( w1, 0, 16 );

			min = 500 - (fo * (128000 / fm)) / 256;
			max = (128000 / fm) + min;*/

			gSPFogFactor( (s16)_SHIFTR( w1, 16, 16 ), (s16)_SHIFTR( w1, 0, 16 ) );
			break;
		case G_MW_LIGHTCOL:
			switch (_SHIFTR( w0, 8, 16 ))
			{
				case F3D_MWO_aLIGHT_1:
					gSPLightColor( LIGHT_1, w1 );
					break;
				case F3D_MWO_aLIGHT_2:
					gSPLightColor( LIGHT_2, w1 );
					break;
				case F3D_MWO_aLIGHT_3:
					gSPLightColor( LIGHT_3, w1 );
					break;
				case F3D_MWO_aLIGHT_4:
					gSPLightColor( LIGHT_4, w1 );
					break;
				case F3D_MWO_aLIGHT_5:
					gSPLightColor( LIGHT_5, w1 );
					break;
				case F3D_MWO_aLIGHT_6:
					gSPLightColor( LIGHT_6, w1 );
					break;
				case F3D_MWO_aLIGHT_7:
					gSPLightColor( LIGHT_7, w1 );
					break;
				case F3D_MWO_aLIGHT_8:
					gSPLightColor( LIGHT_8, w1 );
					break;
			}
			break;
		case G_MW_POINTS:
			gSPModifyVertex( _SHIFTR( w0, 8, 16 ) / 40, _SHIFTR( w0, 0, 8 ) % 40, w1 );
			break;
		case G_MW_PERSPNORM:
			gSPPerspNormalize( w1 );
			break;
	}
}

void F3D_Texture( u32 w0, u32 w1 )
{
	gSPTexture( _FIXED2FLOAT( _SHIFTR( w1, 16, 16 ), 16 ), 
		        _FIXED2FLOAT( _SHIFTR( w1, 0, 16 ), 16 ), 
		        _SHIFTR( w0, 11, 3 ), 
				_SHIFTR( w0, 8, 3 ), 
				_SHIFTR( w0, 0, 8 ) );
}

void F3D_SetOtherMode_H( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 8, 8 ))
	{
		case G_MDSFT_PIPELINE:
			gDPPipelineMode( w1 >> G_MDSFT_PIPELINE );
			break;
		case G_MDSFT_CYCLETYPE:
			gDPSetCycleType( w1 >> G_MDSFT_CYCLETYPE );
			break;
		case G_MDSFT_TEXTPERSP:
			gDPSetTexturePersp( w1 >> G_MDSFT_TEXTPERSP );
			break;
		case G_MDSFT_TEXTDETAIL:
			gDPSetTextureDetail( w1 >> G_MDSFT_TEXTDETAIL );
			break;
		case G_MDSFT_TEXTLOD:
			gDPSetTextureLOD( w1 >> G_MDSFT_TEXTLOD );
			break;
		case G_MDSFT_TEXTLUT:
			gDPSetTextureLUT( w1 >> G_MDSFT_TEXTLUT );
			break;
		case G_MDSFT_TEXTFILT:
			gDPSetTextureFilter( w1 >> G_MDSFT_TEXTFILT );
			break;
		case G_MDSFT_TEXTCONV:
			gDPSetTextureConvert( w1 >> G_MDSFT_TEXTCONV );
			break;
		case G_MDSFT_COMBKEY:
			gDPSetCombineKey( w1 >> G_MDSFT_COMBKEY );
			break;
		case G_MDSFT_RGBDITHER:
			gDPSetColorDither( w1 >> G_MDSFT_RGBDITHER );
			break;
		case G_MDSFT_ALPHADITHER:
			gDPSetAlphaDither( w1 >> G_MDSFT_ALPHADITHER );
			break;
		default:
			u32 shift = _SHIFTR( w0, 8, 8 );
			u32 length = _SHIFTR( w0, 0, 8 );
			u32 mask = ((1 << length) - 1) << shift;

			gDP.otherMode.h &= ~mask;
			gDP.otherMode.h |= w1 & mask;

			gDP.changed |= CHANGED_CYCLETYPE;
			break;
	}
}

void F3D_SetOtherMode_L( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 8, 8 ))
	{
		case G_MDSFT_ALPHACOMPARE:
			gDPSetAlphaCompare( w1 >> G_MDSFT_ALPHACOMPARE );
			break;
		case G_MDSFT_ZSRCSEL:
			gDPSetDepthSource( w1 >> G_MDSFT_ZSRCSEL );
			break;
		case G_MDSFT_RENDERMODE:
			gDPSetRenderMode( w1 & 0xCCCCFFFF, w1 & 0x3333FFFF );
			break;
		default:
			u32 shift = _SHIFTR( w0, 8, 8 );
			u32 length = _SHIFTR( w0, 0, 8 );
			u32 mask = ((1 << length) - 1) << shift;

			gDP.otherMode.l &= ~mask;
			gDP.otherMode.l |= w1 & mask;

			gDP.changed |= CHANGED_RENDERMODE | CHANGED_ALPHACOMPARE;
			break;
	}
}

void F3D_EndDL( u32 w0, u32 w1 )
{
	gSPEndDisplayList();
}

void F3D_SetGeometryMode( u32 w0, u32 w1 )
{
	gSPSetGeometryMode( w1 );
}

void F3D_ClearGeometryMode( u32 w0, u32 w1 )
{
	gSPClearGeometryMode( w1 );
}

void F3D_Line3D( u32 w0, u32 w1 )
{
	// Hmmm...
}

void F3D_Quad( u32 w0, u32 w1 )
{
	gSP1Quadrangle( _SHIFTR( w1, 24, 8 ) / 10, _SHIFTR( w1, 16, 8 ) / 10, _SHIFTR( w1, 8, 8 ) / 10, _SHIFTR( w1, 0, 8 ) / 10 );
}

void F3D_RDPHalf_1( u32 w0, u32 w1 )
{
/*	if (_SHIFTR( w1, 24, 8 ) == 0xCE)
	{
		u32 w2 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];
		RSP.PC[RSP.PCi] += 8;

		gDPTextureRectangle( gDP.scissor.ulx,									// ulx
							_FIXED2FLOAT( _SHIFTR( w2,  0, 16 ), 2 ),			// uly
							gDP.scissor.lrx,			// lrx
							_FIXED2FLOAT( _SHIFTR( w2,  16, 16 ), 2 ),			// lry
							0,													// tile
							0,		// s
							0,		// t
							1,		// dsdx
							1 );		// dsdy
	}*/
	gDP.half_1 = w1;
}

void F3D_RDPHalf_2( u32 w0, u32 w1 )
{
	gDP.half_2 = w1;
}

void F3D_RDPHalf_Cont( u32 w0, u32 w1 )
{
}

void F3D_Tri4( u32 w0, u32 w1 )
{
	gSP4Triangles( _SHIFTR( w0,  0, 4 ), _SHIFTR( w1,  0, 4 ), _SHIFTR( w1,  4, 4 ),
		           _SHIFTR( w0,  4, 4 ), _SHIFTR( w1,  8, 4 ), _SHIFTR( w1, 12, 4 ),
				   _SHIFTR( w0,  8, 4 ), _SHIFTR( w1, 16, 4 ), _SHIFTR( w1, 20, 4 ),
				   _SHIFTR( w0, 12, 4 ), _SHIFTR( w1, 24, 4 ), _SHIFTR( w1, 28, 4 ) );
}

void F3D_Init()
{
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,				F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3D_RESERVED0,			F3D_Reserved0 );
	GBI_SetGBI( G_MOVEMEM,				F3D_MOVEMEM,			F3D_MoveMem );
	GBI_SetGBI( G_VTX,					F3D_VTX,				F3D_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3D_RESERVED1,			F3D_Reserved1 );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_RESERVED2,			F3D_RESERVED2,			F3D_Reserved2 );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );
	GBI_SetGBI( G_SPRITE2D_BASE,		F3D_SPRITE2D_BASE,		F3D_Sprite2D_Base );

	GBI_SetGBI( G_TRI1,					F3D_TRI1,				F3D_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3D_CULLDL,				F3D_CullDL );
	GBI_SetGBI( G_POPMTX,				F3D_POPMTX,				F3D_PopMtx );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_QUAD,					F3D_QUAD,				F3D_Quad );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI( G_RDPHALF_CONT,			F3D_RDPHALF_CONT,		F3D_RDPHalf_Cont );
	GBI_SetGBI( G_TRI4,					F3D_TRI4,				F3D_Tri4 );
}

