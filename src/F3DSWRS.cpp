/* Star Wars Rogue Squadron ucode
 * Ported from Lemmy's LemNemu plugin
 * Incomplete!
 */

#include <array>
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

#define F3DSWRS_SETOTHERMODE_L_EX	0xB3
#define F3DSWRS_TRI2				0xB4
#define F3DSWRS_JUMP2				0xB5
#define F3DSWRS_MOVEWORD			0xBC
#define F3DSWRS_HEIGHTFIELD			0xBD
#define F3DSWRS_SETOTHERMODE_H_EX	0xBE
#define F3DSWRS_TRI1				0xBF

#define F3DSWRS_MV_TEXSCALE			0x82
#define F3DSWRS_MW_FOG_MULTIPLIER	0x08
#define F3DSWRS_MW_FOG_OFFSET		0x0A

static u32 G_SETOTHERMODE_H_EX, G_SETOTHERMODE_L_EX;

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
		DebugMsg(DEBUG_NORMAL, "F3DSWRS_MoveMem Texscale(0x%08x, 0x%08x)\n",
				 gSP.textureCoordScale[0], gSP.textureCoordScale[1]);
		break;
	}
	RSP.PC[RSP.PCi] += 16;
}

void F3DSWRS_Vtx(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_Vtx (0x%08x, 0x%08x)\n", _w0, _w1);

	const u32 address = RSP_SegmentToPhysical(_w1);
	const u32 n = _SHIFTR(_w0, 10, 6);

	if ((address + sizeof(SWVertex)* n) > RDRAMSize)
		return;

	const SWVertex * vertex = (const SWVertex*)&RDRAM[address];
	gSPSWVertex(vertex, n, 0 );
}

void F3DSWRS_Jump2(u32, u32)
{
	RSP.PC[RSP.PCi] = RSP.swDL[RSP.PCi].SWStartDL;
	RSP.swDL[RSP.PCi].SWStartDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi]], 0, 24);
	RSP.swDL[RSP.PCi].SWOtherDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4], 0, 24);
}

static
void F3DSWRS_PrepareVertices(const u32* _vert, const u8* _colorbase, const u32* _color, const u8* _texbase, bool _useTex, u32 _num)
{
	const u32 sscale = _SHIFTR(gSP.textureCoordScale[0], 16, 16);
	const u32 tscale = _SHIFTR(gSP.textureCoordScale[0], 0, 16);
	const u32 sscale1 = _SHIFTR(gSP.textureCoordScale[1], 16, 16);
	const u32 tscale1 = _SHIFTR(gSP.textureCoordScale[1], 0, 16);

	GraphicsDrawer & drawer = dwnd().getDrawer();

	for (u32 i = 0; i < _num; ++i) {
		SPVertex & vtx = drawer.getVertex(_vert[i]);
		const u8 *color = _colorbase + _color[i];
		vtx.r = color[3] * 0.0039215689f;
		vtx.g = color[2] * 0.0039215689f;
		vtx.b = color[1] * 0.0039215689f;
		vtx.a = color[0] * 0.0039215689f;

		if (_useTex) {
			const u32 st = *(u32*)&_texbase[4 * i];
			u32 s = (s16)_SHIFTR(st, 16, 16);
			u32 t = (s16)_SHIFTR(st, 0, 16);
			if ((s & 0x8000) != 0)
				s |= ~0xffff;
			if ((t & 0x8000) != 0)
				t |= ~0xffff;
			const u32 VMUDN_S = s * sscale;
			const u32 VMUDN_T = t * tscale;
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
			}
			else {
				vtx.s = _FIXED2FLOAT(scaledS, 5);
				vtx.t = _FIXED2FLOAT(scaledT, 5);
			}
		}
	}
}

static
void Jump3_2(const u32 * _params, u32 * _result) {
//	if (_params[1] == 0xFDA00050 && _params[9] == 0xE2000200 && _params[8] == 0xF0000000)
//		int k = 0;
	typedef std::array<s16, 4> Vector;
	Vector v0 = { 0, 0, 0, 0 };
	Vector v1 = { 0, 0, 0, 0 };
	Vector v2 = { 0, 0, 0, 0 };
	Vector v3 = { 0, 0, 0, 0 };
	s16 V0 = _SHIFTR(_params[9], 0, 16);
	s16 V1 = _SHIFTR(_params[8], 0, 16);
	V1 <<= 4;
	v0[0] = _SHIFTR(_params[8], 16, 16);
	v0[1] = V1;
	v0[2] = _SHIFTR(_params[9], 16, 16);
	v2[0] = V0;
	v3[2] = V0;
	v1[1] = _SHIFTR(_params[1], 16, 16);
	_result[0] = ((v0[0] + v1[0]) << 16) | ((v0[1] + v1[1]) & 0xFFFF);
	_result[1] = ((v0[2] + v1[2]) << 16) | ((v0[3] + v1[3]) & 0xFFFF);
	v1 = v2;
	v1[1] = _SHIFTR(_params[1], 0, 16);
	_result[2] = ((v0[0] + v1[0]) << 16) | ((v0[1] + v1[1]) & 0xFFFF);
	_result[3] = ((v0[2] + v1[2]) << 16) | ((v0[3] + v1[3]) & 0xFFFF);
	v1 = v3;
	v1[1] = _SHIFTR(_params[2], 16, 16);
	_result[4] = ((v0[0] + v1[0]) << 16) | ((v0[1] + v1[1]) & 0xFFFF);
	_result[5] = ((v0[2] + v1[2]) << 16) | ((v0[3] + v1[3]) & 0xFFFF);
	for (u32 i = 0; i < 4; ++i)
		v1[i] = v2[i] + v3[i];
	v1[1] = _SHIFTR(_params[2], 0, 16);
	_result[6] = ((v0[0] + v1[0]) << 16) | ((v0[1] + v1[1]) & 0xFFFF);
	_result[7] = ((v0[2] + v1[2]) << 16) | ((v0[3] + v1[3]) & 0xFFFF);
}


bool _print = false;
void F3DSWRS_Jump3(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_Jump3 (0x%08x, 0x%08x)\n", _w0, _w1);

	const u32 mode = _SHIFTR(_w0, 8, 8);
	switch (mode) {
	case 0x02:
	{
		/*
		u32 params[10] = {
			0x05050200,
			0xFDA00050,
			0xFDD00190,
			0xF4F4F401,
			0xE4E4E4FF,
			0xFFFFFFFF,
			0xFFFFFFFF,
			0x000007E0,
			0xF0000000,
			0xE2000200 };
			*/
		u32 params[10];
		for (u32 i = 1; i < 10; ++i)
			params[i] = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + i * 4];

		u32 vecdata[8];
		Jump3_2(params, vecdata);
		const SWVertex * vertex = (const SWVertex*)&vecdata[0];
		gSPSWVertex(vertex, 4, 0);
		GraphicsDrawer & drawer = dwnd().getDrawer();
		if (_print) {
			for (u32 i = 0; i < 4; ++i) {
				SPVertex & v = drawer.getVertex(i);
				f32 sx = v.x / v.w * gSP.viewport.vscale[0] + gSP.viewport.vtrans[0];
				f32 sy = v.y / v.w * gSP.viewport.vscale[1] + gSP.viewport.vtrans[1];
				f32 sz = v.z / v.w * gSP.viewport.vscale[2] + gSP.viewport.vtrans[2];
				DebugMsg(DEBUG_NORMAL, "v[%d] x=%02f y=%02f z=%02f\n", i, sx, gDP.scissor.lry - sy, sz);
			}
		}

		const u32 v1 = 0;
		const u32 v2 = 1;
		const u32 v3 = 2;
		const u32 v4 = 3;
		const u32 vert[4] = { v1, v2, v3, v4 };

		const u32 colorbase[4] = { params[3] | 0xFF, params[4] | 0xFF, params[5] | 0xFF, params[6] | 0xFF };
		const u32 color[4] = { 0, 4, 8, 12 };

		const u32 tex = _SHIFTR(params[7], 0, 16);
		const u32 texbase[4] = { tex, tex | (tex << 16), 0, (tex << 16) };


		const bool useTex = true;// (_w0 & 2) != 0;
		F3DSWRS_PrepareVertices(vert, (u8*)colorbase, color, (u8*)texbase, useTex, 4);

		SPVertex & vtx2 = drawer.getVertex(v2);
		SPVertex & vtx3 = drawer.getVertex(v3);

		gSP.swrs_special = true;
		if (vtx3.z / vtx3.w > vtx2.z / vtx2.w)
			gSP2Triangles(v1, v2, v4, 0, v1, v4, v3, 0);
		else
			gSP2Triangles(v1, v4, v3, 0, v1, v2, v4, 0);
		dwnd().getDrawer().drawTriangles();
		gSP.swrs_special = false;

	}
		break;
	default:
		break;
	}
	RSP.PC[RSP.PCi] = RSP.swDL[RSP.PCi].SWOtherDL;
	RSP.swDL[RSP.PCi].SWStartDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi]], 0, 24);
	RSP.swDL[RSP.PCi].SWOtherDL = _SHIFTR(*(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4], 0, 24);
}

void F3DSWRS_DList(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_DList (0x%08x, 0x%08x)\n", _w0, _w1);
	gSPSWDisplayList(_w1);
}

void F3DSWRS_BranchDList(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_BranchDList (0x%08x, 0x%08x)\n", _w0, _w1);
	gSPSWBranchList(_w1);
}

void F3DSWRS_Tri1(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_Tri1 (0x%08x, 0x%08x)\n", _w0, _w1);
	const u32 v1 = (_SHIFTR(_w1, 13, 11) & 0x7F8) / 40;
	const u32 v2 = (_SHIFTR( _w1,  5, 11 ) & 0x7F8) / 40;
	const u32 v3 = ((_w1 <<  3) & 0x7F8) / 40;
	const u32 vert[3] = { v1, v2, v3 };

	const u32 nextCMD = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 8];
	const u32 color[3] = { _SHIFTR(nextCMD, 16, 8), _SHIFTR(nextCMD, 8, 8), _SHIFTR(nextCMD, 0, 8) };

	const bool useTex = (_w0 & 2) != 0;
    const u8 * texbase = RDRAM + RSP.PC[RSP.PCi] + 16;
    F3DSWRS_PrepareVertices(vert, RDRAM + gSP.vertexColorBase, color, texbase, useTex, 3);

	if (useTex)
		RSP.PC[RSP.PCi] += 16;

	gSP1Triangle(v1, v2, v3);
	RSP.PC[RSP.PCi] += 8;
}

void F3DSWRS_Tri2(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_Tri2 (0x%08x, 0x%08x)\n", _w0, _w1);
	const u32 v1 = (_SHIFTR(_w1, 13, 11) & 0x7F8) / 40;
	const u32 v2 = (_SHIFTR( _w1,  5, 11 ) & 0x7F8) / 40;
	const u32 v3 = ((_w1 <<  3) & 0x7F8) / 40;
	const u32 v4 = (_SHIFTR( _w1,  21, 11 ) & 0x7F8) / 40;
	const u32 vert[4] = { v1, v2, v3, v4 };

	const u32 nextCMD = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 8];
	const u32 color[4] = { _SHIFTR(nextCMD, 16, 8), _SHIFTR(nextCMD, 8, 8),
							_SHIFTR(nextCMD, 0, 8), _SHIFTR(nextCMD, 24, 8) };

	const bool useTex = (_w0 & 2) != 0;
    const u8 * texbase = RDRAM + RSP.PC[RSP.PCi] + 16;
    F3DSWRS_PrepareVertices(vert, RDRAM + gSP.vertexColorBase, color, texbase, useTex, 4);

	if (useTex)
		RSP.PC[RSP.PCi] += 16;

	gSP2Triangles(v1, v2, v3, 0, v1, v3, v4, 0);
	RSP.PC[RSP.PCi] += 8;
}

void F3DSWRS_MoveWord(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_MoveWord (0x%08x, 0x%08x)\n", _w0, _w1);
	switch (_SHIFTR(_w0, 0, 8)){
//	case 0x58C: // This PC is used after a texrect in naboo
//		State.NabooPCAfterTexRect = Segment[Command.dl.segment] + Command.dl.addr;
//		break;
	case G_MW_CLIP:
		gSPClipRatio( _w1 );
		break;
	case G_MW_SEGMENT:
		gSPSegment( _SHIFTR( _w0, 8, 16 ) >> 2, _w1 & 0x00FFFFFF );
		break;
	case F3DSWRS_MW_FOG_MULTIPLIER:
		gSP.fog.multiplierf = _FIXED2FLOAT((s32)_w1, 16);
		gSP.changed |= CHANGED_FOGPOSITION;
		break;
	case F3DSWRS_MW_FOG_OFFSET:
		gSP.fog.offsetf = _FIXED2FLOAT((s32)_w1, 16);
		gSP.changed |= CHANGED_FOGPOSITION;
		break;
	case G_MW_PERSPNORM:
		gSPPerspNormalize( _w1 );
		break;
	}
}

void F3DSWRS_HeightField(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_HeightField (0x%08x, 0x%08x)\n", _w0, _w1);
	// Lemmy's note:
	// seems to be similar to JUMP3, but calls actual function with A1=0x2C
	// it *might* need the same jump/branch code as JUMP3
	RSP.PC[RSP.PCi] += 16;
}

void F3DSWRS_SetOtherMode_H_EX(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_SetOtherMode_H_EX (0x%08x, 0x%08x)\n", _w0, _w1);
	RSP.PC[RSP.PCi] += 8;
	gDP.otherMode.h &= *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
	gDP.otherMode.h |= _w1;
}

void F3DSWRS_SetOtherMode_L_EX(u32 _w0, u32 _w1)
{
	DebugMsg(DEBUG_NORMAL, "F3DSWRS_SetOtherMode_L_EX (0x%08x, 0x%08x)\n", _w0, _w1);
	RSP.PC[RSP.PCi] += 8;
	gDP.otherMode.l &= *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
	gDP.otherMode.l |= _w1;
}

void F3DSWRS_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	//          GBI Command             Command Value				Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,					F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,					F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3DSWRS_VTXCOLOR,			F3DSWRS_VertexColor );
	GBI_SetGBI( G_MOVEMEM,				F3DSWRS_MOVEMEM,			F3DSWRS_MoveMem );
	GBI_SetGBI( G_VTX,					F3DSWRS_VTX,				F3DSWRS_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3DSWRS_JUMP3,				F3DSWRS_Jump3 );
	GBI_SetGBI( G_DL,					F3DSWRS_DL,					F3DSWRS_DList );
	GBI_SetGBI( G_RESERVED2,			F3DSWRS_BRANCHDL,			F3DSWRS_BranchDList );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,				F3D_Reserved3 );

	GBI_SetGBI( G_TRI1,					F3DSWRS_TRI1,				F3DSWRS_Tri1 );
	GBI_SetGBI( G_SETOTHERMODE_H_EX,	F3DSWRS_SETOTHERMODE_H_EX,	F3DSWRS_SetOtherMode_H_EX );
	GBI_SetGBI( G_POPMTX,				F3DSWRS_HEIGHTFIELD,		F3DSWRS_HeightField );
	GBI_SetGBI( G_MOVEWORD,				F3DSWRS_MOVEWORD,			F3DSWRS_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,				F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,			F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,			F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,					F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,		F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,		F3D_ClearGeometryMode );
	GBI_SetGBI( G_QUAD,					F3DSWRS_JUMP2,				F3DSWRS_Jump2 );
	GBI_SetGBI( G_RDPHALF_1,			F3DSWRS_TRI2,				F3DSWRS_Tri2 );
	GBI_SetGBI( G_SETOTHERMODE_L_EX,	F3DSWRS_SETOTHERMODE_L_EX,	F3DSWRS_SetOtherMode_L_EX );
}
