#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include "OpenGL.h"

//// paulscode, added for SDL linkage:
#if defined(GLES2)
#include "ae_bridge.h"
#endif // GLES2
////

#include "GLideN64.h"
#include "Types.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "VI.h"
#include "Config.h"
#include "Log.h"

GLInfo OGL;

void OGL_InitExtensions()
{
	const char *version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	u32 uVersion = atol(version);


	if (glGenFramebuffers != NULL)
		OGL.framebufferMode = GLInfo::fbFBO;
	else
		OGL.framebufferMode = GLInfo::fbNone;

#ifndef GLES2
	OGL.bImageTexture = (uVersion >= 4) && (glBindImageTexture != NULL);
#else
	OGL.bImageTexture = false;
#endif
}

void OGL_InitStates()
{
	glEnable( GL_CULL_FACE );
	glEnableVertexAttribArray( SC_POSITION );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_ALWAYS );
	glDepthMask( GL_FALSE );
	glEnable( GL_SCISSOR_TEST );

	if (config.frameBufferEmulation.N64DepthCompare) {
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_POLYGON_OFFSET_FILL );
		glDepthFunc( GL_ALWAYS );
		glDepthMask( FALSE );
	} else
		glPolygonOffset( -3.0f, -3.0f );

	glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	srand( time(NULL) );

	OGL_SwapBuffers();
}

void OGL_UpdateScale()
{
	OGL.scaleX = OGL.width / (float)VI.width;
	OGL.scaleY = OGL.height / (float)VI.height;
}

void OGL_InitData()
{
	OGL_InitGLFunctions();
	OGL_InitExtensions();
	OGL_InitStates();

	textureCache().init();
	DepthBuffer_Init();
	FrameBuffer_Init();
	Combiner_Init();
	OGL.renderState = GLInfo::rsNone;

	gSP.changed = gDP.changed = 0xFFFFFFFF;
	OGL.captureScreen = false;

	memset(OGL.triangles.vertices, 0, VERTBUFF_SIZE * sizeof(SPVertex));
	memset(OGL.triangles.elements, 0, ELEMBUFF_SIZE * sizeof(GLubyte));
	OGL.triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	__indexmap_init();
#endif
}

void OGL_DestroyData()
{
	OGL.renderState = GLInfo::rsNone;
	Combiner_Destroy();
	FrameBuffer_Destroy();
	DepthBuffer_Destroy();
	textureCache().destroy();
}

void OGL_UpdateCullFace()
{
	if (gSP.geometryMode & G_CULL_BOTH)
	{
		glEnable( GL_CULL_FACE );

		if (gSP.geometryMode & G_CULL_BACK)
			glCullFace( GL_BACK );
		else
			glCullFace( GL_FRONT );
	}
	else
		glDisable( GL_CULL_FACE );
}

void OGL_UpdateViewport()
{
	if (!frameBufferList().isFboMode())
		glViewport( gSP.viewport.x * OGL.scaleX, (VI.height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY + OGL.heightOffset,
					gSP.viewport.width * OGL.scaleX, gSP.viewport.height * OGL.scaleY );
	else
		glViewport( gSP.viewport.x * OGL.scaleX, (frameBufferList().getCurrent()->m_height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY,
					gSP.viewport.width * OGL.scaleX, gSP.viewport.height * OGL.scaleY );
}

void OGL_UpdateDepthUpdate()
{
	if (gDP.otherMode.depthUpdate)
		glDepthMask( TRUE );
	else
		glDepthMask( FALSE );
}

//copied from RICE VIDEO
void OGL_SetBlendMode()
{
#define BLEND_NOOP              0x0000
#define BLEND_NOOP5             0xcc48  // Fog * 0 + Mem * 1
#define BLEND_NOOP4             0xcc08  // Fog * 0 + In * 1
#define BLEND_FOG_ASHADE        0xc800
#define BLEND_FOG_3             0xc000  // Fog * AIn + In * 1-A
#define BLEND_FOG_MEM           0xc440  // Fog * AFog + Mem * 1-A
#define BLEND_FOG_APRIM         0xc400  // Fog * AFog + In * 1-A
#define BLEND_BLENDCOLOR        0x8c88
#define BLEND_BI_AFOG           0x8400  // Bl * AFog + In * 1-A
#define BLEND_BI_AIN            0x8040  // Bl * AIn + Mem * 1-A
#define BLEND_MEM               0x4c40  // Mem*0 + Mem*(1-0)?!
#define BLEND_FOG_MEM_3         0x44c0  // Mem * AFog + Fog * 1-A
#define BLEND_NOOP3             0x0c48  // In * 0 + Mem * 1
#define BLEND_PASS              0x0c08  // In * 0 + In * 1
#define BLEND_FOG_MEM_IN_MEM    0x0440  // In * AFog + Mem * 1-A
#define BLEND_FOG_MEM_FOG_MEM   0x04c0  // In * AFog + Fog * 1-A
#define BLEND_OPA               0x0044  //  In * AIn + Mem * AMem
#define BLEND_XLU               0x0040
#define BLEND_MEM_ALPHA_IN      0x4044  //  Mem * AIn + Mem * AMem

	u32 blender = gDP.otherMode.l >> 16;
	u32 blendmode_1 = blender&0xcccc;
	u32 blendmode_2 = blender&0x3333;

	glEnable(GL_BLEND);
	switch(gDP.otherMode.cycleType)
	{
		case G_CYC_FILL:
			glDisable(GL_BLEND);
			break;

		case G_CYC_COPY:
			glBlendFunc(GL_ONE, GL_ZERO);
			break;

		case G_CYC_2CYCLE:
			if (gDP.otherMode.forceBlender && gDP.otherMode.depthCompare)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			}

			switch(blendmode_1+blendmode_2)
			{
				case BLEND_PASS+(BLEND_PASS>>2):    // In * 0 + In * 1
				case BLEND_FOG_APRIM+(BLEND_PASS>>2):
				case BLEND_FOG_MEM_FOG_MEM + (BLEND_OPA>>2):
				case BLEND_FOG_APRIM + (BLEND_OPA>>2):
				case BLEND_FOG_ASHADE + (BLEND_OPA>>2):
				case BLEND_BI_AFOG + (BLEND_OPA>>2):
				case BLEND_FOG_ASHADE + (BLEND_NOOP>>2):
				case BLEND_NOOP + (BLEND_OPA>>2):
				case BLEND_NOOP4 + (BLEND_NOOP>>2):
				case BLEND_FOG_ASHADE+(BLEND_PASS>>2):
				case BLEND_FOG_3+(BLEND_PASS>>2):
					glDisable(GL_BLEND);
					break;

				case BLEND_PASS+(BLEND_OPA>>2):
					if (gDP.otherMode.cvgXAlpha && gDP.otherMode.alphaCvgSel)
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					else
						glDisable(GL_BLEND);
					break;

				case BLEND_PASS + (BLEND_XLU>>2):
				case BLEND_FOG_ASHADE + (BLEND_XLU>>2):
				case BLEND_FOG_APRIM + (BLEND_XLU>>2):
				case BLEND_FOG_MEM_FOG_MEM + (BLEND_PASS>>2):
				case BLEND_XLU + (BLEND_XLU>>2):
				case BLEND_BI_AFOG + (BLEND_XLU>>2):
				case BLEND_XLU + (BLEND_FOG_MEM_IN_MEM>>2):
				case BLEND_PASS + (BLEND_FOG_MEM_IN_MEM>>2):
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;

				case BLEND_FOG_ASHADE+0x0301:
					glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
					break;

				case 0x0c08+0x1111:
					glBlendFunc(GL_ZERO, GL_DST_ALPHA);
					break;

				default:
					if (blendmode_2 == (BLEND_PASS>>2))
						glDisable(GL_BLEND);
					else
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				}
				break;

	default:

		if (gDP.otherMode.forceBlender && gDP.otherMode.depthCompare && blendmode_1 != BLEND_FOG_ASHADE )
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		}

		switch (blendmode_1)
		{
			case BLEND_XLU:
			case BLEND_BI_AIN:
			case BLEND_FOG_MEM:
			case BLEND_FOG_MEM_IN_MEM:
			case BLEND_BLENDCOLOR:
			case 0x00c0:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case BLEND_MEM_ALPHA_IN:
				glBlendFunc(GL_ZERO, GL_DST_ALPHA);
				break;

			case BLEND_OPA:
				glDisable(GL_BLEND);
				break;

			case BLEND_PASS:
			case BLEND_NOOP:
			case BLEND_FOG_ASHADE:
			case BLEND_FOG_MEM_3:
			case BLEND_BI_AFOG:
				glDisable(GL_BLEND);
				break;

			case BLEND_FOG_APRIM:
				glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ZERO);
				break;

			case BLEND_NOOP3:
			case BLEND_NOOP5:
			case BLEND_MEM:
				glBlendFunc(GL_ZERO, GL_ONE);
				break;

			default:
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

}

void OGL_UpdateStates()
{

	if (gDP.otherMode.cycleType == G_CYC_COPY)
		CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
	else if (gDP.otherMode.cycleType == G_CYC_FILL)
		CombinerInfo::get().setCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE));
	else
		CombinerInfo::get().setCombine(gDP.combine.mux);

	if (gSP.changed & CHANGED_GEOMETRYMODE)
		OGL_UpdateCullFace();

	if (gSP.changed & CHANGED_LIGHT)
		currentCombiner()->updateLight();

	if (config.frameBufferEmulation.N64DepthCompare) {
		glDisable( GL_DEPTH_TEST );
		glDepthMask( FALSE );
	} else {
		if (gSP.geometryMode & G_ZBUFFER)
			glEnable( GL_DEPTH_TEST );
		else
			glDisable( GL_DEPTH_TEST );

		if ((gDP.changed & CHANGED_RENDERMODE) > 0) {
			if (gDP.otherMode.depthCompare)
				glDepthFunc( GL_LEQUAL );
			else
				glDepthFunc( GL_ALWAYS );

			OGL_UpdateDepthUpdate();

			if (gDP.otherMode.depthMode == ZMODE_DEC)
				glEnable( GL_POLYGON_OFFSET_FILL );
			else
				glDisable( GL_POLYGON_OFFSET_FILL );
		}
	}

	if ((gDP.changed & (CHANGED_ALPHACOMPARE|CHANGED_RENDERMODE|CHANGED_BLENDCOLOR)) != 0)
		currentCombiner()->updateAlphaTestInfo();

	if (gDP.changed & CHANGED_SCISSOR)
	{
		FrameBufferList & fbList = frameBufferList();
		const u32 screenHeight = (fbList.getCurrent() == NULL || fbList.getCurrent()->m_height == 0 || !fbList.isFboMode()) ? VI.height : fbList.getCurrent()->m_height;
		glScissor( gDP.scissor.ulx * OGL.scaleX, (screenHeight - gDP.scissor.lry) * OGL.scaleY + (fbList.isFboMode() ? 0 : OGL.heightOffset),
			(gDP.scissor.lrx - gDP.scissor.ulx) * OGL.scaleX, (gDP.scissor.lry - gDP.scissor.uly) * OGL.scaleY );
	}

	if (gSP.changed & CHANGED_VIEWPORT)
		OGL_UpdateViewport();

	if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM))
	{
		//For some reason updating the texture cache on the first frame of LOZ:OOT causes a NULL Pointer exception...
		if (currentCombiner() != NULL)
		{
			if (currentCombiner()->usesT0())
				textureCache().update(0);
			else
				textureCache().activateDummy(0);

			//Note: enabling dummies makes some F-zero X textures flicker.... strange.

			if (currentCombiner()->usesT1())
				textureCache().update(1);
			else
				textureCache().activateDummy(1);
			currentCombiner()->updateTextureInfo(true);
		}
	}

	if (gDP.changed & CHANGED_FB_TEXTURE)
		currentCombiner()->updateFBInfo(true);

	if ((gDP.changed & CHANGED_RENDERMODE) || (gSP.geometryMode & G_ZBUFFER))
		currentCombiner()->updateDepthInfo(true);

	if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
	{
#define OLD_BLENDMODE
#ifndef OLD_BLENDMODE
		OGL_SetBlendMode();
#else
		if ((gDP.otherMode.forceBlender) &&
			(gDP.otherMode.cycleType != G_CYC_COPY) &&
			(gDP.otherMode.cycleType != G_CYC_FILL) &&
			!(gDP.otherMode.alphaCvgSel))
		{
			glEnable( GL_BLEND );

			switch (gDP.otherMode.l >> 16)
			{
				case 0x0448: // Add
				case 0x055A:
					glBlendFunc( GL_ONE, GL_ONE );
					break;
				case 0x0C08: // 1080 Sky
				case 0x0F0A: // Used LOTS of places
					glBlendFunc( GL_ONE, GL_ZERO );
					break;

				case 0x0040: // Fzero
				case 0xC810: // Blends fog
				case 0xC811: // Blends fog
				case 0x0C18: // Standard interpolated blend
				case 0x0C19: // Used for antialiasing
				case 0x0050: // Standard interpolated blend
				case 0x0055: // Used for antialiasing
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;

				case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
				case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
				case 0xAF50: // LOT in Zelda: MM
				case 0x0F5A: // LOT in Zelda: MM
					//clr_in * 0 + clr_mem * 1
					glBlendFunc( GL_ZERO, GL_ONE );
					break;

				default:
					LOG(LOG_VERBOSE, "Unhandled blend mode=%x", gDP.otherMode.l >> 16);
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;
			}
		}
		else
		{
			glDisable( GL_BLEND );
		}
#endif
	}

	gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
	gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGL_AddTriangle(int v0, int v1, int v2)
{
	OGL.triangles.elements[OGL.triangles.num++] = v0;
	OGL.triangles.elements[OGL.triangles.num++] = v1;
	OGL.triangles.elements[OGL.triangles.num++] = v2;
}

void OGL_SetColorArray()
{
	if (currentCombiner()->usesShadeColor())
		glEnableVertexAttribArray(SC_COLOR);
	else
		glDisableVertexAttribArray(SC_COLOR);
}

void OGL_SetTexCoordArrays()
{
	if (currentCombiner()->usesT0())
		glEnableVertexAttribArray(SC_TEXCOORD0);
	else
		glDisableVertexAttribArray(SC_TEXCOORD0);

	if (currentCombiner()->usesT1())
		glEnableVertexAttribArray(SC_TEXCOORD1);
	else
		glDisableVertexAttribArray(SC_TEXCOORD1);

	if (OGL.renderState == GLInfo::rsTriangle && (currentCombiner()->usesT0() || currentCombiner()->usesT1()))
		glEnableVertexAttribArray(SC_STSCALED);
	else
		glDisableVertexAttribArray(SC_STSCALED);
}

void OGL_DrawTriangles()
{
	if (OGL.triangles.num == 0) return;

#ifndef GLES2
	if (OGL.bImageTexture)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif

	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	const bool updateArrays = OGL.renderState != GLInfo::rsTriangle;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		OGL.renderState = GLInfo::rsTriangle;
		OGL_SetColorArray();
		OGL_SetTexCoordArrays();
		glDisableVertexAttribArray(SC_TEXCOORD1);
	}

	if (updateArrays) {
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].r);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].s);
		glVertexAttribPointer(SC_STSCALED, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].st_scaled);
		if (config.enableHWLighting) {
			glEnableVertexAttribArray(SC_NUMLIGHTS);
			glVertexAttribPointer(SC_NUMLIGHTS, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].HWLight);
		}

		OGL_UpdateCullFace();
		OGL_UpdateViewport();
		glEnable(GL_SCISSOR_TEST);
		currentCombiner()->updateRenderState();
	}

	currentCombiner()->updateColors(true);
	currentCombiner()->updateLight(true);
	glDrawElements(GL_TRIANGLES, OGL.triangles.num, GL_UNSIGNED_BYTE, OGL.triangles.elements);
	OGL.triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	__indexmap_clear();
#endif
}

void OGL_DrawLine(int v0, int v1, float width )
{
	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	if (OGL.renderState != GLInfo::rsLine || CombinerInfo::get().isChanged()) {
		OGL_SetColorArray();
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &OGL.triangles.vertices[0].r);

		OGL_UpdateCullFace();
		OGL_UpdateViewport();
		OGL.renderState = GLInfo::rsLine;
		currentCombiner()->updateRenderState();
	}

	unsigned short elem[2];
	elem[0] = v0;
	elem[1] = v1;
	glLineWidth( width * OGL.scaleX );
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, elem);
}

void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color )
{
	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	const bool updateArrays = OGL.renderState != GLInfo::rsRect;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		OGL.renderState = GLInfo::rsRect;
		glDisableVertexAttribArray(SC_COLOR);
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glDisableVertexAttribArray(SC_STSCALED);
	}

	if (updateArrays) {
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttrib4f(SC_POSITION, 0, 0, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0);
		glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
		currentCombiner()->updateRenderState();
	}

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	if (!fbList.isFboMode())
		glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );
	else {
		glViewport( 0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY );
	}
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);

	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	OGL.rect[0].x = (float) ulx * (2.0f * scaleX) - 1.0;
	OGL.rect[0].y = (float) uly * (-2.0f * scaleY) + 1.0;
	OGL.rect[1].x = (float) (lrx+1) * (2.0f * scaleX) - 1.0;
	OGL.rect[1].y = OGL.rect[0].y;
	OGL.rect[2].x = OGL.rect[0].x;
	OGL.rect[2].y = (float) (lry+1) * (-2.0f * scaleY) + 1.0;
	OGL.rect[3].x = OGL.rect[1].x;
	OGL.rect[3].y = OGL.rect[2].y;

	glVertexAttrib4fv(SC_COLOR, color);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_SCISSOR_TEST);
	OGL_UpdateViewport();
}

void GLS_SetShadowMapCombiner();
void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip )
{
	if (gSP.changed || gDP.changed)
		OGL_UpdateStates();

	const bool updateArrays = OGL.renderState != GLInfo::rsTexRect;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		OGL.renderState = GLInfo::rsTexRect;
		glDisableVertexAttribArray(SC_COLOR);
		OGL_SetTexCoordArrays();
	}

	if (updateArrays) {
#ifdef RENDERSTATE_TEST
		StateChanges++;
#endif
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttrib4f(SC_POSITION, 0, 0, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0);
		glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s0);
		glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s1);
		currentCombiner()->updateRenderState();
	}

#ifndef GLES2
	//	if ((gDP.otherMode.l >> 16) == 0x3c18 && gDP.combine.muxs0 == 0x00ffffff && gDP.combine.muxs1 == 0xfffff238) //depth image based fog
	if (gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress +  gDP.colorImage.width*gDP.colorImage.width*6/4))
		GLS_SetShadowMapCombiner();
#endif // GLES2

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	if (!fbList.isFboMode())
		glViewport( 0, OGL.heightOffset, OGL.width, OGL.height );
	else
		glViewport( 0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY );
	glDisable( GL_CULL_FACE );

	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	OGL.rect[0].x = (float) ulx * (2.0f * scaleX) - 1.0f;
	OGL.rect[0].y = (float) uly * (-2.0f * scaleY) + 1.0f;
	OGL.rect[1].x = (float) (lrx) * (2.0f * scaleX) - 1.0f;
	OGL.rect[1].y = OGL.rect[0].y;
	OGL.rect[2].x = OGL.rect[0].x;
	OGL.rect[2].y = (float) (lry) * (-2.0f * scaleY) + 1.0f;
	OGL.rect[3].x = OGL.rect[1].x;
	OGL.rect[3].y = OGL.rect[2].y;

	TextureCache & cache = textureCache();
	if (currentCombiner()->usesT0() && cache.current[0] && gSP.textureTile[0]) {
		OGL.rect[0].s0 = uls * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		OGL.rect[0].t0 = ult * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
		OGL.rect[3].s0 = (lrs + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		OGL.rect[3].t0 = (lrt + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

		if ((cache.current[0]->maskS) && !(cache.current[0]->mirrorS) && (fmod( OGL.rect[0].s0, cache.current[0]->width ) == 0.0f)) {
			OGL.rect[3].s0 -= OGL.rect[0].s0;
			OGL.rect[0].s0 = 0.0f;
		}

		if ((cache.current[0]->maskT)  && !(cache.current[0]->mirrorT) && (fmod( OGL.rect[0].t0, cache.current[0]->height ) == 0.0f)) {
			OGL.rect[3].t0 -= OGL.rect[0].t0;
			OGL.rect[0].t0 = 0.0f;
		}

		if (cache.current[0]->frameBufferTexture)
		{
			OGL.rect[0].s0 = cache.current[0]->offsetS + OGL.rect[0].s0;
			OGL.rect[0].t0 = cache.current[0]->offsetT - OGL.rect[0].t0;
			OGL.rect[3].s0 = cache.current[0]->offsetS + OGL.rect[3].s0;
			OGL.rect[3].t0 = cache.current[0]->offsetT - OGL.rect[3].t0;
		}

		glActiveTexture( GL_TEXTURE0 );

		if ((OGL.rect[0].s0 >= 0.0f) && (OGL.rect[3].s0 <= cache.current[0]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((OGL.rect[0].t0 >= 0.0f) && (OGL.rect[3].t0 <= cache.current[0]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		OGL.rect[0].s0 *= cache.current[0]->scaleS;
		OGL.rect[0].t0 *= cache.current[0]->scaleT;
		OGL.rect[3].s0 *= cache.current[0]->scaleS;
		OGL.rect[3].t0 *= cache.current[0]->scaleT;
	}

	if (currentCombiner()->usesT1() && cache.current[1] && gSP.textureTile[1])
	{
		OGL.rect[0].s1 = uls * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		OGL.rect[0].t1 = ult * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
		OGL.rect[3].s1 = (lrs + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		OGL.rect[3].t1 = (lrt + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

		if ((cache.current[1]->maskS) && (fmod( OGL.rect[0].s1, cache.current[1]->width ) == 0.0f) && !(cache.current[1]->mirrorS))
		{
			OGL.rect[3].s1 -= OGL.rect[0].s1;
			OGL.rect[0].s1 = 0.0f;
		}

		if ((cache.current[1]->maskT) && (fmod( OGL.rect[0].t1, cache.current[1]->height ) == 0.0f) && !(cache.current[1]->mirrorT))
		{
			OGL.rect[3].t1 -= OGL.rect[0].t1;
			OGL.rect[0].t1 = 0.0f;
		}

		if (cache.current[1]->frameBufferTexture)
		{
			OGL.rect[0].s1 = cache.current[1]->offsetS + OGL.rect[0].s1;
			OGL.rect[0].t1 = cache.current[1]->offsetT - OGL.rect[0].t1;
			OGL.rect[3].s1 = cache.current[1]->offsetS + OGL.rect[3].s1;
			OGL.rect[3].t1 = cache.current[1]->offsetT - OGL.rect[3].t1;
		}

		glActiveTexture( GL_TEXTURE1 );

		if ((OGL.rect[0].s1 == 0.0f) && (OGL.rect[3].s1 <= cache.current[1]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((OGL.rect[0].t1 == 0.0f) && (OGL.rect[3].t1 <= cache.current[1]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		OGL.rect[0].s1 *= cache.current[1]->scaleS;
		OGL.rect[0].t1 *= cache.current[1]->scaleT;
		OGL.rect[3].s1 *= cache.current[1]->scaleS;
		OGL.rect[3].t1 *= cache.current[1]->scaleT;
	}

	if ((gDP.otherMode.cycleType == G_CYC_COPY) && !config.texture.forceBilinear)
	{
		glActiveTexture( GL_TEXTURE0 );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	if (flip)
	{
		OGL.rect[1].s0 = OGL.rect[0].s0;
		OGL.rect[1].t0 = OGL.rect[3].t0;
		OGL.rect[1].s1 = OGL.rect[0].s1;
		OGL.rect[1].t1 = OGL.rect[3].t1;
		OGL.rect[2].s0 = OGL.rect[3].s0;
		OGL.rect[2].t0 = OGL.rect[0].t0;
		OGL.rect[2].s1 = OGL.rect[3].s1;
		OGL.rect[2].t1 = OGL.rect[0].t1;
	}
	else
	{
		OGL.rect[1].s0 = OGL.rect[3].s0;
		OGL.rect[1].t0 = OGL.rect[0].t0;
		OGL.rect[1].s1 = OGL.rect[3].s1;
		OGL.rect[1].t1 = OGL.rect[0].t1;
		OGL.rect[2].s0 = OGL.rect[0].s0;
		OGL.rect[2].t0 = OGL.rect[3].t0;
		OGL.rect[2].s1 = OGL.rect[0].s1;
		OGL.rect[2].t1 = OGL.rect[3].t1;
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	OGL_UpdateViewport();
}

void OGL_ClearDepthBuffer()
{
	if (config.frameBufferEmulation.enable && frameBufferList().getCurrent() == NULL)
		return;

	depthBufferList().clearBuffer();

	OGL_UpdateStates();
	glDisable( GL_SCISSOR_TEST );
	glDepthMask( TRUE );
	glClear( GL_DEPTH_BUFFER_BIT );

	OGL_UpdateDepthUpdate();

	glEnable( GL_SCISSOR_TEST );
}

void OGL_ClearColorBuffer( float *color )
{
	glDisable( GL_SCISSOR_TEST );

	glClearColor( color[0], color[1], color[2], color[3] );
	glClear( GL_COLOR_BUFFER_BIT );

	glEnable( GL_SCISSOR_TEST );
}

void OGL_ReadScreen( void **dest, long *width, long *height )
{
	*width = OGL.width;
	*height = OGL.height;

	*dest = malloc( OGL.height * OGL.width * 3 );
	if (*dest == NULL)
		return;

#ifndef GLES2
	const GLenum format = GL_BGR_EXT;
	glReadBuffer( GL_FRONT );
#else
	const GLenum format = GL_RGB;
#endif
	glReadPixels( 0, OGL.heightOffset, OGL.width, OGL.height, format, GL_UNSIGNED_BYTE, *dest );
}

bool checkFBO() {
	GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (e) {
//		case GL_FRAMEBUFFER_UNDEFINED:
//			printf("FBO Undefined\n");
//			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Incomplete Attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Missing Attachment\n");
			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
//			printf("FBO Incomplete Draw Buffer\n");
//			break;
		case GL_FRAMEBUFFER_UNSUPPORTED :
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Unsupported\n");
			break;
		case GL_FRAMEBUFFER_COMPLETE:
			LOG(LOG_VERBOSE, "[gles2GlideN64]: FBO OK\n");
			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
//			printf("framebuffer FRAMEBUFFER_DIMENSIONS\n");
//			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
//			printf("framebuffer INCOMPLETE_FORMATS\n");
//			break;
		default:
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Problem?\n");
	}
	return e == GL_FRAMEBUFFER_COMPLETE;
}

const char* GLErrorString(GLenum errorCode)
{
	static const struct {
		GLenum code;
		const char *string;
	} errors[]=
	{
		/* GL */
	{GL_NO_ERROR, "no error"},
	{GL_INVALID_ENUM, "invalid enumerant"},
	{GL_INVALID_VALUE, "invalid value"},
	{GL_INVALID_OPERATION, "invalid operation"},
	{GL_STACK_OVERFLOW, "stack overflow"},
	{GL_STACK_UNDERFLOW, "stack underflow"},
	{GL_OUT_OF_MEMORY, "out of memory"},

	{0, NULL }
};

	int i;

	for (i=0; errors[i].string; i++)
	{
		if (errors[i].code == errorCode)
		{
			return errors[i].string;
		}
	}

	return NULL;
}

bool isGLError()
{
	GLenum errCode;
	const char* errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = GLErrorString(errCode);
		if (errString != NULL)
			fprintf (stderr, "OpenGL Error: %s\n", errString);
		return true;
	}
	return false;
}
