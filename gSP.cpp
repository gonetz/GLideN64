#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <assert.h>
#include "N64.h"
#include "GLideN64.h"
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
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "Config.h"
#include "Log.h"

using namespace std;

void gSPCombineMatrices()
{
	MultMatrix(gSP.matrix.projection, gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.combined);
	gSP.changed &= ~CHANGED_MATRIX;
}

void gSPTriangle(s32 v0, s32 v1, s32 v2)
{
	OGLRender & render = video().getRender();
	if ((v0 < INDEXMAP_SIZE) && (v1 < INDEXMAP_SIZE) && (v2 < INDEXMAP_SIZE)) {
#ifdef __TRIBUFFER_OPT
		v0 = render.getIndexmap(v0);
		v1 = render.getIndexmap(v1);
		v2 = render.getIndexmap(v2);
#endif
		if (render.isClipped(v0, v1, v2))
			return;
		render.addTriangle(v0, v1, v2);
	}

	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = (u32)max( gDP.colorImage.height, (u32)gDP.scissor.lry );
}

void gSP1Triangle( const s32 v0, const s32 v1, const s32 v2)
{
	gSPTriangle( v0, v1, v2);
	gSPFlushTriangles();
}

void gSP2Triangles(const s32 v00, const s32 v01, const s32 v02, const s32 flag0,
					const s32 v10, const s32 v11, const s32 v12, const s32 flag1 )
{
	gSPTriangle( v00, v01, v02);
	gSPTriangle( v10, v11, v12);
	gSPFlushTriangles();
}

void gSP4Triangles(const s32 v00, const s32 v01, const s32 v02,
					const s32 v10, const s32 v11, const s32 v12,
					const s32 v20, const s32 v21, const s32 v22,
					const s32 v30, const s32 v31, const s32 v32 )
{
	gSPTriangle(v00, v01, v02);
	gSPTriangle(v10, v11, v12);
	gSPTriangle(v20, v21, v22);
	gSPTriangle(v30, v31, v32);
	gSPFlushTriangles();
}

gSPInfo gSP;

f32 identityMatrix[4][4] =
{
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};

#ifdef __VEC4_OPT
static void gSPTransformVertex4_default(u32 v, float mtx[4][4])
{
	float x, y, z, w;
	OGLRender & render = video().getRender();
	for (int i = 0; i < 4; ++i) {
		SPVertex & vtx = render.getVertex(v+i);
		x = vtx.x;
		y = vtx.y;
		z = vtx.z;
		w = vtx.w;
		vtx.x = x * mtx[0][0] + y * mtx[1][0] + z * mtx[2][0] + mtx[3][0];
		vtx.y = x * mtx[0][1] + y * mtx[1][1] + z * mtx[2][1] + mtx[3][1];
		vtx.z = x * mtx[0][2] + y * mtx[1][2] + z * mtx[2][2] + mtx[3][2];
		vtx.w = x * mtx[0][3] + y * mtx[1][3] + z * mtx[2][3] + mtx[3][3];
	}
}

static void gSPTransformNormal4_default(u32 v, float mtx[4][4])
{
	float len, x, y, z;
	OGLRender & render = video().getRender();
	for (int i = 0; i < 4; ++i) {
		SPVertex & vtx = render.getVertex(v+i);
		x = vtx.nx;
		y = vtx.ny;
		z = vtx.nz;

		vtx.nx = mtx[0][0]*x + mtx[1][0]*y + mtx[2][0]*z;
		vtx.ny = mtx[0][1]*x + mtx[1][1]*y + mtx[2][1]*z;
		vtx.nz = mtx[0][2]*x + mtx[1][2]*y + mtx[2][2]*z;
		len = vtx.nx*vtx.nx + vtx.ny*vtx.ny + vtx.nz*vtx.nz;
		if (len != 0.0f) {
			len = sqrtf(len);
			vtx.nx /= len;
			vtx.ny /= len;
			vtx.nz /= len;
		}
	}
}

static void gSPLightVertex4_default(u32 v)
{
	gSPTransformNormal4(v, gSP.matrix.modelView[gSP.matrix.modelViewi]);
	OGLRender & render = video().getRender();
	if (!config.enableHWLighting) {
		for(int j = 0; j < 4; ++j) {
			SPVertex & vtx = render.getVertex(v+j);
			vtx.r = gSP.lights[gSP.numLights].r;
			vtx.g = gSP.lights[gSP.numLights].g;
			vtx.b = gSP.lights[gSP.numLights].b;
			vtx.HWLight = 0;

			for (int i = 0; i < gSP.numLights; ++i) {
				f32 intensity = DotProduct( &vtx.nx, &gSP.lights[i].x );
				if (intensity < 0.0f)
					intensity = 0.0f;
				vtx.r += gSP.lights[i].r * intensity;
				vtx.g += gSP.lights[i].g * intensity;
				vtx.b += gSP.lights[i].b * intensity;
			}
			vtx.r = min(1.0f, vtx.r);
			vtx.g = min(1.0f, vtx.g);
			vtx.b = min(1.0f, vtx.b);
		}
	} else {
		for(int j = 0; j < 4; ++j) {
			SPVertex & vtx = render.getVertex(v+j);
			vtx.HWLight = gSP.numLights;
			vtx.r = vtx.nx;
			vtx.g = vtx.ny;
			vtx.b = vtx.nz;
		}
	}
}

static void gSPPointLightVertex4_default(u32 v, float _vPos[4][3])
{
	assert(_vPos != NULL);
	gSPTransformNormal4(v, gSP.matrix.modelView[gSP.matrix.modelViewi]);
	OGLRender & render = video().getRender();
	for(int j = 0; j < 4; ++j) {
		SPVertex & vtx = render.getVertex(v+j);
		float light_intensity = 0.0f;
		vtx.HWLight = 0;
		vtx.r = gSP.lights[gSP.numLights].r;
		vtx.g = gSP.lights[gSP.numLights].g;
		vtx.b = gSP.lights[gSP.numLights].b;
		for (u32 l=0; l < gSP.numLights; ++l) {
			float lvec[3] = {gSP.lights[l].posx, gSP.lights[l].posy, gSP.lights[l].posz};
			lvec[0] -= _vPos[j][0];
			lvec[1] -= _vPos[j][1];
			lvec[2] -= _vPos[j][2];
			const float light_len2 = lvec[0]*lvec[0] + lvec[1]*lvec[1] + lvec[2]*lvec[2];
			const float light_len = sqrtf(light_len2);
			const float at = gSP.lights[l].ca + light_len/65535.0f*gSP.lights[l].la + light_len2/65535.0f*gSP.lights[l].qa;
			if (at > 0.0f)
				light_intensity = 1/at;//DotProduct (lvec, nvec) / (light_len * normal_len * at);
			else
				light_intensity = 0.0f;
			if (light_intensity > 0.0f) {
				vtx.r += gSP.lights[l].r * light_intensity;
				vtx.g += gSP.lights[l].g * light_intensity;
				vtx.b += gSP.lights[l].b * light_intensity;
			}
		}
		if (vtx.r > 1.0f) vtx.r = 1.0f;
		if (vtx.g > 1.0f) vtx.g = 1.0f;
		if (vtx.b > 1.0f) vtx.b = 1.0f;
	}
}

static void gSPBillboardVertex4_default(u32 v)
{
	OGLRender & render = video().getRender();
	int i = 0;
#ifdef __TRIBUFFER_OPT
	i = render.getIndexmap(0);
#endif
	SPVertex & vtx0 = render.getVertex(i);
	for (int j = 0; j < 4; ++j) {
		SPVertex & vtx = render.getVertex(v+j);
		vtx.x = vtx0.x;
		vtx.y = vtx0.y;
		vtx.z = vtx0.z;
		vtx.w = vtx0.w;
	}
}

void gSPClipVertex4(u32 v)
{
	OGLRender & render = video().getRender();
	for(int i = 0; i < 4; ++i) {
		SPVertex & vtx = render.getVertex(v+i);
		vtx.clip = 0;
		if (vtx.x > +vtx.w) vtx.clip |= CLIP_POSX;
		if (vtx.x < -vtx.w) vtx.clip |= CLIP_NEGX;
		if (vtx.y > +vtx.w) vtx.clip |= CLIP_POSY;
		if (vtx.y < -vtx.w) vtx.clip |= CLIP_NEGY;
		if (vtx.w < 0.01f) vtx.clip |= CLIP_Z;
	}
}

void gSPProcessVertex4(u32 v)
{
	if (gSP.changed & CHANGED_MATRIX)
		gSPCombineMatrices();

	OGLRender & render = video().getRender();
	float vPos[4][3];
	for(int i = 0; i < 4; ++i) {
		SPVertex & vtx = render.getVertex(v+i);
		vPos[i][0] = vtx.x;
		vPos[i][1] = vtx.y;
		vPos[i][2] = vtx.z;
	}
	gSPTransformVertex4(v, gSP.matrix.combined );

	if (gDP.otherMode.depthSource) {
		for(int i = 0; i < 4; ++i) {
			SPVertex & vtx = render.getVertex(v+i);
			vtx.z = gDP.primDepth.z * vtx.w;
		}
	}

	if (gSP.matrix.billboard)
		gSPBillboardVertex4(v);

	if (!(gSP.geometryMode & G_ZBUFFER)) {
		for(int i = 0; i < 4; ++i) {
			SPVertex & vtx = render.getVertex(v+i);
			vtx.z = -vtx.w;
		}
	}

	if (gSP.geometryMode & G_LIGHTING) {
		if (gSP.geometryMode & G_POINT_LIGHTING)
			gSPPointLightVertex4(v, vPos);
		else
			gSPLightVertex4(v);

		if (gSP.geometryMode & G_TEXTURE_GEN) {
			for(int i = 0; i < 4; ++i) {
				SPVertex & vtx = render.getVertex(v+i);
				f32 fLightDir[3] = {vtx.nx, vtx.ny, vtx.nz};
				TransformVectorNormalize(fLightDir, gSP.matrix.projection);
				f32 x, y;
				if (gSP.lookatEnable) {
					x = DotProduct(&gSP.lookat[0].x, fLightDir);
					y = DotProduct(&gSP.lookat[1].x, fLightDir);
				} else {
					x = fLightDir[0];
					y = fLightDir[1];
				}
				if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR) {
					vtx.s = acosf(x) * 325.94931f;
					vtx.t = acosf(y) * 325.94931f;
				} else { // G_TEXTURE_GEN
					vtx.s = (x + 1.0f) * 512.0f;
					vtx.t = (y + 1.0f) * 512.0f;
				}
			}
		}
	} else {
		for(int i = 0; i < 4; ++i)
			render.getVertex(v+i).HWLight = 0;
	}

	gSPClipVertex4(v);
}
#endif

static void gSPTransformVertex_default(float vtx[4], float mtx[4][4])
{
	float x, y, z, w;
	x = vtx[0];
	y = vtx[1];
	z = vtx[2];
	w = vtx[3];

	vtx[0] = x * mtx[0][0] + y * mtx[1][0] + z * mtx[2][0] + mtx[3][0];
	vtx[1] = x * mtx[0][1] + y * mtx[1][1] + z * mtx[2][1] + mtx[3][1];
	vtx[2] = x * mtx[0][2] + y * mtx[1][2] + z * mtx[2][2] + mtx[3][2];
	vtx[3] = x * mtx[0][3] + y * mtx[1][3] + z * mtx[2][3] + mtx[3][3];
}

static void gSPLightVertex_default(SPVertex & _vtx)
{
	if (!config.enableHWLighting) {
		_vtx.HWLight = 0;
		_vtx.r = gSP.lights[gSP.numLights].r;
		_vtx.g = gSP.lights[gSP.numLights].g;
		_vtx.b = gSP.lights[gSP.numLights].b;
		for (int i = 0; i < gSP.numLights; ++i){
			f32 intensity = DotProduct( &_vtx.nx, &gSP.lights[i].x );
			if (intensity < 0.0f)
				intensity = 0.0f;
			_vtx.r += gSP.lights[i].r * intensity;
			_vtx.g += gSP.lights[i].g * intensity;
			_vtx.b += gSP.lights[i].b * intensity;
		}
		_vtx.r = min(1.0f, _vtx.r);
		_vtx.g = min(1.0f, _vtx.g);
		_vtx.b = min(1.0f, _vtx.b);
	} else {
		_vtx.HWLight = gSP.numLights;
		_vtx.r = _vtx.nx;
		_vtx.g = _vtx.ny;
		_vtx.b = _vtx.nz;
	}
}

static void gSPPointLightVertex_default(SPVertex & _vtx, float * _vPos)
{
	assert(_vPos != NULL);
	float light_intensity = 0.0f;
	_vtx.HWLight = 0;
	_vtx.r = gSP.lights[gSP.numLights].r;
	_vtx.g = gSP.lights[gSP.numLights].g;
	_vtx.b = gSP.lights[gSP.numLights].b;
	for (u32 l=0; l < gSP.numLights; ++l) {
		float lvec[3] = {gSP.lights[l].posx, gSP.lights[l].posy, gSP.lights[l].posz};
		lvec[0] -= _vPos[0];
		lvec[1] -= _vPos[1];
		lvec[2] -= _vPos[2];
		const float light_len2 = lvec[0]*lvec[0] + lvec[1]*lvec[1] + lvec[2]*lvec[2];
		const float light_len = sqrtf(light_len2);
		const float at = gSP.lights[l].ca + light_len/65535.0f*gSP.lights[l].la + light_len2/65535.0f*gSP.lights[l].qa;
		if (at > 0.0f)
			light_intensity = 1/at;//DotProduct (lvec, nvec) / (light_len * normal_len * at);
		else
			light_intensity = 0.0f;
		if (light_intensity > 0.0f) {
			_vtx.r += gSP.lights[l].r * light_intensity;
			_vtx.g += gSP.lights[l].g * light_intensity;
			_vtx.b += gSP.lights[l].b * light_intensity;
		}
	}
	if (_vtx.r > 1.0f) _vtx.r = 1.0f;
	if (_vtx.g > 1.0f) _vtx.g = 1.0f;
	if (_vtx.b > 1.0f) _vtx.b = 1.0f;
}

static void gSPBillboardVertex_default(u32 v, u32 i)
{
	OGLRender & render = video().getRender();
	SPVertex & vtx0 = render.getVertex(i);
	SPVertex & vtx = render.getVertex(v);
	vtx.x += vtx0.x;
	vtx.y += vtx0.y;
	vtx.z += vtx0.z;
	vtx.w += vtx0.w;
}

void gSPClipVertex(u32 v)
{
	SPVertex & vtx = video().getRender().getVertex(v);
	vtx.clip = 0;
	if (vtx.x > +vtx.w) vtx.clip |= CLIP_POSX;
	if (vtx.x < -vtx.w) vtx.clip |= CLIP_NEGX;
	if (vtx.y > +vtx.w) vtx.clip |= CLIP_POSY;
	if (vtx.y < -vtx.w) vtx.clip |= CLIP_NEGY;
	if (vtx.w < 0.01f)  vtx.clip |= CLIP_Z;
}

void gSPProcessVertex(u32 v)
{
	if (gSP.changed & CHANGED_MATRIX)
		gSPCombineMatrices();

	OGLRender & render = video().getRender();
	SPVertex & vtx = render.getVertex(v);
	float vPos[3] = {(float)vtx.x, (float)vtx.y, (float)vtx.z};
	gSPTransformVertex( &vtx.x, gSP.matrix.combined );

	if (gDP.otherMode.depthSource)
		vtx.z = gDP.primDepth.z * vtx.w;

	if (gSP.matrix.billboard) {
		int i = 0;
#ifdef __TRIBUFFER_OPT
		i = render.getIndexmap(0);
#endif

		gSPBillboardVertex(v, i);
	}

	if (!(gSP.geometryMode & G_ZBUFFER))
		vtx.z = -vtx.w;

	gSPClipVertex(v);

	if (gSP.geometryMode & G_LIGHTING) {
		TransformVectorNormalize( &vtx.nx, gSP.matrix.modelView[gSP.matrix.modelViewi] );
		if (gSP.geometryMode & G_POINT_LIGHTING)
			gSPPointLightVertex(vtx, vPos);
		else
			gSPLightVertex(vtx);

		if (gSP.geometryMode & G_TEXTURE_GEN) {
			f32 fLightDir[3] = {vtx.nx, vtx.ny, vtx.nz};
			TransformVectorNormalize(fLightDir, gSP.matrix.projection);
			f32 x, y;
			if (gSP.lookatEnable) {
				x = DotProduct(&gSP.lookat[0].x, fLightDir);
				y = DotProduct(&gSP.lookat[1].x, fLightDir);
			} else {
				x = fLightDir[0];
				y = fLightDir[1];
			}
			if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR) {
				vtx.s = acosf(x) * 325.94931f;
				vtx.t = acosf(y) * 325.94931f;
			} else { // G_TEXTURE_GEN
				vtx.s = (x + 1.0f) * 512.0f;
				vtx.t = (y + 1.0f) * 512.0f;
			}
		}
	} else
		vtx.HWLight = 0;
}

void gSPLoadUcodeEx( u32 uc_start, u32 uc_dstart, u16 uc_dsize )
{
	gSP.matrix.modelViewi = 0;
	gSP.changed |= CHANGED_MATRIX;
	gSP.status[0] = gSP.status[1] = gSP.status[2] = gSP.status[3] = 0;

	if ((((uc_start & 0x1FFFFFFF) + 4096) > RDRAMSize) || (((uc_dstart & 0x1FFFFFFF) + uc_dsize) > RDRAMSize))
		return;

	GBI.loadMicrocode(uc_start, uc_dstart, uc_dsize);
}

void gSPNoOp()
{
	gSPFlushTriangles();
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gSPNoOp();\n" );
#endif
}

void gSPMatrix( u32 matrix, u8 param )
{
#ifdef __TRIBUFFER_OPT
	gSPFlushTriangles();
#endif

	f32 mtx[4][4];
	u32 address = RSP_SegmentToPhysical( matrix );

	if (address + 64 > RDRAMSize) {
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

	if (param & G_MTX_PROJECTION) {
		if (param & G_MTX_LOAD)
			CopyMatrix( gSP.matrix.projection, mtx );
		else
			MultMatrix2( gSP.matrix.projection, mtx );
	} else {
		if ((param & G_MTX_PUSH) && (gSP.matrix.modelViewi < (gSP.matrix.stackSize - 1))) {
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
			MultMatrix2( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
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

	if (address + 64 > RDRAMSize) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
			matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
		return;
	}

	RSP_LoadMatrix(mtx, address);

	gSP.matrix.modelViewi = index;

	if (multiply) {
//		CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.modelView[0] );
//		MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
		MultMatrix(gSP.matrix.modelView[0], mtx, gSP.matrix.modelView[gSP.matrix.modelViewi]);
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

	if ((address + 16) > RDRAMSize) {
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

	if (address + 64 > RDRAMSize) {
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
	--n;
	u32 addrByte = RSP_SegmentToPhysical( l );

	if ((addrByte + sizeof( Light )) > RDRAMSize) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load light from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
			l, n );
#endif
		return;
	}

	Light *light = (Light*)&RDRAM[addrByte];

	if (n < 8) {
		gSP.lights[n].r = light->r * 0.0039215689f;
		gSP.lights[n].g = light->g * 0.0039215689f;
		gSP.lights[n].b = light->b * 0.0039215689f;

		gSP.lights[n].x = light->x;
		gSP.lights[n].y = light->y;
		gSP.lights[n].z = light->z;

		Normalize( &gSP.lights[n].x );
		u32 addrShort = addrByte >> 1;
		gSP.lights[n].posx = (float)(((short*)RDRAM)[(addrShort+4)^1]);
		gSP.lights[n].posy = (float)(((short*)RDRAM)[(addrShort+5)^1]);
		gSP.lights[n].posz = (float)(((short*)RDRAM)[(addrShort+6)^1]);
		gSP.lights[n].ca = (float)(RDRAM[(addrByte + 3) ^ 3]) / 16.0f;
		gSP.lights[n].la = (float)(RDRAM[(addrByte + 7) ^ 3]);
		gSP.lights[n].qa = (float)(RDRAM[(addrByte + 14) ^ 3]) / 8.0f;
	}

	if (config.enableHWLighting)
		gSP.changed |= CHANGED_LIGHT;

#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// x = %2.6f    y = %2.6f    z = %2.6f\n",
		_FIXED2FLOAT( light->x, 7 ), _FIXED2FLOAT( light->y, 7 ), _FIXED2FLOAT( light->z, 7 ) );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// r = %3i    g = %3i   b = %3i\n",
		light->r, light->g, light->b );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
		l, n );
#endif
}

void gSPLookAt( u32 _l, u32 _n )
{
	u32 address = RSP_SegmentToPhysical(_l);

	if ((address + sizeof(Light)) > RDRAMSize) {
#ifdef DEBUG
		DebugMsg(DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load light from invalid address\n");
		DebugMsg(DEBUG_HIGH | DEBUG_HANDLED, "gSPLookAt( 0x%08X, LOOKAT_%i );\n",
			l, n);
#endif
		return;
	}

	Light *light = (Light*)&RDRAM[address];

	gSP.lookat[_n].x = light->x;
	gSP.lookat[_n].y = light->y;
	gSP.lookat[_n].z = light->z;

	gSP.lookatEnable = (_n == 0) || (_n == 1 && light->x != 0 && light->y != 0);

	Normalize(&gSP.lookat[_n].x);
}

void gSPVertex( u32 a, u32 n, u32 v0 )
{
	//flush batched triangles:
#ifdef __TRIBUFFER_OPT
	gSPFlushTriangles();
#endif

	u32 address = RSP_SegmentToPhysical(a);

	if ((address + sizeof( Vertex ) * n) > RDRAMSize)
		return;


	Vertex *vertex = (Vertex*)&RDRAM[address];

	OGLRender & render = video().getRender();
	if ((n + v0) <= INDEXMAP_SIZE) {
		unsigned int i = v0;
#ifdef __VEC4_OPT
		for (; i < n - (n%4) + v0; i += 4) {
			u32 v = i;
#ifdef __TRIBUFFER_OPT
			v = render.getIndexmapNew(v, 4);
#endif
			for(int j = 0; j < 4; ++j) {
				SPVertex & vtx = render.getVertex(v+j);
				vtx.x = vertex->x;
				vtx.y = vertex->y;
				vtx.z = vertex->z;
				//vtx.flag = vertex->flag;
				vtx.s = _FIXED2FLOAT( vertex->s, 5 );
				vtx.t = _FIXED2FLOAT( vertex->t, 5 );
				vtx.st_scaled = 0;
				if (gSP.geometryMode & G_LIGHTING) {
					vtx.nx = vertex->normal.x;
					vtx.ny = vertex->normal.y;
					vtx.nz = vertex->normal.z;
					vtx.a = vertex->color.a * 0.0039215689f;
				} else {
					vtx.r = vertex->color.r * 0.0039215689f;
					vtx.g = vertex->color.g * 0.0039215689f;
					vtx.b = vertex->color.b * 0.0039215689f;
					vtx.a = vertex->color.a * 0.0039215689f;
				}
				vertex++;
			}
			gSPProcessVertex4(v);
		}
#endif
		for (; i < n + v0; ++i) {
			u32 v = i;
#ifdef __TRIBUFFER_OPT
			v = render.getIndexmapNew(v, 1);
#endif
			SPVertex & vtx = render.getVertex(v);
			vtx.x = vertex->x;
			vtx.y = vertex->y;
			vtx.z = vertex->z;
			vtx.s = _FIXED2FLOAT( vertex->s, 5 );
			vtx.t = _FIXED2FLOAT( vertex->t, 5 );
			vtx.st_scaled = 0;
			if (gSP.geometryMode & G_LIGHTING) {
				vtx.nx = vertex->normal.x;
				vtx.ny = vertex->normal.y;
				vtx.nz = vertex->normal.z;
				vtx.a = vertex->color.a * 0.0039215689f;
			} else {
				vtx.r = vertex->color.r * 0.0039215689f;
				vtx.g = vertex->color.g * 0.0039215689f;
				vtx.b = vertex->color.b * 0.0039215689f;
				vtx.a = vertex->color.a * 0.0039215689f;
			}
			gSPProcessVertex(v);
			vertex++;
		}
	} else {
		LOG(LOG_ERROR, "Using Vertex outside buffer v0=%i, n=%i\n", v0, n);
	}
}

void gSPCIVertex( u32 a, u32 n, u32 v0 )
{

#ifdef __TRIBUFFER_OPT
	gSPFlushTriangles();
#endif

	u32 address = RSP_SegmentToPhysical( a );

	if ((address + sizeof( PDVertex ) * n) > RDRAMSize)
		return;

	PDVertex *vertex = (PDVertex*)&RDRAM[address];

	OGLRender & render = video().getRender();
	if ((n + v0) <= INDEXMAP_SIZE) {
		unsigned int i = v0;
#ifdef __VEC4_OPT
		for (; i < n - (n%4) + v0; i += 4) {
			u32 v = i;
#ifdef __TRIBUFFER_OPT
			v = render.getIndexmapNew(v, 4);
#endif
			for(unsigned int j = 0; j < 4; ++j) {
				SPVertex & vtx = render.getVertex(v + j);
				vtx.x = vertex->x;
				vtx.y = vertex->y;
				vtx.z = vertex->z;
				vtx.s = _FIXED2FLOAT( vertex->s, 5 );
				vtx.t = _FIXED2FLOAT( vertex->t, 5 );
				vtx.st_scaled = 0;
				u8 *color = &RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

				if (gSP.geometryMode & G_LIGHTING) {
					vtx.nx = (s8)color[3];
					vtx.ny = (s8)color[2];
					vtx.nz = (s8)color[1];
					vtx.a = color[0] * 0.0039215689f;
				} else {
					vtx.r = color[3] * 0.0039215689f;
					vtx.g = color[2] * 0.0039215689f;
					vtx.b = color[1] * 0.0039215689f;
					vtx.a = color[0] * 0.0039215689f;
				}
				vertex++;
			}
			gSPProcessVertex4(v);
		}
#endif
		for(; i < n + v0; ++i) {
			u32 v = i;
#ifdef __TRIBUFFER_OPT
			v = render.getIndexmapNew(v, 1);
#endif
			SPVertex & vtx = render.getVertex(v);
			vtx.x = vertex->x;
			vtx.y = vertex->y;
			vtx.z = vertex->z;
			vtx.s = _FIXED2FLOAT( vertex->s, 5 );
			vtx.t = _FIXED2FLOAT( vertex->t, 5 );
			vtx.st_scaled = 0;
			u8 *color = &RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

			if (gSP.geometryMode & G_LIGHTING) {
				vtx.nx = (s8)color[3];
				vtx.ny = (s8)color[2];
				vtx.nz = (s8)color[1];
				vtx.a = color[0] * 0.0039215689f;
			} else {
				vtx.r = color[3] * 0.0039215689f;
				vtx.g = color[2] * 0.0039215689f;
				vtx.b = color[1] * 0.0039215689f;
				vtx.a = color[0] * 0.0039215689f;
			}

			gSPProcessVertex(v);
			vertex++;
		}
	} else {
		LOG(LOG_ERROR, "Using Vertex outside buffer v0=%i, n=%i\n", v0, n);
	}
}

void gSPDMAVertex( u32 a, u32 n, u32 v0 )
{

	u32 address = gSP.DMAOffsets.vtx + RSP_SegmentToPhysical(a);

	if ((address + 10 * n) > RDRAMSize)
		return;

	OGLRender & render = video().getRender();
	if ((n + v0) <= INDEXMAP_SIZE) {
		u32 i = v0;
#ifdef __VEC4_OPT
		for (; i < n - (n%4) + v0; i += 4) {
			u32 v = i;
#ifdef __TRIBUFFER_OPT
			v = render.getIndexmapNew(v, 4);
#endif
			for(int j = 0; j < 4; ++j) {
				SPVertex & vtx = render.getVertex(v + j);
				vtx.x = *(s16*)&RDRAM[address ^ 2];
				vtx.y = *(s16*)&RDRAM[(address + 2) ^ 2];
				vtx.z = *(s16*)&RDRAM[(address + 4) ^ 2];

				if (gSP.geometryMode & G_LIGHTING) {
					vtx.nx = *(s8*)&RDRAM[(address + 6) ^ 3];
					vtx.ny = *(s8*)&RDRAM[(address + 7) ^ 3];
					vtx.nz = *(s8*)&RDRAM[(address + 8) ^ 3];
					vtx.a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
				} else {
					vtx.r = *(u8*)&RDRAM[(address + 6) ^ 3] * 0.0039215689f;
					vtx.g = *(u8*)&RDRAM[(address + 7) ^ 3] * 0.0039215689f;
					vtx.b = *(u8*)&RDRAM[(address + 8) ^ 3] * 0.0039215689f;
					vtx.a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
				}
				address += 10;
			}
			gSPProcessVertex4(v);
		}
#endif
		for (; i < n + v0; ++i) {
			u32 v = i;
#ifdef __TRIBUFFER_OPT
			v = render.getIndexmapNew(v, 1);
#endif
			SPVertex & vtx = render.getVertex(v);
			vtx.x = *(s16*)&RDRAM[address ^ 2];
			vtx.y = *(s16*)&RDRAM[(address + 2) ^ 2];
			vtx.z = *(s16*)&RDRAM[(address + 4) ^ 2];

			if (gSP.geometryMode & G_LIGHTING) {
				vtx.nx = *(s8*)&RDRAM[(address + 6) ^ 3];
				vtx.ny = *(s8*)&RDRAM[(address + 7) ^ 3];
				vtx.nz = *(s8*)&RDRAM[(address + 8) ^ 3];
				vtx.a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
			} else {
				vtx.r = *(u8*)&RDRAM[(address + 6) ^ 3] * 0.0039215689f;
				vtx.g = *(u8*)&RDRAM[(address + 7) ^ 3] * 0.0039215689f;
				vtx.b = *(u8*)&RDRAM[(address + 8) ^ 3] * 0.0039215689f;
				vtx.a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
			}
			vtx.st_scaled = 0;

			gSPProcessVertex(v);
			address += 10;
		}
	} else {
		LOG(LOG_ERROR, "Using Vertex outside buffer v0=%i, n=%i\n", v0, n);
	}
}

void gSPDisplayList( u32 dl )
{
	u32 address = RSP_SegmentToPhysical( dl );

	if ((address + 8) > RDRAMSize) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
			dl );
#endif
		return;
	}

	if (RSP.PCi < (GBI.PCStackSize - 1)) {
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "\n" );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
		dl );
#endif
		RSP.PCi++;
		RSP.PC[RSP.PCi] = address;
		RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[address], 24, 8 );
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
	if ((dl + (n << 3)) > RDRAMSize) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
			dl, n );
#endif
		return;
	}

	u32 curDL = RSP.PC[RSP.PCi];

	RSP.PC[RSP.PCi] = RSP_SegmentToPhysical( dl );

	while ((RSP.PC[RSP.PCi] - dl) < (n << 3)) {
		if ((RSP.PC[RSP.PCi] + 8) > RDRAMSize) {
#ifdef DEBUG
			switch (Debug.level) {
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

	if ((address + 8) > RDRAMSize) {
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
	RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[address], 24, 8 );
}

void gSPBranchLessZ( u32 branchdl, u32 vtx, f32 zval )
{
	u32 address = RSP_SegmentToPhysical( branchdl );

	if ((address + 8) > RDRAMSize) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Specified display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
			branchdl, vtx, zval );
#endif
		return;
	}

	if (video().getRender().getVertex(vtx).z <= zval)
		RSP.PC[RSP.PCi] = address;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
			branchdl, vtx, zval );
#endif
}

void gSPDlistCount(u32 count, u32 v)
{
	u32 address = RSP_SegmentToPhysical( v );
	if (address == 0 || (address + 8) > RDRAMSize) {
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to branch to display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDlistCnt(%d, 0x%08X );\n", count, v );
		return;
	}

	if (RSP.PCi >= 9) {
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// ** DL stack overflow **\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDlistCnt(%d, 0x%08X );\n", count, v );
		return;
	}

	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDlistCnt(%d, 0x%08X );\n", count, v );

	++RSP.PCi;  // go to the next PC in the stack
	RSP.PC[RSP.PCi] = address;  // jump to the address
	RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[address], 24, 8 );
	RSP.count = count + 1;
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

void gSPSetDMATexOffset(u32 _addr)
{
	gSP.DMAOffsets.tex_offset = RSP_SegmentToPhysical(_addr);
	gSP.DMAOffsets.tex_shift = 0;
	gSP.DMAOffsets.tex_count = 0;
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
	dest->HWLight = src->HWLight;
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
	dest->HWLight = first->HWLight;
}

void gSPDMATriangles( u32 tris, u32 n ){
	u32 address = RSP_SegmentToPhysical( tris );

	if (address + sizeof( DKRTriangle ) * n > RDRAMSize) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Attempting to load triangles from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n" );
#endif
		return;
	}

	OGLRender & render = video().getRender();
	render.setDMAVerticesSize(n * 3);
#ifdef __TRIBUFFER_OPT
	render.indexmapUndo();
#endif

	DKRTriangle *triangles = (DKRTriangle*)&RDRAM[address];
	SPVertex * pVtx = render.getDMAVerticesData();
	for (u32 i = 0; i < n; ++i) {
		int mode = 0;
		if (!(triangles->flag & 0x40)) {
			if (gSP.viewport.vscale[0] > 0)
				mode |= G_CULL_BACK;
			else
				mode |= G_CULL_FRONT;
		}
		if ((gSP.geometryMode&G_CULL_BOTH) != mode) {
			render.drawDMATriangles(pVtx - render.getDMAVerticesData());
			pVtx = render.getDMAVerticesData();
			gSP.geometryMode &= ~G_CULL_BOTH;
			gSP.geometryMode |= mode;
			gSP.changed |= CHANGED_GEOMETRYMODE;
		}

		const s32 v0 = triangles->v0;
		const s32 v1 = triangles->v1;
		const s32 v2 = triangles->v2;
		if (render.isClipped(v0, v1, v2)) {
			++triangles;
			continue;
		}
		*pVtx = render.getVertex(v0);
		pVtx->s = _FIXED2FLOAT(triangles->s0, 5);
		pVtx->t = _FIXED2FLOAT(triangles->t0, 5);
		++pVtx;
		*pVtx = render.getVertex(v1);
		pVtx->s = _FIXED2FLOAT(triangles->s1, 5);
		pVtx->t = _FIXED2FLOAT(triangles->t1, 5);
		++pVtx;
		*pVtx = render.getVertex(v2);
		pVtx->s = _FIXED2FLOAT(triangles->s2, 5);
		pVtx->t = _FIXED2FLOAT(triangles->t2, 5);
		++pVtx;
		++triangles;
	}
	render.drawDMATriangles(pVtx - render.getDMAVerticesData());
}

void gSP1Quadrangle( s32 v0, s32 v1, s32 v2, s32 v3 )
{
	gSPTriangle( v0, v1, v2);
	gSPTriangle( v0, v2, v3);
	gSPFlushTriangles();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Quadrangle( %i, %i, %i, %i );\n",
			v0, v1, v2, v3 );
#endif
}

bool gSPCullVertices( u32 v0, u32 vn )
{
	u32 clip = 0;
	OGLRender & render = video().getRender();
	for (u32 i = v0; i <= vn; ++i) {
#ifdef __TRIBUFFER_OPT
		const u32 v = render.getIndexmap(i);
#else
		const u32 v = i;
#endif
		clip |= (~render.getVertex(v).clip) & CLIP_ALL;
		if (clip == CLIP_ALL)
			return false;
	}
	return true;
}

void gSPCullDisplayList( u32 v0, u32 vn )
{
	if (gSPCullVertices( v0, vn )) {
		if (RSP.PCi > 0)
			RSP.PCi--;
		else {
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
	if (gSP.matrix.modelViewi > num - 1) {
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
	if (gSP.matrix.modelViewi > 0) {
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
	if (seg > 0xF) {
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load address into invalid segment\n",
			SegmentText[seg], base );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
			SegmentText[seg], base );
#endif
		return;
	}

	if ((unsigned int)base > RDRAMSize - 1) {
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
		return;

	if (where < 0x20) {
		fraction = modff( gSP.matrix.combined[0][where >> 1], &integer );
		gSP.matrix.combined[0][where >> 1] = (s16)_SHIFTR( num, 16, 16 ) + abs( (int)fraction );

		fraction = modff( gSP.matrix.combined[0][(where >> 1) + 1], &integer );
		gSP.matrix.combined[0][(where >> 1) + 1] = (s16)_SHIFTR( num, 0, 16 ) + abs( (int)fraction );
	} else {
		f32 newValue;

		fraction = modff( gSP.matrix.combined[0][(where - 0x20) >> 1], &integer );
		newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 16, 16 ), 16);

		// Make sure the sign isn't lost
		if ((integer == 0.0f) && (fraction != 0.0f))
			newValue = newValue * (fraction / abs( (int)fraction ));

		gSP.matrix.combined[0][(where - 0x20) >> 1] = newValue;

		fraction = modff( gSP.matrix.combined[0][((where - 0x20) >> 1) + 1], &integer );
		newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 0, 16 ), 16 );

		// Make sure the sign isn't lost
		if ((integer == 0.0f) && (fraction != 0.0f))
			newValue = newValue * (fraction / abs( (int)fraction ));

		gSP.matrix.combined[0][((where - 0x20) >> 1) + 1] = newValue;
	}
}

void gSPModifyVertex( u32 _vtx, u32 _where, u32 _val )
{
	s32 v = _vtx;

	OGLRender & render = video().getRender();
#ifdef __TRIBUFFER_OPT
	v = render.getIndexmap(v);
#endif

	SPVertex & vtx0 = render.getVertex(v);
	switch (_where) {
		case G_MWO_POINT_RGBA:
			vtx0.r = _SHIFTR( _val, 24, 8 ) * 0.0039215689f;
			vtx0.g = _SHIFTR( _val, 16, 8 ) * 0.0039215689f;
			vtx0.b = _SHIFTR( _val, 8, 8 ) * 0.0039215689f;
			vtx0.a = _SHIFTR( _val, 0, 8 ) * 0.0039215689f;
			break;
		case G_MWO_POINT_ST:
			if (gDP.otherMode.texturePersp != 0) {
				vtx0.s = _FIXED2FLOAT( (s16)_SHIFTR( _val, 16, 16 ), 5 );
				vtx0.t = _FIXED2FLOAT( (s16)_SHIFTR( _val, 0, 16 ), 5 );
			} else {
				vtx0.s = _FIXED2FLOAT( (s16)_SHIFTR( _val, 16, 16 ), 6 );
				vtx0.t = _FIXED2FLOAT( (s16)_SHIFTR( _val, 0, 16 ), 6 );
			}
			vtx0.st_scaled = 1;
			break;
		case G_MWO_POINT_XYSCREEN:
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				_vtx, MWOPointText[(_where - 0x10) >> 2], _val );
#endif
			break;
		case G_MWO_POINT_ZSCREEN:
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				_vtx, MWOPointText[(_where - 0x10) >> 2], _val );
#endif
			break;
	}
}

void gSPNumLights( s32 n )
{
	if (n <= 8) {
		gSP.numLights = n;
		if (config.enableHWLighting)
			gSP.changed |= CHANGED_LIGHT;
	}
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
	--lightNum;

	if (lightNum < 8)
	{
		gSP.lights[lightNum].r = _SHIFTR( packedColor, 24, 8 ) * 0.0039215689f;
		gSP.lights[lightNum].g = _SHIFTR( packedColor, 16, 8 ) * 0.0039215689f;
		gSP.lights[lightNum].b = _SHIFTR( packedColor, 8, 8 ) * 0.0039215689f;
		if (config.enableHWLighting)
			gSP.changed |= CHANGED_LIGHT;
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
		--RSP.PCi;
	else
	{
#ifdef DEBUG
		DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// End of display list, halting execution\n" );
#endif
		RSP.halt = TRUE;
	}

#ifdef __TRIBUFFER_OPT
	RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[RSP.PC[RSP.PCi]], 24, 8 );
	gSPFlushTriangles();
#endif
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
	video().getRender().drawLine(v0, v1, 1.5f);

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLine3D( %i, %i, %i );\n", v0, v1, flag );
#endif
}

void gSPLineW3D( s32 v0, s32 v1, s32 wd, s32 flag )
{
	video().getRender().drawLine(v0, v1, 1.5f + wd * 0.5f);
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLineW3D( %i, %i, %i, %i );\n", v0, v1, wd, flag );
#endif
}

#ifdef GL_IMAGE_TEXTURES_SUPPORT
static
void _copyDepthBuffer()
{
	if (!config.frameBufferEmulation.enable)
		return;
	// The game copies content of depth buffer into current color buffer
	// OpenGL has different format for color and depth buffers, so this trick can't be performed directly
	// To do that, depth buffer with address of current color buffer created and attached to the current FBO
	// It will be copy depth buffer
	depthBufferList().saveBuffer(gDP.colorImage.address);
	// Take any frame buffer and attach source depth buffer to it, to blit it into copy depth buffer
	FrameBuffer * pTmpBuffer = frameBufferList().findTmpBuffer(frameBufferList().getCurrent()->m_startAddress);
	DepthBuffer * pTmpBufferDepth = pTmpBuffer->m_pDepthBuffer;
	pTmpBuffer->m_pDepthBuffer = depthBufferList().findBuffer(gSP.bgImage.address);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, pTmpBuffer->m_FBO);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pTmpBuffer->m_pDepthBuffer->m_renderbuf);
	GLuint attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1,  &attachment);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferList().getCurrent()->m_FBO);
	OGLVideo & ogl = video();
	glBlitFramebuffer(
		0, 0, ogl.getWidth(), ogl.getHeight(),
		0, 0, ogl.getWidth(), ogl.getHeight(),
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
	);
	// Restore objects
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pTmpBufferDepth->m_renderbuf);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	// Set back current depth buffer
	depthBufferList().saveBuffer(gDP.depthImageAddress);
}
#endif // GL_IMAGE_TEXTURES_SUPPORT

static
void loadBGImage(const uObjScaleBg * _bgInfo, bool _loadScale)
{
	gSP.bgImage.address = RSP_SegmentToPhysical( _bgInfo->imagePtr );

	gSP.bgImage.width = _bgInfo->imageW >> 2;
	gSP.bgImage.height = _bgInfo->imageH >> 2;
	gSP.bgImage.format = _bgInfo->imageFmt;
	gSP.bgImage.size = _bgInfo->imageSiz;
	gSP.bgImage.palette = _bgInfo->imagePal;
	gDP.tiles[0].textureMode = TEXTUREMODE_BGIMAGE;
	gSP.bgImage.imageX = _FIXED2FLOAT( _bgInfo->imageX, 5 );
	gSP.bgImage.imageY = _FIXED2FLOAT( _bgInfo->imageY, 5 );
	if (_loadScale) {
		gSP.bgImage.scaleW = _FIXED2FLOAT( _bgInfo->scaleW, 10 );
		gSP.bgImage.scaleH = _FIXED2FLOAT( _bgInfo->scaleH, 10 );
	} else
		gSP.bgImage.scaleW = gSP.bgImage.scaleH = 1.0f;

	if (config.frameBufferEmulation.enable)
	{
		FrameBuffer *pBuffer = frameBufferList().findBuffer(gSP.bgImage.address);
		if ((pBuffer != NULL) &&
			((*(u32*)&RDRAM[pBuffer->m_startAddress] & 0xFFFEFFFE) == (pBuffer->m_startAddress & 0xFFFEFFFE)))
		{
			gDP.tiles[0].frameBuffer = pBuffer;
			gDP.tiles[0].textureMode = TEXTUREMODE_FRAMEBUFFER_BG;
			gDP.tiles[0].loadType = LOADTYPE_TILE;
			gDP.changed |= CHANGED_TMEM;
		}
	}
}

void gSPBgRect1Cyc( u32 bg )
{
	u32 address = RSP_SegmentToPhysical( bg );
	uObjScaleBg *objScaleBg = (uObjScaleBg*)&RDRAM[address];
	loadBGImage(objScaleBg, true);

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != NULL)
		_copyDepthBuffer();
	// Zelda MM uses depth buffer copy in LoT and in pause screen.
	// In later case depth buffer is used as temporal color buffer, and usual rendering must be used.
	// Since both situations are hard to distinguish, do the both depth buffer copy and bg rendering.
#endif // GL_IMAGE_TEXTURES_SUPPORT

	f32 imageX = gSP.bgImage.imageX;
	f32 imageY = gSP.bgImage.imageY;
	f32 imageW = _FIXED2FLOAT( objScaleBg->imageW, 2);
	f32 imageH = _FIXED2FLOAT( objScaleBg->imageH, 2);

	f32 frameX = _FIXED2FLOAT( objScaleBg->frameX, 2 );
	f32 frameY = _FIXED2FLOAT( objScaleBg->frameY, 2 );
	f32 frameW = _FIXED2FLOAT( objScaleBg->frameW, 2 );
	f32 frameH = _FIXED2FLOAT( objScaleBg->frameH, 2 );
	f32 scaleW = gSP.bgImage.scaleW;
	f32 scaleH = gSP.bgImage.scaleH;

	f32 frameX0 = frameX;
	f32 frameY0 = frameY;
	f32 frameS0 = imageX;
	f32 frameT0 = imageY;

	f32 frameX1 = frameX + min( (imageW - imageX) / scaleW, frameW );
	f32 frameY1 = frameY + min( (imageH - imageY) / scaleH, frameH );
//	f32 frameS1 = imageX + min( (imageW - imageX) * scaleW, frameW * scaleW );
//	f32 frameT1 = imageY + min( (imageH - imageY) * scaleH, frameH * scaleH );

	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

//	gDPTextureRectangle( frameX0, frameY0, frameX1 - 1, frameY1 - 1, 0, frameS0 - 1, frameT0 - 1, scaleW, scaleH );
	gDPTextureRectangle( frameX0, frameY0, frameX1, frameY1, 0, frameS0, frameT0, scaleW, scaleH );

	/*
	if ((frameX1 - frameX0) < frameW)
	{
		f32 frameX2 = frameW - (frameX1 - frameX0) + frameX1;
		gDPTextureRectangle( frameX1, frameY0, frameX2, frameY1, 0, 0, frameT0, scaleW, scaleH );
	}

	if ((frameY1 - frameY0) < frameH)
	{
		f32 frameY2 = frameH - (frameY1 - frameY0) + frameY1;
		gDPTextureRectangle( frameX0, frameY1, frameX1, frameY2, 0, frameS0, 0, scaleW, scaleH );
	}
	*/
//	gDPTextureRectangle( 0, 0, 319, 239, 0, 0, 0, scaleW, scaleH );
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
	uObjScaleBg *objBg = (uObjScaleBg*)&RDRAM[address];
	loadBGImage(objBg, false);

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (gSP.bgImage.address == gDP.depthImageAddress || depthBufferList().findBuffer(gSP.bgImage.address) != NULL)
		_copyDepthBuffer();
	// See comment to gSPBgRect1Cyc
#endif // GL_IMAGE_TEXTURES_SUPPORT

	f32 frameX = objBg->frameX / 4.0f;
	f32 frameY = objBg->frameY / 4.0f;
	u16 frameW = objBg->frameW >> 2;
	u16 frameH = objBg->frameH >> 2;

	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	gDPTextureRectangle( frameX, frameY, frameX + frameW - 1, frameY + frameH - 1, 0, gSP.bgImage.imageX, gSP.bgImage.imageY, 4, 1 );
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

	if ((gSP.status[objTxtr->block.sid >> 2] & objTxtr->block.mask) != objTxtr->block.flag) {
		switch (objTxtr->block.type) {
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

	s32 v0=0,v1=1,v2=2,v3=3;

	OGLRender & render = video().getRender();
#ifdef __TRIBUFFER_OPT
	v0 = render.getIndexmap(v0);
	v1 = render.getIndexmap(v1);
	v2 = render.getIndexmap(v2);
	v3 = render.getIndexmap(v3);
#endif

	SPVertex & vtx0 = render.getVertex(v0);
	vtx0.x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
	vtx0.y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
	vtx0.z = 0.0f;
	vtx0.w = 1.0f;
	vtx0.s = 0.0f;
	vtx0.t = 0.0f;
	SPVertex & vtx1 = render.getVertex(v1);
	vtx1.x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
	vtx1.y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
	vtx1.z = 0.0f;
	vtx1.w = 1.0f;
	vtx1.s = imageW - 1;
	vtx1.t = 0.0f;
	SPVertex & vtx2 = render.getVertex(v2);
	vtx2.x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
	vtx2.y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
	vtx2.z = 0.0f;
	vtx2.w = 1.0f;
	vtx2.s = imageW - 1;
	vtx2.t = imageH - 1;
	SPVertex & vtx3 = render.getVertex(v3);
	vtx3.x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
	vtx3.y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
	vtx3.z = 0.0f;
	vtx3.w = 1.0f;
	vtx3.s = 0;
	vtx3.t = imageH - 1;

	gDPSetTile( objSprite->imageFmt, objSprite->imageSiz, objSprite->imageStride, objSprite->imageAdrs, 0, objSprite->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( 0, 0, 0, (imageW - 1) << 2, (imageH - 1) << 2 );
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	const FrameBufferList & fbList = frameBufferList();
	FrameBuffer * pBuffer = fbList.getCurrent();
	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	vtx0.x = 2.0f * scaleX * vtx0.x - 1.0f;
	vtx0.y = -2.0f * scaleY * vtx0.y + 1.0f;
	vtx0.z = -1.0f;
	vtx0.w = 1.0f;
	vtx1.x = 2.0f * scaleX * vtx1.x - 1.0f;
	vtx1.y = -2.0f * scaleY * vtx1.y + 1.0f;
	vtx1.z = -1.0f;
	vtx1.w = 1.0f;
	vtx2.x = 2.0f * scaleX * vtx2.x - 1.0f;
	vtx2.y = -2.0f * scaleY * vtx2.y + 1.0f;
	vtx2.z = -1.0f;
	vtx2.w = 1.0f;
	vtx3.x = 2.0f * scaleX * vtx3.x - 1.0f;
	vtx3.y = -2.0f * scaleY * vtx3.y + 1.0f;
	vtx3.z = -1.0f;
	vtx3.w = 1.0f;

	render.addTriangle(v0, v1, v2);
	render.addTriangle(v0, v2, v3);
	render.drawTriangles();

	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = (u32)(max( gDP.colorImage.height, (u32)gDP.scissor.lry ));
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

#ifdef __VEC4_OPT
void (*gSPTransformVertex4)(u32 v, float mtx[4][4]) =
		gSPTransformVertex4_default;
void (*gSPTransformNormal4)(u32 v, float mtx[4][4]) =
		gSPTransformNormal4_default;
void (*gSPLightVertex4)(u32 v) = gSPLightVertex4_default;
void (*gSPPointLightVertex4)(u32 v, float _vPos[4][3]) = gSPPointLightVertex4_default;
void (*gSPBillboardVertex4)(u32 v) = gSPBillboardVertex4_default;
#endif
void (*gSPTransformVertex)(float vtx[4], float mtx[4][4]) =
		gSPTransformVertex_default;
void (*gSPLightVertex)(SPVertex & _vtx) = gSPLightVertex_default;
void (*gSPPointLightVertex)(SPVertex & _vtx, float * _vPos) = gSPPointLightVertex_default;
void (*gSPBillboardVertex)(u32 v, u32 i) = gSPBillboardVertex_default;
