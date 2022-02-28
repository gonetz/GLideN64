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

// BG flags
#define	G_BGLT_LOADBLOCK	0x0033
#define	G_BGLT_LOADTILE		0xFFF4

#define	G_BG_FLAG_FLIPS		0x01
#define	G_BG_FLAG_FLIPT		0x10

// Sprite object render modes
#define	G_OBJRM_NOTXCLAMP		0x01
#define	G_OBJRM_XLU				0x02	/* Ignored */
#define	G_OBJRM_ANTIALIAS		0x04	/* Ignored */
#define	G_OBJRM_BILERP			0x08
#define	G_OBJRM_SHRINKSIZE_1	0x10
#define	G_OBJRM_SHRINKSIZE_2	0x20
#define	G_OBJRM_WIDEN			0x40

// Sprite texture loading types
#define	G_OBJLT_TXTRBLOCK	0x00001033
#define	G_OBJLT_TXTRTILE	0x00fc1034
#define	G_OBJLT_TLUT		0x00000030

// Tile indices
#define G_TX_LOADTILE			0x07
#define G_TX_RENDERTILE			0x00

struct uObjBg {
	u16 imageW;     /* Texture width (8-byte alignment, u10.2) */
	u16 imageX;     /* x-coordinate of upper-left
					position of texture (u10.5) */
	u16 frameW;     /* Transfer destination frame width (u10.2) */
	s16 frameX;     /* x-coordinate of upper-left
					position of transfer destination frame (s10.2) */
	u16 imageH;     /* Texture height (u10.2) */
	u16 imageY;     /* y-coordinate of upper-left position of
					texture (u10.5) */
	u16 frameH;     /* Transfer destination frame height (u10.2) */
	s16 frameY;     /* y-coordinate of upper-left position of transfer
					destination  frame (s10.2) */
	u32 imagePtr;  /* Address of texture source in DRAM*/
	u8  imageSiz;   /* Texel size
					G_IM_SIZ_4b (4 bits/texel)
					G_IM_SIZ_8b (8 bits/texel)
					G_IM_SIZ_16b (16 bits/texel)
					G_IM_SIZ_32b (32 bits/texel) */
	u8  imageFmt;   /*Texel format
					G_IM_FMT_RGBA (RGBA format)
					G_IM_FMT_YUV (YUV format)
					G_IM_FMT_CI (CI format)
					G_IM_FMT_IA (IA format)
					G_IM_FMT_I (I format)  */
	u16 imageLoad;  /* Method for loading the BG image texture
					G_BGLT_LOADBLOCK (use LoadBlock)
					G_BGLT_LOADTILE (use LoadTile) */
	u16 imageFlip;  /* Image inversion on/off (horizontal
					direction only)
					0 (normal display (no inversion))
					G_BG_FLAG_FLIPS (horizontal inversion of texture image) */
	u16 imagePal;   /* Position of palette for 4-bit color
					index texture (4-bit precision, 0~15) */
	u16 tmemH;      /* Quadruple TMEM height(s13.2) which can be loaded at once
					 When normal texture 512/tmemW*4
					 When CI Texture 	256/tmemW*4 */
	u16 tmemW;      /* TMEM width Word size for frame 1 line
					 When LoadBlock GS_PIX2TMEM(imageW/4,imageSiz)
					 When LoadTile 	GS_PIX2TMEM(frameW/4,imageSiz)+1 */
	u16 tmemLoadTH; /* TH value or Stride value
					 When LoadBlock 	GS_CALC_DXT(tmemW)
					 When LoadTile  	tmemH-1 */
	u16 tmemLoadSH; /* SH value
					 When LoadBlock  	tmemSize/2-1
					 When LoadTile 	tmemW*16-1 */
	u16 tmemSize;   /* imagePtr skip value for one load iteration
					 = tmemSizeW*tmemH */
	u16 tmemSizeW;  /* imagePtr skip value for one line of image 1
					 When LoadBlock 	tmemW*2
					 When LoadTile  	GS_PIX2TMEM(imageW/4,imageSiz)*2 */
};   /* 40 bytes */

struct uObjScaleBg
{
	u16 imageW;     /* Texture width (8-byte alignment, u10.2) */
	u16 imageX;     /* x-coordinate of upper-left
					position of texture (u10.5) */
	u16 frameW;     /* Transfer destination frame width (u10.2) */
	s16 frameX;     /* x-coordinate of upper-left
					position of transfer destination frame (s10.2) */

	u16 imageH;     /* Texture height (u10.2) */
	u16 imageY;     /* y-coordinate of upper-left position of
					texture (u10.5) */
	u16 frameH;     /* Transfer destination frame height (u10.2) */
	s16 frameY;     /* y-coordinate of upper-left position of transfer
					destination  frame (s10.2) */

	u32 imagePtr;  /* Address of texture source in DRAM*/
	u8  imageSiz;   /* Texel size
					G_IM_SIZ_4b (4 bits/texel)
					G_IM_SIZ_8b (8 bits/texel)
					G_IM_SIZ_16b (16 bits/texel)
					G_IM_SIZ_32b (32 bits/texel) */
	u8  imageFmt;   /*Texel format
					G_IM_FMT_RGBA (RGBA format)
					G_IM_FMT_YUV (YUV format)
					G_IM_FMT_CI (CI format)
					G_IM_FMT_IA (IA format)
					G_IM_FMT_I (I format)  */
	u16 imageLoad;  /* Method for loading the BG image texture
					G_BGLT_LOADBLOCK (use LoadBlock)
					G_BGLT_LOADTILE (use LoadTile) */
	u16 imageFlip;  /* Image inversion on/off (horizontal
					direction only)
					0 (normal display (no inversion))
					G_BG_FLAG_FLIPS (horizontal inversion of texture image) */
	u16 imagePal;   /* Position of palette for 4-bit color
					index texture (4-bit precision, 0~15) */

	u16 scaleH;      /* y-direction scale value (u5.10) */
	u16 scaleW;      /* x-direction scale value (u5.10) */
	s32 imageYorig;  /* image drawing origin (s20.5)*/

	u8  padding[4];  /* Padding */
};   /* 40 bytes */

struct uObjSprite
{
	u16 scaleW;      /* Width-direction scaling (u5.10) */
	s16 objX;        /* x-coordinate of upper-left corner of OBJ (s10.2) */
	u16 paddingX;    /* Unused (always 0) */
	u16 imageW;      /* Texture width (length in s direction, u10.5)  */
	u16 scaleH;      /* Height-direction scaling (u5.10) */
	s16 objY;        /* y-coordinate of upper-left corner of OBJ (s10.2) */
	u16 paddingY;    /* Unused (always 0) */
	u16 imageH;      /* Texture height (length in t direction, u10.5)  */
	u16 imageAdrs;   /* Texture starting position in TMEM (In units of 64-bit words) */
	u16 imageStride; /* Texel wrapping width (In units of 64-bit words) */
	u8  imageFlags;  /* Display flag
					 (*) More than one of the following flags can be specified as the bit sum of the flags:
					 0 (Normal display (no inversion))
					 G_OBJ_FLAG_FLIPS (s-direction (x) inversion)
					 G_OBJ_FLAG_FLIPT (t-direction (y) inversion)  */
	u8  imagePal;    /* Position of palette for 4-bit color index texture  (4-bit precision, 0~7)  */
	u8  imageSiz;    /* Texel size
					 G_IM_SIZ_4b (4 bits/texel)
					 G_IM_SIZ_8b (8 bits/texel)
					 G_IM_SIZ_16b (16 bits/texel)
					 G_IM_SIZ_32b (32 bits/texel) */
	u8  imageFmt;    /* Texel format
					 G_IM_FMT_RGBA (RGBA format)
					 G_IM_FMT_YUV (YUV format)
					 G_IM_FMT_CI (CI format)
					 G_IM_FMT_IA (IA format)
					 G_IM_FMT_I  (I format) */
};    /* 24 bytes */

struct uObjTxtrBlock
{
	u32   type;   /* Structure identifier (G_OBJLT_TXTRBLOCK) */
	u32   image; /* Texture source address in DRAM (8-byte alignment) */
	u16   tsize;  /* Texture size (specified by GS_TB_TSIZE) */
	u16   tmem;   /* TMEM word address where texture will be loaded (8-byte word) */
	u16   sid;    /* Status ID (multiple of 4: either 0, 4, 8, or 12) */
	u16   tline;  /* Texture line width (specified by GS_TB_TLINE) */
	u32   flag;   /* Status flag */
	u32   mask;   /* Status mask */
};     /* 24 bytes */

struct uObjTxtrTile
{
	u32   type;   /* Structure identifier (G_OBJLT_TXTRTILE) */
	u32   image; /* Texture source address in DRAM (8-byte alignment) */
	u16   twidth; /* Texture width (specified by GS_TT_TWIDTH) */
	u16   tmem;   /* TMEM word address where texture will be loaded (8-byte word) */
	u16   sid;    /* Status ID (multiple of 4: either 0, 4, 8, or 12) */
	u16   theight;/* Texture height (specified by GS_TT_THEIGHT) */
	u32   flag;   /* Status flag */
	u32   mask;   /* Status mask  */
};      /* 24 bytes */

struct uObjTxtrTLUT
{
	u32   type;   /* Structure identifier (G_OBJLT_TLUT) */
	u32   image; /* Texture source address in DRAM */
	u16   pnum;   /* Number of palettes to load - 1 */
	u16   phead;  /* Palette position at start of load (256~511) */
	u16   sid;    /* Status ID (multiple of 4: either 0, 4, 8, or 12) */
	u16   zero;   /* Always assign 0 */
	u32   flag;   /* Status flag */
	u32   mask;   /* Status mask */
};      /* 24 bytes */

typedef union
{
	uObjTxtrBlock      block;
	uObjTxtrTile       tile;
	uObjTxtrTLUT       tlut;
} uObjTxtr;

struct uObjTxSprite
{
	uObjTxtr      txtr;
	uObjSprite    sprite;
};

struct uObjMtx
{
	s32 A, B, C, D;   /* s15.16 */
	s16 Y, X;         /* s10.2 */
	u16 BaseScaleY;   /* u5.10 */
	u16 BaseScaleX;   /* u5.10 */
};

struct uObjSubMtx
{
	s16 Y, X;		/* s10.2  */
	u16 BaseScaleY;	/* u5.10  */
	u16 BaseScaleX;	/* u5.10  */
};

static uObjMtx objMtx;

void resetObjMtx()
{
	objMtx.A = 1 << 16;
	objMtx.B = 0;
	objMtx.C = 0;
	objMtx.D = 1 << 16;
	objMtx.X = 0;
	objMtx.Y = 0;
	objMtx.BaseScaleX = 1 << 10;
	objMtx.BaseScaleY = 1 << 10;
}

enum S2DEXVersion
{
	eVer1_3,
	eVer1_5,
	eVer1_7
};

static
S2DEXVersion gs_s2dexversion = S2DEXVersion::eVer1_7;

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

		const u32 O1 = (gSP.objRendermode & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_SHRINKSIZE_2 | G_OBJRM_WIDEN)) >> 3;
		A0 = CorrectorsA01_16[(0 + O1) ^ 1];
		A1 = CorrectorsA01_16[(1 + O1) ^ 1];
		const u32 O2 = (gSP.objRendermode & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_BILERP)) >> 2;
		A2 = CorrectorsA23_16[(0 + O2) ^ 1];
		A3 = CorrectorsA23_16[(1 + O2) ^ 1];

		const s16 * CorrectorsB03_16 = nullptr;
		u32 O3 = 0;
		if (gs_s2dexversion == eVer1_3) {
			static const u32 CorrectorsB03_v1_3[] = {
				0xFFFC0000,
				0x00000000,
				0x00000001,
				0x00000000,
				0xFFFC0000,
				0x00000000,
				0x00000001,
				0xFFFF0001,
				0xFFFC0000,
				0x00030000,
				0x00000001,
				0x00000000,
				0xFFFC0000,
				0x00030000,
				0x00000001,
				0xFFFF0000,
				0xFFFF0003,
				0x0000FFF0,
				0x00000001,
				0x0000FFFF,
				0xFFFF0003,
				0x0000FFF0,
				0x00000001,
				0xFFFFFFFF,
				0xFFFF0003,
				0x0000FFF0,
				0x00000000,
				0x00000000,
				0xFFFF0003,
				0x0000FFF0,
				0x00000000,
				0xFFFF0000
			};
			CorrectorsB03_16 = reinterpret_cast<const s16*>(CorrectorsB03_v1_3);
			O3 = (_SHIFTL(gSP.objRendermode, 3, 16) & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_SHRINKSIZE_2 | G_OBJRM_WIDEN)) >> 1;
			B0 = CorrectorsB03_16[(0 + O3) ^ 1];
			B2 = CorrectorsB03_16[(2 + O3) ^ 1];
			B3 = CorrectorsB03_16[(3 + O3) ^ 1];
			B5 = CorrectorsB03_16[(5 + O3) ^ 1];
			B7 = CorrectorsB03_16[(7 + O3) ^ 1];
		} else {
			static const u32 CorrectorsB03[] = {
				0xFFFC0000,
				0x00000001,
				0xFFFF0003,
				0xFFF00000
			};
			CorrectorsB03_16 = reinterpret_cast<const s16*>(CorrectorsB03);
			O3 = (gSP.objRendermode & G_OBJRM_BILERP) >> 1;
			B0 = CorrectorsB03_16[(0 + O3) ^ 1];
			B2 = CorrectorsB03_16[(2 + O3) ^ 1];
			B3 = CorrectorsB03_16[(3 + O3) ^ 1];
			B5 = 0;
			B7 = 0;
		}
	}

	s16 A0, A1, A2, A3, B0, B2, B3, B5, B7;
};

struct ObjCoordinates
{
	f32 ulx, uly, lrx, lry;
	f32 uls, ult, lrs, lrt;
	f32 z, w;

	ObjCoordinates(const uObjSprite *_pObjSprite, bool _useMatrix)
	{
		/* Fixed point coordinates calculation. Decoded by olivieryuyu */
		S2DEXCoordCorrector CC;
		s16 xh, xl, yh, yl;
		s16 sh, sl, th, tl;
		auto calcST = [&](s16 B, u32 scaleH) {
			sh = CC.A0 + B;
			sl = sh + _pObjSprite->imageW + CC.A0 - CC.A1 - 1;
			th = sh - (((yh & 3) * 0x0200 * scaleH) >> 16);
			tl = th + _pObjSprite->imageH + CC.A0 - CC.A1 - 1;
		};
		const u16 objSpriteScaleW = std::max(_pObjSprite->scaleW, u16(1));
		const u16 objSpriteScaleH = std::max(_pObjSprite->scaleH, u16(1));
		if (_useMatrix) {
			const u32 scaleW = (u32(objMtx.BaseScaleX) * 0x40 * objSpriteScaleW) >> 16;
			const u32 scaleH = (u32(objMtx.BaseScaleY) * 0x40 * objSpriteScaleH) >> 16;
			if (gs_s2dexversion == eVer1_3) {
				// XH = AND ((((objX << 0x10) * 0x0800 * (0x80007FFF/BaseScaleX)) >> 0x30) + X + A2) by B0
				// XL = XH + AND (((((imageW - A1) * 0x100) *  (0x80007FFF/scaleW)) >> 0x20) + B2) by B0
				// YH = AND ((((objY << 0x10) * 0x0800 * (0x80007FFF/BaseScaleY)) >> 0x30) + Y + A2) by B0
				// YL = YH + AND (((((imageH - A1) * 0x100) *  (0x80007FFF/scaleH)) >> 0x20) + B2) by B0
				xh = static_cast<s16>(((((s64(_pObjSprite->objX) << 27) * (0x80007FFFU / u32(objMtx.BaseScaleX))) >> 0x30) + objMtx.X + CC.A2) & CC.B0);
				xl = static_cast<s16>((((((s64(_pObjSprite->imageW) - CC.A1) << 8) * (0x80007FFFU / scaleW)) >> 0x20) + CC.B2) & CC.B0) + xh;
				yh = static_cast<s16>(((((s64(_pObjSprite->objY) << 27) * (0x80007FFFU / u32(objMtx.BaseScaleY))) >> 0x30) + objMtx.Y + CC.A2) & CC.B0);
				yl = static_cast<s16>((((((s64(_pObjSprite->imageH) - CC.A1) << 8) * (0x80007FFFU / scaleH)) >> 0x20) + CC.B2) & CC.B0) + yh;
				calcST(CC.B3, scaleH);
			} else {
				// XHP = ((objX << 16) * 0x0800 * (0x80007FFF / BaseScaleX)) >> 16 + ((AND(X + A2) by B0) << 16))
				// XH = XHP >> 16
				// XLP = XHP + (((ImageW - A1) << 24) * (0x80007FFF / scaleW)) >> 32
				// XL = XLP >> 16
				// YHP = ((objY << 16) * 0x0800 * (0x80007FFF / BaseScaleY)) >> 16 + ((AND(Y + A2) by B0) << 16))
				// YH = YHP >> 16
				// YLP = YHP + (((ImageH - A1) << 24) * (0x80007FFF / scaleH)) >> 32
				// YL = YLP >> 16
				const s32 xhp = ((((s64(_pObjSprite->objX) << 16) * 0x0800) * (0x80007FFFU / u32(objMtx.BaseScaleX))) >> 32) + (((objMtx.X + CC.A2) & CC.B0) << 16);
				xh = static_cast<s16>(xhp >> 16);
				const s32 xlp = xhp + ((((u64(_pObjSprite->imageW) - CC.A1) << 24) * (0x80007FFFU / scaleW)) >> 32);
				xl = static_cast<s16>(xlp >> 16);
				const s32 yhp = ((((s64(_pObjSprite->objY) << 16) * 0x0800) * (0x80007FFFU / u32(objMtx.BaseScaleY))) >> 32) + (((objMtx.Y + CC.A2) & CC.B0) << 16);
				yh = static_cast<s16>(yhp >> 16);
				const s32 ylp = yhp + ((((u64(_pObjSprite->imageH) - CC.A1) << 24) * (0x80007FFFU / scaleH)) >> 32);
				yl = static_cast<s16>(ylp >> 16);
				calcST(CC.B2, scaleH);
			}
		} else {
			// XH = AND(objX + A2) by B0
			// XL = ((AND(objX + A2) by B0) << 16) + (((ImageW - A1) << 24)*(0x80007FFF / scaleW)) >> 48
			// YH = AND(objY + A2) by B0
			// YL = ((AND(objY + A2) by B0) << 16) + (((ImageH - A1) << 24)*(0x80007FFF / scaleH)) >> 48
			xh = (_pObjSprite->objX + CC.A2) & CC.B0;
			xl = static_cast<s16>((((u64(_pObjSprite->imageW) - CC.A1) << 24) * (0x80007FFFU / u32(objSpriteScaleW))) >> 48) + xh;
			yh = (_pObjSprite->objY + CC.A2) & CC.B0;
			yl = static_cast<s16>((((u64(_pObjSprite->imageH) - CC.A1) << 24) * (0x80007FFFU / u32(objSpriteScaleH))) >> 48) + yh;
			calcST(CC.B2, objSpriteScaleH);
		}

		ulx = _FIXED2FLOAT(xh, 2);
		lrx = _FIXED2FLOAT(xl, 2);
		uly = _FIXED2FLOAT(yh, 2);
		lry = _FIXED2FLOAT(yl, 2);

		uls = _FIXED2FLOAT(sh, 5);
		lrs = _FIXED2FLOAT(sl, 5);
		ult = _FIXED2FLOAT(th, 5);
		lrt = _FIXED2FLOAT(tl, 5);

		if ((_pObjSprite->imageFlags & G_BG_FLAG_FLIPS) != 0)
			std::swap(uls, lrs);
		if ((_pObjSprite->imageFlags & G_BG_FLAG_FLIPT) != 0)
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

		// BgRectCopyOnePiece() does not support scaleW and scaleH
		if (gDP.otherMode.cycleType == G_CYC_COPY) {
			scaleW = 1.0f;
			scaleH = 1.0f;
		}

		f32 frameW = _FIXED2FLOAT(_pObjScaleBg->frameW, 2);
		f32 frameH = _FIXED2FLOAT(_pObjScaleBg->frameH, 2);
		f32 imageW = (f32)((_pObjScaleBg->imageW >> 2) & 0xFFFFFFFE);
		f32 imageH = (f32)((_pObjScaleBg->imageH >> 2) & 0xFFFFFFFE);

		if (u32(imageW) == 512 && (config.generalEmulation.hacks & hack_RE2) != 0u) {
			const f32 width = f32(*REG.VI_WIDTH);
			const f32 scale = imageW / width;
			imageW = width;
			frameW = width;
			imageH *= scale;
			frameH *= scale;
			scaleW = 1.0f;
			scaleH = 1.0f;
		}

		ulx = frameX;
		uly = frameY;
		lrx = ulx + std::min(frameW, imageW/scaleW);
		lry = uly + std::min(frameH, imageH/scaleH);

		uls = imageX;
		ult = imageY;
		lrs = uls + (lrx - ulx) * scaleW;
		lrt = ult + (lry - uly) * scaleH;

		// G_CYC_COPY (BgRectCopyOnePiece()) does not allow texture filtering
		if (gDP.otherMode.cycleType != G_CYC_COPY) {
			// Correct texture coordinates if G_OBJRM_BILERP
			// bilinear interpolation is set
			if ((gSP.objRendermode & G_OBJRM_BILERP) != 0u) {
				// No correction gives the best picture, but is this correct?
				//uls -= 0.5f;
				//ult -= 0.5f;
				//lrs -= 0.5f;
				//lrt -= 0.5f;
			}
			// SHRINKSIZE_1 adds a 0.5f perimeter around the image
			// upper left texture coords += 0.5f; lower left texture coords -= 0.5f
			if ((gSP.objRendermode&G_OBJRM_SHRINKSIZE_1) != 0u) {
				uls += 0.5f;
				ult += 0.5f;
				lrs -= 0.5f;
				lrt -= 0.5f;
			}
			// SHRINKSIZE_2 adds a 1.0f perimeter
			// upper left texture coords += 1.0f; lower left texture coords -= 1.0f
			else if ((gSP.objRendermode&G_OBJRM_SHRINKSIZE_2) != 0u) {
				uls += 1.0f;
				ult += 1.0f;
				lrs -= 1.0f;
				lrt -= 1.0f;
			}
		}

		if (config.graphics2D.enableTexCoordBounds != 0u) {
			gDP.m_texCoordBounds.valid = true;
			gDP.m_texCoordBounds.uls = uls;
			gDP.m_texCoordBounds.lrs = lrs - 1.0f;
			gDP.m_texCoordBounds.ult = ult;
			gDP.m_texCoordBounds.lrt = lrt - 1.0f;
		}

		// BgRect1CycOnePiece() and BgRectCopyOnePiece() do only support
		// imageFlip in horizontal direction
		if ((_pObjScaleBg->imageFlip & G_BG_FLAG_FLIPS) != 0u) {
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
	if (r > 31) r = 31;
	if (g > 31) g = 31;
	if (b > 31) b = 31;
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

	gDPSetTile( _pObjSprite->imageFmt, _pObjSprite->imageSiz, _pObjSprite->imageStride, _pObjSprite->imageAdrs, G_TX_RENDERTILE, _pObjSprite->imagePal, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( G_TX_RENDERTILE, 0, 0, (w - 1) << 2, (h - 1) << 2 );
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
				gDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, objTxtr->block.tsize + 1, objTxtr->block.image);
				gDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, objTxtr->block.tmem,
						   G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 0, 0, 0, 0);
				gDPLoadBlock( G_TX_LOADTILE, 0, 0, objTxtr->block.tsize << 2, objTxtr->block.tline );
				DebugMsg(DEBUG_NORMAL, "gSPObjLoadTxtr: load block\n");
				break;
			case G_OBJLT_TXTRTILE:
				gDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, objTxtr->tile.twidth + 1, objTxtr->tile.image);
				gDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem,
						   G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 0, 0, 0, 0);
				gDPLoadTile( G_TX_LOADTILE, 0, 0, objTxtr->tile.twidth << 2, objTxtr->tile.theight );
				DebugMsg(DEBUG_NORMAL, "gSPObjLoadTxtr: load tile\n");
				break;
			case G_OBJLT_TLUT:
				gDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, objTxtr->tlut.image);
				gDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, objTxtr->tlut.phead,
						   G_TX_LOADTILE, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 0, 0, 0, 0);
				gDPLoadTLUT( G_TX_LOADTILE, 0, 0, objTxtr->tlut.pnum << 2, 0 );
				DebugMsg(DEBUG_NORMAL, "gSPObjLoadTxtr: load tlut\n");
				break;
		}
		gSP.status[objTxtr->block.sid >> 2] =
			(gSP.status[objTxtr->block.sid >> 2] & ~objTxtr->block.mask) | (objTxtr->block.flag & objTxtr->block.mask);
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
	uObjSprite *pObjSprite = (uObjSprite*)&RDRAM[address];
	gSPSetSpriteTile(pObjSprite);

	/* Fixed point coordinates calculation. Decoded by olivieryuyu */
	//	X1 = AND (X + B3) by B0 + ((objX + A3) * A) >> 16 + ((objY + A3) * B) >> 16
	//	Y1 = AND (Y + B3) by B0 + ((objX + A3) * C) >> 16 + ((objY + A3) * D) >> 16
	//	X2 = AND (X + B3) by B0 + (((((imageW - A1) * 0x0100)* (0x80007FFF/scaleW)) >> 32+ objX + A3) * A) >> 16  + (((((imageH - A1) * 0x0100)* (0x80007FFF/scaleH)) >> 32 + objY + A3) * B) >> 16
	//	Y2 = AND (Y + B3) by B0 + (((((imageW - A1) * 0x0100)* (0x80007FFF/scaleW)) >> 32 + objX + A3) * C) >> 16 + (((((imageH - A1) * 0x0100)* (0x80007FFF/scaleH)) >> 32 + objY + A3) * D) >> 16
	S2DEXCoordCorrector CC;
	const s16 x0 = (gs_s2dexversion == eVer1_3) ?
					((objMtx.X + CC.B5) & CC.B0) + CC.B7 :
					((objMtx.X + CC.B3) & CC.B0);
	const s16 y0 = (gs_s2dexversion == eVer1_3) ?
					((objMtx.Y + CC.B5) & CC.B0) + CC.B7 :
					((objMtx.Y + CC.B3) & CC.B0);
	const s16 ulx = pObjSprite->objX + CC.A3;
	const s16 uly = pObjSprite->objY + CC.A3;
	const u32 objSpriteScaleW = std::max(u32(pObjSprite->scaleW), 1U);
	const u32 objSpriteScaleH = std::max(u32(pObjSprite->scaleH), 1U);
	const s16 lrx = ((((u64(pObjSprite->imageW) - CC.A1) << 8) * (0x80007FFFU / objSpriteScaleW)) >> 32) + ulx;
	const s16 lry = ((((u64(pObjSprite->imageH) - CC.A1) << 8) * (0x80007FFFU / objSpriteScaleH)) >> 32) + uly;

	auto calcX = [&](s16 _x, s16 _y) -> f32
	{
		const s16 X = x0 + static_cast<s16>(((_x * objMtx.A) >> 16)) + static_cast<s16>(((_y * objMtx.B) >> 16));
		return _FIXED2FLOAT(X, 2);
	};

	auto calcY = [&](s16 _x, s16 _y) -> f32
	{
		const s16 Y = y0 + static_cast<s16>(((_x * objMtx.C) >> 16)) + static_cast<s16>(((_y * objMtx.D) >> 16));
		return _FIXED2FLOAT(Y, 2);
	};

	f32 uls = 0.0f;
	f32 lrs = _FIXED2FLOAT(pObjSprite->imageW, 5) - 1.0f;
	f32 ult = 0.0f;
	f32 lrt = _FIXED2FLOAT(pObjSprite->imageH, 5) - 1.0f;

	if (pObjSprite->imageFlags & G_BG_FLAG_FLIPS)
		std::swap(uls, lrs);

	if (pObjSprite->imageFlags & G_BG_FLAG_FLIPT)
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
	objMtx = *reinterpret_cast<const uObjMtx *>(RDRAM + RSP_SegmentToPhysical(mtx));
	DebugMsg(DEBUG_NORMAL, "gSPObjMatrix\n");
}

static
void gSPObjSubMatrix(u32 mtx)
{
	const uObjSubMtx * pObjSubMtx = reinterpret_cast<const uObjSubMtx*>(RDRAM + RSP_SegmentToPhysical(mtx));
	objMtx.X = pObjSubMtx->X;
	objMtx.Y = pObjSubMtx->Y;
	objMtx.BaseScaleX = pObjSubMtx->BaseScaleX;
	objMtx.BaseScaleY = pObjSubMtx->BaseScaleY;
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
void _loadBGImage(const uObjScaleBg * _pBgInfo, bool _loadScale, bool _fbImage)
{
	gSP.bgImage.address = RSP_SegmentToPhysical(_pBgInfo->imagePtr);

	const u32 imageW = _pBgInfo->imageW >> 2;
	const u32 imageH = _pBgInfo->imageH >> 2;
	if (imageW == 512 && (config.generalEmulation.hacks & hack_RE2) != 0) {
		gSP.bgImage.width = *REG.VI_WIDTH;
		gSP.bgImage.height = (imageH * imageW) / gSP.bgImage.width;
	}
	else {
		gSP.bgImage.width = imageW - imageW % 2;
		gSP.bgImage.height = imageH - imageH % 2;
	}
	gSP.bgImage.format = _pBgInfo->imageFmt;
	gSP.bgImage.size = _pBgInfo->imageSiz;
	gSP.bgImage.palette = _pBgInfo->imagePal;
	gSP.bgImage.imageX = _FIXED2FLOAT(_pBgInfo->imageX, 5);
	gSP.bgImage.imageY = _FIXED2FLOAT(_pBgInfo->imageY, 5);
	if (_loadScale) {
		gSP.bgImage.scaleW = _FIXED2FLOAT(_pBgInfo->scaleW, 10);
		gSP.bgImage.scaleH = _FIXED2FLOAT(_pBgInfo->scaleH, 10);
	}
	else
		gSP.bgImage.scaleW = gSP.bgImage.scaleH = 1.0f;

	gDP.tiles[0].textureMode = TEXTUREMODE_BGIMAGE;

	if (_fbImage) {
		FrameBuffer *pBuffer = frameBufferList().findBuffer(gSP.bgImage.address);
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

static
bool _useOnePieceBgCode(u32 address, bool & fbImage)
{
	fbImage = false;
	if (config.frameBufferEmulation.enable != 0) {
		uObjScaleBg *pObjScaleBg = (uObjScaleBg*)&RDRAM[address];
		FrameBuffer *pBuffer = frameBufferList().findBuffer(RSP_SegmentToPhysical(pObjScaleBg->imagePtr));
		fbImage = pBuffer != nullptr &&
			pBuffer->m_size == pObjScaleBg->imageSiz &&
			(!pBuffer->m_isDepthBuffer || pBuffer->m_changed) &&
			(pObjScaleBg->imageFmt != G_IM_FMT_CI || pObjScaleBg->imageSiz != G_IM_SIZ_8b);
		if (fbImage && (pBuffer->m_cfb || !pBuffer->isValid(false))) {
			frameBufferList().removeBuffer(pBuffer->m_startAddress);
			fbImage = false;
		}
	}

	if (config.graphics2D.bgMode == Config::BGMode::bgOnePiece)
		return true;

	if ((config.generalEmulation.hacks & hack_RE2) != 0)
		return true;

	return fbImage;
}

static
void BgRect1CycOnePiece(u32 _bg, bool _fbImage)
{
	uObjScaleBg *pObjScaleBg = (uObjScaleBg*)&RDRAM[_bg];
	_loadBGImage(pObjScaleBg, true, _fbImage);

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

	ObjCoordinates objCoords(pObjScaleBg);
	gSPDrawObjRect(objCoords);

	DebugMsg(DEBUG_NORMAL, "BgRect1CycOnePiece\n");
}

static
void BgRectCopyOnePiece(u32 _bg, bool _fbImage)
{
	uObjScaleBg *pObjBg = (uObjScaleBg*)&RDRAM[_bg];
	_loadBGImage(pObjBg, false, _fbImage);

	// See comment to BgRect1CycOnePiece
	if ((config.generalEmulation.hacks & hack_ZeldaMM) != 0 &&
		(gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != nullptr)
		)
		_copyDepthBuffer();

	gDP.otherMode.cycleType = G_CYC_COPY;
	gDP.changed |= CHANGED_CYCLETYPE;
	gSPTexture(1.0f, 1.0f, 0, 0, TRUE);

	ObjCoordinates objCoords(pObjBg);
	gSPDrawObjRect(objCoords);

	DebugMsg(DEBUG_NORMAL, "BgRectCopyOnePiece\n");
}

//#define runCommand(w0, w1) GBI.cmd[_SHIFTR(w0, 24, 8)](w0, w1)
inline
void runCommand(u32 w0, u32 w1)
{
	GBI.cmd[_SHIFTR(w0, 24, 8)](w0, w1);
};

static
void BgRect1CycStripped(u32 _bgAddr)
{
	uObjScaleBg objBg = *reinterpret_cast<const uObjScaleBg*>(RDRAM + _bgAddr);
	const u32 imagePtr = RSP_SegmentToPhysical(objBg.imagePtr);
	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;
	ValueKeeper<bool> otherMode(RSP.LLE, true);

	s32 E2_1;
	u16 F1_1;
	u16 P;
	s16 H2;

	// Part 1
	{
		// Step 1 & 2
		s16 Aw = objBg.frameW - ((((objBg.imageW << 10) / objBg.scaleW) - 1) & 0xFFFC);
		if (Aw < 0)
			Aw = 0;
		if ((objBg.imageFlip & G_BG_FLAG_FLIPS) != 0)
			objBg.frameX += Aw;
		s16 Bw = std::max(0, gDP.scissor.xh - objBg.frameX);
		s16 Cw = std::max(0, objBg.frameX + objBg.frameW - gDP.scissor.xl - Aw);
		if ((s16)objBg.frameW - Aw - Bw - Cw <= 0)
			return;
		s16 Dw = objBg.frameX + Bw;
		s16 Ew = objBg.frameX + objBg.frameW - Aw - Cw;

		s16 Ah = objBg.frameH - ((((objBg.imageH << 10) / objBg.scaleH) - 1) & 0xFFFC);
		if (Ah < 0)
			Ah = 0;
		s16 Bh = std::max(0, gDP.scissor.yh - objBg.frameY);
		s16 Ch = std::max(0, objBg.frameY + objBg.frameH - gDP.scissor.yl - Ah);
		if ((s16)objBg.frameH - Ah - Bh - Ch <= 0)
			return;
		s16 Dh = ((objBg.frameY + Bh) * 0x4000) >> 16;
		s16 Eh = ((objBg.frameH - Ah - Bh - Ch) * 0x4000) >> 16;

		*reinterpret_cast<u32*>(DMEM + 0x548) = (Dw << 16) | Ew;
		*reinterpret_cast<u32*>(DMEM + 0x54C) = (Dh << 16) | Eh;

		// Step 3
		u16 Fw = objBg.imageW << 3;
		if ((objBg.imageFlip & G_BG_FLAG_FLIPS) != 0)
			Bw = Cw;
		s16 Gw = ((objBg.scaleW * Bw * 0x0200) >> 16) + objBg.imageX;
		s32 Hw = Gw - Fw;
		u16 Fh = objBg.imageH << 3;
		s16 Gh = ((objBg.scaleH * Bh * 0x0200) >> 16) + objBg.imageY;
		while (Hw >= 0) {
			Gw -= Fw;
			Gh += 0x20;
			objBg.imageYorig += 0x20;
			Hw = Gw - Fw;
		}
		s32 Hh = Gh - Fh;
		while (Hh >= 0) {
			Gh -= Fh;
			objBg.imageYorig -= Fh;
			Hh = Gh - Fh;
		}

		s32 I = (s32(Gh) - objBg.imageYorig) << 5;
		s16 J = (objBg.scaleW * (objBg.frameW - Aw - Bw - Cw)) >> 7;
		u8 J_2 = 1;
		if (J + Gw + 0x0B < Fw)
			J_2 = 0;
		u8 K = (gSP.objRendermode & 0x08) >> 3;

		// Step 4
		static const u32 aSize[] = {
			0x01FF0080,
			0x00FF0100,
			0x007F0200,
			0x003F0400
		};
		static const u32 aFormat[] = {
			0x04000400,
			0x02000400,
			0x04000000
		};
		const u16 *  aFormat16 = reinterpret_cast<const u16*>(aFormat);
		u16 L = aFormat16[objBg.imageFmt ^ 1];
		u32 M = aSize[objBg.imageSiz];
		u16 N = ((objBg.frameW * objBg.scaleW) >> 7) + (K << 5);
		u16 O = N;
		if (N >= Fw)
			O = Fw;
		P = (((O + (M >> 16)) * (M & 0xFFFF)) >> 16) + 1;

		*reinterpret_cast<u32*>(DMEM + 0x550) = (K << 24) | (J_2 << 16) | P;
		*reinterpret_cast<s32*>(DMEM + 0x554) = I;

		// Step 5
		u16 Q = L / (P * 2) + K * 0xFFFF;
		s32 R = (0x100000 * Q) / objBg.scaleH;

		*reinterpret_cast<u32*>(DMEM + 0x558) = R;

		// Step 6
		s32 S = ((s64(I) * 0x4000000 / objBg.scaleH) >> 16) & 0xFFFFFC00;
		//s16 T = (S / R);
		s16 T = static_cast<s16>(((0xFFFFFFFF / R) * s64(S)) >> 0x20);
		s32 U = R * (T + 1);
		s16 V = T;
		if (U <= S)
			V += 1;
		s32 W = R * V;
		s32 Z = R - S + W;

		*reinterpret_cast<u32*>(DMEM + 0x55C) = Z;
		*reinterpret_cast<s32*>(DMEM + 0x560) = objBg.imageYorig;

		// Step 7
		u32 A1 = S - (W & 0xFFFFFC00);
		u32 B1 = (A1 * 0x0040) >> 16;
		u32 C1 = B1 * objBg.scaleH;
		u16 D1 = static_cast<u16>(((C1 * 0x0040) >> 16) & 0x0000FFFF);
		u16 E1 = Q - D1;
		u16 F1 = C1 & 0xFFFF;
		F1_1 = ((F1 <<11) >> 16) & 0x001F;

		// Step 8
		s16 A2 = static_cast<s16>((u32(Q) * u32(V) + u32(D1) + ((objBg.imageYorig << 11) >> 16)) & 0xFFFF);
		u16 B2 = (objBg.imageH << 14) >> 16;
		s16 A2_1 = (A2 >= 0) ? A2 : A2 + B2;
		if (A2 - B2 >= 0)
			A2_1 -= B2;
		s16 C2 = static_cast<s16>(((s32(Gw) * (M & 0xFFFF)) >> 16) << 3);
		s16 D2 = static_cast<s16>(((s32(Fw) * (M & 0xFFFF)) >> 16) << 3);
		s32 E2 = A2_1 * D2 + C2;
		E2_1 = E2 + imagePtr;
		s32 F2 = E1 * D2;
		s32 G2 = Q * D2;
		H2 = Gw & (M >> 16);
		if ((objBg.imageFlip & G_BG_FLAG_FLIPS) != 0)
			H2 = (H2 + J) * 0xFFFF;
		u32 I2 = 0xFD100000 | ((D2 >> 1) - 1);
		u32 J2 = 0xF5100000 | (P << 9);
		u32 J2_1 = (J2 & 0xFF00FFFF) | (((objBg.imageFmt << 5) | (objBg.imageSiz << 3)) << 16);
		u32 K2 = (objBg.imagePal << 20) | 0x0007C1F0;
		u16 L2 = objBg.imageH >> 2;

		*reinterpret_cast<u32*>(DMEM + 0x564) = (C2 << 16) | D2;
		*reinterpret_cast<u32*>(DMEM + 0x568) = I2;
		*reinterpret_cast<u32*>(DMEM + 0x56C) = J2;
		//*reinterpret_cast<u32*>(DMEM + 0x570) = (Q << 16) | E1;
		*reinterpret_cast<u32*>(DMEM + 0x570) = (E1 << 16) | Q;
		*reinterpret_cast<u32*>(DMEM + 0x574) = F2;
		*reinterpret_cast<u32*>(DMEM + 0x578) = G2;
		*reinterpret_cast<u32*>(DMEM + 0x57C) = (L2 << 16) | A2_1;

		runCommand(J2, 0x27000000);
		runCommand(J2_1, K2);
		runCommand((G_SETTILESIZE<<24), 0);
	}

	if (config.graphics2D.enableNativeResTexrects != 0)
		dwnd().getDrawer().setBackgroundDrawingMode(true);

	//Part two
	{

	u32 VV = *reinterpret_cast<u32*>(DMEM + 0x57C);
	s16 AA = _SHIFTR(VV, 16, 16) - _SHIFTR(VV, 0, 16);
	VV = *reinterpret_cast<u32*>(DMEM + 0x570);
	s32 CC = VV >> 16;
	u32 DD = *reinterpret_cast<u32*>(DMEM + 0x55C);
	u32 EE = *reinterpret_cast<u32*>(DMEM + 0x558);
	VV = *reinterpret_cast<u32*>(DMEM + 0x54C);
	s32 FF = _SHIFTR(VV,  0, 16);
	u16 JJ = _SHIFTR(VV, 16, 16);
	u32 GG = *reinterpret_cast<u32*>(DMEM + 0x574);
	VV = *reinterpret_cast<u32*>(DMEM + 0x548);
	u32 HH = _SHIFTR(VV, 16, 16) << 0x0C;
	u32 II = _SHIFTR(VV,  0, 16) << 0x0C;

	u32 step = 2;
	s32 KK;
	s16 LL, MM, NN, RR, AAA;
	u32 SS;

	auto setTexture = [&]() {
//		0x28000000, 0x00000000
		runCommand((*reinterpret_cast<u32*>(DMEM + 0x56C) | AAA), 0x27000000);
		runCommand((*reinterpret_cast<u32*>(DMEM + 0x568)), SS);
//		0x26000000, 0x00000000
		runCommand(0xF4000000, (((P + 0x6FF)<<16) | ((RR << 2) - 1)));
	};

	bool stop = false;
	while (!stop) {
		switch (step) {
		case 2:
			KK = DD >> 10;
			if (KK > 0)
				step = 5;
			else
				step = 3;
			break;
		case 3:
			AA -= CC;
			if (AA > 0)
				E2_1 += GG;
			else {
				VV = *reinterpret_cast<u32*>(DMEM + 0x564);
				E2_1 = imagePtr + _SHIFTR(VV, 16, 16) + _SHIFTR(VV, 0, 16) * (-AA);
				VV = *reinterpret_cast<u32*>(DMEM + 0x57C);
				AA += _SHIFTR(VV, 16, 16);
			}
			step = 4;
			break;
		case 4:
			DD += EE;
			VV = *reinterpret_cast<u32*>(DMEM + 0x570);
			CC = s32(_SHIFTR(VV, 0, 16));
			GG = *reinterpret_cast<u32*>(DMEM + 0x578);
			F1_1 = 0;
			step = 2;
			break;
		case 5:
			FF -= KK;
			DD &= 0x03FF;
			if (FF < 0) {
				CC += ((objBg.scaleH * FF) >> 10) + 1;
				KK += FF;
				VV = *reinterpret_cast<u32*>(DMEM + 0x570);
				if (CC - s32(_SHIFTR(VV, 0, 16)) > 0)
					CC = s32(_SHIFTR(VV, 0, 16));
			}
			step = 6;
			break;
		case 6:
			LL = JJ + KK;
			VV = *reinterpret_cast<u32*>(DMEM + 0x550);
			P = _SHIFTR(VV, 0, 16);
			MM = CC + _SHIFTR(VV, 24, 8);
			NN = AA - _SHIFTR(VV, 16, 8);
			if (NN - MM < 0)
				step = 7;
			else
				step = 77;
			break;
		case 7:
			RR = MM - AA;
			AAA = AA;
			if (RR > 0) {
				VV = *reinterpret_cast<u32*>(DMEM + 0x564);
				SS = imagePtr + _SHIFTR(VV, 16, 16);
				if ((AAA & 1) != 0) {
					AAA--;
					RR++;
					SS -= _SHIFTR(VV, 0, 16);
				}
				AAA *= P;
				setTexture();
			}
			step = 8;
			break;
		case 77:
			RR = MM;
			SS = E2_1;
			runCommand(*reinterpret_cast<u32*>(DMEM + 0x568), SS);
			//0x26000000, 0x00000000
			runCommand(0xF4000000, (((P + 0x6FF) << 16) | ((RR << 2) - 1)));
			AA -= CC;
			E2_1 += GG;
			step = 11;
			break;
		case 8:
			VV = *reinterpret_cast<u32*>(DMEM + 0x550);
			if (_SHIFTR(VV, 16, 8) != 0) {
				SS = imagePtr;
				s16 BBB = NN;
				RR = NN & 1;
				VV = *reinterpret_cast<u32*>(DMEM + 0x564);
				if (RR != 0) {
					BBB--;
					SS -= _SHIFTR(VV, 0, 16);
				}
				u32 CCC = E2_1 + BBB * _SHIFTR(VV, 0, 16);
				RR++;
				u16 DDD = ((_SHIFTR(VV, 0, 16) - _SHIFTR(VV, 16, 16)) * 0x2000) >> 16;
				u32 ZZZ = BBB * P;
				P -= DDD;
				AAA = ZZZ + DDD;
				setTexture();
				SS = CCC;
				AAA = ZZZ;
				P = DDD;
				setTexture();
			}
			step = 9;
			break;
		case 9:
			AA -= CC;
			if (NN <= 0) {
				runCommand(*reinterpret_cast<u32*>(DMEM + 0x56C), 0x27000000);
			} else {
				VV = *reinterpret_cast<u32*>(DMEM + 0x550);
				P = _SHIFTR(VV, 0, 16);
				SS = E2_1;
				RR = NN;
				AAA = 0;
				setTexture();
			}
			step = 10;
			break;
		case 10:
			if (AA > 0) {
				E2_1 += GG;
			} else {
				VV = *reinterpret_cast<u32*>(DMEM + 0x564);
				E2_1 = imagePtr + _SHIFTR(VV, 16, 16) + _SHIFTR(VV, 0, 16) * (-AA);
				VV = *reinterpret_cast<u32*>(DMEM + 0x57C);
				AA += _SHIFTR(VV, 16, 16);
			}
			step = 11;
			break;
		case 11:
			//0x27000000, 0x00000000
			const u32 w0 = (G_TEXRECT << 24) | (LL << 2) | II;
			const u32 w1 = (JJ << 2) | HH;
			RDP.w2 = (H2 << 16) | F1_1;
			RDP.w3 = (objBg.scaleW << 16) | objBg.scaleH;
			RDP_TexRect(w0, w1);
			if (FF <= 0)
				stop = true;
			else {
				JJ = LL;
				DD = DD + EE;
				VV = *reinterpret_cast<u32*>(DMEM + 0x570);
				CC = _SHIFTR(VV, 0, 16);
				GG = *reinterpret_cast<u32*>(DMEM + 0x578);
				F1_1 = 0;
				step = 2;
			}
			break;
		}
	}
	}

	if (config.graphics2D.enableNativeResTexrects != 0) {
		GraphicsDrawer & drawer = dwnd().getDrawer();
		drawer.flush();
		drawer.setBackgroundDrawingMode(false);
	}
}

static
void BgRectCopyStripped(u32 _bgAddr)
{
	// Step 1
	uObjBg objBg = *reinterpret_cast<const uObjBg*>(RDRAM + _bgAddr);
	const u32 imagePtr = RSP_SegmentToPhysical(objBg.imagePtr);
	gDP.otherMode.cycleType = G_CYC_COPY;
	gDP.changed |= CHANGED_CYCLETYPE;
	ValueKeeper<bool> otherMode(RSP.LLE, true);

	// Step 2
	s16 Aw = std::max(0, objBg.frameX + objBg.frameW - gDP.scissor.xl);
	s16 Bw = std::min(0, objBg.frameX - gDP.scissor.xh);
	s16 Cw = objBg.frameW + Bw - Aw;
	if (Cw <= 0)
		return;
	s16 Dw = (((objBg.imageX * 0x2000) >> 16) & 0xFFFC) - Bw;
	s16 Ew = objBg.frameX - Bw;

	s16 Ah = std::max(0, objBg.frameY + objBg.frameH - gDP.scissor.yl);
	s16 Bh = std::min(0, objBg.frameY - gDP.scissor.yh);
	s16 Ch = objBg.frameH + Bh - Ah;
	if (Ch <= 0)
		return;
	s16 Dh = (((objBg.imageY * 0x2000) >> 16) & 0xFFFC) - Bh;
	s16 Eh = objBg.frameY - Bh;

	s16 F = Dh - objBg.imageH;
	s16 G = (F >= 0) ? F : Dh;
	s16 H= (objBg.imageFlip != 0) ? Dw + Aw : Dw;

	// Step 3
	u32 I = (objBg.imageLoad == G_BGLT_LOADTILE) ? 0xFFFFFFFF : 0U;
	u32 J = objBg.tmemW << 9;
	u32 K = (G_SETTILE << 24) | 0x100000 | (J & I);
	u32 L = (objBg.imageFmt << 2) | objBg.imageSiz;
	L = (L << 0x13) | J;
	u32 M = (objBg.imagePal << 0x14) | 0x0007C1F0;

	runCommand(K, 0x27000000);
	runCommand((G_SETTILESIZE<<24), 0);
	runCommand(((G_SETTILE<<24) | L), M);

	// Step 4
	static const u32 aSize[] = {
		0x003F0800,
		0x10000080,
		0x001F1000,
		0x20000100,
		0x000F2000,
		0x40000200,
		0x00074000,
		0x80000400
	};
	const u16 * aSize16 = reinterpret_cast<const u16*>(aSize);
	u16 imageSzIdx = objBg.imageSiz << 2;
	u16 N0 = aSize16[(0 + imageSzIdx) ^ 1];
	u16 N1 = aSize16[(1 + imageSzIdx) ^ 1];
	u16 N2 = aSize16[(2 + imageSzIdx) ^ 1];
	G = (G * 0x4000) >> 16;
	u16 O = (N0 & H) + Cw;
	u32 P = ((N1 * H) >> 16) + objBg.tmemSizeW * G;
	u16 Q = (N2 * O) >> 16;
	u32 R = (objBg.imageFlip != 0) ? (((1 - O) * 8) << 16) : (((N0 & H) * 8) << 16);
	u32 S = ((P >> 1) << 3) + imagePtr;
	u16 T = Ew + Cw - 1;

	u16 A1 = objBg.imageH & 0xFFFC;
	u16 A2 = G <<2;
	u16 A3 = (N1 * H) >> 16;
	u32 T0 = Ew << 0x0C;
	u16 T1 = Eh;
	u32 T2 = T << 0x0C;
	s16 AT = Ch;
	s16 U = A1 - A2;

	if (config.graphics2D.enableNativeResTexrects != 0)
		dwnd().getDrawer().setBackgroundDrawingMode(true);

	u32 V, X, Y, Z, AA, w0, w1;
	u16 S5, BB;
	u32 step = 4;
	bool stop = false;
	while (!stop) {
		switch (step) {
		case 4:
			if (U <= 0)
				stop = true;
			step = 5;
		break;
		case 5:
			if (A3 > 0)
				U -= 4;
			if (U > AT)
				U = AT;
			V =  0xE4000000 | T2;
			if (gs_s2dexversion == eVer1_7)
				X = (objBg.imageLoad == G_BGLT_LOADTILE) ? (Q << 2) - 1 : objBg.tmemLoadSH;
			else
				X = (objBg.imageLoad == G_BGLT_LOADTILE) ? (Q << 2) : objBg.tmemLoadSH;
			X = (X | 0x7000) << 0x0C;
			Y = 0xFD100000 | ((objBg.tmemSizeW << 1) - 1);
			AT -= U;
			step = (U <= 0) ? 8 : 55;
		break;
		case 55:
			if (gs_s2dexversion == eVer1_7)
				Z = (objBg.imageLoad == G_BGLT_LOADTILE) ? (objBg.tmemSize << 0x10) | objBg.tmemLoadSH : objBg.tmemSize;
			else
				Z = objBg.tmemSize;
			S5 = objBg.tmemH;
			AA = X | objBg.tmemLoadTH;
			step = 6;
		break;
		case 6:
			U -= S5;
			if (U < 0) {
				Z += objBg.tmemSizeW * U;
				S5 += U;
				AA = (objBg.imageLoad == G_BGLT_LOADTILE) ? X | (S5 - 1) : (((Z - 2) | 0xE000) << 0x0B) | objBg.tmemLoadTH;
			}
			step = 7;
			break;
		case 7:
			BB = T1 + S5 - 1;
			runCommand(Y, S);
			if (objBg.imageLoad == G_BGLT_LOADTILE)
				runCommand((G_LOADTILE<<24), AA);
			else
				runCommand((G_LOADBLOCK<<24), AA);
			w0 = V | BB;
			w1 = T0 | T1;
			RDP.w2 = R;
			RDP.w3 = 0x10000400;
			RDP_TexRect(w0, w1);
			T1 = BB + 1;
			S += Z;
			if (U > 0)
				step = 6;
			else {
				if (AT <= 0)
					stop = true;
				step = 8;
			}
			break;
		case 8:
			if (A3 > 0) {
				A3 >>= 1;
				runCommand(Y, S);
				runCommand(((G_SETTILE<<24) | 0x35100000), 0x06000000);
				runCommand((G_LOADBLOCK<<24), (0x06000000 | (((((objBg.tmemSizeW >> 1) - A3) << 2) - 1) << 12)));
				runCommand(Y, imagePtr);
				runCommand(((G_SETTILE << 24) | 0x35100000 | ((objBg.tmemSizeW >> 1) - A3)), 0x06000000);
				runCommand((G_LOADBLOCK<<24), (0x06000000 | (((A3 << 2) - 1) << 12)));
				w0 = V | T1;
				w1 = T0 | T1;
				RDP.w2 = R;
				RDP.w3 = 0x10000400;
				RDP_TexRect(w0, w1);
				T1 += 4;
				AT -= 4;
				if (AT <= 0)
					stop = true;
			}
			step = 9;
			break;
		case 9:
			S = imagePtr + (A3 << 3);
			U = AT;
			AT = 0;
			step = 55;
			break;
		}
	}

	if (config.graphics2D.enableNativeResTexrects != 0) {
		GraphicsDrawer & drawer = dwnd().getDrawer();
		drawer.flush();
		drawer.setBackgroundDrawingMode(false);
	}
}

void S2DEX_BG_1Cyc(u32 w0, u32 w1)
{
	const u32 bgAddr = RSP_SegmentToPhysical(w1);
	bool fbImage = false;
	if (_useOnePieceBgCode(bgAddr, fbImage))
		BgRect1CycOnePiece(bgAddr, fbImage);
	else
		BgRect1CycStripped(bgAddr);
}

void S2DEX_BG_Copy(u32 w0, u32 w1)
{
	const u32 bgAddr = RSP_SegmentToPhysical(w1);
	bool fbImage = false;
	if (_useOnePieceBgCode(bgAddr, fbImage))
		BgRectCopyOnePiece(bgAddr, fbImage);
	else
		BgRectCopyStripped(bgAddr);
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
	switch (_SHIFTR(w0, 0, 8))
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
	resetObjMtx();

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

void S2DEX_1_03_Init()
{
	S2DEX_Init();
	gs_s2dexversion = eVer1_3;
}

void S2DEX_1_05_Init()
{
	S2DEX_Init();
	gs_s2dexversion = eVer1_5;
}

void S2DEX_1_07_Init()
{
	S2DEX_Init();
	gs_s2dexversion = eVer1_7;
}
