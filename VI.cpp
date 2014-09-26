#include "GLideN64.h"
#include "Types.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "FrameBuffer.h"
#include "Config.h"
#include "Debug.h"

VIInfo VI;

void VI_UpdateSize()
{
	const f32 xScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 0, 12 ), 10 );
//	f32 xOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 16, 12 ), 10 );

	const f32 yScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 0, 12 ), 10 );
//	f32 yOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 16, 12 ), 10 );

	const u32 hEnd = _SHIFTR( *REG.VI_H_START, 0, 10 );
	const u32 hStart = _SHIFTR( *REG.VI_H_START, 16, 10 );

	// These are in half-lines, so shift an extra bit
	const u32 vEnd = _SHIFTR( *REG.VI_V_START, 1, 9 );
	const u32 vStart = _SHIFTR( *REG.VI_V_START, 17, 9 );
	VI.interlaced = (*REG.VI_STATUS & 0x40) != 0;

	VI.width = (hEnd - hStart) * xScale;
	VI.real_height = (vEnd - vStart) * yScale;
	VI.height = VI.real_height*1.0126582f;

	if (VI.vStart == 0) {
		VI.vStart = vStart;
		VI.vEnd = vEnd;
		VI.vHeight = VI.height;
	}

	if (VI.width == 0.0f)
		VI.width = *REG.VI_WIDTH;
	if (VI.height == 0.0f)
		VI.height = 240.0f;
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
	ogl.saveScreenshot();
	if (((*REG.VI_STATUS)&3) == 0)
		VI.vStart = VI.vEnd = 0;

	if (config.frameBufferEmulation.enable) {
		const bool bCFB = !config.frameBufferEmulation.ignoreCFB && (gSP.changed&CHANGED_CPU_FB_WRITE) == CHANGED_CPU_FB_WRITE;
		const bool bNeedUpdate = bCFB ? true : (*REG.VI_ORIGIN != VI.lastOrigin);// && gDP.colorImage.changed;

		if (bNeedUpdate) {
			FrameBuffer * pBuffer = frameBufferList().findBuffer(*REG.VI_ORIGIN);
			if (pBuffer == NULL || pBuffer->m_width != *REG.VI_WIDTH) {
				VI_UpdateSize();
				ogl.updateScale();
				const u32 size = *REG.VI_STATUS & 3;
				if (VI.height > 0 && size > G_IM_SIZ_8b  && _SHIFTR( *REG.VI_H_START, 0, 10 ) > 0)
					frameBufferList().saveBuffer( *REG.VI_ORIGIN, G_IM_FMT_RGBA, size, *REG.VI_WIDTH, VI.height );
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
