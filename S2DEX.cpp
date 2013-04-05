#include "OpenGL.h"
#include "S2DEX.h"
#include "F3D.h"
#include "F3DEX.h"
#include "GBI.h"
#include "gSP.h" 
#include "gDP.h"
#include "RSP.h"
#include "Types.h"

void S2DEX_BG_1Cyc( u32 w0, u32 w1 )
{ 
	gSPBgRect1Cyc( w1 );
}

void S2DEX_BG_Copy( u32 w0, u32 w1 )
{
	gSPBgRectCopy( w1 );
}

void S2DEX_Obj_Rectangle( u32 w0, u32 w1 )
{
	gSPObjRectangle( w1 );
}

void S2DEX_Obj_Sprite( u32 w0, u32 w1 )
{
	gSPObjSprite( w1 );
}

void S2DEX_Obj_MoveMem( u32 w0, u32 w1 )
{
	if (_SHIFTR( w0, 0, 16 ) == 0)
		gSPObjMatrix( w1 );
	else
		gSPObjSubMatrix( w1 );
}

void S2DEX_Select_DL( u32 w0, u32 w1 )
{
}

void S2DEX_Obj_RenderMode( u32 w0, u32 w1 )
{
}

void S2DEX_Obj_Rectangle_R( u32 w0, u32 w1 )
{
}

void S2DEX_Obj_LoadTxtr( u32 w0, u32 w1 )
{
	gSPObjLoadTxtr( w1 );
}

void S2DEX_Obj_LdTx_Sprite( u32 w0, u32 w1 )
{
	gSPObjLoadTxSprite( w1 );
}

void S2DEX_Obj_LdTx_Rect( u32 w0, u32 w1 )
{
}

void S2DEX_Obj_LdTx_Rect_R( u32 w0, u32 w1 )
{
	gSPObjLoadTxRectR( w1 );
}

void S2DEX_Init()
{
	// Set GeometryMode flags
	GBI_InitFlags( F3DEX );

	gSP.geometryMode = 0;

	GBI.PCStackSize = 18;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_BG_1CYC,				S2DEX_BG_1CYC,			S2DEX_BG_1Cyc );
	GBI_SetGBI( G_BG_COPY,				S2DEX_BG_COPY,			S2DEX_BG_Copy );
	GBI_SetGBI( G_OBJ_RECTANGLE,		S2DEX_OBJ_RECTANGLE,	S2DEX_Obj_Rectangle );
	GBI_SetGBI( G_OBJ_SPRITE,			S2DEX_OBJ_SPRITE,		S2DEX_Obj_Sprite );
	GBI_SetGBI( G_OBJ_MOVEMEM,			S2DEX_OBJ_MOVEMEM,		S2DEX_Obj_MoveMem );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_SELECT_DL,			S2DEX_SELECT_DL,		S2DEX_Select_DL );
	GBI_SetGBI( G_OBJ_RENDERMODE,		S2DEX_OBJ_RENDERMODE,	S2DEX_Obj_RenderMode );
	GBI_SetGBI( G_OBJ_RECTANGLE_R,		S2DEX_OBJ_RECTANGLE_R,	S2DEX_Obj_Rectangle_R );
	GBI_SetGBI( G_OBJ_LOADTXTR,			S2DEX_OBJ_LOADTXTR,		S2DEX_Obj_LoadTxtr );
	GBI_SetGBI( G_OBJ_LDTX_SPRITE,		S2DEX_OBJ_LDTX_SPRITE,	S2DEX_Obj_LdTx_Sprite );
	GBI_SetGBI( G_OBJ_LDTX_RECT,		S2DEX_OBJ_LDTX_RECT,	S2DEX_Obj_LdTx_Rect );
	GBI_SetGBI( G_OBJ_LDTX_RECT_R,		S2DEX_OBJ_LDTX_RECT_R,	S2DEX_Obj_LdTx_Rect_R );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI(	G_LOAD_UCODE,			S2DEX_LOAD_UCODE,		F3DEX_Load_uCode );
}

