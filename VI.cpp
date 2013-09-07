#include "GLideN64.h"
#include "Types.h"
#include "VI.h"
#include "OpenGL.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "RSP.h"
#include "FrameBuffer.h"
#include "Debug.h"

VIInfo VI;

void VI_UpdateSize()
{
	f32 xScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 0, 12 ), 10 );
	f32 xOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_X_SCALE, 16, 12 ), 10 );

	f32 yScale = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 0, 12 ), 10 );
	f32 yOffset = _FIXED2FLOAT( _SHIFTR( *REG.VI_Y_SCALE, 16, 12 ), 10 );

	u32 hEnd = _SHIFTR( *REG.VI_H_START, 0, 10 );
	u32 hStart = _SHIFTR( *REG.VI_H_START, 16, 10 );

	// These are in half-lines, so shift an extra bit
	u32 vEnd = _SHIFTR( *REG.VI_V_START, 1, 9 );
	u32 vStart = _SHIFTR( *REG.VI_V_START, 17, 9 );

	VI.width = (hEnd - hStart) * xScale;
	VI.height = (vEnd - vStart) * yScale * 1.0126582f;

	if (VI.width == 0.0f) VI.width = 320.0f;
	if (VI.height == 0.0f) VI.height = 240.0f;
}

void VI_UpdateScreen()
{
	glFinish();

	if (OGL.captureScreen) {
		OGL_SaveScreenshot();
		OGL.captureScreen = false;
	}

	if (OGL.frameBufferTextures) {
		const bool bDListUpdated = (gSP.changed&CHANGED_CPU_FB_WRITE) == 0;
		const bool bNeedUpdate = bDListUpdated ? (*REG.VI_ORIGIN != VI.lastOrigin) && gDP.colorImage.changed : true;

		if (bNeedUpdate) {
			FrameBuffer * pBuffer = FrameBuffer_FindBuffer(*REG.VI_ORIGIN);
			if (pBuffer == NULL || pBuffer->width != *REG.VI_WIDTH) {
				VI_UpdateSize();
				OGL_UpdateScale();
				const u32 size = *REG.VI_STATUS & 3;
				if (VI.height > 0 && size > G_IM_SIZ_8b)
					FrameBuffer_SaveBuffer( *REG.VI_ORIGIN, G_IM_FMT_RGBA, size, *REG.VI_WIDTH, VI.height );
			}
			if (g_bCopyFromRDRAM || !bDListUpdated)
				FrameBuffer_CopyFromRDRAM( *REG.VI_ORIGIN );
			if (g_bCopyToRDRAM)
				FrameBuffer_CopyToRDRAM( *REG.VI_ORIGIN, false );
			FrameBuffer_RenderBuffer( *REG.VI_ORIGIN );

			gDP.colorImage.changed = FALSE;
			VI.lastOrigin = *REG.VI_ORIGIN;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		}
	}
	else {
		if (gSP.changed & CHANGED_COLORBUFFER) {
#ifndef __LINUX__
			SwapBuffers( OGL.hDC );
#else
			OGL_SwapBuffers();
#endif
			gSP.changed &= ~CHANGED_COLORBUFFER;
#ifdef DEBUG
			while (Debug.paused && !Debug.step);
			Debug.step = FALSE;
#endif
		}
	}
	glFinish();
}
