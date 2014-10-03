#include "GLideN64.h"
#include <math.h>
#include "Types.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "Config.h"
#include "Debug.h"

VIInfo VI;

void VI_UpdateSize()
{
	const f32 xScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 0, 12 ), 10 );
//	f32 xOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 16, 12 ), 10 );

	const u32 vScale = _SHIFTR(*REG.VI_Y_SCALE, 0, 12);
	const f32 yScale = _FIXED2FLOAT(vScale, 10);
//	f32 yOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 16, 12 ), 10 );

	const u32 hEnd = _SHIFTR( *REG.VI_H_START, 0, 10 );
	const u32 hStart = _SHIFTR( *REG.VI_H_START, 16, 10 );

	// These are in half-lines, so shift an extra bit
	const u32 vEnd = _SHIFTR( *REG.VI_V_START, 0, 10 );
	const u32 vStart = _SHIFTR( *REG.VI_V_START, 16, 10 );
	const bool interlacedPrev = VI.interlaced;
	const u32 widthPrev = VI.width;
	const u32 heightPrev = VI.height;

	VI.interlaced = (*REG.VI_STATUS & 0x40) != 0;

	VI.width = (u32)floor((hEnd - hStart) * xScale + 0.5f);
	if (*REG.VI_WIDTH > 0)
		VI.width = min(VI.width, *REG.VI_WIDTH);

	if (VI.interlaced &&  _SHIFTR(*REG.VI_Y_SCALE, 0, 12) == 1024)
		VI.real_height = (vEnd - vStart);
	else
		VI.real_height = (u32)floor(((vEnd - vStart)>>1) * yScale + 0.5f);
	if (VI.interlaced && VI.real_height % 2 == 1)
		--VI.real_height;
	const bool isPAL = (*REG.VI_V_SYNC & 0x3ff) > 550;
	if (isPAL && (vEnd - vStart) > 480)
		VI.height = VI.real_height*1.0041841f;
	else
		VI.height = VI.real_height*1.0126582f;

//	const int fsaa = ((*REG.VI_STATUS) >> 8) & 3;
//	const int divot = ((*REG.VI_STATUS) >> 4) & 1;

	if (VI.width == 0)
		VI.width = *REG.VI_WIDTH;
	if (VI.height == 0)
		VI.height = 240;

	if (config.frameBufferEmulation.enable && (interlacedPrev != VI.interlaced || widthPrev != VI.width || heightPrev != VI.height)) {
		frameBufferList().destroy();
		depthBufferList().destroy();
		depthBufferList().init();
		frameBufferList().init();
	}

	VI.rwidth = 1.0f / VI.width;
	VI.rheight = 1.0f / VI.height;
}

void VI_UpdateScreen()
{
	static u32 uNumCurFrameIsShown = 0;
	glFinish();

	OGLVideo & ogl = video();
	if (ogl.changeWindow())
		return;
	if (ogl.resizeWindow())
		return;
	ogl.saveScreenshot();

	if (config.frameBufferEmulation.enable) {
		const bool bCFB = !config.frameBufferEmulation.ignoreCFB && (gSP.changed&CHANGED_CPU_FB_WRITE) == CHANGED_CPU_FB_WRITE;
		const bool bNeedUpdate = bCFB ? true : (*REG.VI_ORIGIN != VI.lastOrigin);// && gDP.colorImage.changed;

		if (bNeedUpdate) {
			if ((gSP.changed&CHANGED_CPU_FB_WRITE) == CHANGED_CPU_FB_WRITE) {
				FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
				if (pBuffer == NULL || pBuffer->m_width != VI.width) {
					VI_UpdateSize();
					ogl.updateScale();
					const u32 size = *REG.VI_STATUS & 3;
					if (VI.height > 0 && size > G_IM_SIZ_8b  && _SHIFTR( *REG.VI_H_START, 0, 10 ) > 0)
						frameBufferList().saveBuffer(*REG.VI_ORIGIN, G_IM_FMT_RGBA, size, VI.width, VI.height, true);
				}
			}
			if ((((*REG.VI_STATUS)&3) > 0) && (config.frameBufferEmulation.copyFromRDRAM || bCFB)) {
				VI_UpdateSize();
				FrameBuffer_CopyFromRDRAM( *REG.VI_ORIGIN, config.frameBufferEmulation.copyFromRDRAM && !bCFB );
			}
			frameBufferList().renderBuffer(*REG.VI_ORIGIN);

			gDP.colorImage.changed = FALSE;
			VI.lastOrigin = *REG.VI_ORIGIN;
			uNumCurFrameIsShown = 0;;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		} else {
			uNumCurFrameIsShown++;
			if (uNumCurFrameIsShown > 25)
				gSP.changed |= CHANGED_CPU_FB_WRITE;
		}
	}
	else {
		if (gSP.changed & CHANGED_COLORBUFFER) {
			ogl.swapBuffers();
			gSP.changed &= ~CHANGED_COLORBUFFER;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		}
	}
	glFinish();
}
