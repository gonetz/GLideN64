/* Star Wars Rogue Squadron ucode
 * Ported from Lemmy's LemNemu plugin
 * Incomplete!
 */

#include "GLideN64.h"
#include "DebugDump.h"
#include "F3D.h"
#include "F3DEX.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"
#include "DisplayWindow.h"

#define F3DSWRS_VTXCOLOR			0x02
#define F3DSWRS_MOVEMEM				0x03
#define F3DSWRS_VTX					0x04
#define F3DSWRS_JUMP3				0x05
#define F3DSWRS_DL					0x06
#define F3DSWRS_BRANCHDL			0x07

#define F3DSWRS_TRI2				0xB4
#define F3DSWRS_JUMP2				0xB5
#define F3DSWRS_MOVEWORD			0xBC
#define F3DSWRS_HEIGHTFIELD			0xBD
#define F3DSWRS_SETOTHERMODE_H_EX	0xBE
#define F3DSWRS_TRI1				0xBF

#define F3DSWRS_MV_TEXSCALE			0x82

void F3DSWRS_VertexColor(u32, u32 _w1)
{
	gSPSetVertexColorBase(_w1);
}

void F3DSWRS_MoveMem(u32 _w0, u32)
{
	switch (_SHIFTR(_w0, 16, 8)) {
	case F3D_MV_VIEWPORT://G_MV_VIEWPORT:
		gSPViewport(RSP.PC[RSP.PCi] + 8);
		break;
	case F3DSWRS_MV_TEXSCALE:
		gSP.textureCoordScale[0] = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 16];
		gSP.textureCoordScale[1] = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 12];
		break;
	}
	RSP.PC[RSP.PCi] += 16;
}

void F3DSWRS_Vtx(u32 _w0, u32 _w1)
{
	gSPSWVertex( _w1, _SHIFTR( _w0, 10, 6 ), 0 );
}

void F3DSWRS_Jump2(u32, u32)
{
	RSP.PC[RSP.PCi] = RSP.swDL[RSP.PCi].SWStartDL;
	RSP.swDL[RSP.PCi].SWStartDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi]], 0, 24);
	RSP.swDL[RSP.PCi].SWOtherDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4], 0, 24);
}

void F3DSWRS_Jump3(u32, u32)
{
	RSP.PC[RSP.PCi] = RSP.swDL[RSP.PCi].SWOtherDL;
	RSP.swDL[RSP.PCi].SWStartDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi]], 0, 24);
	RSP.swDL[RSP.PCi].SWOtherDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4], 0, 24);
}

void F3DSWRS_DList(u32, u32 _w1)
{
	gSPSWDisplayList(_w1);
}

void F3DSWRS_BranchDList(u32, u32 _w1)
{
	gSPSWBranchList(_w1);
}

static
void F3DSWRS_PrepareVertices(const u32* _vert, const u32* _color, bool _useTex, u32 _num)
{
	const u32 sscale0 = _SHIFTR(gSP.textureCoordScale[0], 16, 16);
	const u32 tscale0 = _SHIFTR(gSP.textureCoordScale[0], 0, 16);
	const u32 sscale1 = _SHIFTR(gSP.textureCoordScale[1], 16, 16);
	const u32 tscale1 = _SHIFTR(gSP.textureCoordScale[1], 0, 16);

	GraphicsDrawer & drawer = dwnd().getDrawer();

	for (u32 i = 0; i < _num; ++i) {
		SPVertex & vtx = drawer.getVertex(_vert[i]);
		u8 *color = &RDRAM[gSP.vertexColorBase + _color[i]];
		vtx.r = color[3] * 0.0039215689f;
		vtx.g = color[2] * 0.0039215689f;
		vtx.b = color[1] * 0.0039215689f;
		vtx.a = color[0] * 0.0039215689f;

		if (_useTex) {
			const u32 st = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 16 + 4*i];
			u32 s = (s16)_SHIFTR(st, 16, 16);
			u32 t = (s16)_SHIFTR(st, 0, 16);
			if ((s & 0x8000) != 0)
				s |= ~0xffff;
			if ((t & 0x8000) != 0)
				t |= ~0xffff;
			const u32 VMUDN_S = s * sscale0;
			const u32 VMUDN_T = t * tscale0;
			const s16 low_acum_S = _SHIFTR(VMUDN_S, 16, 16);
			const s16 low_acum_T = _SHIFTR(VMUDN_T, 16, 16);
			const u32 VMADH_S = s * sscale1;
			const u32 VMADH_T = t * tscale1;
			const s16 hi_acum_S = _SHIFTR(VMADH_S, 0, 16);
			const s16 hi_acum_T = _SHIFTR(VMADH_T, 0, 16);
			const s16 scaledS = low_acum_S + hi_acum_S;
			const s16 scaledT = low_acum_T + hi_acum_T;

			if (gDP.otherMode.texturePersp == 0) {
				vtx.s = _FIXED2FLOAT(scaledS, 4);
				vtx.t = _FIXED2FLOAT(scaledT, 4);
			} else {
				vtx.s = _FIXED2FLOAT(scaledS, 5);
				vtx.t = _FIXED2FLOAT(scaledT, 5);
			}
		}
	}
}

void F3DSWRS_Tri1(u32 _w0, u32 _w1)
{
	const u32 v1 = (_SHIFTR( _w1, 13, 11 ) & 0x7F8) / 40;
	const u32 v2 = (_SHIFTR( _w1,  5, 11 ) & 0x7F8) / 40;
	const u32 v3 = ((_w1 <<  3) & 0x7F8) / 40;
	const u32 vert[3] = { v1, v2, v3 };

	const u32 nextCMD = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 8];
	const u32 color[3] = { _SHIFTR(nextCMD, 16, 8), _SHIFTR(nextCMD, 8, 8), _SHIFTR(nextCMD, 0, 8) };

	const bool useTex = (_w0 & 2) != 0;
	F3DSWRS_PrepareVertices(vert, color, useTex, 3);

	if (useTex)
		RSP.PC[RSP.PCi] += 16;

	gSP1Triangle(v1, v2, v3);
	RSP.PC[RSP.PCi] += 8;
}

void F3DSWRS_Tri2(u32 _w0, u32 _w1)
{
	const u32 v1 = (_SHIFTR( _w1, 13, 11 ) & 0x7F8) / 40;
	const u32 v2 = (_SHIFTR( _w1,  5, 11 ) & 0x7F8) / 40;
	const u32 v3 = ((_w1 <<  3) & 0x7F8) / 40;
	const u32 v4 = (_SHIFTR( _w1,  21, 11 ) & 0x7F8) / 40;
	const u32 vert[4] = { v1, v2, v3, v4 };

	const u32 nextCMD = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 8];
	const u32 color[4] = { _SHIFTR(nextCMD, 16, 8), _SHIFTR(nextCMD, 8, 8),
							_SHIFTR(nextCMD, 0, 8), _SHIFTR(nextCMD, 24, 8) };

	const bool useTex = (_w0 & 2) != 0;
	F3DSWRS_PrepareVertices(vert, color, useTex, 4);

	if (useTex)
		RSP.PC[RSP.PCi] += 16;

	gSP2Triangles(v1, v2, v3, 0, v1, v3, v4, 0);
	RSP.PC[RSP.PCi] += 8;
}

void F3DSWRS_MoveWord(u32 _w0, u32 _w1)
{
	switch (_SHIFTR( _w0, 0, 8 )){
//	case 0x58C: // This PC is used after a texrect in naboo
//		State.NabooPCAfterTexRect = Segment[Command.dl.segment] + Command.dl.addr;
//		break;
	case G_MW_CLIP:
		gSPClipRatio( _w1 );
		break;
	case G_MW_SEGMENT:
		gSPSegment( _SHIFTR( _w0, 8, 16 ) >> 2, _w1 & 0x00FFFFFF );
		break;
	case G_MW_FOG:
		gSPFogFactor( (s16)_SHIFTR( _w1, 16, 16 ), (s16)_SHIFTR( _w1, 0, 16 ) );
		break;
	case G_MW_LIGHTCOL:
		gSPLightColor((_SHIFTR( _w0, 8, 8 ) / 32) + 1, _w1 );
		break;
	case G_MW_PERSPNORM:
		gSPPerspNormalize( _w1 );
		break;
	}
}

void F3DSWRS_HeightField(u32, u32)
{
	// Lemmy's note:
	// seems to be similar to JUMP3, but calls actual function with A1=0x2C
	// it *might* need the same jump/branch code as JUMP3
	RSP.PC[RSP.PCi] += 16;
}

void F3DSWRS_SetOtherMode_H_EX(u32, u32 _w1)
{
	RSP.PC[RSP.PCi] += 8;
	gDP.otherMode.h &= *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
	gDP.otherMode.h |= _w1;
}

void F3DSWRS_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,				F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3DSWRS_VTXCOLOR,		F3DSWRS_VertexColor );
	GBI_SetGBI( G_MOVEMEM,				F3DSWRS_MOVEMEM,		F3DSWRS_MoveMem );
	GBI_SetGBI( G_VTX,					F3DSWRS_VTX,			F3DSWRS_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3DSWRS_JUMP3,			F3DSWRS_Jump3 );
	GBI_SetGBI( G_DL,					F3DSWRS_DL,				F3DSWRS_DList );
	GBI_SetGBI( G_RESERVED2,			F3DSWRS_BRANCHDL,		F3DSWRS_BranchDList );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );

	GBI_SetGBI( G_TRI1,					F3DSWRS_TRI1,			F3DSWRS_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3DSWRS_SETOTHERMODE_H_EX,F3DSWRS_SetOtherMode_H_EX );
	GBI_SetGBI( G_POPMTX,				F3DSWRS_HEIGHTFIELD,	F3DSWRS_HeightField );
	GBI_SetGBI( G_MOVEWORD,				F3DSWRS_MOVEWORD,		F3DSWRS_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_QUAD,					F3DSWRS_JUMP2,			F3DSWRS_Jump2 );
	GBI_SetGBI( G_RDPHALF_1,			F3DSWRS_TRI2,			F3DSWRS_Tri2 );
}
