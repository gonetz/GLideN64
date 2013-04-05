#include <math.h>
#include "glN64.h"
#include "Debug.h"
#include "Types.h"
#include "RSP.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "3DMath.h"
#include "OpenGL.h"
#include "CRC.h"
#include <string.h>
#include "convert.h"
#include "S2DEX.h"
#include "VI.h"
#include "DepthBuffer.h"
#ifndef __LINUX__
# include "Resource.h"
#endif // !__LINUX__

#ifdef DEBUG
extern u32 uc_crc, uc_dcrc;
extern char uc_str[256];
#endif

#define gSPFlushTriangles() \
	if ((OGL.numTriangles > 0) && \
		(RSP.nextCmd != G_TRI1) && \
		(RSP.nextCmd != G_TRI2) && \
		(RSP.nextCmd != G_TRI4) && \
		(RSP.nextCmd != G_QUAD) && \
		(RSP.nextCmd != G_DMA_TRI)) \
		OGL_DrawTriangles()

gSPInfo gSP;

f32 identityMatrix[4][4] =
{
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};


void gSPLoadUcodeEx( u32 uc_start, u32 uc_dstart, u16 uc_dsize )
{
	RSP.PCi = 0;
	gSP.matrix.modelViewi = 0;
	gSP.changed |= CHANGED_MATRIX;
	gSP.status[0] = gSP.status[1] = gSP.status[2] = gSP.status[3] = 0;

	if ((((uc_start & 0x1FFFFFFF) + 4096) > RDRAMSize) || (((uc_dstart & 0x1FFFFFFF) + uc_dsize) > RDRAMSize))
	{
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to loud ucode out of invalid address\n" );
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLoadUcodeEx( 0x%08X, 0x%08X, %i );\n", uc_start, uc_dstart, uc_dsize );
#endif
			return;
	}

	MicrocodeInfo *ucode = GBI_DetectMicrocode( uc_start, uc_dstart, uc_dsize );

	if (ucode->type != NONE)
		GBI_MakeCurrent( ucode );
	else
#ifdef RSPTHREAD
		SetEvent( RSP.threadMsg[RSPMSG_CLOSE] );
#else
		puts( "Warning: Unknown UCODE!!!" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Unknown microcode: 0x%08X, 0x%08X, %s\n", uc_crc, uc_dcrc, uc_str );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLoadUcodeEx( 0x%08X, 0x%08X, %i );\n", uc_start, uc_dstart, uc_dsize );
#endif
}

void gSPCombineMatrices()
{
	CopyMatrix( gSP.matrix.combined, gSP.matrix.projection );
	MultMatrix( gSP.matrix.combined, gSP.matrix.modelView[gSP.matrix.modelViewi] );

	gSP.changed &= ~CHANGED_MATRIX;
}

void gSPProcessVertex( u32 v )
{
	f32 intensity;
	f32 r, g, b;

	if (gSP.changed & CHANGED_MATRIX)
		gSPCombineMatrices();

	TransformVertex( &gSP.vertices[v].x, gSP.matrix.combined );

	if (gSP.matrix.billboard)
	{
		gSP.vertices[v].x += gSP.vertices[0].x;
		gSP.vertices[v].y += gSP.vertices[0].y;
		gSP.vertices[v].z += gSP.vertices[0].z;
		gSP.vertices[v].w += gSP.vertices[0].w;
	}

	if (!(gSP.geometryMode & G_ZBUFFER))
	{
		gSP.vertices[v].z = -gSP.vertices[v].w;
	}

	if (gSP.geometryMode & G_LIGHTING)
	{
		TransformVector( &gSP.vertices[v].nx, gSP.matrix.modelView[gSP.matrix.modelViewi] );
		Normalize( &gSP.vertices[v].nx );

		r = gSP.lights[gSP.numLights].r;
		g = gSP.lights[gSP.numLights].g;
		b = gSP.lights[gSP.numLights].b;

		for (int i = 0; i < gSP.numLights; i++)
		{
			intensity = DotProduct( &gSP.vertices[v].nx, &gSP.lights[i].x );

			if (intensity < 0.0f) intensity = 0.0f;

			r += gSP.lights[i].r * intensity;
			g += gSP.lights[i].g * intensity;
			b += gSP.lights[i].b * intensity;
		}

		gSP.vertices[v].r = r;
		gSP.vertices[v].g = g;
		gSP.vertices[v].b = b;

		if (gSP.geometryMode & G_TEXTURE_GEN)
		{
			TransformVector( &gSP.vertices[v].nx, gSP.matrix.projection );

			Normalize( &gSP.vertices[v].nx );

			if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
			{   
				gSP.vertices[v].s = acosf(gSP.vertices[v].nx) * 325.94931f;
				gSP.vertices[v].t = acosf(gSP.vertices[v].ny) * 325.94931f;
			}
			else // G_TEXTURE_GEN
			{
				gSP.vertices[v].s = (gSP.vertices[v].nx + 1.0f) * 512.0f;
				gSP.vertices[v].t = (gSP.vertices[v].ny + 1.0f) * 512.0f;
			}
		}
	}

	if (gSP.vertices[v].x < -gSP.vertices[v].w)
		gSP.vertices[v].xClip = -1.0f;
	else if (gSP.vertices[v].x > gSP.vertices[v].w)
		gSP.vertices[v].xClip = 1.0f;
	else
		gSP.vertices[v].xClip = 0.0f;

	if (gSP.vertices[v].y < -gSP.vertices[v].w)
		gSP.vertices[v].yClip = -1.0f;
	else if (gSP.vertices[v].y > gSP.vertices[v].w)
		gSP.vertices[v].yClip = 1.0f;
	else
		gSP.vertices[v].yClip = 0.0f;

	if (gSP.vertices[v].w <= 0.0f)
		gSP.vertices[v].zClip = -1.0f;
	else if (gSP.vertices[v].z < -gSP.vertices[v].w)
		gSP.vertices[v].zClip = -0.1f;
	else if (gSP.vertices[v].z > gSP.vertices[v].w)
		gSP.vertices[v].zClip = 1.0f;
	else
		gSP.vertices[v].zClip = 0.0f;
}

void gSPNoOp()
{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gSPNoOp();\n" );
#endif
}

void gSPMatrix( u32 matrix, u8 param )
{
	f32 mtx[4][4];
	u32 address = RSP_SegmentToPhysical( matrix );

	if (address + 64 > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPMatrix( 0x%08X, %s | %s | %s );\n",
			matrix,
			(param & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
			(param & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
			(param & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH" );
#endif
		return;
	}

	RSP_LoadMatrix( mtx, address );

	if (param & G_MTX_PROJECTION)
	{
		if (param & G_MTX_LOAD)
			CopyMatrix( gSP.matrix.projection, mtx );
		else
			MultMatrix( gSP.matrix.projection, mtx );
	}
	else
	{
		if ((param & G_MTX_PUSH) && (gSP.matrix.modelViewi < (gSP.matrix.stackSize - 1)))
		{
			CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi + 1], gSP.matrix.modelView[gSP.matrix.modelViewi] );
			gSP.matrix.modelViewi++;
		}
#ifdef DEBUG
		else
			DebugMsg( DEBUG_ERROR | DEBUG_MATRIX, "// Modelview stack overflow\n" );
#endif

		if (param & G_MTX_LOAD)
			CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
		else
			MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
	}

	gSP.changed |= CHANGED_MATRIX;

#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPMatrix( 0x%08X, %s | %s | %s );\n",
		matrix,
		(param & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
		(param & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
		(param & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH" );
#endif
}

void gSPDMAMatrix( u32 matrix, u8 index, u8 multiply )
{
	f32 mtx[4][4];
	u32 address = gSP.DMAOffsets.mtx + RSP_SegmentToPhysical( matrix );

	if (address + 64 > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
			matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
		return;
	}

	RSP_LoadMatrix( mtx, address );

	gSP.matrix.modelViewi = index;

	if (multiply)
	{
		CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.modelView[0] );
		MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
	}
	else
		CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );

	CopyMatrix( gSP.matrix.projection, identityMatrix );


	gSP.changed |= CHANGED_MATRIX;
#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
		matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
}

void gSPViewport( u32 v )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + 16) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load viewport from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPViewport( 0x%08X );\n", v );
#endif
		return;
	}

	gSP.viewport.vscale[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  2], 2 );
	gSP.viewport.vscale[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address     ], 2 );
	gSP.viewport.vscale[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  6], 10 );// * 0.00097847357f;
	gSP.viewport.vscale[3] = *(s16*)&RDRAM[address +  4];
	gSP.viewport.vtrans[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 10], 2 );
	gSP.viewport.vtrans[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  8], 2 );
	gSP.viewport.vtrans[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 14], 10 );// * 0.00097847357f;
	gSP.viewport.vtrans[3] = *(s16*)&RDRAM[address + 12];

	gSP.viewport.x		= gSP.viewport.vtrans[0] - gSP.viewport.vscale[0];
	gSP.viewport.y		= gSP.viewport.vtrans[1] - gSP.viewport.vscale[1];
	gSP.viewport.width	= gSP.viewport.vscale[0] * 2;
	gSP.viewport.height	= gSP.viewport.vscale[1] * 2;
	gSP.viewport.nearz	= gSP.viewport.vtrans[2] - gSP.viewport.vscale[2];
	gSP.viewport.farz	= (gSP.viewport.vtrans[2] + gSP.viewport.vscale[2]) ;

	gSP.changed |= CHANGED_VIEWPORT;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPViewport( 0x%08X );\n", v );
#endif
}

void gSPForceMatrix( u32 mptr )
{
	u32 address = RSP_SegmentToPhysical( mptr );

	if (address + 64 > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load from invalid address" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPForceMatrix( 0x%08X );\n", mptr );
#endif
		return;
	}

	RSP_LoadMatrix( gSP.matrix.combined, RSP_SegmentToPhysical( mptr ) );

	gSP.changed &= ~CHANGED_MATRIX;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPForceMatrix( 0x%08X );\n", mptr );
#endif
}

void gSPLight( u32 l, s32 n )
{
	n--;
	u32 address = RSP_SegmentToPhysical( l );

	if ((address + sizeof( Light )) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load light from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
			l, n );
#endif
		return;
	}

	Light *light = (Light*)&RDRAM[address];

	if (n < 8)
	{
		gSP.lights[n].r = light->r * 0.0039215689f;
		gSP.lights[n].g = light->g * 0.0039215689f;
		gSP.lights[n].b = light->b * 0.0039215689f;

		gSP.lights[n].x = light->x;
		gSP.lights[n].y = light->y;
		gSP.lights[n].z = light->z;

		Normalize( &gSP.lights[n].x );
	}

#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// x = %2.6f    y = %2.6f    z = %2.6f\n",
		_FIXED2FLOAT( light->x, 7 ), _FIXED2FLOAT( light->y, 7 ), _FIXED2FLOAT( light->z, 7 ) );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// r = %3i    g = %3i   b = %3i\n",
		light->r, light->g, light->b );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
		l, n );
#endif
}

void gSPLookAt( u32 l )
{
}

void gSPVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + sizeof( Vertex ) * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPVertex( 0x%08X, %i, %i );\n",
			v, n, v0 );
#endif
		return;
	}

	Vertex *vertex = (Vertex*)&RDRAM[address];

	if ((n + v0) < (80))
	{
		for (int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = vertex->x;
			gSP.vertices[i].y = vertex->y;
			gSP.vertices[i].z = vertex->z;
			gSP.vertices[i].flag = vertex->flag;
			gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
			gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = vertex->normal.x;
				gSP.vertices[i].ny = vertex->normal.y;
				gSP.vertices[i].nz = vertex->normal.z;
				gSP.vertices[i].a = vertex->color.a * 0.0039215689f;
			}
			else
			{
				gSP.vertices[i].r = vertex->color.r * 0.0039215689f;
				gSP.vertices[i].g = vertex->color.g * 0.0039215689f;
				gSP.vertices[i].b = vertex->color.b * 0.0039215689f;
				gSP.vertices[i].a = vertex->color.a * 0.0039215689f;
			}

#ifdef DEBUG
			DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// x = %6i    y = %6i    z = %6i \n",
				vertex->x, vertex->y, vertex->z );
			DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// s = %5.5f    t = %5.5f    flag = %i \n",
				vertex->s, vertex->t, vertex->flag );

			if (gSP.geometryMode & G_LIGHTING)
			{
				DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// nx = %2.6f    ny = %2.6f    nz = %2.6f\n",
					_FIXED2FLOAT( vertex->normal.x, 7 ), _FIXED2FLOAT( vertex->normal.y, 7 ), _FIXED2FLOAT( vertex->normal.z, 7 ) );
			}
			else
			{
				DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// r = %3u    g = %3u    b = %3u    a = %3u\n",
					vertex->color.r, vertex->color.g, vertex->color.b, vertex->color.a );
			}
#endif

			gSPProcessVertex( i );

			vertex++;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPVertex( 0x%08X, %i, %i );\n",
		v, n, v0 );
#endif
}

void gSPCIVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + sizeof( PDVertex ) * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPCIVertex( 0x%08X, %i, %i );\n",
			v, n, v0 );
#endif
		return;
	}

	PDVertex *vertex = (PDVertex*)&RDRAM[address];

	if ((n + v0) < (80))
	{
		for (int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = vertex->x;
			gSP.vertices[i].y = vertex->y;
			gSP.vertices[i].z = vertex->z;
			gSP.vertices[i].flag = 0;
			gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
			gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );

			u8 *color = &RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = (s8)color[3];
				gSP.vertices[i].ny = (s8)color[2];
				gSP.vertices[i].nz = (s8)color[1];
				gSP.vertices[i].a = color[0] * 0.0039215689f;
			}
			else
			{
				gSP.vertices[i].r = color[3] * 0.0039215689f;
				gSP.vertices[i].g = color[2] * 0.0039215689f;
				gSP.vertices[i].b = color[1] * 0.0039215689f;
				gSP.vertices[i].a = color[0] * 0.0039215689f;
			}

			gSPProcessVertex( i );

			vertex++;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPCIVertex( 0x%08X, %i, %i );\n",
		v, n, v0 );
#endif
}

void gSPDMAVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = gSP.DMAOffsets.vtx + RSP_SegmentToPhysical( v );

	if ((address + 10 * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPDMAVertex( 0x%08X, %i, %i );\n",
			v, n, v0 );
#endif
		return;
	}

	if ((n + v0) < (80))
	{
		for (int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = *(s16*)&RDRAM[address ^ 2];
			gSP.vertices[i].y = *(s16*)&RDRAM[(address + 2) ^ 2];
			gSP.vertices[i].z = *(s16*)&RDRAM[(address + 4) ^ 2];

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = *(s8*)&RDRAM[(address + 6) ^ 3];
				gSP.vertices[i].ny = *(s8*)&RDRAM[(address + 7) ^ 3];
				gSP.vertices[i].nz = *(s8*)&RDRAM[(address + 8) ^ 3];
				gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
			}
			else
			{
				gSP.vertices[i].r = *(u8*)&RDRAM[(address + 6) ^ 3] * 0.0039215689f;
				gSP.vertices[i].g = *(u8*)&RDRAM[(address + 7) ^ 3] * 0.0039215689f;
				gSP.vertices[i].b = *(u8*)&RDRAM[(address + 8) ^ 3] * 0.0039215689f;
				gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
			}

			gSPProcessVertex( i );

			address += 10;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPDMAVertex( 0x%08X, %i, %i );\n",
		v, n, v0 );
#endif
}

void gSPDisplayList( u32 dl )
{
	u32 address = RSP_SegmentToPhysical( dl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
			dl );
#endif
		return;
	}

	if (RSP.PCi < (GBI.PCStackSize - 1))
	{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "\n" );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
		dl );
#endif
		RSP.PCi++;
		RSP.PC[RSP.PCi] = address;
	}
#ifdef DEBUG
	else
	{
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// PC stack overflow\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
			dl );
	}
#endif
}

void gSPDMADisplayList( u32 dl, u32 n )
{
	if ((dl + (n << 3)) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
			dl, n );
#endif
		return;
	}

	u32 curDL = RSP.PC[RSP.PCi];

	RSP.PC[RSP.PCi] = RSP_SegmentToPhysical( dl );

	while ((RSP.PC[RSP.PCi] - dl) < (n << 3))
	{
		if ((RSP.PC[RSP.PCi] + 8) > RDRAMSize)
		{
#ifdef DEBUG
			switch (Debug.level)
			{
				case DEBUG_LOW:
                    DebugMsg( DEBUG_LOW | DEBUG_ERROR, "ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n" );
					break;
				case DEBUG_MEDIUM:
                    DebugMsg( DEBUG_MEDIUM | DEBUG_ERROR, "Attempting to execute RSP command at invalid RDRAM location\n" );
					break;
				case DEBUG_HIGH:
                    DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to execute RSP command at invalid RDRAM location\n" );
					break;
			}
#endif
			break;
		}

		u32 w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
		u32 w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];

#ifdef DEBUG
		DebugRSPState( RSP.PCi, RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
		DebugMsg( DEBUG_LOW | DEBUG_HANDLED, "0x%08lX: CMD=0x%02lX W0=0x%08lX W1=0x%08lX\n", RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
#endif

		RSP.PC[RSP.PCi] += 8;
		RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[RSP.PC[RSP.PCi]], 24, 8 );

		GBI.cmd[_SHIFTR( w0, 24, 8 )]( w0, w1 );
	}

	RSP.PC[RSP.PCi] = curDL;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
		dl, n );
#endif
}

void gSPBranchList( u32 dl )
{
	u32 address = RSP_SegmentToPhysical( dl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to branch to display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchList( 0x%08X );\n",
			dl );
#endif
		return;
	}

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchList( 0x%08X );\n",
		dl );
#endif

	RSP.PC[RSP.PCi] = address;
}

void gSPBranchLessZ( u32 branchdl, u32 vtx, f32 zval )
{
	u32 address = RSP_SegmentToPhysical( branchdl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Specified display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
			branchdl, vtx, zval );
#endif
		return;
	}

	if (gSP.vertices[vtx].z <= zval)
		RSP.PC[RSP.PCi] = address;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
			branchdl, vtx, zval );
#endif
}

void gSPSetDMAOffsets( u32 mtxoffset, u32 vtxoffset )
{
	gSP.DMAOffsets.mtx = mtxoffset;
	gSP.DMAOffsets.vtx = vtxoffset;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetDMAOffsets( 0x%08X, 0x%08X );\n",
			mtxoffset, vtxoffset );
#endif
}

void gSPSetVertexColorBase( u32 base )
{
 	gSP.vertexColorBase = RSP_SegmentToPhysical( base );

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetVertexColorBase( 0x%08X );\n",
			base );
#endif
}

void gSPSprite2DBase( u32 base )
{
}

void gSPCopyVertex( SPVertex *dest, SPVertex *src )
{
	dest->x = src->x;
	dest->y = src->y;
	dest->z = src->z;
	dest->w = src->w;
	dest->r = src->r;
	dest->g = src->g;
	dest->b = src->b;
	dest->a = src->a;
	dest->s = src->s;
	dest->t = src->t;
}

void gSPInterpolateVertex( SPVertex *dest, f32 percent, SPVertex *first, SPVertex *second )
{
	dest->x = first->x + percent * (second->x - first->x);
	dest->y = first->y + percent * (second->y - first->y);
	dest->z = first->z + percent * (second->z - first->z);
	dest->w = first->w + percent * (second->w - first->w);
	dest->r = first->r + percent * (second->r - first->r);
	dest->g = first->g + percent * (second->g - first->g);
	dest->b = first->b + percent * (second->b - first->b);
	dest->a = first->a + percent * (second->a - first->a);
	dest->s = first->s + percent * (second->s - first->s);
	dest->t = first->t + percent * (second->t - first->t);
}

void gSPTriangle( s32 v0, s32 v1, s32 v2, s32 flag )
{
	if ((v0 < 80) && (v1 < 80) && (v2 < 80))
	{
		// Don't bother with triangles completely outside clipping frustrum
		if (((gSP.vertices[v0].xClip < 0.0f) &&
			 (gSP.vertices[v1].xClip < 0.0f) &&
			 (gSP.vertices[v2].xClip < 0.0f)) ||
		    ((gSP.vertices[v0].xClip > 0.0f) &&
			 (gSP.vertices[v1].xClip > 0.0f) &&
			 (gSP.vertices[v2].xClip > 0.0f)) ||
		    ((gSP.vertices[v0].yClip < 0.0f) &&
			 (gSP.vertices[v1].yClip < 0.0f) &&
			 (gSP.vertices[v2].yClip < 0.0f)) ||
		    ((gSP.vertices[v0].yClip > 0.0f) &&
			 (gSP.vertices[v1].yClip > 0.0f) &&
			 (gSP.vertices[v2].yClip > 0.0f)) ||
			((gSP.vertices[v0].zClip > 0.1f) &&
			 (gSP.vertices[v1].zClip > 0.1f) &&
			 (gSP.vertices[v2].zClip > 0.1f)) ||
			((gSP.vertices[v0].zClip < -0.1f) &&
			 (gSP.vertices[v1].zClip < -0.1f) &&
			 (gSP.vertices[v2].zClip < -0.1f)))
			 return;

		// NoN work-around, clips triangles, and draws the clipped-off parts with clamped z
		if (GBI.current->NoN &&
			((gSP.vertices[v0].zClip < 0.0f) ||
			(gSP.vertices[v1].zClip < 0.0f) ||
			(gSP.vertices[v2].zClip < 0.0f)))
		{
			SPVertex nearVertices[4];
			SPVertex clippedVertices[4];
			s32 numNearTris = 0;
			s32 numClippedTris = 0;
			s32 nearIndex = 0;
			s32 clippedIndex = 0;

			s32 v[3] = { v0, v1, v2 };

			for (s32 i = 0; i < 3; i++)
			{
				s32 j = i + 1;
				if (j == 3) j = 0;

				if (((gSP.vertices[v[i]].zClip < 0.0f) && (gSP.vertices[v[j]].zClip >= 0.0f)) ||
					((gSP.vertices[v[i]].zClip >= 0.0f) && (gSP.vertices[v[j]].zClip < 0.0f)))
				{
					f32 percent = (-gSP.vertices[v[i]].w - gSP.vertices[v[i]].z) / ((gSP.vertices[v[j]].z - gSP.vertices[v[i]].z) + (gSP.vertices[v[j]].w - gSP.vertices[v[i]].w));

					gSPInterpolateVertex( &clippedVertices[clippedIndex], percent, &gSP.vertices[v[i]], &gSP.vertices[v[j]] );

					gSPCopyVertex( &nearVertices[nearIndex], &clippedVertices[clippedIndex] );
					nearVertices[nearIndex].z = -nearVertices[nearIndex].w;

					clippedIndex++;
					nearIndex++;
				}

				if (((gSP.vertices[v[i]].zClip < 0.0f) && (gSP.vertices[v[j]].zClip >= 0.0f)) ||
					((gSP.vertices[v[i]].zClip >= 0.0f) && (gSP.vertices[v[j]].zClip >= 0.0f)))
				{
					gSPCopyVertex( &clippedVertices[clippedIndex], &gSP.vertices[v[j]] );
					clippedIndex++;
				}
				else
				{
					gSPCopyVertex( &nearVertices[nearIndex], &gSP.vertices[v[j]] );
					nearVertices[nearIndex].z = -nearVertices[nearIndex].w;// + 0.00001f;
					nearIndex++;
				}
			}

			OGL_AddTriangle( clippedVertices, 0, 1, 2 );

			if (clippedIndex == 4)
				OGL_AddTriangle( clippedVertices, 0, 2, 3 );

			glDisable( GL_POLYGON_OFFSET_FILL );

//			glDepthFunc( GL_LEQUAL );

			OGL_AddTriangle( nearVertices, 0, 1, 2 );
			if (nearIndex == 4)
				OGL_AddTriangle( nearVertices, 0, 2, 3 );

			if (gDP.otherMode.depthMode == ZMODE_DEC)
				glEnable( GL_POLYGON_OFFSET_FILL );

//			if (gDP.otherMode.depthCompare)
//				glDepthFunc( GL_LEQUAL );
		}
		else
			OGL_AddTriangle( gSP.vertices, v0, v1, v2 );
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Vertex index out of range\n" );
#endif

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = max( gDP.colorImage.height, gDP.scissor.lry );
}

void gSP1Triangle( s32 v0, s32 v1, s32 v2, s32 flag )
{
	gSPTriangle( v0, v1, v2, flag );

	gSPFlushTriangles();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Triangle( %i, %i, %i, %i );\n",
		v0, v1, v2, flag );
#endif
}

void gSP2Triangles( s32 v00, s32 v01, s32 v02, s32 flag0, 
				    s32 v10, s32 v11, s32 v12, s32 flag1 )
{
	gSPTriangle( v00, v01, v02, flag0 );
	gSPTriangle( v10, v11, v12, flag1 );

	gSPFlushTriangles();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP2Triangles( %i, %i, %i, %i,\n",
		v00, v01, v02, flag0 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i, %i );\n",
		v10, v11, v12, flag1 );
#endif
}

void gSP4Triangles( s32 v00, s32 v01, s32 v02,
				    s32 v10, s32 v11, s32 v12,
					s32 v20, s32 v21, s32 v22,
					s32 v30, s32 v31, s32 v32 )
{
	gSPTriangle( v00, v01, v02, 0 );
	gSPTriangle( v10, v11, v12, 0 );
	gSPTriangle( v20, v21, v22, 0 );
	gSPTriangle( v30, v31, v32, 0 );

	gSPFlushTriangles();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP4Triangles( %i, %i, %i,\n",
		v00, v01, v02 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i,\n",
		v10, v11, v12 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i,\n",
		v20, v21, v22 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i );\n",
		v30, v31, v32 );
#endif
}

void gSPDMATriangles( u32 tris, u32 n )
{
	u32 address = RSP_SegmentToPhysical( tris );

	if (address + sizeof( DKRTriangle ) * n > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Attempting to load triangles from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n" );
#endif
		return;
	}

	DKRTriangle *triangles = (DKRTriangle*)&RDRAM[address];

	for (u32 i = 0; i < n; i++)
	{
		gSP.geometryMode &= ~G_CULL_BOTH;

		if (!(triangles->flag & 0x40))
		{
			if (gSP.viewport.vscale[0] > 0)
				gSP.geometryMode |= G_CULL_BACK;
			else
				gSP.geometryMode |= G_CULL_FRONT;
		}
		gSP.changed |= CHANGED_GEOMETRYMODE;
		
		gSP.vertices[triangles->v0].s = _FIXED2FLOAT( triangles->s0, 5 );
		gSP.vertices[triangles->v0].t = _FIXED2FLOAT( triangles->t0, 5 );

		gSP.vertices[triangles->v1].s = _FIXED2FLOAT( triangles->s1, 5 );
		gSP.vertices[triangles->v1].t = _FIXED2FLOAT( triangles->t1, 5 );

		gSP.vertices[triangles->v2].s = _FIXED2FLOAT( triangles->s2, 5 );
		gSP.vertices[triangles->v2].t = _FIXED2FLOAT( triangles->t2, 5 );

		gSPTriangle( triangles->v0, triangles->v1, triangles->v2, 0 );

		triangles++;
	}

	gSPFlushTriangles();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n",
			tris, n );
#endif
}

void gSP1Quadrangle( s32 v0, s32 v1, s32 v2, s32 v3 )
{
	gSPTriangle( v0, v1, v2, 0 );
	gSPTriangle( v0, v2, v3, 0 );

	gSPFlushTriangles();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Quadrangle( %i, %i, %i, %i );\n",
			v0, v1, v2, v3 );
#endif
}

bool gSPCullVertices( u32 v0, u32 vn )
{
	float xClip, yClip, zClip;

	xClip = yClip = zClip = 0.0f;

	for (int i = v0; i <= vn; i++)
	{
		if (gSP.vertices[i].xClip == 0.0f)
			return FALSE;
		else if (gSP.vertices[i].xClip < 0.0f)
		{
			if (xClip > 0.0f)
				return FALSE;
			else 
				xClip = gSP.vertices[i].xClip;
		}
		else if (gSP.vertices[i].xClip > 0.0f)
		{
			if (xClip < 0.0f)
				return FALSE;
			else 
				xClip = gSP.vertices[i].xClip;
		}

		if (gSP.vertices[i].yClip == 0.0f)
			return FALSE;
		else if (gSP.vertices[i].yClip < 0.0f)
		{
			if (yClip > 0.0f)
				return FALSE;
			else 
				yClip = gSP.vertices[i].yClip;
		}
		else if (gSP.vertices[i].yClip > 0.0f)
		{
			if (yClip < 0.0f)
				return FALSE;
			else 
				yClip = gSP.vertices[i].yClip;
		}

		if (gSP.vertices[i].zClip == 0.0f)
			return FALSE;
		else if (gSP.vertices[i].zClip < 0.0f)
		{
			if (zClip > 0.0f)
				return FALSE;
			else 
				zClip = gSP.vertices[i].zClip;
		}
		else if (gSP.vertices[i].zClip > 0.0f)
		{
			if (zClip < 0.0f)
				return FALSE;
			else 
				zClip = gSP.vertices[i].zClip;
		}
	}

	return TRUE;
}

void gSPCullDisplayList( u32 v0, u32 vn )
{
	if (gSPCullVertices( v0, vn ))
	{
		if (RSP.PCi > 0)
			RSP.PCi--;
		else
		{
#ifdef DEBUG
			DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// End of display list, halting execution\n" );
#endif
			RSP.halt = TRUE;
		}
#ifdef DEBUG
		DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// Culling display list\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPCullDisplayList( %i, %i );\n\n",
			v0, vn );
#endif
	}
#ifdef DEBUG
	else
	{
		DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// Not culling display list\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPCullDisplayList( %i, %i );\n",
			v0, vn );
	}
#endif
}

void gSPPopMatrixN( u32 param, u32 num )
{
	if (gSP.matrix.modelViewi > num - 1)
	{
		gSP.matrix.modelViewi -= num;

		gSP.changed |= CHANGED_MATRIX;
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );

	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrixN( %s, %i );\n",
		(param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" : 
	    (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID",
		num );
#endif
}

void gSPPopMatrix( u32 param )
{
	if (gSP.matrix.modelViewi > 0)
	{
		gSP.matrix.modelViewi--;

		gSP.changed |= CHANGED_MATRIX;
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );

	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrix( %s );\n",
		(param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" : 
	    (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID" );
#endif
}

void gSPSegment( s32 seg, s32 base )
{
	if (seg > 0xF)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load address into invalid segment\n",
			SegmentText[seg], base );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
			SegmentText[seg], base );
#endif
		return;
	}

	if (base > RDRAMSize - 1)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load invalid address into segment array\n",
			SegmentText[seg], base );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
			SegmentText[seg], base );
#endif
		return;
	}

	gSP.segment[seg] = base;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
		SegmentText[seg], base );
#endif
}

void gSPClipRatio( u32 r )
{
}

void gSPInsertMatrix( u32 where, u32 num )
{
	f32 fraction, integer;

	if (gSP.changed & CHANGED_MATRIX)
		gSPCombineMatrices();

	if ((where & 0x3) || (where > 0x3C))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Invalid matrix elements\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPInsertMatrix( 0x%02X, %i );\n",
			where, num );
#endif
		return;
	}

	if (where < 0x20)
	{
		fraction = modff( gSP.matrix.combined[0][where >> 1], &integer );
		gSP.matrix.combined[0][where >> 1] = (s16)_SHIFTR( num, 16, 16 ) + abs( fraction );

		fraction = modff( gSP.matrix.combined[0][(where >> 1) + 1], &integer );
		gSP.matrix.combined[0][(where >> 1) + 1] = (s16)_SHIFTR( num, 0, 16 ) + abs( fraction );
	}
	else
	{
		f32 newValue;

		fraction = modff( gSP.matrix.combined[0][(where - 0x20) >> 1], &integer );
		newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 16, 16 ), 16);

		// Make sure the sign isn't lost
		if ((integer == 0.0f) && (fraction != 0.0f))
			newValue = newValue * (fraction / abs( fraction ));

		gSP.matrix.combined[0][(where - 0x20) >> 1] = newValue;

		fraction = modff( gSP.matrix.combined[0][((where - 0x20) >> 1) + 1], &integer );
		newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 0, 16 ), 16 );

		// Make sure the sign isn't lost
		if ((integer == 0.0f) && (fraction != 0.0f))
			newValue = newValue * (fraction / abs( fraction ));

		gSP.matrix.combined[0][((where - 0x20) >> 1) + 1] = newValue;
	}

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPInsertMatrix( %s, %i );\n",
		MWOMatrixText[where >> 2], num );
#endif
}

void gSPModifyVertex( u32 vtx, u32 where, u32 val )
{
	switch (where)
	{
		case G_MWO_POINT_RGBA:
			gSP.vertices[vtx].r = _SHIFTR( val, 24, 8 ) * 0.0039215689f;
			gSP.vertices[vtx].g = _SHIFTR( val, 16, 8 ) * 0.0039215689f;
			gSP.vertices[vtx].b = _SHIFTR( val, 8, 8 ) * 0.0039215689f;
			gSP.vertices[vtx].a = _SHIFTR( val, 0, 8 ) * 0.0039215689f;
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
		case G_MWO_POINT_ST:
			gSP.vertices[vtx].s = _FIXED2FLOAT( (s16)_SHIFTR( val, 16, 16 ), 5 );
			gSP.vertices[vtx].t = _FIXED2FLOAT( (s16)_SHIFTR( val, 0, 16 ), 5 );
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
		case G_MWO_POINT_XYSCREEN:
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
		case G_MWO_POINT_ZSCREEN:
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
	}
}

void gSPNumLights( s32 n )
{
	if (n <= 8)
		gSP.numLights = n;
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Setting an invalid number of lights\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPNumLights( %i );\n",
		n );
#endif
}

void gSPLightColor( u32 lightNum, u32 packedColor )
{
	lightNum--;

	if (lightNum < 8)
	{
		gSP.lights[lightNum].r = _SHIFTR( packedColor, 24, 8 ) * 0.0039215689f;
		gSP.lights[lightNum].g = _SHIFTR( packedColor, 16, 8 ) * 0.0039215689f;
		gSP.lights[lightNum].b = _SHIFTR( packedColor, 8, 8 ) * 0.0039215689f;
	}
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLightColor( %i, 0x%08X );\n",
		lightNum, packedColor );
#endif
}

void gSPFogFactor( s16 fm, s16 fo )
{
    gSP.fog.multiplier = fm;
	gSP.fog.offset = fo;

	gSP.changed |= CHANGED_FOGPOSITION;
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPFogFactor( %i, %i );\n", fm, fo );
#endif
}

void gSPPerspNormalize( u16 scale )
{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPPerspNormalize( %i );\n", scale );
#endif
}

void gSPTexture( f32 sc, f32 tc, s32 level, s32 tile, s32 on )
{
	gSP.texture.scales = sc;
	gSP.texture.scalet = tc;

	if (gSP.texture.scales == 0.0f) gSP.texture.scales = 1.0f;
	if (gSP.texture.scalet == 0.0f) gSP.texture.scalet = 1.0f;

	gSP.texture.level = level;
	gSP.texture.on = on;

	gSP.texture.tile = tile;
	gSP.textureTile[0] = &gDP.tiles[tile];
	gSP.textureTile[1] = &gDP.tiles[(tile < 7) ? (tile + 1) : tile];

	gSP.changed |= CHANGED_TEXTURE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gSPTexture( %f, %f, %i, %i, %i );\n",
		sc, tc, level, tile, on );
#endif
}

void gSPEndDisplayList()
{
	if (RSP.PCi > 0)
		RSP.PCi--;
	else
	{
#ifdef DEBUG
		DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// End of display list, halting execution\n" );
#endif
		RSP.halt = TRUE;
	}

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPEndDisplayList();\n\n" );
#endif
}

void gSPGeometryMode( u32 clear, u32 set )
{
	gSP.geometryMode = (gSP.geometryMode & ~clear) | set;

	gSP.changed |= CHANGED_GEOMETRYMODE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPGeometryMode( %s%s%s%s%s%s%s%s%s%s, %s%s%s%s%s%s%s%s%s%s );\n",
		clear & G_SHADE ? "G_SHADE | " : "",
		clear & G_LIGHTING ? "G_LIGHTING | " : "",
		clear & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		clear & G_ZBUFFER ? "G_ZBUFFER | " : "",
		clear & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		clear & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		clear & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		clear & G_CULL_BACK ? "G_CULL_BACK | " : "",
		clear & G_FOG ? "G_FOG | " : "",
		clear & G_CLIPPING ? "G_CLIPPING" : "",
		set & G_SHADE ? "G_SHADE | " : "",
		set & G_LIGHTING ? "G_LIGHTING | " : "",
		set & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		set & G_ZBUFFER ? "G_ZBUFFER | " : "",
		set & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		set & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		set & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		set & G_CULL_BACK ? "G_CULL_BACK | " : "",
		set & G_FOG ? "G_FOG | " : "",
		set & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPSetGeometryMode( u32 mode )
{
	gSP.geometryMode |= mode;

	gSP.changed |= CHANGED_GEOMETRYMODE;
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetGeometryMode( %s%s%s%s%s%s%s%s%s%s );\n",
		mode & G_SHADE ? "G_SHADE | " : "",
		mode & G_LIGHTING ? "G_LIGHTING | " : "",
		mode & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		mode & G_ZBUFFER ? "G_ZBUFFER | " : "",
		mode & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		mode & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		mode & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		mode & G_CULL_BACK ? "G_CULL_BACK | " : "",
		mode & G_FOG ? "G_FOG | " : "",
		mode & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPClearGeometryMode( u32 mode )
{
	gSP.geometryMode &= ~mode;

	gSP.changed |= CHANGED_GEOMETRYMODE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPClearGeometryMode( %s%s%s%s%s%s%s%s%s%s );\n",
		mode & G_SHADE ? "G_SHADE | " : "",
		mode & G_LIGHTING ? "G_LIGHTING | " : "",
		mode & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		mode & G_ZBUFFER ? "G_ZBUFFER | " : "",
		mode & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		mode & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		mode & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		mode & G_CULL_BACK ? "G_CULL_BACK | " : "",
		mode & G_FOG ? "G_FOG | " : "",
		mode & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPLine3D( s32 v0, s32 v1, s32 flag )
{
	OGL_DrawLine( gSP.vertices, v0, v1, 1.5f );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLine3D( %i, %i, %i );\n", v0, v1, flag );
#endif
}

void gSPLineW3D( s32 v0, s32 v1, s32 wd, s32 flag )
{
	OGL_DrawLine( gSP.vertices, v0, v1, 1.5f + wd * 0.5f );
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLineW3D( %i, %i, %i, %i );\n", v0, v1, wd, flag );
#endif
}

void gSPBgRect1Cyc( u32 bg )
{
	u32 address = RSP_SegmentToPhysical( bg );
	uObjScaleBg *objScaleBg = (uObjScaleBg*)&RDRAM[address];

	gSP.bgImage.address = RSP_SegmentToPhysical( objScaleBg->imagePtr );
	gSP.bgImage.width = objScaleBg->imageW >> 2;
	gSP.bgImage.height = objScaleBg->imageH >> 2;
	gSP.bgImage.format = objScaleBg->imageFmt;
	gSP.bgImage.size = objScaleBg->imageSiz;
	gSP.bgImage.palette = objScaleBg->imagePal;
	gDP.textureMode = TEXTUREMODE_BGIMAGE;

	f32 imageX = _FIXED2FLOAT( objScaleBg->imageX, 5 );
	f32 imageY = _FIXED2FLOAT( objScaleBg->imageY, 5 );
	f32 imageW = objScaleBg->imageW >> 2;
	f32 imageH = objScaleBg->imageH >> 2;

	f32 frameX = _FIXED2FLOAT( objScaleBg->frameX, 2 );
	f32 frameY = _FIXED2FLOAT( objScaleBg->frameY, 2 );
	f32 frameW = _FIXED2FLOAT( objScaleBg->frameW, 2 );
	f32 frameH = _FIXED2FLOAT( objScaleBg->frameH, 2 );
	f32 scaleW = _FIXED2FLOAT( objScaleBg->scaleW, 10 );
	f32 scaleH = _FIXED2FLOAT( objScaleBg->scaleH, 10 );

	f32 frameX0 = frameX;
	f32 frameY0 = frameY;
	f32 frameS0 = imageX;
	f32 frameT0 = imageY;

	f32 frameX1 = frameX + min( (imageW - imageX) / scaleW, frameW );
	f32 frameY1 = frameY + min( (imageH - imageY) / scaleH, frameH );
	f32 frameS1 = imageX + min( (imageW - imageX) * scaleW, frameW * scaleW );
	f32 frameT1 = imageY + min( (imageH - imageY) * scaleH, frameH * scaleH );
	
	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );
	gDPTextureRectangle( frameX0, frameY0, frameX1 - 1, frameY1 - 1, 0, frameS0 - 1, frameT0 - 1, scaleW, scaleH );

	if ((frameX1 - frameX0) < frameW)
	{
		f32 frameX2 = frameW - (frameX1 - frameX0) + frameX1;
		gDPTextureRectangle( frameX1, frameY0, frameX2 - 1, frameY1 - 1, 0, 0, frameT0, scaleW, scaleH );
	}

	if ((frameY1 - frameY0) < frameH)
	{
		f32 frameY2 = frameH - (frameY1 - frameY0) + frameY1;
		gDPTextureRectangle( frameX0, frameY1, frameX1 - 1, frameY2 - 1, 0, frameS0, 0, scaleW, scaleH );
	}

	gDPTextureRectangle( 0, 0, 319, 239, 0, 0, 0, scaleW, scaleH );
/*	u32 line = (u32)(frameS1 - frameS0 + 1) << objScaleBg->imageSiz >> 4;
	u16 loadHeight;
	if (objScaleBg->imageFmt == G_IM_FMT_CI)
		loadHeight = 256 / line;
	else
		loadHeight = 512 / line;
	
	gDPSetTile( objScaleBg->imageFmt, objScaleBg->imageSiz, line, 0, 7, objScaleBg->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTile( objScaleBg->imageFmt, objScaleBg->imageSiz, line, 0, 0, objScaleBg->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( 0, 0, 0, frameS1 * 4, frameT1 * 4 );
	gDPSetTextureImage( objScaleBg->imageFmt, objScaleBg->imageSiz, imageW, objScaleBg->imagePtr );

	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	for (u32 i = 0; i < frameT1 / loadHeight; i++)
	{
		//if (objScaleBg->imageLoad == G_BGLT_LOADTILE)
			gDPLoadTile( 7, frameS0 * 4, (frameT0 + loadHeight * i) * 4, frameS1 * 4, (frameT1 + loadHeight * (i + 1) * 4 );
		//else
		//{
//			gDPSetTextureImage( objScaleBg->imageFmt, objScaleBg->imageSiz, imageW, objScaleBg->imagePtr + (i + imageY) * (imageW << objScaleBg->imageSiz >> 1) + (imageX << objScaleBg->imageSiz >> 1) );
//			gDPLoadBlock( 7, 0, 0, (loadHeight * frameW << objScaleBg->imageSiz >> 1) - 1, 0 );
// 		}

		gDPTextureRectangle( frameX0, frameY0 + loadHeight * i, 
			frameX1, frameY0 + loadHeight * (i + 1) - 1, 0, 0, 0, 4, 1 );
	}*/
}

void gSPBgRectCopy( u32 bg )
{
	u32 address = RSP_SegmentToPhysical( bg );
	uObjBg *objBg = (uObjBg*)&RDRAM[address];

	gSP.bgImage.address = RSP_SegmentToPhysical( objBg->imagePtr );
	gSP.bgImage.width = objBg->imageW >> 2;
	gSP.bgImage.height = objBg->imageH >> 2;
	gSP.bgImage.format = objBg->imageFmt;
	gSP.bgImage.size = objBg->imageSiz;
	gSP.bgImage.palette = objBg->imagePal;
	gDP.textureMode = TEXTUREMODE_BGIMAGE;

	u16 imageX = objBg->imageX >> 5;
	u16 imageY = objBg->imageY >> 5;

	s16 frameX = objBg->frameX / 4;
	s16 frameY = objBg->frameY / 4;
	u16 frameW = objBg->frameW >> 2;
	u16 frameH = objBg->frameH >> 2;
	
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	gDPTextureRectangle( frameX, frameY, frameX + frameW - 1, frameY + frameH - 1, 0, imageX, imageY, 4, 1 );
}

void gSPObjRectangle( u32 sp )
{
	u32 address = RSP_SegmentToPhysical( sp );
	uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];

	f32 scaleW = _FIXED2FLOAT( objSprite->scaleW, 10 );
	f32 scaleH = _FIXED2FLOAT( objSprite->scaleH, 10 );
	f32 objX = _FIXED2FLOAT( objSprite->objX, 2 );
	f32 objY = _FIXED2FLOAT( objSprite->objY, 2 );
	u32 imageW = objSprite->imageW >> 2;
	u32 imageH = objSprite->imageH >> 2;

	gDPTextureRectangle( objX, objY, objX + imageW / scaleW - 1, objY + imageH / scaleH - 1, 0, 0.0f, 0.0f, scaleW * (gDP.otherMode.cycleType == G_CYC_COPY ? 4.0f : 1.0f), scaleH );
}

void gSPObjLoadTxtr( u32 tx )
{
	u32 address = RSP_SegmentToPhysical( tx );
	uObjTxtr *objTxtr = (uObjTxtr*)&RDRAM[address];

	if ((gSP.status[objTxtr->block.sid >> 2] & objTxtr->block.mask) != objTxtr->block.flag)
	{
		switch (objTxtr->block.type)
		{
			case G_OBJLT_TXTRBLOCK:
				gDPSetTextureImage( 0, 1, 0, objTxtr->block.image );
				gDPSetTile( 0, 1, 0, objTxtr->block.tmem, 7, 0, 0, 0, 0, 0, 0, 0 );
				gDPLoadBlock( 7, 0, 0, ((objTxtr->block.tsize + 1) << 3) - 1, objTxtr->block.tline );
				break;
			case G_OBJLT_TXTRTILE:
				gDPSetTextureImage( 0, 1, (objTxtr->tile.twidth + 1) << 1, objTxtr->tile.image );
				gDPSetTile( 0, 1, (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem, 7, 0, 0, 0, 0, 0, 0, 0 );
				gDPLoadTile( 7, 0, 0, (((objTxtr->tile.twidth + 1) << 1) - 1) << 2, (((objTxtr->tile.theight + 1) >> 2) - 1) << 2 );
				break;
			case G_OBJLT_TLUT:
				gDPSetTextureImage( 0, 2, 1, objTxtr->tlut.image );
				gDPSetTile( 0, 2, 0, objTxtr->tlut.phead, 7, 0, 0, 0, 0, 0, 0, 0 );
				gDPLoadTLUT( 7, 0, 0, objTxtr->tlut.pnum << 2, 0 );
				break;
		}
		gSP.status[objTxtr->block.sid >> 2] = (gSP.status[objTxtr->block.sid >> 2] & ~objTxtr->block.mask) | (objTxtr->block.flag & objTxtr->block.mask);
	}
}

void gSPObjSprite( u32 sp )
{
	u32 address = RSP_SegmentToPhysical( sp );
	uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];

	f32 scaleW = _FIXED2FLOAT( objSprite->scaleW, 10 );
	f32 scaleH = _FIXED2FLOAT( objSprite->scaleH, 10 );
	f32 objX = _FIXED2FLOAT( objSprite->objX, 2 );
	f32 objY = _FIXED2FLOAT( objSprite->objY, 2 );
	u32 imageW = objSprite->imageW >> 5;
	u32 imageH = objSprite->imageH >> 5;

	f32 x0 = objX;
	f32 y0 = objY;
	f32 x1 = objX + imageW / scaleW - 1;
	f32 y1 = objY + imageH / scaleH - 1;

	gSP.vertices[0].x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
	gSP.vertices[0].y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
	gSP.vertices[0].z = 0.0f;
	gSP.vertices[0].w = 1.0f;
	gSP.vertices[0].s = 0.0f;
	gSP.vertices[0].t = 0.0f;

	gSP.vertices[1].x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
	gSP.vertices[1].y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
	gSP.vertices[1].z = 0.0f;
	gSP.vertices[1].w = 1.0f;
	gSP.vertices[1].s = imageW - 1;
	gSP.vertices[1].t = 0.0f;

	gSP.vertices[2].x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
	gSP.vertices[2].y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
	gSP.vertices[2].z = 0.0f;
	gSP.vertices[2].w = 1.0f;
	gSP.vertices[2].s = imageW - 1;
	gSP.vertices[2].t = imageH - 1;

	gSP.vertices[3].x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
	gSP.vertices[3].y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
	gSP.vertices[3].z = 0.0f;
	gSP.vertices[3].w = 1.0f;
	gSP.vertices[3].s = 0;
	gSP.vertices[3].t = imageH - 1;

	gDPSetTile( objSprite->imageFmt, objSprite->imageSiz, objSprite->imageStride, objSprite->imageAdrs, 0, objSprite->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( 0, 0, 0, (imageW - 1) << 2, (imageH - 1) << 2 );
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, VI.width, VI.height, 0, 0.0f, 32767.0f );
	OGL_AddTriangle( gSP.vertices, 0, 1, 2 );
	OGL_AddTriangle( gSP.vertices, 0, 2, 3 );
	OGL_DrawTriangles();
	glLoadIdentity();

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = max( gDP.colorImage.height, gDP.scissor.lry );
}

void gSPObjLoadTxSprite( u32 txsp )
{
	gSPObjLoadTxtr( txsp );
	gSPObjSprite( txsp + sizeof( uObjTxtr ) );
}

void gSPObjLoadTxRectR( u32 txsp )
{
	gSPObjLoadTxtr( txsp );
//	gSPObjRectangleR( txsp + sizeof( uObjTxtr ) );
}

void gSPObjMatrix( u32 mtx )
{
	u32 address = RSP_SegmentToPhysical( mtx );
	uObjMtx *objMtx = (uObjMtx*)&RDRAM[address];

	gSP.objMatrix.A = _FIXED2FLOAT( objMtx->A, 16 );
	gSP.objMatrix.B = _FIXED2FLOAT( objMtx->B, 16 );
	gSP.objMatrix.C = _FIXED2FLOAT( objMtx->C, 16 );
	gSP.objMatrix.D = _FIXED2FLOAT( objMtx->D, 16 );
	gSP.objMatrix.X = _FIXED2FLOAT( objMtx->X, 2 );
	gSP.objMatrix.Y = _FIXED2FLOAT( objMtx->Y, 2 );
	gSP.objMatrix.baseScaleX = _FIXED2FLOAT( objMtx->BaseScaleX, 10 );
	gSP.objMatrix.baseScaleY = _FIXED2FLOAT( objMtx->BaseScaleY, 10 );
}

void gSPObjSubMatrix( u32 mtx )
{
}
