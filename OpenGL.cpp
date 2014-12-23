#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */

//// paulscode, added for SDL linkage:
#if defined(GLES2)
#include "ae_bridge.h"
#endif // GLES2
////

#include "Types.h"
#include "GLideN64.h"
#include "OpenGL.h"
#include "RDP.h"
#include "RSP.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "GLideNHQ/Ext_TxFilter.h"
#include "VI.h"
#include "Config.h"
#include "Log.h"
#include "TextDrawer.h"

using namespace std;

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
	gDPSetRenderMode(0, 0);
	gDPSetTextureLUT(G_TT_NONE);
	gDPSetAlphaCompare(G_AC_NONE);
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

void OGLVideo::setWindowSize(u32 _width, u32 _height)
{
	if (m_width != _width || m_height != _height) {
		m_resizeWidth = _width;
		m_resizeHeight = _height;
		m_bResizeWindow = true;
	}
}

bool OGLVideo::resizeWindow()
{
	if (!m_bResizeWindow)
		return false;
	m_render._destroyData();
	if (!_resizeWindow())
		_start();
	m_render._initData();
	m_bResizeWindow = false;
	return true;
}

void OGLVideo::updateScale()
{
	m_scaleX = m_width / (float)VI.width;
	m_scaleY = m_height / (float)VI.height;
}

void OGLVideo::_setBufferSize()
{
	if (m_bFullscreen && config.frameBufferEmulation.enable) {
		switch (config.video.aspect) {
			case 0: // stretch
			m_width = m_screenWidth;
			m_height = m_screenHeight;
			break;
			case 1: // force 4/3
			if (m_screenWidth * 3 / 4 > m_screenHeight) {
				m_height = m_screenHeight;
				m_width = m_screenHeight * 4 / 3;
			} else if (m_screenHeight * 4 / 3 > m_screenWidth) {
				m_width = m_screenWidth;
				m_height = m_screenWidth * 3 / 4;
			} else {
				m_width = m_screenWidth;
				m_height = m_screenHeight;
			}
			break;
			case 2: // force 16/9
			if (m_screenWidth * 9 / 16 > m_screenHeight) {
				m_height = m_screenHeight;
				m_width = m_screenHeight * 16 / 9;
			} else if (m_screenHeight * 16 / 9 > m_screenWidth) {
				m_width = m_screenWidth;
				m_height = m_screenWidth * 9 / 16;
			} else {
				m_width = m_screenWidth;
				m_height = m_screenHeight;
			}
			break;
			default:
			assert(false && "Unknown aspect ratio");
			m_width = m_screenWidth;
			m_height = m_screenHeight;
		}
	} else {
		m_width = m_screenWidth;
		m_height = m_screenHeight;
	}
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

void OGLVideo::readScreen2(void * _dest, int * _width, int * _height, int _front)
{
	if (_width == NULL || _height == NULL)
		return;

	*_width = m_screenWidth;
	*_height = m_screenHeight;

	if (_dest == NULL)
		return;

	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	if (_front != 0)
		glReadBuffer(GL_FRONT);
	else
		glReadBuffer(GL_BACK);
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGB, GL_UNSIGNED_BYTE, _dest);
	glReadBuffer(oldMode);
}

void OGLRender::addTriangle(int _v0, int _v1, int _v2)
{
	triangles.elements[triangles.num++] = _v0;
	triangles.elements[triangles.num++] = _v1;
	triangles.elements[triangles.num++] = _v2;

	if ((gSP.geometryMode & G_SHADE) == 0) {
		// Prim shading
		for (u32 i = triangles.num - 3; i < triangles.num; ++i) {
			SPVertex & vtx = triangles.vertices[triangles.elements[i]];
			vtx.flat_r = gDP.primColor.r;
			vtx.flat_g = gDP.primColor.g;
			vtx.flat_b = gDP.primColor.b;
			vtx.flat_a = vtx.a;
		}
	} else if ((gSP.geometryMode & G_SHADING_SMOOTH) == 0) {
		// Flat shading
		SPVertex & vtx0 = triangles.vertices[_v0];
		for (u32 i = triangles.num - 3; i < triangles.num; ++i) {
			SPVertex & vtx = triangles.vertices[triangles.elements[i]];
			vtx.flat_r = vtx0.r;
			vtx.flat_g = vtx0.g;
			vtx.flat_b = vtx0.b;
			vtx.flat_a = vtx.a;
		}
	}

	if (gDP.otherMode.depthSource == G_ZS_PRIM) {
		for (u32 i = triangles.num - 3; i < triangles.num; ++i) {
			SPVertex & vtx = triangles.vertices[triangles.elements[i]];
			vtx.z = gDP.primDepth.z * vtx.w;
		}
	}
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
	const u32 blendmode = gDP.otherMode.l >> 16;
	// 0x7000 = CVG_X_ALPHA|ALPHA_CVG_SEL|FORCE_BL
	if (gDP.otherMode.alphaCvgSel != 0 && (gDP.otherMode.l & 0x7000) != 0x7000) {
		if (blendmode == 0x4055) { // Mario Golf
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_ONE);
		} else
			glDisable(GL_BLEND);
		return;
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		glEnable( GL_BLEND );

		switch (blendmode)
		{
			// Mace objects
			case 0x0382:
			case 0x0091:
			case 0x0C08: // 1080 Sky
			case 0x0F0A: // Used LOTS of places
			//DK64 blue prints
			case 0x0302:
			//Sin and Punishment
			case 0xCB02:
			// Battlezone
			// clr_in * a + clr_in * (1-a)
			case 0xC800:
			case 0x00C0:
			//ISS64
			case 0xC302:
				glBlendFunc(GL_ONE, GL_ZERO);
				break;

			//Space Invaders
			case 0x0448: // Add
			case 0x055A:
				glBlendFunc( GL_ONE, GL_ONE );
				break;

			case 0xAF50: // LOT in Zelda: MM
			case 0x0F5A: // LOT in Zelda: MM
			case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
			case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
				//clr_in * 0 + clr_mem * 1
				glBlendFunc( GL_ZERO, GL_ONE );
				break;

			case 0x5F50: //clr_mem * 0 + clr_mem * (1-a)
				glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_ALPHA );
				break;

			case 0xF550: //clr_fog * a_fog + clr_mem * (1-a)
			case 0x0150: // spiderman
			case 0x0550: // bomberman 64
			case 0x0D18: //clr_in * a_fog + clr_mem * (1-a)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;

			case 0xC912: //40 winks, clr_in * a_fog + clr_mem * 1
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
			glCullFace(GL_BACK);
		else
			glCullFace(GL_FRONT);
	}
	else
		glDisable( GL_CULL_FACE );
}

void OGLRender::_updateViewport() const
{
	OGLVideo & ogl = video();
	if (!frameBufferList().isFboMode()) {
		const GLint X = gSP.viewport.vscale[0] < 0 ? (GLint)((gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) * ogl.getScaleX()) : (GLint)(gSP.viewport.x * ogl.getScaleX());
		const GLint Y = gSP.viewport.vscale[1] < 0 ? (GLint)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * ogl.getScaleY()) : (GLint)((VI.height - (gSP.viewport.y + gSP.viewport.height)) * ogl.getScaleY());
		glViewport( X, Y + ogl.getHeightOffset(),
			max((GLint)(gSP.viewport.width * ogl.getScaleX()), 0), max((GLint)(gSP.viewport.height * ogl.getScaleY()), 0) );
	} else {
		const GLint X = gSP.viewport.vscale[0] < 0 ? (GLint)((gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) * ogl.getScaleX()) : (GLint)(gSP.viewport.x * ogl.getScaleX());
		const GLint Y = gSP.viewport.vscale[1] < 0 ? (GLint)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * ogl.getScaleY()) : (GLint)((frameBufferList().getCurrent()->m_height - (gSP.viewport.y + gSP.viewport.height)) * ogl.getScaleY());
		glViewport(X, Y,
			max((GLint)(gSP.viewport.width * ogl.getScaleX()), 0), max((GLint)(gSP.viewport.height * ogl.getScaleY()), 0) );
	}
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
	} else if ((gDP.changed & (CHANGED_RENDERMODE | CHANGED_CYCLETYPE)) != 0) {
		if (((gSP.geometryMode & G_ZBUFFER) || gDP.otherMode.depthSource == G_ZS_PRIM) && gDP.otherMode.cycleType <= G_CYC_2CYCLE) {
			if (gDP.otherMode.depthCompare) {
				switch (gDP.otherMode.depthMode) {
					case ZMODE_OPA:
					glDisable(GL_POLYGON_OFFSET_FILL);
					glDepthFunc(GL_LEQUAL);
					break;
					case ZMODE_INTER:
					glDisable(GL_POLYGON_OFFSET_FILL);
					glDepthFunc(GL_LEQUAL);
					break;
					case ZMODE_XLU:
					// Max || Infront;
					glDisable(GL_POLYGON_OFFSET_FILL);
					if (gDP.otherMode.depthSource == G_ZS_PRIM && gDP.primDepth.z == 1.0f)
						// Max
						glDepthFunc(GL_LEQUAL);
					else
						// Infront
						glDepthFunc(GL_LESS);
					break;
					case ZMODE_DEC:
					glEnable(GL_POLYGON_OFFSET_FILL);
					glDepthFunc(GL_LEQUAL);
					break;
				}
			} else {
				glDisable(GL_POLYGON_OFFSET_FILL);
				glDepthFunc(GL_ALWAYS);
			}

			_updateDepthUpdate();

			glEnable(GL_DEPTH_TEST);
			if (!GBI.isNoN())
				glDisable(GL_DEPTH_CLAMP);
		} else {
			glDisable(GL_DEPTH_TEST);
			if (!GBI.isNoN())
				glEnable(GL_DEPTH_CLAMP);
		}
	}

	if ((gDP.changed & (CHANGED_ALPHACOMPARE|CHANGED_RENDERMODE|CHANGED_BLENDCOLOR)) != 0)
		currentCombiner()->updateAlphaTestInfo();

	if (gDP.changed & CHANGED_SCISSOR) {
		FrameBufferList & fbList = frameBufferList();
		const u32 screenHeight = (fbList.getCurrent() == NULL || fbList.getCurrent()->m_height == 0 || !fbList.isFboMode()) ? VI.height : fbList.getCurrent()->m_height;
		glScissor( (GLint)(gDP.scissor.ulx * ogl.getScaleX()), (GLint)((screenHeight - gDP.scissor.lry) * ogl.getScaleY() + (fbList.isFboMode() ? 0 : ogl.getHeightOffset())),
			max((GLint)((gDP.scissor.lrx - gDP.scissor.ulx) * ogl.getScaleX()), 0), max((GLint)((gDP.scissor.lry - gDP.scissor.uly) * ogl.getScaleY()), 0) );
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

	currentCombiner()->updateFBInfo(false);

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
	if (m_renderState == rsTriangle) {
		glDisableVertexAttribArray(SC_TEXCOORD1);
		if (currentCombiner()->usesT0() || currentCombiner()->usesT1())
			glEnableVertexAttribArray(SC_TEXCOORD0);
		else
			glDisableVertexAttribArray(SC_TEXCOORD0);
	} else {
		if (currentCombiner()->usesT0())
			glEnableVertexAttribArray(SC_TEXCOORD0);
		else
			glDisableVertexAttribArray(SC_TEXCOORD0);

		if (currentCombiner()->usesT1())
			glEnableVertexAttribArray(SC_TEXCOORD1);
		else
			glDisableVertexAttribArray(SC_TEXCOORD1);
	}
}

void OGLRender::_prepareDrawTriangle(bool _dma)
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (m_bImageTexture)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif // GL_IMAGE_TEXTURES_SUPPORT

	if (gSP.changed || gDP.changed)
		_updateStates();

	const bool updateArrays = m_renderState != rsTriangle;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		m_renderState = rsTriangle;
		_setColorArray();
		_setTexCoordArrays();
	}

	const bool updateColorArrays = m_bFlatColors != (!RSP.bLLE && (gSP.geometryMode & G_SHADING_SMOOTH) == 0);
	if (updateColorArrays)
		m_bFlatColors = !m_bFlatColors;

	if (updateArrays) {
		SPVertex * pVtx = _dma ? triangles.dmaVertices.data() : &triangles.vertices[0];
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->x);
		if (m_bFlatColors)
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->flat_r);
		else
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->r);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->s);
		if (config.enableHWLighting) {
			glEnableVertexAttribArray(SC_NUMLIGHTS);
			glVertexAttribPointer(SC_NUMLIGHTS, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), &pVtx->HWLight);
		}

		_updateCullFace();
		_updateViewport();
		glEnable(GL_SCISSOR_TEST);
		currentCombiner()->updateRenderState();
	} else if (updateColorArrays) {
		SPVertex * pVtx = _dma ? triangles.dmaVertices.data() : &triangles.vertices[0];
		if (m_bFlatColors)
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->flat_r);
		else
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->r);
	}

	currentCombiner()->updateColors();
	currentCombiner()->updateLight();
}

void OGLRender::drawLLETriangle(u32 _numVtx)
{
	if (_numVtx == 0)
		return;

	_prepareDrawTriangle(false);
	glDisable(GL_CULL_FACE);

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	OGLVideo & ogl = video();
	if (!fbList.isFboMode())
		glViewport( 0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
	else
		glViewport(0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY);

	const float scaleX = fbList.isFboMode() ? 1.0f / pBuffer->m_width : VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f / pBuffer->m_height : VI.rheight;

	for (u32 i = 0; i < _numVtx; ++i) {
		SPVertex & vtx = triangles.vertices[i];
		vtx.HWLight = 0;
		vtx.x = vtx.x * (2.0f * scaleX) - 1.0f;
		vtx.x *= vtx.w;
		vtx.y = vtx.y * (-2.0f * scaleY) + 1.0f;
		vtx.y *= vtx.w;
		vtx.z *= vtx.w;
		if (gDP.otherMode.texturePersp == 0) {
			vtx.s *= 2.0f;
			vtx.t *= 2.0f;
		}
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVtx);
	triangles.num = 0;

	gSP.changed |= CHANGED_VIEWPORT | CHANGED_GEOMETRYMODE;

#ifdef __TRIBUFFER_OPT
	_indexmap_clear();
#endif
}

void OGLRender::drawDMATriangles(u32 _numVtx)
{
	if (_numVtx == 0)
		return;
	_prepareDrawTriangle(true);
	glDrawArrays(GL_TRIANGLES, 0, _numVtx);
}

void OGLRender::drawTriangles()
{
	if (triangles.num == 0) return;

	_prepareDrawTriangle(false);
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
	}

	if (updateArrays) {
		glVertexAttrib4f(SC_COLOR, 0, 0, 0, 0);
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
		currentCombiner()->updateRenderState();
	}

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	OGLVideo & ogl = video();
	if (!fbList.isFboMode())
		glViewport( 0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
	else {
		glViewport( 0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY );
	}
	glDisable(GL_CULL_FACE);

	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	const float Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
	const float W = 1.0f;
	m_rect[0].x = (float)_ulx * (2.0f * scaleX) - 1.0;
	m_rect[0].y = (float)_uly * (-2.0f * scaleY) + 1.0;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = (float)(_lrx + 1) * (2.0f * scaleX) - 1.0;
	m_rect[1].y = m_rect[0].y;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = (float)(_lry + 1) * (-2.0f * scaleY) + 1.0;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	glVertexAttrib4fv(SC_COLOR, _pColor);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	_updateViewport();
}

static
bool texturedRectShadowMap(const OGLRender::TexturedRectParams &)
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
//	if ((gDP.otherMode.l >> 16) == 0x3c18 && gDP.combine.muxs0 == 0x00ffffff && gDP.combine.muxs1 == 0xfffff238) //depth image based fog
	if (gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress +  gDP.colorImage.width*gDP.colorImage.width*6/4))
		SetMonochromeCombiner(g_draw_shadow_map_program);
#endif // GL_IMAGE_TEXTURES_SUPPORT
	return false;
}

static
bool texturedRectDepthBufferCopy(const OGLRender::TexturedRectParams & _params)
{
	// Copy one line from depth buffer into auxiliary color buffer with height = 1.
	// Data from depth buffer loaded into TMEM and then rendered to RDRAM by texrect.
	// Works only with depth buffer emulation enabled.
	// Load of arbitrary data to that area causes weird camera rotation in CBFD.
	const gDPTile * pTile = gSP.textureTile[0];
	if (pTile->loadType == LOADTYPE_BLOCK && gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*gDP.colorImage.width * 6 / 4)) {
		if (!config.frameBufferEmulation.enable)
			return true;
		frameBufferList().getCurrent()->m_cleared = true;
		if (!config.frameBufferEmulation.copyDepthToRDRAM)
			return true;
		if (FrameBuffer_CopyDepthBuffer(gDP.colorImage.address))
			RDP_RepeatLastLoadBlock();

		const u32 width = (u32)(_params.lrx - _params.ulx);
		const u32 ulx = (u32)_params.ulx;
		u16 * pSrc = ((u16*)TMEM) + (u32)floorf(_params.uls + 0.5f);
		u16 *pDst = (u16*)(RDRAM + gDP.colorImage.address);
		for (u32 x = 0; x < width; ++x)
			pDst[(ulx + x) ^ 1] = swapword(pSrc[x]);

		return true;
	}
	return false;
}

static
bool texturedRectBGCopy(const OGLRender::TexturedRectParams & _params)
{
	if (GBI.getMicrocodeType() != S2DEX)
		return false;

	float flry = _params.lry;
	if (flry > gDP.scissor.lry)
		flry = gDP.scissor.lry;

	const u32 width = (u32)(_params.lrx - _params.ulx);
	const u32 tex_width = gSP.textureTile[0]->line << 3;
	const u32 uly = (u32)_params.uly;
	const u32 lry = flry;

	u8 * texaddr = RDRAM + gSP.textureTile[0]->imageAddress + tex_width*(u32)_params.ult + (u32)_params.uls;
	u8 * fbaddr = RDRAM + gDP.colorImage.address + (u32)_params.ulx;
	LOG(LOG_VERBOSE, "memrect (%d, %d, %d, %d), ci_width: %d texaddr: 0x%08lx fbaddr: 0x%08lx\n", (u32)_params.ulx, uly, (u32)_params.lrx, lry, gDP.colorImage.width, gSP.textureTile[0]->imageAddress + tex_width*(u32)_params.ult + (u32)_params.uls, gDP.colorImage.address + (u32)_params.ulx);

	for (u32 y = uly; y < lry; ++y) {
		u8 *src = texaddr + (y - uly) * tex_width;
		u8 *dst = fbaddr + y * gDP.colorImage.width;
		memcpy(dst, src, width);
	}
	return true;
}

static
bool texturedRectPaletteMod(const OGLRender::TexturedRectParams & _params)
{
	if (gDP.scissor.lrx != 16 || gDP.scissor.lry != 1 || _params.lrx != 16 || _params.lry != 1)
		return false;
	u8 envr = (u8)(gDP.envColor.r * 31.0f);
	u8 envg = (u8)(gDP.envColor.g * 31.0f);
	u8 envb = (u8)(gDP.envColor.b * 31.0f);
	u16 env16 = (u16)((envr << 11) | (envg << 6) | (envb << 1) | 1);
	u8 prmr = (u8)(gDP.primColor.r * 31.0f);
	u8 prmg = (u8)(gDP.primColor.g * 31.0f);
	u8 prmb = (u8)(gDP.primColor.b * 31.0f);
	u16 prim16 = (u16)((prmr << 11) | (prmg << 6) | (prmb << 1) | 1);
	u16 * src = (u16*)&TMEM[256];
	u16 * dst = (u16*)(RDRAM + gDP.colorImage.address);
	for (u32 i = 0; i < 16; ++i)
		dst[i ^ 1] = (src[i<<2] & 0x100) ? prim16 : env16;
	return true;
}

static
bool texturedRectMonochromeBackground(const OGLRender::TexturedRectParams & _params)
{
	if (gDP.textureImage.address >= gDP.colorImage.address && gDP.textureImage.address <= (gDP.colorImage.address + gDP.colorImage.width*gDP.colorImage.height * 2)) {
#ifdef GL_IMAGE_TEXTURES_SUPPORT
		FrameBufferList & fb = frameBufferList();
		if (fb.isFboMode()) {
			FrameBuffer_ActivateBufferTexture(0, fb.getCurrent());
			SetMonochromeCombiner(g_monochrome_image_program);
			return false;
		} else
#endif
			return true;
	}
	return false;
}

// Special processing of textured rect.
// Return true if actuial rendering is not necessary
bool(*texturedRectSpecial)(const OGLRender::TexturedRectParams & _params) = NULL;

void OGLRender::drawTexturedRect(const TexturedRectParams & _params)
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
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s0);
		glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s1);
		currentCombiner()->updateRenderState();
	}

	if (RSP.cmd == 0xE4 && texturedRectSpecial != NULL && texturedRectSpecial(_params))
		return;

	FrameBufferList & fbList = frameBufferList();
	FrameBuffer* pBuffer = fbList.getCurrent();
	OGLVideo & ogl = video();
	if (!fbList.isFboMode())
		glViewport( 0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
	else
		glViewport( 0, 0, pBuffer->m_width*pBuffer->m_scaleX, pBuffer->m_height*pBuffer->m_scaleY );
	glDisable( GL_CULL_FACE );

	const float scaleX = fbList.isFboMode() ? 1.0f/pBuffer->m_width :  VI.rwidth;
	const float scaleY = fbList.isFboMode() ? 1.0f/pBuffer->m_height :  VI.rheight;
	const float Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
	const float W = 1.0f;
	m_rect[0].x = (float)_params.ulx * (2.0f * scaleX) - 1.0f;
	m_rect[0].y = (float)_params.uly * (-2.0f * scaleY) + 1.0f;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = (float)(_params.lrx) * (2.0f * scaleX) - 1.0f;
	m_rect[1].y = m_rect[0].y;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = (float)(_params.lry) * (-2.0f * scaleY) + 1.0f;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	TextureCache & cache = textureCache();
	struct
	{
		float s0, t0, s1, t1;
	} texST[2]; //struct for texture coordinates

	if (currentCombiner()->usesT0() && cache.current[0] && gSP.textureTile[0]) {
		texST[0].s0 = _params.uls * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		texST[0].t0 = _params.ult * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
		texST[0].s1 = (_params.lrs + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
		texST[0].t1 = (_params.lrt + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

		if ((cache.current[0]->maskS) && !(cache.current[0]->mirrorS) && (fmod(texST[0].s0, cache.current[0]->width) == 0.0f)) {
			texST[0].s1 -= texST[0].s0;
			texST[0].s0 = 0.0f;
		}

		if ((cache.current[0]->maskT) && !(cache.current[0]->mirrorT) && (fmod(texST[0].t0, cache.current[0]->height) == 0.0f)) {
			texST[0].t1 -= texST[0].t0;
			texST[0].t0 = 0.0f;
		}

		if (cache.current[0]->frameBufferTexture) {
			texST[0].s0 = cache.current[0]->offsetS + texST[0].s0;
			texST[0].t0 = cache.current[0]->offsetT - texST[0].t0;
			texST[0].s1 = cache.current[0]->offsetS + texST[0].s1;
			texST[0].t1 = cache.current[0]->offsetT - texST[0].t1;
		}

		glActiveTexture( GL_TEXTURE0 );

		if ((texST[0].s0 >= 0.0 && texST[0].s1 <= cache.current[0]->width) || (cache.current[0]->maskS + cache.current[0]->mirrorS == 0 && (texST[0].s0 < -1024.0f || texST[0].s1 > 1023.99f)))
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		if (texST[0].t0 >= 0.0f && texST[0].t1 <= cache.current[0]->height)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		texST[0].s0 *= cache.current[0]->scaleS;
		texST[0].t0 *= cache.current[0]->scaleT;
		texST[0].s1 *= cache.current[0]->scaleS;
		texST[0].t1 *= cache.current[0]->scaleT;
	}

	if (currentCombiner()->usesT1() && cache.current[1] && gSP.textureTile[1]) {
		texST[1].s0 = _params.uls * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		texST[1].t0 = _params.ult * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
		texST[1].s1 = (_params.lrs + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
		texST[1].t1 = (_params.lrt + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

		if ((cache.current[1]->maskS) && (fmod(texST[1].s0, cache.current[1]->width) == 0.0f) && !(cache.current[1]->mirrorS)) {
			texST[1].s1 -= texST[1].s0;
			texST[1].s0 = 0.0f;
		}

		if ((cache.current[1]->maskT) && (fmod(texST[1].t0, cache.current[1]->height) == 0.0f) && !(cache.current[1]->mirrorT)) {
			texST[1].t1 -= texST[1].t0;
			texST[1].t0 = 0.0f;
		}

		if (cache.current[1]->frameBufferTexture) {
			texST[1].s0 = cache.current[1]->offsetS + texST[1].s0;
			texST[1].t0 = cache.current[1]->offsetT - texST[1].t0;
			texST[1].s1 = cache.current[1]->offsetS + texST[1].s1;
			texST[1].t1 = cache.current[1]->offsetT - texST[1].t1;
		}

		glActiveTexture( GL_TEXTURE1 );

		if ((texST[1].s0 == 0.0f) && (texST[1].s1 <= cache.current[1]->width))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

		if ((texST[1].t0 == 0.0f) && (texST[1].t1 <= cache.current[1]->height))
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		texST[1].s0 *= cache.current[1]->scaleS;
		texST[1].t0 *= cache.current[1]->scaleT;
		texST[1].s1 *= cache.current[1]->scaleS;
		texST[1].t1 *= cache.current[1]->scaleT;
	}

	if ((gDP.otherMode.cycleType == G_CYC_COPY) && !config.texture.forceBilinear) {
		glActiveTexture( GL_TEXTURE0 );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	m_rect[1].s0 = texST[0].s1;
	m_rect[1].t0 = texST[0].t0;
	m_rect[1].s1 = texST[1].s1;
	m_rect[1].t1 = texST[1].t0;
	m_rect[2].s0 = texST[0].s0;
	m_rect[2].t0 = texST[0].t1;
	m_rect[2].s1 = texST[1].s0;
	m_rect[2].t1 = texST[1].t1;
	if (_params.flip) {
		m_rect[0].s0 = texST[0].s1;
		m_rect[0].t0 = texST[0].t1;
		m_rect[0].s1 = texST[1].s1;
		m_rect[0].t1 = texST[1].t1;
		m_rect[3].s0 = texST[0].s0;
		m_rect[3].t0 = texST[0].t0;
		m_rect[3].s1 = texST[1].s0;
		m_rect[3].t1 = texST[1].t0;
	} else {
		m_rect[0].s0 = texST[0].s0;
		m_rect[0].t0 = texST[0].t0;
		m_rect[0].s1 = texST[1].s0;
		m_rect[0].t1 = texST[1].t0;
		m_rect[3].s0 = texST[0].s1;
		m_rect[3].t0 = texST[0].t1;
		m_rect[3].s1 = texST[1].s1;
		m_rect[3].t1 = texST[1].t1;
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	_updateViewport();
}

void OGLRender::drawText(const char *_pText, float x, float y)
{
	m_renderState = rsNone;
	TextDrawer::get().renderText(_pText, x, y);
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
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	const char *version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	u32 uVersion = atol(version);

	m_bImageTexture = (uVersion >= 4) && (glBindImageTexture != NULL);
#else
	m_bImageTexture = false;
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
	glViewport( 0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	srand( time(NULL) );

	ogl.swapBuffers();
}

void OGLRender::_initData()
{
	_initExtensions();
	_initStates();
	_setSpecialTexrect();

	textureCache().init();
	DepthBuffer_Init();
	FrameBuffer_Init();
	Combiner_Init();
	TextDrawer::get().init();
	TFH.init();
	m_renderState = rsNone;

	gSP.changed = gDP.changed = 0xFFFFFFFF;

	memset(triangles.vertices, 0, VERTBUFF_SIZE * sizeof(SPVertex));
	memset(triangles.elements, 0, ELEMBUFF_SIZE * sizeof(GLubyte));
	for (u32 i = 0; i < VERTBUFF_SIZE; ++i)
		triangles.vertices[i].w = 1.0f;
	triangles.num = 0;

#ifdef __TRIBUFFER_OPT
	_indexmap_init();
#endif
}

void OGLRender::_destroyData()
{
	m_renderState = rsNone;
	TextDrawer::get().destroy();
	Combiner_Destroy();
	FrameBuffer_Destroy();
	DepthBuffer_Destroy();
	textureCache().destroy();
}

void OGLRender::_setSpecialTexrect() const
{
	const char * name = RSP.romname;
	if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV")
		|| strstr(name, (const char *)"DUCK DODGERS") || strstr(name, (const char *)"DAFFY DUCK"))
		texturedRectSpecial = texturedRectShadowMap;
	else if (strstr(name, (const char *)"CONKER BFD") || strstr(name, (const char *)"Perfect Dark") || strstr(name, (const char *)"PERFECT DARK"))
		texturedRectSpecial = texturedRectDepthBufferCopy; // See comments to that function!
	else if (strstr(name, (const char *)"YOSHI STORY"))
		texturedRectSpecial = texturedRectBGCopy;
	else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
		texturedRectSpecial = texturedRectPaletteMod;
	else if (strstr(name, (const char *)"ZELDA"))
		texturedRectSpecial = texturedRectMonochromeBackground;
	else
		texturedRectSpecial = NULL;
}

static
u32 textureFilters[] = {
	NO_FILTER, //"None"
	SMOOTH_FILTER_1, //"Smooth filtering 1"
	SMOOTH_FILTER_2, //"Smooth filtering 2"
	SMOOTH_FILTER_3, //"Smooth filtering 3"
	SMOOTH_FILTER_4, //"Smooth filtering 4"
	SHARP_FILTER_1,  //"Sharp filtering 1"
	SHARP_FILTER_2,  //"Sharp filtering 2"
};

static
u32 textureEnhancements[] = {
	NO_ENHANCEMENT,    //"None"
	NO_ENHANCEMENT,    //"Store"
	X2_ENHANCEMENT,    //"X2"
	X2SAI_ENHANCEMENT, //"X2SAI"
	HQ2X_ENHANCEMENT,  //"HQ2X"
	HQ2XS_ENHANCEMENT, //"HQ2XS"
	LQ2X_ENHANCEMENT,  //"LQ2X"
	LQ2XS_ENHANCEMENT, //"LQ2XS"
	HQ4X_ENHANCEMENT,  //"HQ4X"
	BRZ2X_ENHANCEMENT, //"2XBRZ"
	BRZ3X_ENHANCEMENT, //"3XBRZ"
	BRZ4X_ENHANCEMENT, //"4XBRZ"
	BRZ5X_ENHANCEMENT  //"5XBRZ"
};

void displayLoadProgress(const wchar_t *format, ...)
{
	va_list args;
	wchar_t wbuf[INFO_BUF];
	char buf[INFO_BUF];

	// process input
	va_start(args, format);
	vswprintf(wbuf, INFO_BUF, format, args);
	va_end(args);

	// XXX: convert to multibyte
	wcstombs(buf, wbuf, INFO_BUF);

	FrameBuffer* pBuffer = frameBufferList().getCurrent();
	if (pBuffer != NULL)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	OGLRender & render = video().getRender();
	float black[4] = {0, 0, 0, 0};
	render.clearColorBuffer(black);
	render.drawText(buf, -0.9f, 0);
	video().swapBuffers();

	if (pBuffer != NULL)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pBuffer->m_FBO);
	//OutputDebugStringW(wbuf);
}

void TextureFilterHandler::init()
{
	if (!isInited()) {
		m_inited = config.textureFilter.txFilterMode | config.textureFilter.txEnhancementMode | config.textureFilter.txHiresEnable;
		if (m_inited != 0) {
			u32 options = textureFilters[config.textureFilter.txFilterMode] | textureEnhancements[config.textureFilter.txEnhancementMode];
			if (config.textureFilter.txHiresEnable)
				options |= RICE_HIRESTEXTURES;
			if (config.textureFilter.txFilterForce16bpp)
				options |= FORCE16BPP_TEX;
			if (config.textureFilter.txHiresForce16bpp)
				options |= FORCE16BPP_HIRESTEX;
			if (config.textureFilter.txFilterCacheCompression)
				options |= GZ_TEXCACHE;
			if (config.textureFilter.txHiresCacheCompression)
				options |= GZ_HIRESTEXCACHE;
			if (config.textureFilter.txSaveCache)
				options |= (DUMP_TEXCACHE | DUMP_HIRESTEXCACHE);
			if (config.textureFilter.txHiresFullAlphaChannel)
				options |= LET_TEXARTISTS_FLY;
			if (config.textureFilter.txDump)
				options |= DUMP_TEX;

			GLint maxTextureSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
			wchar_t wRomName[32];
			::mbstowcs(wRomName, RSP.romname, 32);

			m_inited = txfilter_init(maxTextureSize, // max texture width supported by hardware
				maxTextureSize, // max texture height supported by hardware
				32, // max texture bpp supported by hardware
				options,
				config.textureFilter.txCacheSize, // cache texture to system memory
				RSP.pluginpath, // plugin path
				wRomName, // name of ROM. must be no longer than 256 characters
				displayLoadProgress);
		}
	}
}

void TextureFilterHandler::shutdown()
{
	if (isInited()) {
		txfilter_shutdown();
		m_inited = 0;
	}
}

TextureFilterHandler TFH;

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
