#include "glN64.h"
#include "N64.h"
#include "GBI.h"
#include "RSP.h"
#include "gDP.h"
#include "gSP.h"
#include "Types.h"
#include "Debug.h"
#include "convert.h"
#include "OpenGL.h"
#include "CRC.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "VI.h"

gDPInfo gDP;

void gDPSetOtherMode( u32 mode0, u32 mode1 )
{
	gDP.otherMode.h = mode0;
	gDP.otherMode.l = mode1;

	gDP.changed |= CHANGED_RENDERMODE | CHANGED_CYCLETYPE | CHANGED_ALPHACOMPARE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetOtherMode( %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s, %s | %s | %s%s%s%s%s | %s | %s%s%s );\n",
		AlphaDitherText[gDP.otherMode.alphaDither],
		ColorDitherText[gDP.otherMode.colorDither],
		CombineKeyText[gDP.otherMode.combineKey],
		TextureConvertText[gDP.otherMode.textureConvert],
		TextureFilterText[gDP.otherMode.textureFilter],
		TextureLUTText[gDP.otherMode.textureLUT],
		TextureLODText[gDP.otherMode.textureLOD],
		TextureDetailText[gDP.otherMode.textureDetail],
		TexturePerspText[gDP.otherMode.texturePersp],
		CycleTypeText[gDP.otherMode.cycleType],
		PipelineModeText[gDP.otherMode.pipelineMode],
		AlphaCompareText[gDP.otherMode.alphaCompare],
		DepthSourceText[gDP.otherMode.depthSource],
		gDP.otherMode.AAEnable ? "AA_EN | " : "",
		gDP.otherMode.depthCompare ? "Z_CMP | " : "",
		gDP.otherMode.depthUpdate ? "Z_UPD | " : "",
		gDP.otherMode.imageRead ? "IM_RD | " : "",
		CvgDestText[gDP.otherMode.cvgDest],
		DepthModeText[gDP.otherMode.depthMode],
		gDP.otherMode.cvgXAlpha ? "CVG_X_ALPHA | " : "",
		gDP.otherMode.alphaCvgSel ? "ALPHA_CVG_SEL | " : "",
		gDP.otherMode.forceBlender ? "FORCE_BL" : "" );
#endif
}

void gDPSetPrimDepth( u16 z, u16 dz )
{
	gDP.primDepth.z = min( 1.0f, max( 0.0f, (_FIXED2FLOAT( z, 15 ) - gSP.viewport.vtrans[2]) / gSP.viewport.vscale[2] ) );
	gDP.primDepth.deltaZ = dz;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetPrimDepth( %f, %f );\n",
		gDP.primDepth.z,
		gDP.primDepth.deltaZ);
#endif
}

void gDPPipelineMode( u32 mode )
{
	gDP.otherMode.pipelineMode = mode;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPPipelineMode( %s );\n",
		PipelineModeText[gDP.otherMode.pipelineMode] );
#endif
}

void gDPSetCycleType( u32 type )
{
	gDP.otherMode.cycleType = type;

	gDP.changed |= CHANGED_CYCLETYPE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetCycleType( %s );\n",
		CycleTypeText[gDP.otherMode.cycleType] );
#endif
}

void gDPSetTexturePersp( u32 enable )
{
	gDP.otherMode.texturePersp = enable;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTexturePersp( %s );\n",
		TexturePerspText[gDP.otherMode.texturePersp] );
#endif
}

void gDPSetTextureDetail( u32 type )
{
	gDP.otherMode.textureDetail = type;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTextureDetail( %s );\n",
		TextureDetailText[gDP.otherMode.textureDetail] );
#endif
}

void gDPSetTextureLOD( u32 mode )
{
	gDP.otherMode.textureLOD = mode;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTextureLOD( %s );\n",
		TextureLODText[gDP.otherMode.textureLOD] );
#endif
}

void gDPSetTextureLUT( u32 mode )
{
	gDP.otherMode.textureLUT = mode;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTextureLUT( %s );\n",
		TextureLUTText[gDP.otherMode.textureLUT] );
#endif
}

void gDPSetTextureFilter( u32 type )
{
	gDP.otherMode.textureFilter = type;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTextureFilter( %s );\n",
		TextureFilterText[gDP.otherMode.textureFilter] );
#endif
}

void gDPSetTextureConvert( u32 type )
{
	gDP.otherMode.textureConvert = type;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTextureConvert( %s );\n",
		TextureConvertText[gDP.otherMode.textureConvert] );
#endif
}

void gDPSetCombineKey( u32 type )
{
	gDP.otherMode.combineKey = type;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_COMBINE, "gDPSetCombineKey( %s );\n",
		CombineKeyText[gDP.otherMode.combineKey] );
#endif
}

void gDPSetColorDither( u32 type )
{
	gDP.otherMode.colorDither = type;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetColorDither( %s );\n",
		ColorDitherText[gDP.otherMode.colorDither] );
#endif
}

void gDPSetAlphaDither( u32 type )
{
	gDP.otherMode.alphaDither = type;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetAlphaDither( %s );\n",
		AlphaDitherText[gDP.otherMode.alphaDither] );
#endif
}

void gDPSetAlphaCompare( u32 mode )
{
	gDP.otherMode.alphaCompare = mode;

	gDP.changed |= CHANGED_ALPHACOMPARE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetAlphaCompare( %s );\n",
		AlphaCompareText[gDP.otherMode.alphaCompare] );
#endif
}

void gDPSetDepthSource( u32 source )
{
	gDP.otherMode.depthSource = source;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetDepthSource( %s );\n",
		DepthSourceText[gDP.otherMode.depthSource] );
#endif
}

void gDPSetRenderMode( u32 mode1, u32 mode2 )
{
	gDP.otherMode.l &= 0x00000007;
	gDP.otherMode.l |= mode1 | mode2;

	gDP.changed |= CHANGED_RENDERMODE;

#ifdef DEBUG
	// THIS IS INCOMPLETE!!!
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetRenderMode( %s%s%s%s%s | %s | %s%s%s );\n",
		gDP.otherMode.AAEnable ? "AA_EN | " : "",
		gDP.otherMode.depthCompare ? "Z_CMP | " : "",
		gDP.otherMode.depthUpdate ? "Z_UPD | " : "",
		gDP.otherMode.imageRead ? "IM_RD | " : "",
		CvgDestText[gDP.otherMode.cvgDest],
		DepthModeText[gDP.otherMode.depthMode],
		gDP.otherMode.cvgXAlpha ? "CVG_X_ALPHA | " : "",
		gDP.otherMode.alphaCvgSel ? "ALPHA_CVG_SEL | " : "",
		gDP.otherMode.forceBlender ? "FORCE_BL" : "" );
#endif
}

void gDPSetCombine( s32 muxs0, s32 muxs1 )
{
	gDP.combine.muxs0 = muxs0;
	gDP.combine.muxs1 = muxs1;

	gDP.changed |= CHANGED_COMBINE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_COMBINE, "gDPSetCombine( %s, %s, %s, %s, %s, %s, %s, %s,\n",
		saRGBText[gDP.combine.saRGB0],
		sbRGBText[gDP.combine.sbRGB0],
		mRGBText[gDP.combine.mRGB0],
		aRGBText[gDP.combine.aRGB0],
		saAText[gDP.combine.saA0],
		sbAText[gDP.combine.sbA0],
		mAText[gDP.combine.mA0],
		aAText[gDP.combine.aA0] );

	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_COMBINE, "               %s, %s, %s, %s, %s, %s, %s, %s );\n",
		saRGBText[gDP.combine.saRGB1],
		sbRGBText[gDP.combine.sbRGB1],
		mRGBText[gDP.combine.mRGB1],
		aRGBText[gDP.combine.aRGB1],
		saAText[gDP.combine.saA1],
		sbAText[gDP.combine.sbA1],
		mAText[gDP.combine.mA1],
		aAText[gDP.combine.aA1] );

#endif
}

/*void RSP_UpdateColorImage()
{
	WORD *colorBuffer = (WORD*)&RDRAM[RDP.colorImage.address];
	BYTE *frameBuffer = (BYTE*)malloc( OGL.width * OGL.height * 3 );
	BYTE *framePixel;

	int x, y, frameX, frameY, i;

	glReadBuffer( GL_BACK );
	glReadPixels( 0, 0, OGL.width - 1, OGL.height - 1, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer );
	
	i = 0;
	for (y = 0; y < RDP.height; y++)
	{
		frameY = OGL.height - (y * OGL.scaleY);
		for (x = 0; x < RDP.width; x++)
		{
			frameX = x * OGL.scaleX;
			framePixel = &frameBuffer[(OGL.width * frameY + frameX) * 3];
			colorBuffer[i^1] =	((framePixel[0] >> 3) << 11) |
								((framePixel[1] >> 3) <<  6) |
								((framePixel[2] >> 3) <<  1);
			i++;
		}
	}
	free( frameBuffer );
}*/

void gDPUpdateColorImage()
{
	return;
	if ((gDP.colorImage.size == G_IM_SIZ_16b) && (gDP.colorImage.format == G_IM_FMT_RGBA))
	{
		u16 *frameBuffer = (u16*)malloc( gDP.colorImage.width * OGL.scaleX * gDP.colorImage.height * OGL.scaleY * 2 );
		u16 *colorImage = (u16*)&RDRAM[gDP.colorImage.address];
		u32 frameX, frameY;
		u32 i = 0;

		glReadBuffer( GL_BACK );
		glReadPixels( 0, OGL.height - gDP.colorImage.height * OGL.scaleY + OGL.heightOffset, gDP.colorImage.width * OGL.scaleX, gDP.colorImage.height * OGL.scaleY, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1_EXT, frameBuffer );

		for (u32 y = 0; y < gDP.colorImage.height; y++)
		{
			frameY = (gDP.colorImage.height - 1) * OGL.scaleY - y * OGL.scaleY;
			for (u32 x = 0; x < gDP.colorImage.width; x++)
			{
				frameX = x * OGL.scaleX;
				colorImage[i^1] = frameBuffer[(u32)(gDP.colorImage.width * OGL.scaleX) * frameY + frameX];

				i++;
			}
		}

		free( frameBuffer );
	}
	else if ((gDP.colorImage.size == G_IM_SIZ_8b) && (gDP.colorImage.format == G_IM_FMT_I))
	{
		u8 *frameBuffer = (u8*)malloc( gDP.colorImage.width * OGL.scaleX * gDP.colorImage.height * OGL.scaleY );
		u8 *colorImage = (u8*)&RDRAM[gDP.colorImage.address];
		u32 frameX, frameY;
		u32 i = 0;

		glReadPixels( 0, OGL.height - gDP.colorImage.height * OGL.scaleY + OGL.heightOffset, gDP.colorImage.width * OGL.scaleX, gDP.colorImage.height * OGL.scaleY, GL_LUMINANCE, GL_UNSIGNED_BYTE, frameBuffer );

		for (u32 y = 0; y < gDP.colorImage.height; y++)
		{
			frameY = (gDP.colorImage.height - 1) * OGL.scaleY - y * OGL.scaleY;
			for (u32 x = 0; x < gDP.colorImage.width; x++)
			{
				frameX = x * OGL.scaleX;
				colorImage[i^3] = frameBuffer[(u32)(gDP.colorImage.width * OGL.scaleX) * frameY + frameX];

				i++;
			}
		}

		free( frameBuffer );
	}
}

void gDPSetColorImage( u32 format, u32 size, u32 width, u32 address )
{
/*	if (gDP.colorImage.changed &&
//		(gDP.colorImage.address != gDP.depthImageAddress) &&
		(gDP.colorImage.address != RSP_SegmentToPhysical( address )))
	{
		gDPUpdateColorImage();
		OGL_ClearDepthBuffer();
		gDP.colorImage.changed = FALSE;
		gDP.colorImage.height = 1;
	}*/

/*	for (int i = 0; i < (gDP.colorImage.width * gDP.colorImage.height ) << gDP.colorImage.size >> 1; i++)
	{
		RDRAM[gDP.colorImage.address + i] = 0;
	}*/
	address = RSP_SegmentToPhysical( address );

	if (gDP.colorImage.address != address)
	{
		if (OGL.frameBufferTextures)
		{
			if (gDP.colorImage.changed)
				FrameBuffer_SaveBuffer( gDP.colorImage.address, gDP.colorImage.size, gDP.colorImage.width, gDP.colorImage.height );

			if (address != gDP.depthImageAddress)
				FrameBuffer_RestoreBuffer( address, size, width );

			//OGL_ClearDepthBuffer();
		}

		gDP.colorImage.changed = FALSE;

		if (width == VI.width)
			gDP.colorImage.height = VI.height;
 		else
			gDP.colorImage.height = 1;
	}

	gDP.colorImage.format = format;
	gDP.colorImage.size = size;
	gDP.colorImage.width = width;
	gDP.colorImage.address = RSP_SegmentToPhysical( address );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetColorImage( %s, %s, %i, 0x%08X );\n",
		ImageFormatText[gDP.colorImage.format],
		ImageSizeText[gDP.colorImage.size],
		gDP.colorImage.width,
		gDP.colorImage.address );
#endif
}

void gDPSetTextureImage( u32 format, u32 size, u32 width, u32 address )
{
	gDP.textureImage.format = format;
	gDP.textureImage.size = size;
	gDP.textureImage.width = width;
	gDP.textureImage.address = RSP_SegmentToPhysical( address );
	gDP.textureImage.bpl = gDP.textureImage.width << gDP.textureImage.size >> 1;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTextureImage( %s, %s, %i, 0x%08X );\n",
		ImageFormatText[gDP.textureImage.format],
		ImageSizeText[gDP.textureImage.size],
		gDP.textureImage.width,
		gDP.textureImage.address );
#endif
}

void gDPSetDepthImage( u32 address )
{
//	if (address != gDP.depthImageAddress)
//		OGL_ClearDepthBuffer();

	DepthBuffer_SetBuffer( RSP_SegmentToPhysical( address ) );

	if (depthBuffer.current->cleared)
		OGL_ClearDepthBuffer();

	gDP.depthImageAddress = RSP_SegmentToPhysical( address );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetDepthImage( 0x%08X );\n", gDP.depthImageAddress );
#endif
}

void gDPSetEnvColor( u32 r, u32 g, u32 b, u32 a )
{
	gDP.envColor.r = r * 0.0039215689f;
	gDP.envColor.g = g * 0.0039215689f;
	gDP.envColor.b = b * 0.0039215689f;
	gDP.envColor.a = a * 0.0039215689f;

	gDP.changed |= CHANGED_COMBINE_COLORS;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_COMBINE, "gDPSetEnvColor( %i, %i, %i, %i );\n",
		r, g, b, a );
#endif
}

void gDPSetBlendColor( u32 r, u32 g, u32 b, u32 a )
{
	gDP.blendColor.r = r * 0.0039215689f;
	gDP.blendColor.g = g * 0.0039215689f;
	gDP.blendColor.b = b * 0.0039215689f;
	gDP.blendColor.a = a * 0.0039215689f;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetBlendColor( %i, %i, %i, %i );\n",
		r, g, b, a );
#endif
}

void gDPSetFogColor( u32 r, u32 g, u32 b, u32 a )
{
	gDP.fogColor.r = r * 0.0039215689f;
	gDP.fogColor.g = g * 0.0039215689f;
	gDP.fogColor.b = b * 0.0039215689f;
	gDP.fogColor.a = a * 0.0039215689f;

	gDP.changed |= CHANGED_FOGCOLOR;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetFogColor( %i, %i, %i, %i );\n",
		r, g, b, a );
#endif
}

void gDPSetFillColor( u32 c )
{
	gDP.fillColor.r = _SHIFTR( c, 11, 5 ) * 0.032258064f;
	gDP.fillColor.g = _SHIFTR( c,  6, 5 ) * 0.032258064f;
	gDP.fillColor.b = _SHIFTR( c,  1, 5 ) * 0.032258064f;
	gDP.fillColor.a = _SHIFTR( c,  0, 1 );

	gDP.fillColor.z = _SHIFTR( c,  2, 14 );
	gDP.fillColor.dz = _SHIFTR( c, 0, 2 );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPSetFillColor( 0x%08X );\n", c );
#endif
}

void gDPSetPrimColor( u32 m, u32 l, u32 r, u32 g, u32 b, u32 a )
{
	gDP.primColor.m = m;
	gDP.primColor.l = l * 0.0039215689f;
	gDP.primColor.r = r * 0.0039215689f;
	gDP.primColor.g = g * 0.0039215689f;
	gDP.primColor.b = b * 0.0039215689f;
	gDP.primColor.a = a * 0.0039215689f;

	gDP.changed |= CHANGED_COMBINE_COLORS;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_COMBINE, "gDPSetPrimColor( %i, %i, %i, %i, %i, %i );\n",
		m, l, r, g, b, a );
#endif
}

void gDPSetTile( u32 format, u32 size, u32 line, u32 tmem, u32 tile, u32 palette, u32 cmt, u32 cms, u32 maskt, u32 masks, u32 shiftt, u32 shifts )
{
	if (((size == G_IM_SIZ_4b) || (size == G_IM_SIZ_8b)) && (format == G_IM_FMT_RGBA))
		format = G_IM_FMT_CI;

	gDP.tiles[tile].format = format;
	gDP.tiles[tile].size = size;
	gDP.tiles[tile].line = line;
	gDP.tiles[tile].tmem = tmem;
	gDP.tiles[tile].palette = palette;
	gDP.tiles[tile].cmt = cmt;
	gDP.tiles[tile].cms = cms;
	gDP.tiles[tile].maskt = maskt;
	gDP.tiles[tile].masks = masks;
	gDP.tiles[tile].shiftt = shiftt;
	gDP.tiles[tile].shifts = shifts;

	if (!gDP.tiles[tile].masks) gDP.tiles[tile].clamps = 1;
    if (!gDP.tiles[tile].maskt) gDP.tiles[tile].clampt = 1;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTile( %s, %s, %i, %i, %i, %i, %s%s, %s%s, %i, %i, %i, %i );\n",
		ImageFormatText[format],
		ImageSizeText[size],
		line,
		tmem,
		tile,
		palette,
		cmt & G_TX_MIRROR ? "G_TX_MIRROR" : "G_TX_NOMIRROR",
		cmt & G_TX_CLAMP ? " | G_TX_CLAMP" : "",
		cms & G_TX_MIRROR ? "G_TX_MIRROR" : "G_TX_NOMIRROR",
		cms & G_TX_CLAMP ? " | G_TX_CLAMP" : "",
		maskt,
		masks,
		shiftt,
		shifts );
#endif
}


void gDPSetTileSize( u32 tile, u32 uls, u32 ult, u32 lrs, u32 lrt )
{
	gDP.tiles[tile].uls = _SHIFTR( uls, 2, 10 );
	gDP.tiles[tile].ult = _SHIFTR( ult, 2, 10 );
	gDP.tiles[tile].lrs = _SHIFTR( lrs, 2, 10 );
	gDP.tiles[tile].lrt = _SHIFTR( lrt, 2, 10 );

	gDP.tiles[tile].fuls = _FIXED2FLOAT( uls, 2 );
	gDP.tiles[tile].fult = _FIXED2FLOAT( ult, 2 );
	gDP.tiles[tile].flrs = _FIXED2FLOAT( lrs, 2 );
	gDP.tiles[tile].flrt = _FIXED2FLOAT( lrt, 2 );

	gDP.changed |= CHANGED_TILE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPSetTileSize( %i, %.2f, %.2f, %.2f, %.2f );\n",
		tile,
		gDP.tiles[tile].fuls,
		gDP.tiles[tile].fult,
		gDP.tiles[tile].flrs,
		gDP.tiles[tile].flrt );
#endif
}

void gDPLoadTile( u32 tile, u32 uls, u32 ult, u32 lrs, u32 lrt )
{
	void (*Interleave)( void *mem, u32 numDWords );

	u32 address, height, bpl, line, y;
	u64 *dest;
	u8 *src;

	gDPSetTileSize( tile, uls, ult, lrs, lrt );
	gDP.loadTile = &gDP.tiles[tile];

	if (gDP.loadTile->line == 0)
		return;

	address = gDP.textureImage.address + gDP.loadTile->ult * gDP.textureImage.bpl + (gDP.loadTile->uls << gDP.textureImage.size >> 1);
	dest = &TMEM[gDP.loadTile->tmem];
	bpl = (gDP.loadTile->lrs - gDP.loadTile->uls + 1) << gDP.loadTile->size >> 1;
	height = gDP.loadTile->lrt - gDP.loadTile->ult + 1;
	src = &RDRAM[address];

	if (((address + height * bpl) > RDRAMSize) ||
		(((gDP.loadTile->tmem << 3) + bpl * height) > 4096)) // Stay within TMEM
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TEXTURE, "// Attempting to load texture tile out of range\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPLoadTile( %i, %i, %i, %i, %i );\n",
			tile, gDP.loadTile->uls, gDP.loadTile->ult, gDP.loadTile->lrs, gDP.loadTile->lrt );
#endif
		return;
	}

	if (OGL.frameBufferTextures)
	{
		FrameBuffer *buffer;
		if (((buffer = FrameBuffer_FindBuffer( address )) != NULL) &&
			((*(u32*)&RDRAM[buffer->startAddress] & 0xFFFEFFFE) == (buffer->startAddress & 0xFFFEFFFE)))
		{
			gDP.loadTile->frameBuffer = buffer;
			gDP.textureMode = TEXTUREMODE_FRAMEBUFFER;
			gDP.loadType = LOADTYPE_TILE;
			gDP.changed |= CHANGED_TMEM;
			return;
		}
	}

	// Line given for 32-bit is half what it seems it should since they split the
	// high and low words. I'm cheating by putting them together.
	if (gDP.loadTile->size == G_IM_SIZ_32b)
	{
		line = gDP.loadTile->line << 1;
		Interleave = QWordInterleave;
	}
	else
	{
		line = gDP.loadTile->line;
		Interleave = DWordInterleave;
	}

	for (y = 0; y < height; y++)
	{
		UnswapCopy( src, dest, bpl );
		if (y & 1) Interleave( dest, line );

		src += gDP.textureImage.bpl;
 		dest += line;
	}

	gDP.textureMode = TEXTUREMODE_NORMAL;
	gDP.loadType = LOADTYPE_TILE;
	gDP.changed |= CHANGED_TMEM;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPLoadTile( %i, %i, %i, %i, %i );\n",
			tile, gDP.loadTile->uls, gDP.loadTile->ult, gDP.loadTile->lrs, gDP.loadTile->lrt );
#endif
}

void gDPLoadBlock( u32 tile, u32 uls, u32 ult, u32 lrs, u32 dxt )
{
	gDPSetTileSize( tile, uls, ult, lrs, dxt );
	gDP.loadTile = &gDP.tiles[tile];

 	u32 bytes = (lrs + 1) << gDP.loadTile->size >> 1;
	u32 address = gDP.textureImage.address + ult * gDP.textureImage.bpl + (uls << gDP.textureImage.size >> 1);

	if ((bytes == 0) || 
		((address + bytes) > RDRAMSize) || 
		(((gDP.loadTile->tmem << 3) + bytes) > 4096))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TEXTURE, "// Attempting to load texture block out of range\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPLoadBlock( %i, %i, %i, %i, %i );\n",
			tile, uls, ult, lrs, dxt );
#endif
//		bytes = min( bytes, min( RDRAMSize - gDP.textureImage.address, 4096 - (gDP.loadTile->tmem << 3) ) );
		return;
	}

	if (OGL.frameBufferTextures)
	{
		FrameBuffer *buffer;
		if (((buffer = FrameBuffer_FindBuffer( address )) != NULL) &&
			((*(u32*)&RDRAM[buffer->startAddress] & 0xFFFEFFFE) == (buffer->startAddress & 0xFFFEFFFE)))
		{
			gDP.loadTile->frameBuffer = buffer;
			gDP.textureMode = TEXTUREMODE_FRAMEBUFFER;
			gDP.loadType = LOADTYPE_BLOCK;
			gDP.changed |= CHANGED_TMEM;
			return;
		}
	}

	u64* src = (u64*)&RDRAM[address];
	u64* dest = &TMEM[gDP.loadTile->tmem];

	if (dxt > 0)
	{
		void (*Interleave)( void *mem, u32 numDWords );

		u32 line = (2047 + dxt) / dxt;
		u32 bpl = line << 3;
		u32 height = bytes / bpl;

		if (gDP.loadTile->size == G_IM_SIZ_32b)
			Interleave = QWordInterleave;
		else
			Interleave = DWordInterleave;

		for (u32 y = 0; y < height; y++)
		{
			UnswapCopy( src, dest, bpl );
			if (y & 1) Interleave( dest, line );

			src += line;
			dest += line;
		}
	}
	else
		UnswapCopy( src, dest, bytes );

	gDP.textureMode = TEXTUREMODE_NORMAL;
	gDP.loadType = LOADTYPE_BLOCK;
	gDP.changed |= CHANGED_TMEM;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPLoadBlock( %i, %i, %i, %i, %i );\n",
		tile, uls, ult, lrs, dxt );
#endif
}

void gDPLoadTLUT( u32 tile, u32 uls, u32 ult, u32 lrs, u32 lrt )
{
	gDPSetTileSize( tile, uls, ult, lrs, lrt );

    u16 count = (gDP.tiles[tile].lrs - gDP.tiles[tile].uls + 1) * (gDP.tiles[tile].lrt - gDP.tiles[tile].ult + 1);
	u32	address = gDP.textureImage.address + gDP.tiles[tile].ult * gDP.textureImage.bpl + (gDP.tiles[tile].uls << gDP.textureImage.size >> 1);

	u16 *dest = (u16*)&TMEM[gDP.tiles[tile].tmem]; 
	u16 *src = (u16*)&RDRAM[address];

	u16 pal = (gDP.tiles[tile].tmem - 256) >> 4;

	int i = 0;
	while (i < count)
	{
		for (u16 j = 0; (j < 16) && (i < count); j++, i++)
		{
			u16 color = swapword( src[i^1] );

			*dest = color;
			//dest[1] = color;
			//dest[2] = color;
			//dest[3] = color;

			dest += 4;
		}
        
		gDP.paletteCRC16[pal] = CRC_CalculatePalette( 0xFFFFFFFF, &TMEM[256 + (pal << 4)], 16 );
		pal++;
	}

	gDP.paletteCRC256 = CRC_Calculate( 0xFFFFFFFF, gDP.paletteCRC16, 64 );

	gDP.changed |= CHANGED_TMEM;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gDPLoadTLUT( %i, %i, %i, %i, %i );\n",
		tile, gDP.tiles[tile].uls, gDP.tiles[tile].ult, gDP.tiles[tile].lrs, gDP.tiles[tile].lrt );
#endif
}

void gDPSetScissor( u32 mode, f32 ulx, f32 uly, f32 lrx, f32 lry )
{
	gDP.scissor.mode = mode;
	gDP.scissor.ulx = ulx;
	gDP.scissor.uly = uly;
	gDP.scissor.lrx = lrx;
	gDP.scissor.lry = lry;

	gDP.changed |= CHANGED_SCISSOR;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gDPSetScissor( %s, %.2f, %.2f, %.2f, %.2f );\n",
		ScissorModeText[gDP.scissor.mode],
		gDP.scissor.ulx,
		gDP.scissor.uly,
		gDP.scissor.lrx,
		gDP.scissor.lry );
#endif
}

void gDPFillRectangle( s32 ulx, s32 uly, s32 lrx, s32 lry )
{
	DepthBuffer *buffer = DepthBuffer_FindBuffer( gDP.colorImage.address );

	if (buffer)
		buffer->cleared = TRUE;

	if (gDP.depthImageAddress == gDP.colorImage.address)
	{
		OGL_ClearDepthBuffer();
		return;
	}

	if (gDP.otherMode.cycleType == G_CYC_FILL)
	{
		//if (gDP.fillColor.a == 0.0f)
		//	return;

		lrx++;
		lry++;

		if ((ulx == 0) && (uly == 0) && (lrx == VI.width) && (lry == VI.height))
		{
			OGL_ClearColorBuffer( &gDP.fillColor.r );
			return;
		}
	}

	OGL_DrawRect( ulx, uly, lrx, lry, (gDP.otherMode.cycleType == G_CYC_FILL) ? &gDP.fillColor.r : &gDP.blendColor.r );

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = max( gDP.colorImage.height, lry );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPFillRectangle( %i, %i, %i, %i );\n",
		ulx, uly, lrx, lry );
#endif
}

void gDPSetConvert( s32 k0, s32 k1, s32 k2, s32 k3, s32 k4, s32 k5 )
{
	gDP.convert.k0 = k0;
	gDP.convert.k1 = k1;
	gDP.convert.k2 = k2;
	gDP.convert.k3 = k3;
	gDP.convert.k4 = k4;
	gDP.convert.k5 = k5;
}

void gDPSetKeyR( u32 cR, u32 sR, u32 wR )
{
	gDP.key.center.r = cR * 0.0039215689f;;
	gDP.key.scale.r = sR * 0.0039215689f;;
	gDP.key.width.r = wR * 0.0039215689f;;
}

void gDPSetKeyGB(u32 cG, u32 sG, u32 wG, u32 cB, u32 sB, u32 wB )
{
	gDP.key.center.g = cG * 0.0039215689f;;
	gDP.key.scale.g = sG * 0.0039215689f;;
	gDP.key.width.g = wG * 0.0039215689f;;
	gDP.key.center.b = cB * 0.0039215689f;;
	gDP.key.scale.b = sB * 0.0039215689f;;
	gDP.key.width.b = wB * 0.0039215689f;;
}

void gDPTextureRectangle( f32 ulx, f32 uly, f32 lrx, f32 lry, s32 tile, f32 s, f32 t, f32 dsdx, f32 dtdy )
{
 	if (gDP.otherMode.cycleType == G_CYC_COPY)
	{
		dsdx = 1.0f;
		lrx += 1.0f;
		lry += 1.0f;
	}

	gSP.textureTile[0] = &gDP.tiles[tile];
	gSP.textureTile[1] = &gDP.tiles[tile < 7 ? tile + 1 : tile];

	f32 lrs = s + (lrx - ulx - 1) * dsdx;
	f32 lrt = t + (lry - uly - 1) * dtdy;

	if (gDP.textureMode == TEXTUREMODE_NORMAL)
		gDP.textureMode = TEXTUREMODE_TEXRECT;

	gDP.texRect.width = max( lrs, s ) + dsdx;
	gDP.texRect.height = max( lrt, t ) + dtdy;

	if (lrs > s)
	{
		if (lrt > t)
			OGL_DrawTexturedRect( ulx, uly, lrx, lry, s, t, lrs, lrt, (RSP.cmd == G_TEXRECTFLIP) );
		else
			OGL_DrawTexturedRect( ulx, lry, lrx, uly, s, lrt, lrs, t, (RSP.cmd == G_TEXRECTFLIP) );
	}
	else
	{
		if (lrt > t)
			OGL_DrawTexturedRect( lrx, uly, ulx, lry, lrs, t, s, lrt, (RSP.cmd == G_TEXRECTFLIP) );
		else
			OGL_DrawTexturedRect( lrx, lry, ulx, uly, lrs, lrt, s, t, (RSP.cmd == G_TEXRECTFLIP) );
	}

	gSP.textureTile[0] = &gDP.tiles[gSP.texture.tile];
	gSP.textureTile[1] = &gDP.tiles[gSP.texture.tile < 7 ? gSP.texture.tile + 1 : gSP.texture.tile];

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = max( gDP.colorImage.height, gDP.scissor.lry );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPTextureRectangle( %f, %f, %f, %f, %i, %i, %f, %f, %f, %f );\n",
		ulx, uly, lrx, lry, tile, s, t, dsdx, dtdy );
#endif
}

void gDPTextureRectangleFlip( f32 ulx, f32 uly, f32 lrx, f32 lry, s32 tile, f32 s, f32 t, f32 dsdx, f32 dtdy )
{
	gDPTextureRectangle( ulx, uly, lrx, lry, tile, s + (lrx - ulx) * dsdx, t + (lry - uly) * dtdy, -dsdx, -dtdy );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPTextureRectangleFlip( %f, %f, %f, %f, %i, %i, %f, %f, %f, %f );\n",
		ulx, uly, lrx, lry, tile, s, t, dsdx, dtdy );
#endif
}

void gDPFullSync()
{
	*REG.MI_INTR |= MI_INTR_DP;

	CheckInterrupts();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gDPFullSync();\n" );
#endif
}

void gDPTileSync()
{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED | DEBUG_TEXTURE, "gDPTileSync();\n" );
#endif
}

void gDPPipeSync()
{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gDPPipeSync();\n" );
#endif
}

void gDPLoadSync()
{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gDPLoadSync();\n" );
#endif
}

void gDPNoOp()
{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gDPNoOp();\n" );
#endif
}

