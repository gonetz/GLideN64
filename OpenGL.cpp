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

void OGLVideo::start()
{
	_start(); // TODO: process initialization error
	initGLFunctions();
	m_render._initData();
}

void OGLVideo::stop()
{
	m_render._destroyData();
	_stop();
}

void OGLVideo::swapBuffers()
{
	_swapBuffers();
}

void OGLVideo::setCaptureScreen(const char * const _strDirectory)
{
	m_strScreenDirectory = _strDirectory;
	m_bCaptureScreen = true;
}

void OGLVideo::saveScreenshot()
{
	if (!m_bCaptureScreen)
		return;
	_saveScreenshot();
	m_bCaptureScreen = false;
}

bool OGLVideo::changeWindow()
{
	if (!m_bToggleFullscreen)
		return false;
	m_render._destroyData();
	_changeWindow();
	m_render._initData();
	m_bToggleFullscreen = false;
	return true;
}

void OGLVideo::resizeWindow()
{
	_resizeWindow();
}

void OGLVideo::updateScale()
{
	m_scaleX = m_width / (float)VI.width;
	m_scaleY = m_height / (float)VI.height;
}

void OGLVideo::readScreen(void **_pDest, long *_pWidth, long *_pHeight )
{
	*_pWidth = m_width;
	*_pHeight = m_height;

	*_pDest = malloc( m_height * m_width * 3 );
	if (*_pDest == NULL)
		return;

#ifndef GLES2
	const GLenum format = GL_BGR_EXT;
	glReadBuffer( GL_FRONT );
#else
	const GLenum format = GL_RGB;
#endif
	glReadPixels( 0, m_heightOffset, m_width, m_height, format, GL_UNSIGNED_BYTE, *_pDest );
}

void OGLRender::addTriangle(int _v0, int _v1, int _v2)
{
	triangles.elements[triangles.num++] = _v0;
	triangles.elements[triangles.num++] = _v1;
	triangles.elements[triangles.num++] = _v2;
}

#define ORKIN_BLENDMODE
#ifdef RICE_BLENDMODE
//copied from RICE VIDEO
void OGLVideo::_setBlendMode() const
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
#else
void OGLRender::_setBlendMode() const
{
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
}
#endif

void OGLRender::_updateCullFace() const
{
	if (gSP.geometryMode & G_CULL_BOTH) {
		glEnable( GL_CULL_FACE );

		if (gSP.geometryMode & G_CULL_BACK)
			glCullFace( GL_BACK );
		else
			glCullFace( GL_FRONT );
	}
	else
		glDisable( GL_CULL_FACE );
}

void OGLRender::_updateViewport() const
{
	OGLVideo & ogl = video();
	if (!frameBufferList().isFboMode())
		glViewport(gSP.viewport.x * ogl.getScaleX(), (VI.height - (gSP.viewport.y + gSP.viewport.height)) * ogl.getScaleY() + ogl.getHeightOffset(),
			gSP.viewport.width * ogl.getScaleX(), gSP.viewport.height * ogl.getScaleY());
	else
		glViewport(gSP.viewport.x * ogl.getScaleX(), (frameBufferList().getCurrent()->m_height - (gSP.viewport.y + gSP.viewport.height)) * ogl.getScaleY(),
			gSP.viewport.width * ogl.getScaleX(), gSP.viewport.height * ogl.getScaleY());
}

void OGLRender::_updateDepthUpdate() const
{
	if (gDP.otherMode.depthUpdate)
		glDepthMask( TRUE );
	else
		glDepthMask( FALSE );
}

void OGLRender::_updateStates() const
{
	OGLVideo & ogl = video();

	CombinerInfo::get().update();

	if (gSP.changed & CHANGED_GEOMETRYMODE)
		_updateCullFace();

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

			_updateDepthUpdate();

			if (gDP.otherMode.depthMode == ZMODE_DEC)
				glEnable( GL_POLYGON_OFFSET_FILL );
			else
				glDisable( GL_POLYGON_OFFSET_FILL );
		}
	}

	if ((gDP.changed & (CHANGED_ALPHACOMPARE|CHANGED_RENDERMODE|CHANGED_BLENDCOLOR)) != 0)
		currentCombiner()->updateAlphaTestInfo();

	if (gDP.changed & CHANGED_SCISSOR) {
		FrameBufferList & fbList = frameBufferList();
		const u32 screenHeight = (fbList.getCurrent() == NULL || fbList.getCurrent()->m_height == 0 || !fbList.isFboMode()) ? VI.height : fbList.getCurrent()->m_height;
		glScissor( gDP.scissor.ulx * ogl.getScaleX(), (screenHeight - gDP.scissor.lry) * ogl.getScaleY() + (fbList.isFboMode() ? 0 : ogl.getHeightOffset()),
			(gDP.scissor.lrx - gDP.scissor.ulx) * ogl.getScaleX(), (gDP.scissor.lry - gDP.scissor.uly) * ogl.getScaleY() );
	}

	if (gSP.changed & CHANGED_VIEWPORT)
		_updateViewport();

	if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM)) {
		//For some reason updating the texture cache on the first frame of LOZ:OOT causes a NULL Pointer exception...
		if (currentCombiner() != NULL) {
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
		_setBlendMode();

	gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
	gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGLRender::_setColorArray() const
{
	if (currentCombiner()->usesShadeColor())
		glEnableVertexAttribArray(SC_COLOR);
	else
		glDisableVertexAttribArray(SC_COLOR);
}

void OGLRender::_setTexCoordArrays() const
{
	if (currentCombiner()->usesT0())
		glEnableVertexAttribArray(SC_TEXCOORD0);
	else
		glDisableVertexAttribArray(SC_TEXCOORD0);

	if (currentCombiner()->usesT1())
		glEnableVertexAttribArray(SC_TEXCOORD1);
	else
		glDisableVertexAttribArray(SC_TEXCOORD1);

	if (m_renderState == rsTriangle && (currentCombiner()->usesT0() || currentCombiner()->usesT1()))
		glEnableVertexAttribArray(SC_STSCALED);
	else
		glDisableVertexAttribArray(SC_STSCALED);
}

void OGLRender::drawTriangles()
{
	if (triangles.num == 0) return;

#ifndef GLES2
	if (m_bImageTexture)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif

	if (gSP.changed || gDP.changed)
		_updateStates();

	const bool updateArrays = m_renderState != rsTriangle;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		m_renderState = rsTriangle;
		_setColorArray();
		_setTexCoordArrays();
		glDisableVertexAttribArray(SC_TEXCOORD1);
	}

	if (updateArrays) {
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].r);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].s);
		glVertexAttribPointer(SC_STSCALED, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].st_scaled);
		if (config.enableHWLighting) {
			glEnableVertexAttribArray(SC_NUMLIGHTS);
			glVertexAttribPointer(SC_NUMLIGHTS, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].HWLight);
		}

		_updateCullFace();
		_updateViewport();
		glEnable(GL_SCISSOR_TEST);
		currentCombiner()->updateRenderState();
	}

	currentCombiner()->updateColors(true);
	currentCombiner()->updateLight(true);
	glDrawElements(GL_TRIANGLES, triangles.num, GL_UNSIGNED_BYTE, triangles.elements);
	triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	_indexmap_clear();
#endif
}

void OGLRender::drawLine(int _v0, int _v1, float _width)
{
	if (gSP.changed || gDP.changed)
		_updateStates();

	if (m_renderState != rsLine || CombinerInfo::get().isChanged()) {
		_setColorArray();
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].r);

		_updateCullFace();
		_updateViewport();
		m_renderState = rsLine;
		currentCombiner()->updateRenderState();
	}

	unsigned short elem[2];
	elem[0] = _v0;
	elem[1] = _v1;
	glLineWidth(_width * video().getScaleX());
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, elem);

}

void OGLRender::drawRect(int _ulx, int _uly, int _lrx, int _lry, float *_pColor)
{
	if (gSP.changed || gDP.changed)
		_updateStates();

	const bool updateArrays = m_renderState != rsRect;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		m_renderState = rsRect;
		glDisableVertexAttribArray(SC_COLOR);
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glDisableVertexAttribArray(SC_STSCALED);
	}

	if (updateArrays) {
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttrib4f(SC_POSITION, 0, 0, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0);
		glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
		currentCombiner()->updateRenderState();
	}

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	OGLVideo & ogl = video();
	if (!fbList.isFboMode())
		glViewport( 0, ogl.getHeightOffset(), ogl.getWidth(), ogl.getHeight());
	else {
		glViewport( 0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY );
	}
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);

	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	m_rect[0].x = (float) _ulx * (2.0f * scaleX) - 1.0;
	m_rect[0].y = (float) _uly * (-2.0f * scaleY) + 1.0;
	m_rect[1].x = (float) (_lrx+1) * (2.0f * scaleX) - 1.0;
	m_rect[1].y = m_rect[0].y;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = (float) (_lry+1) * (-2.0f * scaleY) + 1.0;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;

	glVertexAttrib4fv(SC_COLOR, _pColor);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_SCISSOR_TEST);
	_updateViewport();
}

void OGLRender::drawTexturedRect(float _ulx, float _uly, float _lrx, float _lry, float _uls, float _ult, float _lrs, float _lrt, bool _flip)
{
	if (gSP.changed || gDP.changed)
		_updateStates();

	const bool updateArrays = m_renderState != rsTexRect;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		m_renderState = rsTexRect;
		glDisableVertexAttribArray(SC_COLOR);
		_setTexCoordArrays();
	}

	if (updateArrays) {
#ifdef RENDERSTATE_TEST
		StateChanges++;
#endif
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttrib4f(SC_POSITION, 0, 0, (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz, 1.0);
		glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s0);
		glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s1);
		currentCombiner()->updateRenderState();
	}

#ifndef GLES2
	//	if ((gDP.otherMode.l >> 16) == 0x3c18 && gDP.combine.muxs0 == 0x00ffffff && gDP.combine.muxs1 == 0xfffff238) //depth image based fog
	if (gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress +  gDP.colorImage.width*gDP.colorImage.width*6/4))
		SetShadowMapCombiner();
#endif // GLES2

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	OGLVideo & ogl = video();
	if (!fbList.isFboMode())
		glViewport( 0, ogl.getHeightOffset(), ogl.getWidth(), ogl.getHeight());
	else
		glViewport( 0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY );
	glDisable( GL_CULL_FACE );

	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	m_rect[0].x = (float) _ulx * (2.0f * scaleX) - 1.0f;
	m_rect[0].y = (float) _uly * (-2.0f * scaleY) + 1.0f;
	m_rect[1].x = (float) (_lrx) * (2.0f * scaleX) - 1.0f;
	m_rect[1].y = m_rect[0].y;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = (float) (_lry) * (-2.0f * scaleY) + 1.0f;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;

	TextureCache & cache = textureCache();
	if (currentCombiner()->usesT0() && cache.current[0] && gSP.textureTile[0]) {
		m_rect[0].s0 = _uls * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		m_rect[0].t0 = _ult * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
		m_rect[3].s0 = (_lrs + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		m_rect[3].t0 = (_lrt + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

		if ((cache.current[0]->maskS) && !(cache.current[0]->mirrorS) && (fmod(m_rect[0].s0, cache.current[0]->width) == 0.0f)) {
			m_rect[3].s0 -= m_rect[0].s0;
			m_rect[0].s0 = 0.0f;
		}

		if ((cache.current[0]->maskT)  && !(cache.current[0]->mirrorT) && (fmod(m_rect[0].t0, cache.current[0]->height) == 0.0f)) {
			m_rect[3].t0 -= m_rect[0].t0;
			m_rect[0].t0 = 0.0f;
		}

		if (cache.current[0]->frameBufferTexture) {
			m_rect[0].s0 = cache.current[0]->offsetS + m_rect[0].s0;
			m_rect[0].t0 = cache.current[0]->offsetT - m_rect[0].t0;
			m_rect[3].s0 = cache.current[0]->offsetS + m_rect[3].s0;
			m_rect[3].t0 = cache.current[0]->offsetT - m_rect[3].t0;
		}

		glActiveTexture( GL_TEXTURE0 );

		if ((m_rect[0].s0 >= 0.0f) && (m_rect[3].s0 <= cache.current[0]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((m_rect[0].t0 >= 0.0f) && (m_rect[3].t0 <= cache.current[0]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		m_rect[0].s0 *= cache.current[0]->scaleS;
		m_rect[0].t0 *= cache.current[0]->scaleT;
		m_rect[3].s0 *= cache.current[0]->scaleS;
		m_rect[3].t0 *= cache.current[0]->scaleT;
	}

	if (currentCombiner()->usesT1() && cache.current[1] && gSP.textureTile[1]) {
		m_rect[0].s1 = _uls * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		m_rect[0].t1 = _ult * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
		m_rect[3].s1 = (_lrs + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		m_rect[3].t1 = (_lrt + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

		if ((cache.current[1]->maskS) && (fmod(m_rect[0].s1, cache.current[1]->width) == 0.0f) && !(cache.current[1]->mirrorS)) {
			m_rect[3].s1 -= m_rect[0].s1;
			m_rect[0].s1 = 0.0f;
		}

		if ((cache.current[1]->maskT) && (fmod(m_rect[0].t1, cache.current[1]->height ) == 0.0f) && !(cache.current[1]->mirrorT)) {
			m_rect[3].t1 -= m_rect[0].t1;
			m_rect[0].t1 = 0.0f;
		}

		if (cache.current[1]->frameBufferTexture) {
			m_rect[0].s1 = cache.current[1]->offsetS + m_rect[0].s1;
			m_rect[0].t1 = cache.current[1]->offsetT - m_rect[0].t1;
			m_rect[3].s1 = cache.current[1]->offsetS + m_rect[3].s1;
			m_rect[3].t1 = cache.current[1]->offsetT - m_rect[3].t1;
		}

		glActiveTexture( GL_TEXTURE1 );

		if ((m_rect[0].s1 == 0.0f) && (m_rect[3].s1 <= cache.current[1]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((m_rect[0].t1 == 0.0f) && (m_rect[3].t1 <= cache.current[1]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		m_rect[0].s1 *= cache.current[1]->scaleS;
		m_rect[0].t1 *= cache.current[1]->scaleT;
		m_rect[3].s1 *= cache.current[1]->scaleS;
		m_rect[3].t1 *= cache.current[1]->scaleT;
	}

	if ((gDP.otherMode.cycleType == G_CYC_COPY) && !config.texture.forceBilinear) {
		glActiveTexture( GL_TEXTURE0 );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	if (_flip) {
		m_rect[1].s0 = m_rect[0].s0;
		m_rect[1].t0 = m_rect[3].t0;
		m_rect[1].s1 = m_rect[0].s1;
		m_rect[1].t1 = m_rect[3].t1;
		m_rect[2].s0 = m_rect[3].s0;
		m_rect[2].t0 = m_rect[0].t0;
		m_rect[2].s1 = m_rect[3].s1;
		m_rect[2].t1 = m_rect[0].t1;
	} else {
		m_rect[1].s0 = m_rect[3].s0;
		m_rect[1].t0 = m_rect[0].t0;
		m_rect[1].s1 = m_rect[3].s1;
		m_rect[1].t1 = m_rect[0].t1;
		m_rect[2].s0 = m_rect[0].s0;
		m_rect[2].t0 = m_rect[3].t0;
		m_rect[2].s1 = m_rect[0].s1;
		m_rect[2].t1 = m_rect[3].t1;
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	_updateViewport();
}

void OGLRender::clearDepthBuffer()
{
	if (config.frameBufferEmulation.enable && frameBufferList().getCurrent() == NULL)
		return;

	depthBufferList().clearBuffer();

	_updateStates();
	glDisable( GL_SCISSOR_TEST );
	glDepthMask( TRUE );
	glClear( GL_DEPTH_BUFFER_BIT );

	_updateDepthUpdate();

	glEnable( GL_SCISSOR_TEST );
}

void OGLRender::clearColorBuffer(float *_pColor )
{
	glDisable( GL_SCISSOR_TEST );

	glClearColor( _pColor[0], _pColor[1], _pColor[2], _pColor[3] );
	glClear( GL_COLOR_BUFFER_BIT );

	glEnable( GL_SCISSOR_TEST );
}

void OGLRender::_initExtensions()
{
	const char *version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	u32 uVersion = atol(version);

#ifndef GLES2
	m_bImageTexture = (uVersion >= 4) && (glBindImageTexture != NULL);
#else
	bImageTexture = false;
#endif
}

void OGLRender::_initStates()
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

	OGLVideo & ogl = video();
	glViewport( 0, ogl.getHeightOffset(), ogl.getWidth(), ogl.getHeight());

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	srand( time(NULL) );

	ogl.swapBuffers();
}

void OGLRender::_initData()
{
	_initExtensions();
	_initStates();

	textureCache().init();
	DepthBuffer_Init();
	FrameBuffer_Init();
	Combiner_Init();
	m_renderState = rsNone;

	gSP.changed = gDP.changed = 0xFFFFFFFF;

	memset(triangles.vertices, 0, VERTBUFF_SIZE * sizeof(SPVertex));
	memset(triangles.elements, 0, ELEMBUFF_SIZE * sizeof(GLubyte));
	triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	_indexmap_init();
#endif
}

void OGLRender::_destroyData()
{
	m_renderState = rsNone;
	Combiner_Destroy();
	FrameBuffer_Destroy();
	DepthBuffer_Destroy();
	textureCache().destroy();
}

#ifdef __TRIBUFFER_OPT
void OGLRender::_indexmap_init()
{
	memset(triangles.indexmapinv, 0xFF, VERTBUFF_SIZE*sizeof(u32));
	for(int i = 0; i<INDEXMAP_SIZE; ++i) {
		triangles.indexmap[i] = i;
		//triangles.indexmapinv[i] = i;
	}

	triangles.indexmap_prev = -1;
	triangles.indexmap_nomap = 0;
}

void OGLRender::_indexmap_clear()
{
	memset(triangles.indexmapinv, 0xFF, VERTBUFF_SIZE * sizeof(u32));
	for(int i = 0; i < INDEXMAP_SIZE; ++i)
		triangles.indexmapinv[triangles.indexmap[i]] = i;
}

u32 OGLRender::_indexmap_findunused(u32 num)
{
	u32 c = 0;
	u32 i = std::min(triangles.indexmap_prev + 1, VERTBUFF_SIZE - 1);
	u32 n = 0;
	while(n < VERTBUFF_SIZE) {
		c = (triangles.indexmapinv[i] == 0xFFFFFFFF) ? (c + 1) : 0;
		if ((c == num) && (i < (VERTBUFF_SIZE - num)))
			break;
		i = i + 1;
		if (i >= VERTBUFF_SIZE)
			{i = 0; c = 0;}
		++n;
	}
	return (c == num) ? (i-num+1) : (0xFFFFFFFF);
}

void OGLRender::indexmapUndo()
{
	SPVertex tmp[INDEXMAP_SIZE];
	memset(triangles.indexmapinv, 0xFF, VERTBUFF_SIZE * sizeof(u32));

	for(int i=0; i < INDEXMAP_SIZE; ++i) {
		u32 ind = triangles.indexmap[i];
		tmp[i] = triangles.vertices[ind];
		triangles.indexmap[i] = i;
		triangles.indexmapinv[i] = i;
	}

	memcpy(triangles.vertices, tmp, INDEXMAP_SIZE * sizeof(SPVertex));
	triangles.indexmap_nomap = 1;
}

u32 OGLRender::getIndexmapNew(u32 _index, u32 _num)
{
	u32 ind;

	//test to see if unmapped
	u32 unmapped = 1;
	for(int i = 0; i < _num; ++i) {
		if (triangles.indexmap[i] != 0xFFFFFFFF) {
			unmapped = 0;
			break;
		}
	}

	if (unmapped)
		ind = _index;
	else {
		ind = _indexmap_findunused(_num);

		//no more room in buffer....
		if (ind > VERTBUFF_SIZE) {
			drawTriangles();
			ind = _indexmap_findunused(_num);

			//OK the indices are spread so sparsely, we cannot find a num element block.
			if (ind > VERTBUFF_SIZE) {
				indexmapUndo();
				ind = _indexmap_findunused(_num);
				if (ind > VERTBUFF_SIZE) {
					LOG(LOG_ERROR, "Could not allocate %i indices\n", _num);

					LOG(LOG_VERBOSE, "indexmap=[");
					for(int i=0;i<INDEXMAP_SIZE;i++)
						LOG(LOG_VERBOSE, "%i,", triangles.indexmap[i]);
					LOG(LOG_VERBOSE, "]\n");

					LOG(LOG_VERBOSE, "indexmapinv=[");
					for(int i=0;i<VERTBUFF_SIZE;i++)
						LOG(LOG_VERBOSE, "%i,", triangles.indexmapinv[i]);
					LOG(LOG_VERBOSE, "]\n");
				}
				return ind;
			}
		}
	}

	for(int i = 0; i < _num; ++i) {
		triangles.indexmap[_index + i] = ind + i;
		triangles.indexmapinv[ind + i] = _index + i;
	}

	triangles.indexmap_prev = ind + _num - 1;
	triangles.indexmap_nomap = 0;

	return ind;
}
#endif // __TRIBUFFER_OPT
