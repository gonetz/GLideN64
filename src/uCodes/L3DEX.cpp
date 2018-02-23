#include <GLideN64.h>
#include <DebugDump.h>
#include "F3D.h"
#include "F3DEX.h"
#include "L3D.h"
#include "L3DEX.h"
#include <N64.h>
#include <RSP.h>
#include <RDP.h>
#include <gSP.h>
#include <gDP.h>
#include <GBI.h>

void L3DEX_Line3D( u32 w0, u32 w1 )
{
	u32 wd = _SHIFTR( w1, 0, 8 );

	if (wd == 0)
		gSPLine3D( _SHIFTR( w1, 17, 7 ), _SHIFTR( w1, 9, 7 ), 0 );
	else
		gSPLineW3D( _SHIFTR( w1, 17, 7 ), _SHIFTR( w1, 9, 7 ), wd, 0 );
}

void L3DEX_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3DEX );

	GBI.PCStackSize = 18;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,				F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3D_RESERVED0,			F3D_Reserved0 );
	GBI_SetGBI( G_MOVEMEM,				F3D_MOVEMEM,			F3D_MoveMem );
	GBI_SetGBI( G_VTX,					F3D_VTX,				F3DEX_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3D_RESERVED1,			F3D_Reserved1 );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_RESERVED2,			F3D_RESERVED2,			F3D_Reserved2 );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );
	GBI_SetGBI( G_SPRITE2D_BASE,		F3D_SPRITE2D_BASE,		F3D_Sprite2D_Base );

//	GBI_SetGBI( G_TRI1,					F3D_TRI1,				F3DEX_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3D_CULLDL,				F3DEX_CullDL );
	GBI_SetGBI( G_POPMTX,				F3D_POPMTX,				F3D_PopMtx );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_LINE3D,				L3D_LINE3D,				L3DEX_Line3D );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI( G_MODIFYVTX,			F3DEX_MODIFYVTX,		F3DEX_ModifyVtx );
//	GBI_SetGBI( G_TRI2,					F3DEX_TRI2,				F3DEX_Tri2 );
	GBI_SetGBI( G_BRANCH_Z,				F3DEX_BRANCH_Z,			F3DEX_Branch_Z );
	GBI_SetGBI(	G_LOAD_UCODE,			F3DEX_LOAD_UCODE,		F3DEX_Load_uCode );
}

