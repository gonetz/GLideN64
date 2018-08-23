#include <assert.h>
#include <algorithm>
#include "S2DEX.h"
#include "F3D.h"
#include "F3DEX.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "RDP.h"
#include "Config.h"
#include "Log.h"
#include "DebugDump.h"
#include "DepthBuffer.h"
#include "FrameBuffer.h"

#include <Graphics/Context.h>
#include <Graphics/Parameters.h>
#include "DisplayWindow.h"

using namespace graphics;

#define S2DEX_MV_MATRIX			0
#define S2DEX_MV_SUBMUTRIX		2
#define S2DEX_MV_VIEWPORT		8

#define	S2DEX_BG_1CYC			0x01
#define	S2DEX_BG_COPY			0x02
#define	S2DEX_OBJ_RECTANGLE		0x03
#define	S2DEX_OBJ_SPRITE		0x04
#define	S2DEX_OBJ_MOVEMEM		0x05
#define	S2DEX_LOAD_UCODE		0xAF
#define	S2DEX_SELECT_DL			0xB0
#define	S2DEX_OBJ_RENDERMODE	0xB1
#define	S2DEX_OBJ_RECTANGLE_R	0xB2
#define	S2DEX_OBJ_LOADTXTR		0xC1
#define	S2DEX_OBJ_LDTX_SPRITE	0xC2
#define	S2DEX_OBJ_LDTX_RECT		0xC3
#define	S2DEX_OBJ_LDTX_RECT_R	0xC4
#define	S2DEX_RDPHALF_0			0xE4

struct S2DEXCoordCorrector
{
	S2DEXCoordCorrector()
	{
		static const u32 CorrectorsA01[] = {
			0x00000000,
			0x00100020,
			0x00200040,
			0x00300060,
			0x0000FFF4,
			0x00100014,
			0x00200034,
			0x00300054
		};
		static const s16 * CorrectorsA01_16 = reinterpret_cast<const s16*>(CorrectorsA01);

		static const u32 CorrectorsA23[] = {
			0x0001FFFE,
			0xFFFEFFFE,
			0x00010000,
			0x00000000
		};
		static const s16 * CorrectorsA23_16 = reinterpret_cast<const s16*>(CorrectorsA23);

		static const u32 CorrectorsB03[] = {
			0xFFFC0000,
			0x00000001,
			0xFFFF0003,
			0xFFF00000
		};
		static const s16 * CorrectorsB03_16 = reinterpret_cast<const s16*>(CorrectorsB03);

		const u32 O1 = (gSP.objRendermode & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_SHRINKSIZE_2 | G_OBJRM_WIDEN)) >> 3;
		A1 = CorrectorsA01_16[(1 + O1) ^ 1];
		const u32 O2 = (gSP.objRendermode & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_BILERP)) >> 2;
		A2 = CorrectorsA23_16[(0 + O2) ^ 1];
		A3 = CorrectorsA23_16[(1 + O2) ^ 1];
		const u32 O3 = (gSP.objRendermode & G_OBJRM_BILERP) >> 1;
		B0 = CorrectorsB03_16[(0 + O3) ^ 1];
		B3 = CorrectorsB03_16[(3 + O3) ^ 1];
	}

	s16 A1, A2, A3, B0, B3;
};

struct ObjCoordinates
{
	f32 ulx, uly, lrx, lry;
	f32 uls, ult, lrs, lrt;
	f32 z, w;

	ObjCoordinates(const uObjSprite *_pObjSprite, bool _useMatrix)
	{
		/* Fixed point coordinates calculation. Decoded by olivieryuyu */
		//XH = AND(objX + A2) by B0
		//XL = AND(objX + A2) by B0 + ((ImageW - A1) * 0x100) / scaleW * 2
		//YH = AND(objY + A2) by B0
		//YL = AND(objY + A2) by B0 + ((ImageH - A1) * 0x100) / scaleH * 2
		S2DEXCoordCorrector CC;
		const s16 xh = (_pObjSprite->objX + CC.A2) & CC.B0;
		const s16 xl = ((_pObjSprite->imageW - CC.A1) << 8) / (_pObjSprite->scaleW << 1) + xh;
		const s16 yh = (_pObjSprite->objY + CC.A2) & CC.B0;
		const s16 yl = ((_pObjSprite->imageH - CC.A1) << 8) / (_pObjSprite->scaleH << 1) + yh;

		ulx = _FIXED2FLOAT(xh, 2);
		lrx = _FIXED2FLOAT(xl, 2);
		uly = _FIXED2FLOAT(yh, 2);
		lry = _FIXED2FLOAT(yl, 2);

		if (_useMatrix) {
			// TODO: use fixed point math
			ulx = ulx / gSP.objMatrix.baseScaleX + gSP.objMatrix.X;
			lrx = lrx / gSP.objMatrix.baseScaleX + gSP.objMatrix.X;
			uly = uly / gSP.objMatrix.baseScaleY + gSP.objMatrix.Y;
			lry = lry / gSP.objMatrix.baseScaleY + gSP.objMatrix.Y;
		}

		uls = ult = 0;
		lrs = _FIXED2FLOAT(_pObjSprite->imageW, 5);
		lrt = _FIXED2FLOAT(_pObjSprite->imageH, 5);
		if ((_pObjSprite->imageFlags & 0x01) != 0) // flipS
			std::swap(uls, lrs);
		if ((_pObjSprite->imageFlags & 0x10) != 0) // flipT
			std::swap(ult, lrt);

		z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
		w = 1.0f;
	}

	ObjCoordinates(const uObjScaleBg * _pObjScaleBg)
	{
		const f32 frameX = _FIXED2FLOAT(_pObjScaleBg->frameX, 2);
		const f32 frameY = _FIXED2FLOAT(_pObjScaleBg->frameY, 2);
		const f32 imageX = gSP.bgImage.imageX;
		const f32 imageY = gSP.bgImage.imageY;
		f32 scaleW = gSP.bgImage.scaleW;
		f32 scaleH = gSP.bgImage.scaleH;

		// gSPBgRectCopy() does not support scaleW and scaleH
		if (gDP.otherMode.cycleType == G_CYC_COPY) {
			scaleW = 1.0f;
			scaleH = 1.0f;
		}

		f32 frameW = _FIXED2FLOAT(_pObjScaleBg->frameW, 2);
		f32 frameH = _FIXED2FLOAT(_pObjScaleBg->frameH, 2);
		f32 imageW = (f32)(_pObjScaleBg->imageW >> 2);
		f32 imageH = (f32)(_pObjScaleBg->imageH >> 2);
		//		const f32 imageW = (f32)gSP.bgImage.width;
		//		const f32 imageH = (f32)gSP.bgImage.height;

		if (u32(imageW) == 512 && (config.generalEmulation.hacks & hack_RE2) != 0) {
			const f32 width = f32(*REG.VI_WIDTH);
			const f32 scale = imageW / width;
			imageW = width;
			frameW = width;
			imageH *= scale;
			frameH *= scale;
			scaleW = 1.0f;
			scaleH = 1.0f;
		}

		uls = imageX;
		ult = imageY;
		lrs = uls + std::min(imageW, frameW * scaleW) - 1;
		lrt = ult + std::min(imageH, frameH * scaleH) - 1;

		// G_CYC_COPY (gSPBgRectCopy()) does not allow texture filtering
		if (gDP.otherMode.cycleType != G_CYC_COPY) {
			// Correct texture coordinates -0.5f and +0.5 if G_OBJRM_BILERP 
			// bilinear interpolation is set
			if ((gSP.objRendermode&G_OBJRM_BILERP) != 0 &&
				((gDP.otherMode.textureFilter == G_TF_BILERP) ||											// Kirby Crystal Shards
				(gDP.otherMode.textureFilter == G_TF_POINT && (gSP.objRendermode&G_OBJRM_NOTXCLAMP) != 0)) // Worms Armageddon
				) {
				uls -= 0.5f;
				ult -= 0.5f;
				lrs += 0.5f;
				lrt += 0.5f;
			}
			// SHRINKSIZE_1 adds a 0.5f perimeter around the image
			// upper left texture coords += 0.5f; lower left texture coords -= 0.5f
			if ((gSP.objRendermode&G_OBJRM_SHRINKSIZE_1) != 0) {
				uls += 0.5f;
				ult += 0.5f;
				lrs -= 0.5f;
				lrt -= 0.5f;
				// SHRINKSIZE_2 adds a 1.0f perimeter 
				// upper left texture coords += 1.0f; lower left texture coords -= 1.0f
			}
			else if ((gSP.objRendermode&G_OBJRM_SHRINKSIZE_2) != 0) {
				uls += 1.0f;
				ult += 1.0f;
				lrs -= 1.0f;
				lrt -= 1.0f;
			}
		}

		// Calculate lrx and lry width new ST values
		ulx = frameX;
		uly = frameY;
		lrx = ulx + (lrs - uls) / scaleW;
		lry = uly + (lrt - ult) / scaleH;
		if ((gSP.objRendermode&G_OBJRM_BILERP) == 0) {
			lrx += 1.0f / scaleW;
			lry += 1.0f / scaleH;
		}

		// gSPBgRect1Cyc() and gSPBgRectCopy() do only support 
		// imageFlip in horizontal direction
		if ((_pObjScaleBg->imageFlip & 0x01) != 0) {
			std::swap(ulx, lrx);
		}

		z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
		w = 1.0f;
	}
};

static
u16 _YUVtoRGBA(u8 y, u8 u, u8 v)
{
	float r = y + (1.370705f * (v - 128));
	float g = y - (0.698001f * (v - 128)) - (0.337633f * (u - 128));
	float b = y + (1.732446f * (u - 128));
	r *= 0.125f;
	g *= 0.125f;
	b *= 0.125f;
	//clipping the result
	if (r > 32) r = 32;
	if (g > 32) g = 32;
	if (b > 32) b = 32;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;

	u16 c = (u16)(((u16)(r) << 11) |
		((u16)(g) << 6) |
		((u16)(b) << 1) | 1);
	return c;
}

static
void _drawYUVImageToFrameBuffer(const ObjCoordinates & _objCoords)
{
	const u32 ulx = (u32)_objCoords.ulx;
	const u32 uly = (u32)_objCoords.uly;
	const u32 lrx = (u32)_objCoords.lrx;
	const u32 lry = (u32)_objCoords.lry;
	const u32 ci_width = gDP.colorImage.width;
	const u32 ci_height = (u32)gDP.scissor.lry;
	if (ulx >= ci_width)
		return;
	if (uly >= ci_height)
		return;
	u32 width = 16, height = 16;
	if (lrx > ci_width)
		width = ci_width - ulx;
	if (lry > ci_height)
		height = ci_height - uly;
	u32 * mb = (u32*)(RDRAM + gDP.textureImage.address); //pointer to the first macro block
	u16 * dst = (u16*)(RDRAM + gDP.colorImage.address);
	dst += ulx + uly * ci_width;
	//yuv macro block contains 16x16 texture. we need to put it in the proper place inside cimg
	for (u16 h = 0; h < 16; h++) {
		for (u16 w = 0; w < 16; w += 2) {
			u32 t = *(mb++); //each u32 contains 2 pixels
			if ((h < height) && (w < width)) //clipping. texture image may be larger than color image
			{
				u8 y0 = (u8)t & 0xFF;
				u8 v = (u8)(t >> 8) & 0xFF;
				u8 y1 = (u8)(t >> 16) & 0xFF;
				u8 u = (u8)(t >> 24) & 0xFF;
				*(dst++) = _YUVtoRGBA(y0, u, v);
				*(dst++) = _YUVtoRGBA(y1, u, v);
			}
		}
		dst += ci_width - 16;
	}
	FrameBuffer *pBuffer = frameBufferList().getCurrent();
	if (pBuffer != nullptr)
		pBuffer->m_isOBScreen = true;
}

static
void gSPDrawObjRect(const ObjCoordinates & _coords)
{
	GraphicsDrawer & drawer = dwnd().getDrawer();
	drawer.setDMAVerticesSize(4);
	SPVertex * pVtx = drawer.getDMAVerticesData();
	SPVertex & vtx0 = pVtx[0];
	vtx0.x = _coords.ulx;
	vtx0.y = _coords.uly;
	vtx0.z = _coords.z;
	vtx0.w = _coords.w;
	vtx0.s = _coords.uls;
	vtx0.t = _coords.ult;
	SPVertex & vtx1 = pVtx[1];
	vtx1.x = _coords.lrx;
	vtx1.y = _coords.uly;
	vtx1.z = _coords.z;
	vtx1.w = _coords.w;
	vtx1.s = _coords.lrs;
	vtx1.t = _coords.ult;
	SPVertex & vtx2 = pVtx[2];
	vtx2.x = _coords.ulx;
	vtx2.y = _coords.lry;
	vtx2.z = _coords.z;
	vtx2.w = _coords.w;
	vtx2.s = _coords.uls;
	vtx2.t = _coords.lrt;
	SPVertex & vtx3 = pVtx[3];
	vtx3.x = _coords.lrx;
	vtx3.y = _coords.lry;
	vtx3.z = _coords.z;
	vtx3.w = _coords.w;
	vtx3.s = _coords.lrs;
	vtx3.t = _coords.lrt;

	drawer.drawScreenSpaceTriangle(4);
}

static
void gSPSetSpriteTile(const uObjSprite *_pObjSprite)
{
	const u32 w = std::max(_pObjSprite->imageW >> 5, 1);
	const u32 h = std::max(_pObjSprite->imageH >> 5, 1);

	gDPSetTile(_pObjSprite->imageFmt, _pObjSprite->imageSiz, _pObjSprite->imageStride, _pObjSprite->imageAdrs, 0, _pObjSprite->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0);
	gDPSetTileSize(0, 0, 0, (w - 1) << 2, (h - 1) << 2);
	gSPTexture(1.0f, 1.0f, 0, 0, TRUE);
}

static
void gSPObjLoadTxtr(u32 tx)
{
	const u32 address = RSP_SegmentToPhysical(tx);
	uObjTxtr *objTxtr = (uObjTxtr*)&RDRAM[address];

	if ((gSP.status[objTxtr->block.sid >> 2] & objTxtr->block.mask) != objTxtr->block.flag) {
		switch (objTxtr->block.type) {
		case G_OBJLT_TXTRBLOCK:
			gDPSetTextureImage(0, 1, 0, objTxtr->block.image);
			gDPSetTile(0, 1, 0, objTxtr->block.tmem, 7, 0, 0, 0, 0, 0, 0, 0);
			gDPLoadBlock(7, 0, 0, ((objTxtr->block.tsize + 1) << 3) - 1, objTxtr->block.tline);
			DebugMsg(DEBUG_NORMAL, "gSPObjLoadTxtr: load block\n");
			break;
		case G_OBJLT_TXTRTILE:
			gDPSetTextureImage(0, 1, (objTxtr->tile.twidth + 1) << 1, objTxtr->tile.image);
			gDPSetTile(0, 1, (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem, 0, 0, 0, 0, 0, 0, 0, 0);
			gDPSetTile(0, 1, (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem, 7, 0, 0, 0, 0, 0, 0, 0);
			gDPLoadTile(7, 0, 0, (((objTxtr->tile.twidth + 1) << 1) - 1) << 2, (((objTxtr->tile.theight + 1) >> 2) - 1) << 2);
			DebugMsg(DEBUG_NORMAL, "gSPObjLoadTxtr: load tile\n");
			break;
		case G_OBJLT_TLUT:
			gDPSetTextureImage(0, 2, 1, objTxtr->tlut.image);
			gDPSetTile(0, 2, 0, objTxtr->tlut.phead, 7, 0, 0, 0, 0, 0, 0, 0);
			gDPLoadTLUT(7, 0, 0, objTxtr->tlut.pnum << 2, 0);
			DebugMsg(DEBUG_NORMAL, "gSPObjLoadTxtr: load tlut\n");
			break;
		}
		gSP.status[objTxtr->block.sid >> 2] = (gSP.status[objTxtr->block.sid >> 2] & ~objTxtr->block.mask) | (objTxtr->block.flag & objTxtr->block.mask);
	}
}

static
void gSPObjRectangle(u32 _sp)
{
	const u32 address = RSP_SegmentToPhysical(_sp);
	uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];
	gSPSetSpriteTile(objSprite);

	ObjCoordinates objCoords(objSprite, false);
	gSPDrawObjRect(objCoords);
	DebugMsg(DEBUG_NORMAL, "gSPObjRectangle\n");
}

static
void gSPObjRectangleR(u32 _sp)
{
	const u32 address = RSP_SegmentToPhysical(_sp);
	const uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];
	gSPSetSpriteTile(objSprite);
	ObjCoordinates objCoords(objSprite, true);

	if (objSprite->imageFmt == G_IM_FMT_YUV && (config.generalEmulation.hacks&hack_Ogre64)) //Ogre Battle needs to copy YUV texture to frame buffer
		_drawYUVImageToFrameBuffer(objCoords);
	gSPDrawObjRect(objCoords);

	DebugMsg(DEBUG_NORMAL, "gSPObjRectangleR\n");
}

static
void gSPObjSprite(u32 _sp)
{
	const u32 address = RSP_SegmentToPhysical(_sp);
	uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];
	gSPSetSpriteTile(objSprite);

	/* Fixed point coordinates calculation. Decoded by olivieryuyu */
	//	X1 = AND (X + B3) by B0 + ((objX + A3) * A) >> 16 + ((objY + A3) * B) >> 16
	//	Y1 = AND (Y + B3) by B0 + ((objX + A3) * C) >> 16 + ((objY + A3) * D) >> 16
	//	X2 = AND (X + B3) by B0 + ((((imageW - A1) * 0x0100)/(scaleW * 2) + objX + A3) * A) >> 16  + ((((imageH - A1) * 0x0100)/(scaleH * 2) + objY + A3) * B) >> 16
	//	Y2 = AND (Y + B3) by B0 + ((((imageW - A1) * 0x0100)/(scaleW * 2) + objX + A3) * C) >> 16 + ((((imageH - A1) * 0x0100)/(scaleH * 2) + objY + A3) * D) >> 16

	S2DEXCoordCorrector CC;
	const uObjMtx *objMtx = reinterpret_cast<const uObjMtx *>(RDRAM + gSP.objMatrix.address);
	const s16 x0 = (objMtx->X + CC.B3) & CC.B0;
	const s16 y0 = (objMtx->Y + CC.B3) & CC.B0;
	const s16 ulx = objSprite->objX + CC.A3;
	const s16 uly = objSprite->objY + CC.A3;
	const s16 lrx = ((objSprite->imageW - CC.A1) << 8) / (objSprite->scaleW << 1) + ulx;
	const s16 lry = ((objSprite->imageH - CC.A1) << 8) / (objSprite->scaleH << 1) + uly;

	auto calcX = [&](s16 _x, s16 _y) -> f32
	{
		const s16 X = x0 + static_cast<s16>(((_x * objMtx->A) >> 16)) + static_cast<s16>(((_y * objMtx->B) >> 16));
		return _FIXED2FLOAT(X, 2);
	};

	auto calcY = [&](s16 _x, s16 _y) -> f32
	{
		const s16 Y = y0 + static_cast<s16>(((_x * objMtx->C) >> 16)) + static_cast<s16>(((_y * objMtx->D) >> 16));
		return _FIXED2FLOAT(Y, 2);
	};

	f32 uls = 0.0f;
	f32 lrs = _FIXED2FLOAT(objSprite->imageW, 5);
	f32 ult = 0.0f;
	f32 lrt = _FIXED2FLOAT(objSprite->imageH, 5);

	if (objSprite->imageFlags & 0x01) // flipS
		std::swap(uls, lrs);

	if (objSprite->imageFlags & 0x10) // flipT
		std::swap(ult, lrt);

	const float z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;

	GraphicsDrawer & drawer = dwnd().getDrawer();
	drawer.setDMAVerticesSize(4);
	SPVertex * pVtx = drawer.getDMAVerticesData();

	SPVertex & vtx0 = pVtx[0];
	vtx0.x = calcX(ulx, uly);
	vtx0.y = calcY(ulx, uly);
	vtx0.z = z;
	vtx0.w = 1.0f;
	vtx0.s = uls;
	vtx0.t = ult;
	SPVertex & vtx1 = pVtx[1];
	vtx1.x = calcX(lrx, uly);
	vtx1.y = calcY(lrx, uly);
	vtx1.z = z;
	vtx1.w = 1.0f;
	vtx1.s = lrs;
	vtx1.t = ult;
	SPVertex & vtx2 = pVtx[2];
	vtx2.x = calcX(ulx, lry);
	vtx2.y = calcY(ulx, lry);
	vtx2.z = z;
	vtx2.w = 1.0f;
	vtx2.s = uls;
	vtx2.t = lrt;
	SPVertex & vtx3 = pVtx[3];
	vtx3.x = calcX(lrx, lry);
	vtx3.y = calcY(lrx, lry);
	vtx3.z = z;
	vtx3.w = 1.0f;
	vtx3.s = lrs;
	vtx3.t = lrt;

	drawer.drawScreenSpaceTriangle(4);

	DebugMsg(DEBUG_NORMAL, "gSPObjSprite\n");
}

static
void gSPObjMatrix(u32 mtx)
{
	gSP.objMatrix.address = RSP_SegmentToPhysical(mtx);
	const uObjMtx *objMtx = reinterpret_cast<const uObjMtx *>(RDRAM + gSP.objMatrix.address);

	gSP.objMatrix.A = _FIXED2FLOAT(objMtx->A, 16);
	gSP.objMatrix.B = _FIXED2FLOAT(objMtx->B, 16);
	gSP.objMatrix.C = _FIXED2FLOAT(objMtx->C, 16);
	gSP.objMatrix.D = _FIXED2FLOAT(objMtx->D, 16);
	gSP.objMatrix.X = _FIXED2FLOAT(objMtx->X, 2);
	gSP.objMatrix.Y = _FIXED2FLOAT(objMtx->Y, 2);
	gSP.objMatrix.baseScaleX = _FIXED2FLOAT(objMtx->BaseScaleX, 10);
	gSP.objMatrix.baseScaleY = _FIXED2FLOAT(objMtx->BaseScaleY, 10);

	DebugMsg(DEBUG_NORMAL, "gSPObjMatrix\n");
}

static
void gSPObjSubMatrix(u32 mtx)
{
	u32 address = RSP_SegmentToPhysical(mtx);
	uObjSubMtx *objMtx = (uObjSubMtx*)&RDRAM[address];
	gSP.objMatrix.X = _FIXED2FLOAT(objMtx->X, 2);
	gSP.objMatrix.Y = _FIXED2FLOAT(objMtx->Y, 2);
	gSP.objMatrix.baseScaleX = _FIXED2FLOAT(objMtx->BaseScaleX, 10);
	gSP.objMatrix.baseScaleY = _FIXED2FLOAT(objMtx->BaseScaleY, 10);

	DebugMsg(DEBUG_NORMAL, "gSPObjSubMatrix\n");
}

static
void _copyDepthBuffer()
{
	if (!config.frameBufferEmulation.enable)
		return;

	if (!Context::BlitFramebuffer)
		return;

	// The game copies content of depth buffer into current color buffer
	// OpenGL has different format for color and depth buffers, so this trick can't be performed directly
	// To do that, depth buffer with address of current color buffer created and attached to the current FBO
	// It will be copy depth buffer
	DepthBufferList & dbList = depthBufferList();
	dbList.saveBuffer(gDP.colorImage.address);
	// Take any frame buffer and attach source depth buffer to it, to blit it into copy depth buffer
	FrameBufferList & fbList = frameBufferList();
	FrameBuffer * pTmpBuffer = fbList.findTmpBuffer(fbList.getCurrent()->m_startAddress);
	if (pTmpBuffer == nullptr)
		return;
	DepthBuffer * pCopyBufferDepth = dbList.findBuffer(gSP.bgImage.address);
	if (pCopyBufferDepth == nullptr)
		return;
	pCopyBufferDepth->setDepthAttachment(pTmpBuffer->m_FBO, bufferTarget::READ_FRAMEBUFFER);

	DisplayWindow & wnd = dwnd();
	Context::BlitFramebuffersParams blitParams;
	blitParams.readBuffer = pTmpBuffer->m_FBO;
	blitParams.drawBuffer = fbList.getCurrent()->m_FBO;
	blitParams.srcX0 = 0;
	blitParams.srcY0 = 0;
	blitParams.srcX1 = wnd.getWidth();
	blitParams.srcY1 = wnd.getHeight();
	blitParams.dstX0 = 0;
	blitParams.dstY0 = 0;
	blitParams.dstX1 = wnd.getWidth();
	blitParams.dstY1 = wnd.getHeight();
	blitParams.mask = blitMask::DEPTH_BUFFER;
	blitParams.filter = textureParameters::FILTER_NEAREST;

	gfxContext.blitFramebuffers(blitParams);

	// Restore objects
	if (pTmpBuffer->m_pDepthBuffer != nullptr)
		pTmpBuffer->m_pDepthBuffer->setDepthAttachment(fbList.getCurrent()->m_FBO, bufferTarget::READ_FRAMEBUFFER);
	gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, ObjectHandle::defaultFramebuffer);

	// Set back current depth buffer
	dbList.saveBuffer(gDP.depthImageAddress);
}

static
void _loadBGImage(const uObjScaleBg * _bgInfo, bool _loadScale)
{
	gSP.bgImage.address = RSP_SegmentToPhysical(_bgInfo->imagePtr);

	const u32 imageW = _bgInfo->imageW >> 2;
	const u32 imageH = _bgInfo->imageH >> 2;
	if (imageW == 512 && (config.generalEmulation.hacks & hack_RE2) != 0) {
		gSP.bgImage.width = *REG.VI_WIDTH;
		gSP.bgImage.height = (imageH * imageW) / gSP.bgImage.width;
	}
	else {
		gSP.bgImage.width = imageW - imageW % 2;
		gSP.bgImage.height = imageH - imageH % 2;
	}
	gSP.bgImage.format = _bgInfo->imageFmt;
	gSP.bgImage.size = _bgInfo->imageSiz;
	gSP.bgImage.palette = _bgInfo->imagePal;
	gDP.tiles[0].textureMode = TEXTUREMODE_BGIMAGE;
	gSP.bgImage.imageX = _FIXED2FLOAT(_bgInfo->imageX, 5);
	gSP.bgImage.imageY = _FIXED2FLOAT(_bgInfo->imageY, 5);
	if (_loadScale) {
		gSP.bgImage.scaleW = _FIXED2FLOAT(_bgInfo->scaleW, 10);
		gSP.bgImage.scaleH = _FIXED2FLOAT(_bgInfo->scaleH, 10);
	}
	else
		gSP.bgImage.scaleW = gSP.bgImage.scaleH = 1.0f;

	if (config.frameBufferEmulation.enable) {
		FrameBuffer *pBuffer = frameBufferList().findBuffer(gSP.bgImage.address);
		if ((pBuffer != nullptr) && pBuffer->m_size == gSP.bgImage.size && (!pBuffer->m_isDepthBuffer || pBuffer->m_changed)) {
			if (gSP.bgImage.format == G_IM_FMT_CI && gSP.bgImage.size == G_IM_SIZ_8b) {
				// Can't use 8bit CI buffer as texture
				return;
			}

			if (pBuffer->m_cfb || !pBuffer->isValid(false)) {
				frameBufferList().removeBuffer(pBuffer->m_startAddress);
				return;
			}

			gDP.tiles[0].frameBufferAddress = pBuffer->m_startAddress;
			gDP.tiles[0].textureMode = TEXTUREMODE_FRAMEBUFFER_BG;
			gDP.tiles[0].loadType = LOADTYPE_TILE;
			gDP.changed |= CHANGED_TMEM;

			if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0) {
				if (gDP.colorImage.address == gDP.depthImageAddress)
					frameBufferList().setCopyBuffer(frameBufferList().getCurrent());
			}
		}
	}
}

static
void gSPBgRect1Cyc(u32 _bg)
{
	const u32 address = RSP_SegmentToPhysical(_bg);
	uObjScaleBg *objScaleBg = (uObjScaleBg*)&RDRAM[address];
	_loadBGImage(objScaleBg, true);

	// Zelda MM uses depth buffer copy in LoT and in pause screen.
	// In later case depth buffer is used as temporal color buffer, and usual rendering must be used.
	// Since both situations are hard to distinguish, do the both depth buffer copy and bg rendering.
	if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0 &&
		(gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != nullptr)
		)
		_copyDepthBuffer();

	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;
	gSPTexture(1.0f, 1.0f, 0, 0, TRUE);

	ObjCoordinates objCoords(objScaleBg);
	gSPDrawObjRect(objCoords);

	DebugMsg(DEBUG_NORMAL, "gSPBgRect1Cyc\n");
}

static
void gSPBgRectCopy(u32 _bg)
{
	const u32 address = RSP_SegmentToPhysical(_bg);
	uObjScaleBg *objBg = (uObjScaleBg*)&RDRAM[address];
	_loadBGImage(objBg, false);

	// See comment to gSPBgRect1Cyc
	if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0 &&
		(gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != nullptr)
		)
		_copyDepthBuffer();

	gDP.otherMode.cycleType = G_CYC_COPY;
	gDP.changed |= CHANGED_CYCLETYPE;
	gSPTexture(1.0f, 1.0f, 0, 0, TRUE);

	ObjCoordinates objCoords(objBg);
	gSPDrawObjRect(objCoords);

	DebugMsg(DEBUG_NORMAL, "gSPBgRectCopy\n");
}

void S2DEX_BG_1Cyc(u32 w0, u32 w1)
{
	gSPBgRect1Cyc(w1);
}

void S2DEX_BG_Copy(u32 w0, u32 w1)
{
	gSPBgRectCopy(w1);
}

void S2DEX_Obj_MoveMem(u32 w0, u32 w1)
{
	switch (_SHIFTR(w0, 0, 16)) {
	case S2DEX_MV_MATRIX:
		gSPObjMatrix(w1);
		break;
	case S2DEX_MV_SUBMUTRIX:
		gSPObjSubMatrix(w1);
		break;
	case S2DEX_MV_VIEWPORT:
		gSPViewport(w1);
		break;
	}
}

void S2DEX_MoveWord(u32 w0, u32 w1)
{
	switch (_SHIFTR(w0, 16, 8))
	{
	case G_MW_GENSTAT:
		gSPSetStatus(_SHIFTR(w0, 0, 16), w1);
		break;
	default:
		F3D_MoveWord(w0, w1);
		break;
	}
}

void S2DEX_RDPHalf_0(u32 w0, u32 w1) {
	if (RSP.nextCmd == G_SELECT_DL) {
		gSP.selectDL.addr = _SHIFTR(w0, 0, 16);
		gSP.selectDL.sid = _SHIFTR(w0, 18, 8);
		gSP.selectDL.flag = w1;
		return;
	}
	if (RSP.nextCmd == G_RDPHALF_1) {
		RDP_TexRect(w0, w1);
		return;
	}
	assert(false);
}

void S2DEX_Select_DL(u32 w0, u32 w1)
{
	gSP.selectDL.addr |= (_SHIFTR(w0, 0, 16)) << 16;
	const u8 sid = gSP.selectDL.sid;
	const u32 flag = gSP.selectDL.flag;
	const u32 mask = w1;
	if ((gSP.status[sid] & mask) == flag)
		// Do nothing;
		return;

	gSP.status[sid] = (gSP.status[sid] & ~mask) | (flag & mask);

	switch (_SHIFTR(w0, 16, 8))
	{
	case G_DL_PUSH:
		gSPDisplayList(gSP.selectDL.addr);
		break;
	case G_DL_NOPUSH:
		gSPBranchList(gSP.selectDL.addr);
		break;
	}
}

void S2DEX_Obj_RenderMode(u32 w0, u32 w1)
{
	gSP.objRendermode = w1;
	DebugMsg(DEBUG_NORMAL, "gSPObjRendermode(0x%08x)\n", gSP.objRendermode);
}

void S2DEX_Obj_Rectangle(u32 w0, u32 w1)
{
	gSPObjRectangle(w1);
}

void S2DEX_Obj_Rectangle_R(u32 w0, u32 w1)
{
	gSPObjRectangleR(w1);
}

void S2DEX_Obj_Sprite(u32 w0, u32 w1)
{
	gSPObjSprite(w1);
}

void S2DEX_Obj_LoadTxtr(u32 w0, u32 w1)
{
	gSPObjLoadTxtr(w1);
}

void S2DEX_Obj_LdTx_Rect(u32 w0, u32 w1)
{
	gSPObjLoadTxtr(w1);
	gSPObjRectangle(w1 + sizeof(uObjTxtr));
}

void S2DEX_Obj_LdTx_Rect_R(u32 w0, u32 w1)
{
	gSPObjLoadTxtr(w1);
	gSPObjRectangleR(w1 + sizeof(uObjTxtr));
}

void S2DEX_Obj_LdTx_Sprite(u32 w0, u32 w1)
{
	gSPObjLoadTxtr(w1);
	gSPObjSprite(w1 + sizeof(uObjTxtr));
}

void S2DEX_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3DEX );

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
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			S2DEX_MoveWord );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_RDPHALF_0,			S2DEX_RDPHALF_0,		S2DEX_RDPHalf_0 );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI(	G_LOAD_UCODE,			S2DEX_LOAD_UCODE,		F3DEX_Load_uCode );
}

