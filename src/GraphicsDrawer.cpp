#include <algorithm>
#include <string>
#include <thread>
#include <assert.h>
#include <cmath>
#include "Platform.h"
#include "Graphics/Context.h"
#include "DisplayWindow.h"
#include "SoftwareRender.h"
#include "GraphicsDrawer.h"
#include "Performance.h"
#include "TextureFilterHandler.h"
#include "PostProcessor.h"
#include "ZlutTexture.h"
#include "PaletteTexture.h"
#include "TextDrawer.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "FrameBufferInfo.h"
#include "Config.h"
#include "Debugger.h"
#include "RSP.h"
#include "RDP.h"
#include "VI.h"
#include "Log.h"

using namespace graphics;

GraphicsDrawer::GraphicsDrawer()
{
	memset(m_rect, 0, sizeof(m_rect));
}

GraphicsDrawer::~GraphicsDrawer()
{
	while (!m_osdMessages.empty())
		std::this_thread::sleep_for(Milliseconds(1));
}

void GraphicsDrawer::addTriangle(u32 _v0, u32 _v1, u32 _v2)
{
	m_statistics.drawnTris++;
	const u32 firstIndex = triangles.num;
	triangles.elements[triangles.num++] = static_cast<u16>(_v0);
	triangles.elements[triangles.num++] = static_cast<u16>(_v1);
	triangles.elements[triangles.num++] = static_cast<u16>(_v2);
	triangles.maxElement = std::max(triangles.maxElement, _v0);
	triangles.maxElement = std::max(triangles.maxElement, _v1);
	triangles.maxElement = std::max(triangles.maxElement, _v2);

	m_modifyVertices |= triangles.vertices[_v0].modify |
		triangles.vertices[_v1].modify |
		triangles.vertices[_v2].modify;

	for (u32 i = firstIndex; i < triangles.num; ++i) {
		SPVertex& vtx = triangles.vertices[triangles.elements[i]];
		vtx.bc0 = i - firstIndex == 0 ? 1.0f : 0.0f;
		vtx.bc1 = i - firstIndex == 1 ? 1.0f : 0.0f;
	}

	if ((gSP.geometryMode & G_LIGHTING) == 0) {
		if ((gSP.geometryMode & G_SHADE) == 0) {
			// Prim shading
			for (u32 i = firstIndex; i < triangles.num; ++i) {
				SPVertex & vtx = triangles.vertices[triangles.elements[i]];
				vtx.flat_r = gDP.primColor.r;
				vtx.flat_g = gDP.primColor.g;
				vtx.flat_b = gDP.primColor.b;
				vtx.flat_a = gDP.primColor.a;
			}
		}
		else if ((gSP.geometryMode & G_SHADING_SMOOTH) == 0) {
			// Flat shading
			SPVertex & vtx0 = triangles.vertices[triangles.elements[firstIndex + (((RSP.w1 >> 24) & 3) % 3)]];
			for (u32 i = firstIndex; i < triangles.num; ++i) {
				SPVertex & vtx = triangles.vertices[triangles.elements[i]];
				vtx.r = vtx.flat_r = vtx0.r;
				vtx.g = vtx.flat_g = vtx0.g;
				vtx.b = vtx.flat_b = vtx0.b;
				vtx.a = vtx.flat_a = vtx0.a;
			}
		}
	}

	if (gDP.otherMode.depthSource == G_ZS_PRIM) {
		for (u32 i = firstIndex; i < triangles.num; ++i) {
			SPVertex & vtx = triangles.vertices[triangles.elements[i]];
			vtx.z = gDP.primDepth.z * vtx.w;
		}
	}
}

void GraphicsDrawer::_updateCullFace() const
{
	if (gSP.geometryMode & G_CULL_BOTH) {
		gfxContext.enable(enable::CULL_FACE, true);

		if ((gSP.geometryMode & G_CULL_BOTH) == G_CULL_BOTH && GBI.isCullBoth())
			gfxContext.cullFace(cullMode::FRONT_AND_BACK);
		else if ((gSP.geometryMode & G_CULL_BACK) == G_CULL_BACK)
			gfxContext.cullFace(cullMode::BACK);
		else
			gfxContext.cullFace(cullMode::FRONT);
	} else
		gfxContext.enable(enable::CULL_FACE, false);
}

void GraphicsDrawer::_updateDepthUpdate() const
{
	gfxContext.enableDepthWrite(gDP.otherMode.depthUpdate != 0);
}

void GraphicsDrawer::_updateDepthCompare() const
{
	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
		gfxContext.enable(enable::DEPTH_TEST, false);
		gfxContext.enableDepthWrite(false);
	}
	else if ((gDP.changed & (CHANGED_RENDERMODE | CHANGED_CYCLETYPE)) != 0) {
		if (((gSP.geometryMode & G_ZBUFFER) || gDP.otherMode.depthSource == G_ZS_PRIM) && gDP.otherMode.cycleType <= G_CYC_2CYCLE) {
			if (gDP.otherMode.depthCompare != 0) {
				switch (gDP.otherMode.depthMode) {
				case ZMODE_INTER:
					gfxContext.enable(enable::POLYGON_OFFSET_FILL, false);
					gfxContext.setDepthCompare(compare::LEQUAL);
					break;
				case ZMODE_OPA:
				case ZMODE_XLU:
					// Max || Infront;
					gfxContext.enable(enable::POLYGON_OFFSET_FILL, false);
					if (gDP.otherMode.depthSource == G_ZS_PRIM && gDP.primDepth.z == 1.0f)
						// Max
						gfxContext.setDepthCompare(compare::LEQUAL);
					else
						// Infront
						gfxContext.setDepthCompare(compare::LESS);
					break;
				case ZMODE_DEC:
					gfxContext.enable(enable::POLYGON_OFFSET_FILL, true);
					gfxContext.setDepthCompare(compare::LEQUAL);
					break;
				}
			} else {
				gfxContext.enable(enable::POLYGON_OFFSET_FILL, false);
				gfxContext.setDepthCompare(compare::ALWAYS);
			}

			_updateDepthUpdate();

			gfxContext.enable(enable::DEPTH_TEST, true);
			if (!GBI.isNoN())
				gfxContext.setClampMode(graphics::ClampMode::ClippingEnabled);
		} else {
			gfxContext.enable(enable::DEPTH_TEST, false);
			if (!GBI.isNoN())
				gfxContext.setClampMode(graphics::ClampMode::NoClipping);
		}
	}
}

SPVertex & GraphicsDrawer::getCurrentDMAVertex()
{
	if (m_dmaVerticesNum >= m_dmaVertices.size())
		m_dmaVertices.resize(std::max(static_cast<std::vector<SPVertex>::size_type>(64), m_dmaVertices.size() * 2));
	return m_dmaVertices[m_dmaVerticesNum++];
}

inline
bool _needAdjustCoordinate(DisplayWindow & _wnd)
{
	return _wnd.isAdjustScreen() &&
		gSP.viewport.width < gDP.colorImage.width &&
		u32(gSP.viewport.width + gSP.viewport.x * 2.0f) != gDP.colorImage.width &&
		gDP.colorImage.width > VI.width * 98 / 100;
}

inline
void _adjustScissorX(f32 & _X0, f32 & _X1, float _scale)
{
	const float halfX = gDP.colorImage.width / 2.0f;
	_X0 = (_X0 - halfX) * _scale + halfX;
	_X1 = (_X1 - halfX) * _scale + halfX;
}

inline
s32 roundup(f32 _v, f32 _scale)
{
	return static_cast<s32>(floorf(_v * _scale + 0.5f));
}

void GraphicsDrawer::updateScissor(FrameBuffer * _pBuffer) const
{
	DisplayWindow & wnd = DisplayWindow::get();
	f32 scaleX, scaleY;
	f32 offsetX = 0.0f, offsetY = 0.0f;
	if (_pBuffer == nullptr) {
		scaleX = wnd.getScaleX();
		scaleY = wnd.getScaleY();
	} else {
		scaleX = _pBuffer->m_scale;
		scaleY = _pBuffer->m_scale;
		offsetX = f32(_pBuffer->m_originX);
		offsetY = f32(_pBuffer->m_originY);
	}

	f32 SX0 = gDP.scissor.ulx + offsetX;
	f32 SX1 = gDP.scissor.lrx + offsetX;
	f32 SY0 = gDP.scissor.uly + offsetY;
	f32 SY1 = gDP.scissor.lry + offsetY;

	if (u32(SX1) == 512 && (config.generalEmulation.hacks & hack_RE2) != 0) {
		SX1 = f32(*REG.VI_WIDTH);
		SY1 *= 512.0f / SX1;
	}

	if (_needAdjustCoordinate(wnd))
		_adjustScissorX(SX0, SX1, wnd.getAdjustScale());

	gfxContext.setScissor(roundup(SX0, scaleX), roundup(SY0, scaleY),
		std::max(roundup(SX1 - SX0, scaleX), 0), std::max(roundup(SY1 - SY0, scaleY), 0));

	gDP.changed &= ~CHANGED_SCISSOR;
}

void GraphicsDrawer::_updateViewport(const FrameBuffer* _pBuffer, const f32 scale) const
{
	s32 X, Y, WIDTH, HEIGHT;
	f32 scaleX, scaleY;
	if (scale == 0.0f) {
		const FrameBuffer* pCurrentBuffer = _pBuffer != nullptr ? _pBuffer : frameBufferList().getCurrent();
		if (pCurrentBuffer != nullptr) {
			scaleX = scaleY = pCurrentBuffer->m_scale;
		} else {
			scaleX = dwnd().getScaleX();
			scaleY = dwnd().getScaleY();
		}
	} else {
		scaleX = scaleY = scale;
	}
	X = 0;
	Y = 0;
	WIDTH = roundup(SCREEN_SIZE_DIM, scaleX);
	HEIGHT = roundup(SCREEN_SIZE_DIM, scaleY);
	gfxContext.setViewport(X, Y, WIDTH, HEIGHT);
	gSP.changed |= CHANGED_VIEWPORT;
}

void GraphicsDrawer::_legacyBlending() const
{
	const u32 blendmode = gDP.otherMode.l >> 16;
	// 0x7000 = CVG_X_ALPHA|ALPHA_CVG_SEL|FORCE_BL
	if (gDP.otherMode.alphaCvgSel != 0 && (gDP.otherMode.l & 0x7000) != 0x7000) {
		switch (blendmode) {
		case 0x4055: // Mario Golf
		case 0x5055: // Paper Mario intro clr_mem * a_in + clr_mem * a_mem
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::ZERO, blend::ONE);
			break;
		default:
			gfxContext.enable(enable::BLEND, false);
		}
		return;
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		BlendParam sfactor, dfactor;

		switch (blendmode)
		{
			// Mace objects
		case 0x0382:
			// Mace special blend mode, see GLSLCombiner.cpp
		case 0x0091:
			// 1080 Sky
		case 0x0C08:
			// Used LOTS of places
		case 0x0F0A:
			//DK64 blue prints
		case 0x0302:
			// Bomberman 2 special blend mode, see GLSLCombiner.cpp
		case 0xA500:
			//Sin and Punishment
		case 0xCB02:
			// Battlezone
			// clr_in * a + clr_in * (1-a)
		case 0xC800:
			// Conker BFD
			// clr_in * a_fog + clr_fog * (1-a)
			// clr_in * 0 + clr_in * 1
		case 0x07C2:
		case 0x00C0:
			//ISS64
		case 0xC302:
			// Donald Duck
		case 0xC702:
			sfactor = blend::ONE;
			dfactor = blend::ZERO;
			break;

		case 0x55f0:
			// Bust-A-Move 3 DX
			// CLR_MEM * A_FOG + CLR_FOG * 1MA
			sfactor = blend::ONE;
			dfactor = blend::SRC_ALPHA;
			break;

		case 0x0F1A:
			if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
				sfactor = blend::ONE;
				dfactor = blend::ZERO;
			} else {
				sfactor = blend::ZERO;
				dfactor = blend::ONE;
			}
			break;

			//Space Invaders
		case 0x0448: // Add
		case 0x055A:
			sfactor = blend::ONE;
			dfactor = blend::ONE;
			break;

		case 0xc712: // Pokemon Stadium?
		case 0xAF50: // LOT in Zelda: MM
		case 0x0F5A: // LOT in Zelda: MM
		case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
		case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
			//clr_in * 0 + clr_mem * 1
			sfactor = blend::ZERO;
			dfactor = blend::ONE;
			break;

		case 0x5F50: //clr_mem * 0 + clr_mem * (1-a)
			sfactor = blend::ZERO;
			dfactor = blend::ONE_MINUS_SRC_ALPHA;
			break;

		case 0xF550: //clr_fog * a_fog + clr_mem * (1-a)
		case 0x0150: // spiderman
		case 0x0550: // bomberman 64
		case 0x0D18: //clr_in * a_fog + clr_mem * (1-a)
			sfactor = blend::SRC_ALPHA;
			dfactor = blend::ONE_MINUS_SRC_ALPHA;
			break;

		case 0xC912: //40 winks, clr_in * a_fog + clr_mem * 1
			sfactor = blend::SRC_ALPHA;
			dfactor = blend::ONE;
			break;

		case 0x0040: // Fzero
		case 0xC810: // Blends fog
		case 0x0C18: // Standard interpolated blend
		case 0x0050: // Standard interpolated blend
		case 0x0051: // Standard interpolated blend
		case 0x0055: // Used for antialiasing
			sfactor = blend::SRC_ALPHA;
			dfactor = blend::ONE_MINUS_SRC_ALPHA;
			break;

		case 0x0C19: // Used for antialiasing
		case 0xC811: // Blends fog
			sfactor = blend::SRC_ALPHA;
			dfactor = blend::DST_ALPHA;
			break;

		case 0x5000: // V8 explosions
			sfactor = blend::ONE_MINUS_SRC_ALPHA;
			dfactor = blend::SRC_ALPHA;
			break;

		case 0xFA00: // Bomberman second attack
			sfactor = blend::ONE;
			dfactor = blend::ZERO;
			break;

		default:
			//LOG(LOG_VERBOSE, "Unhandled blend mode=%x", gDP.otherMode.l >> 16);
			sfactor = blend::SRC_ALPHA;
			dfactor = blend::ONE_MINUS_SRC_ALPHA;
			break;
		}

		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(sfactor, dfactor);
	} else if (gDP.otherMode.colorOnCvg != 0) {
		// CLR_ON_CVG - just use second mux of blender
		bool useMemColor = false;
		if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
			if (gDP.otherMode.c1_m2a == 1)
				useMemColor = true;
		} else if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
			if (gDP.otherMode.c2_m2a == 1)
				useMemColor = true;
		}
		if (useMemColor) {
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::ZERO, blend::ONE);
		} else {
			gfxContext.enable(enable::BLEND, false);
		}
	} else {
		gfxContext.enable(enable::BLEND, false);
	}
}

void GraphicsDrawer::_ordinaryBlending() const
{
	// Set unsupported blend modes
	if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
		const u32 mode = _SHIFTR(gDP.otherMode.l, 16, 16);
		switch (mode) {
		case 0x0040:
			// Mia Hamm Soccer
			// clr_in * a_in + clr_mem * (1-a)
			// clr_in * a_in + clr_in * (1-a)
		case 0x0050:
			// A Bug's Life
			// clr_in * a_in + clr_mem * (1-a)
			// clr_in * a_in + clr_mem * (1-a)
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::SRC_ALPHA, blend::ONE_MINUS_SRC_ALPHA);
			return;
		case 0x0150:
			// Tony Hawk
			// clr_in * a_in + clr_mem * (1-a)
			// clr_in * a_fog + clr_mem * (1-a_fog)
			if ((config.generalEmulation.hacks & hack_TonyHawk) != 0) {
				gfxContext.enable(enable::BLEND, true);
				gfxContext.setBlending(blend::SRC_ALPHA, blend::ONE_MINUS_SRC_ALPHA);
				return;
			}
			break;
		}
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		BlendParam srcFactor = blend::ONE;
		BlendParam dstFactor = blend::ZERO;
		u32 memFactorSource = 2, muxA, muxB;
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
			muxA = gDP.otherMode.c2_m1b;
			muxB = gDP.otherMode.c2_m2b;
			if (gDP.otherMode.c2_m1a == 1) {
				if (gDP.otherMode.c2_m2a == 1) {
					gfxContext.enable(enable::BLEND, true);
					gfxContext.setBlending(blend::ZERO, blend::ONE);
					return;
				}
				memFactorSource = 0;
			}
			else if (gDP.otherMode.c2_m2a == 1) {
				memFactorSource = 1;
			}
			if (gDP.otherMode.c2_m2a == 0 && gDP.otherMode.c2_m2b == 1) {
				// c_in * a_mem
				srcFactor = blend::DST_ALPHA;
			}
		} else {
			muxA = gDP.otherMode.c1_m1b;
			muxB = gDP.otherMode.c1_m2b;
			if (gDP.otherMode.c1_m1a == 1) {
				if (gDP.otherMode.c1_m2a == 1) {
					gfxContext.enable(enable::BLEND, true);
					gfxContext.setBlending(blend::ZERO, blend::ONE);
					return;
				}
				memFactorSource = 0;
			} else if (gDP.otherMode.c1_m2a == 1) {
				memFactorSource = 1;
			}
			if (gDP.otherMode.c1_m2a == 0 && gDP.otherMode.c1_m2b == 1) {
				// c_pixel * a_mem
				srcFactor = blend::DST_ALPHA;
			}
		}
		switch (memFactorSource) {
		case 0:
			switch (muxA) {
			case 0:
				dstFactor = blend::SRC_ALPHA;
				break;
			case 1:
				gfxContext.setBlendColor(gDP.fogColor.r, gDP.fogColor.g, gDP.fogColor.b, gDP.fogColor.a);
				dstFactor = blend::CONSTANT_ALPHA;
				break;
			case 2:
				assert(false); // shade alpha
				dstFactor = blend::SRC_ALPHA;
				break;
			case 3:
				dstFactor = blend::ZERO;
				break;
			}
			break;
		case 1:
			switch (muxB) {
			case 0:
				// 1.0 - muxA
				switch (muxA) {
				case 0:
					dstFactor = blend::ONE_MINUS_SRC_ALPHA;
					break;
				case 1:
					gfxContext.setBlendColor(gDP.fogColor.r, gDP.fogColor.g, gDP.fogColor.b, gDP.fogColor.a);
					dstFactor = blend::ONE_MINUS_CONSTANT_ALPHA;
					break;
				case 2:
					assert(false); // shade alpha
					dstFactor = blend::ONE_MINUS_SRC_ALPHA;
					break;
				case 3:
					dstFactor = blend::ONE;
					break;
				}
				break;
			case 1:
				dstFactor = blend::DST_ALPHA;
				break;
			case 2:
				dstFactor = blend::ONE;
				break;
			case 3:
				dstFactor = blend::ZERO;
				break;
			}
			break;
		default:
			dstFactor = blend::ZERO;
		}
		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(srcFactor, dstFactor);
	} else if ((gDP.otherMode.forceBlender == 0 && gDP.otherMode.cycleType < G_CYC_COPY)) {
		// Just use first mux of blender
		bool useMemColor = false;
		if (gDP.otherMode.cycleType == G_CYC_1CYCLE) {
			if (gDP.otherMode.c1_m1a == 1)
				useMemColor = true;
		} else if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
			if (gDP.otherMode.c2_m1a == 1)
				useMemColor = true;
		}
		if (useMemColor) {
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::ZERO, blend::ONE);
		} else {
			gfxContext.enable(enable::BLEND, false);
		}
	} else {
		gfxContext.enable(enable::BLEND, false);
	}
}

void GraphicsDrawer::_dualSourceBlending() const
{
	if (gDP.otherMode.cycleType < G_CYC_COPY) {
		BlendParam srcFactor = blend::ONE;
		BlendParam dstFactor = blend::SRC1_COLOR;
		BlendParam srcFactorAlpha = blend::ONE;
		BlendParam dstFactorAlpha = blend::SRC1_ALPHA;
		if (gDP.otherMode.forceBlender != 0) {
			if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
				if (gDP.otherMode.c2_m2a != 1 && gDP.otherMode.c2_m2b == 1) {
					srcFactor = blend::DST_ALPHA;
				}
				if (gDP.otherMode.c2_m2a == 1 && gDP.otherMode.c2_m2b == 1) {
					dstFactor = blend::DST_ALPHA;
				}
			} else {
				if (gDP.otherMode.c1_m2a != 1 && gDP.otherMode.c1_m2b == 1) {
					srcFactor = blend::DST_ALPHA;
				}
				if (gDP.otherMode.c1_m2a == 1 && gDP.otherMode.c2_m2b == 1) {
					dstFactor = blend::DST_ALPHA;
				}
			}
		}

		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlendingSeparate(srcFactor, dstFactor, srcFactorAlpha, dstFactorAlpha);
	} else {
		gfxContext.enable(enable::BLEND, false);
	}
}

void GraphicsDrawer::setBlendMode(bool _forceLegacyBlending) const
{
	bool blastCorpsHack = (config.generalEmulation.hacks & hack_blastCorps) != 0 &&
						  gSP.texture.on == 0 && gDP.otherMode.cycleType < G_CYC_COPY && currentCombiner()->usesTexture();

	if (blastCorpsHack) {
		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(blend::ZERO, blend::ONE);
		return;
	}

	if (_forceLegacyBlending || config.generalEmulation.enableLegacyBlending != 0) {
		_legacyBlending();
		return;
	}

	if (Context::DualSourceBlending && !isTexrectDrawerMode()) {
		_dualSourceBlending();
		return;
	}

	if (Context::FramebufferFetchColor && !isTexrectDrawerMode()) {
		gfxContext.enable(enable::BLEND, false);
		return;
	}

	_ordinaryBlending();
}

void GraphicsDrawer::setBgDepthCopyMode(BgDepthCopyMode mode)
{
	m_depthCopyMode = mode;
}

GraphicsDrawer::BgDepthCopyMode GraphicsDrawer::getBgDepthCopyMode() const
{
	return m_depthCopyMode;
}

void GraphicsDrawer::_updateTextures() const
{
	//For some reason updating the texture cache on the first frame of LOZ:OOT causes a nullptr Pointer exception...
	CombinerInfo & cmbInfo = CombinerInfo::get();
	CombinerProgram * pCurrentCombiner = cmbInfo.getCurrent();
	if (pCurrentCombiner != nullptr) {
		for (u32 t = 0; t < 2; ++t) {
			if (pCurrentCombiner->usesTile(t))
				textureCache().update(t);
			else
				textureCache().activateDummy(t);
		}
	}
	gDP.changed &= ~(CHANGED_TILE | CHANGED_TMEM);
	gSP.changed &= ~(CHANGED_TEXTURE);
}

void GraphicsDrawer::_updateStates(DrawingState _drawingState) const
{
	CombinerInfo & cmbInfo = CombinerInfo::get();
	cmbInfo.setPolygonMode(_drawingState);
	cmbInfo.update();

	if (gSP.changed & CHANGED_GEOMETRYMODE) {
		_updateCullFace();
		gSP.changed &= ~CHANGED_GEOMETRYMODE;
	}

	_updateDepthCompare();

	if (gDP.changed & CHANGED_SCISSOR)
		updateScissor(frameBufferList().getCurrent());

	if (gSP.changed & CHANGED_VIEWPORT)
		_updateViewport();

	if ((gSP.changed & CHANGED_TEXTURE) ||
		(gDP.changed & (CHANGED_TILE | CHANGED_TMEM)) ||
		cmbInfo.isChanged() ||
		_drawingState == DrawingState::TexRect) {
		_updateTextures();
	}

	if ((gDP.changed & (CHANGED_RENDERMODE | CHANGED_CYCLETYPE))) {
		setBlendMode();
		gDP.changed &= ~(CHANGED_RENDERMODE | CHANGED_CYCLETYPE);
	}

	cmbInfo.updateParameters();

	if (config.generalEmulation.enableFragmentDepthWrite == 0u)
		return;

	auto setDepthCopyParameters = []() {
		gfxContext.enable(enable::DEPTH_TEST, true);
		gfxContext.setDepthCompare(compare::ALWAYS);
		gfxContext.enableDepthWrite(true);
		gDP.changed |= CHANGED_RENDERMODE;
	};

	if (m_depthCopyMode >= BgDepthCopyMode::eBg1cyc) {
		DepthBufferList & dbList = depthBufferList();
		FrameBufferList & fbList = frameBufferList();

		// The game copies content of depth buffer into current color buffer
		// OpenGL has different format for color and depth buffers, so this trick can't be performed directly
		// To do that, depth buffer with address of current color buffer created and attached to the current FBO
		// It will be copy depth buffer
		dbList.saveBuffer(gDP.colorImage.address);

		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			DepthBuffer * pFromDepthBuffer = dbList.findBuffer(gSP.bgImage.address);
			if (pFromDepthBuffer == nullptr)
				return;

			DepthBuffer * pToDepthBuffer = dbList.findBuffer(gDP.colorImage.address);
			if (pToDepthBuffer == nullptr)
				return;

			if (Context::FramebufferFetchDepth) {
				FrameBuffer * pFrameBuffer = fbList.findBuffer(gDP.colorImage.address);
				if (pFrameBuffer == nullptr)
					return;
				Context::FrameBufferRenderTarget targetParams;
				targetParams.bufferHandle = pFrameBuffer->m_FBO;
				targetParams.bufferTarget = bufferTarget::DRAW_FRAMEBUFFER;
				targetParams.attachment = bufferAttachment::COLOR_ATTACHMENT1;
				targetParams.textureHandle = pFromDepthBuffer->m_pDepthImageZTexture->name;
				targetParams.textureTarget = textureTarget::TEXTURE_2D;
				gfxContext.addFrameBufferRenderTarget(targetParams);

				targetParams.attachment = bufferAttachment::COLOR_ATTACHMENT2;
				targetParams.textureHandle = pFromDepthBuffer->m_pDepthImageDeltaZTexture->name;
				gfxContext.addFrameBufferRenderTarget(targetParams);

				targetParams.attachment = bufferAttachment::COLOR_ATTACHMENT3;
				targetParams.textureHandle = pToDepthBuffer->m_pDepthImageZTexture->name;
				gfxContext.addFrameBufferRenderTarget(targetParams);

				targetParams.attachment = bufferAttachment::COLOR_ATTACHMENT4;
				targetParams.textureHandle = pToDepthBuffer->m_pDepthImageDeltaZTexture->name;
				gfxContext.addFrameBufferRenderTarget(targetParams);

				gfxContext.setDrawBuffers(5);
			} else if (Context::ImageTextures) {
				Context::BindImageTextureParameters bindParams;
				bindParams.imageUnit = textureImageUnits::DepthZ;
				bindParams.texture = pFromDepthBuffer->m_pDepthImageZTexture->name;
				bindParams.accessMode = textureImageAccessMode::READ_WRITE;
				bindParams.textureFormat = gfxContext.getFramebufferTextureFormats().depthImageInternalFormat;
				gfxContext.bindImageTexture(bindParams);

				bindParams.imageUnit = textureImageUnits::DepthDeltaZ;
				bindParams.texture = pFromDepthBuffer->m_pDepthImageDeltaZTexture->name;
				gfxContext.bindImageTexture(bindParams);

				bindParams.imageUnit = textureImageUnits::DepthZCopy;
				bindParams.texture = pToDepthBuffer->m_pDepthImageZTexture->name;
				gfxContext.bindImageTexture(bindParams);

				bindParams.imageUnit = textureImageUnits::DepthDeltaZCopy;
				bindParams.texture = pToDepthBuffer->m_pDepthImageDeltaZTexture->name;
				gfxContext.bindImageTexture(bindParams);
			}
			return;
		}

		FrameBuffer * pCopyDepthFrameBuffer = fbList.findBuffer(gSP.bgImage.address);
		if (pCopyDepthFrameBuffer == nullptr)
			return;

		DepthBuffer * pCopyDepthBuffer = dbList.findBuffer(gSP.bgImage.address);
		if (pCopyDepthBuffer == nullptr)
			return;

		CachedTexture * pCopyDepthTex = pCopyDepthBuffer->resolveDepthBufferTexture(pCopyDepthFrameBuffer);
		if (pCopyDepthTex == nullptr)
			return;

		Context::TexParameters params;
		params.handle = pCopyDepthTex->name;
		params.target = textureTarget::TEXTURE_2D;
		params.textureUnitIndex = textureIndices::Tex[0];
		params.maxMipmapLevel = 0;
		params.minFilter = textureParameters::FILTER_NEAREST;
		params.magFilter = textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(params);

		if (m_depthCopyMode == BgDepthCopyMode::eBgCopy) {
			FrameBuffer * pCurDepthFrameBuffer = fbList.findBuffer(gDP.depthImageAddress);
			if (pCurDepthFrameBuffer == nullptr)
				return;
			CachedTexture * pCurDepthTexture = pCurDepthFrameBuffer->m_pDepthBuffer->copyDepthBufferTexture(pCurDepthFrameBuffer);
			if (pCurDepthTexture == nullptr)
				return;

			params.handle = pCurDepthTexture->name;
			params.textureUnitIndex = textureIndices::DepthTex;
			gfxContext.setTextureParameters(params);
		}

		setDepthCopyParameters();
		gDP.changed |= CHANGED_TMEM;
	} else if (m_depthCopyMode != BgDepthCopyMode::eCopyDone &&
		isCurrentColorImageDepthImage() &&
		config.generalEmulation.enableFragmentDepthWrite != 0 &&
		config.frameBufferEmulation.N64DepthCompare == Config::dcDisable) {
		// Current render target is depth buffer.
		// Shader will set gl_FragDepth to shader color, see ShaderCombiner ctor
		// Here we enable depth buffer write.
		if (gDP.otherMode.cycleType <= G_CYC_2CYCLE && gDP.otherMode.depthCompare != 0) {
			// Render to depth buffer with depth compare. Need to get copy of current depth buffer.
			FrameBuffer * pCurBuf = frameBufferList().getCurrent();
			if (pCurBuf != nullptr && pCurBuf->m_pDepthBuffer != nullptr) {
				CachedTexture * pDepthTexture = pCurBuf->m_pDepthBuffer->copyDepthBufferTexture(pCurBuf);
				if (pDepthTexture == nullptr)
					return;
				Context::TexParameters params;
				params.handle = pDepthTexture->name;
				params.target = textureTarget::TEXTURE_2D;
				params.textureUnitIndex = textureIndices::DepthTex;
				params.maxMipmapLevel = 0;
				params.minFilter = textureParameters::FILTER_NEAREST;
				params.magFilter = textureParameters::FILTER_NEAREST;
				gfxContext.setTextureParameters(params);
			}
		} else if (frameBufferList().getCurrent() == nullptr) {
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::ZERO, blend::ONE);
		}
		setDepthCopyParameters();
	}
}

void GraphicsDrawer::_prepareDrawTriangle(DrawingState _drawingState)
{
	m_texrectDrawer.draw();

	if ((m_modifyVertices & MODIFY_XY) != 0)
		gSP.changed &= ~CHANGED_VIEWPORT;

	m_drawingState = _drawingState;

	if (gSP.changed || gDP.changed)
		_updateStates(_drawingState);

	bool bFlatColors = false;
	if (!RSP.LLE && (gSP.geometryMode & G_LIGHTING) == 0) {
		bFlatColors = (gSP.geometryMode & G_SHADE) == 0;
		bFlatColors |= (gSP.geometryMode & G_SHADING_SMOOTH) == 0;
	}
	m_bFlatColors = bFlatColors;

	if ((m_modifyVertices & MODIFY_XY) != 0)
		_updateViewport();
	m_modifyVertices = 0;
}

bool GraphicsDrawer::_canDraw() const
{
	return config.frameBufferEmulation.enable == 0 || frameBufferList().getCurrent() != nullptr;
}

void GraphicsDrawer::drawTriangles()
{
	if (triangles.num == 0 || !_canDraw()) {
		triangles.num = 0;
		triangles.maxElement = 0;
		return;
	}

	_prepareDrawTriangle(DrawingState::Triangle);
	Context::DrawTriangleParameters triParams;
	triParams.mode = drawmode::TRIANGLES;
	triParams.flatColors = m_bFlatColors;
	triParams.elementsType = datatype::UNSIGNED_BYTE;
	triParams.verticesCount = static_cast<u32>(triangles.maxElement) + 1;
	triParams.elementsCount = triangles.num;
	triParams.vertices = triangles.vertices.data();
	triParams.elements = triangles.elements.data();
	triParams.combiner = currentCombiner();
	g_debugger.addTriangles(triParams);

	if (config.frameBufferEmulation.enable != 0) {
		f32 maxY;
		if (config.generalEmulation.enableClipping != 0) {
			maxY = renderAndDrawTriangles(triangles.vertices.data(), triangles.elements.data(), triangles.num, m_bFlatColors, m_statistics);
		} else {
			gfxContext.drawTriangles(triParams);
			maxY = renderTriangles(triangles.vertices.data(), triangles.elements.data(), triangles.num);
		}
		frameBufferList().setBufferChanged(maxY);
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdSoftwareRender &&
			gDP.otherMode.depthUpdate != 0) {
			FrameBuffer * pCurrentDepthBuffer = frameBufferList().findBuffer(gDP.depthImageAddress);
			if (pCurrentDepthBuffer != nullptr)
				pCurrentDepthBuffer->setDirty();
		}
	} else {
		gfxContext.drawTriangles(triParams);
	}

	triangles.num = 0;
	triangles.maxElement = 0;
	dropRenderState();
}

void GraphicsDrawer::drawScreenSpaceTriangle(u32 _numVtx, graphics::DrawModeParam _mode)
{
	if (_numVtx == 0 || !_canDraw())
		return;

	f32 maxY = 0;
	for (u32 i = 0; i < _numVtx; ++i) {
		SPVertex & vtx = m_dmaVertices[i];
		vtx.modify = MODIFY_ALL;
		maxY = std::max(maxY, vtx.y);

		vtx.clip = 0;
		if (vtx.x > gSP.viewport.width) vtx.clip |= CLIP_POSX;
		if (vtx.x < 0) vtx.clip |= CLIP_NEGX;
		if (vtx.y > gSP.viewport.height) vtx.clip |= CLIP_POSY;
		if (vtx.y < 0) vtx.clip |= CLIP_NEGY;

		vtx.bc0 = (i % 3 == 0) ? 1.0f : 0.0f;
		vtx.bc1 = (i % 3 == 1) ? 1.0f : 0.0f;
	}
	m_modifyVertices = MODIFY_ALL;

	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	_prepareDrawTriangle(DrawingState::ScreenSpaceTriangle);
	gfxContext.enable(enable::CULL_FACE, false);

	Context::DrawTriangleParameters triParams;
	triParams.mode = _mode;
	triParams.flatColors = m_bFlatColors;
	triParams.verticesCount = _numVtx;
	triParams.vertices = m_dmaVertices.data();
	triParams.combiner = currentCombiner();
	gfxContext.drawTriangles(triParams);
	g_debugger.addTriangles(triParams);
	m_dmaVerticesNum = 0;

	if (config.frameBufferEmulation.enable != 0) {
		const f32 maxY = renderScreenSpaceTriangles(m_dmaVertices.data(), _numVtx, _mode);
		frameBufferList().setBufferChanged(maxY);
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdSoftwareRender &&
			gDP.otherMode.depthUpdate != 0) {
			FrameBuffer * pCurrentDepthBuffer = frameBufferList().findBuffer(gDP.depthImageAddress);
			if (pCurrentDepthBuffer != nullptr)
				pCurrentDepthBuffer->setDirty();
		}
	}
	gSP.changed |= CHANGED_GEOMETRYMODE;
	if (_mode == graphics::drawmode::TRIANGLES)
		m_statistics.drawnTris += _numVtx / 3;
	else if (_mode == graphics::drawmode::TRIANGLE_STRIP)
		m_statistics.drawnTris += _numVtx - 2;
	dropRenderState();
}

void GraphicsDrawer::drawDMATriangles(u32 _numVtx)
{
	if (_numVtx == 0 || !_canDraw())
		return;
	_prepareDrawTriangle(DrawingState::Triangle);

	Context::DrawTriangleParameters triParams;
	triParams.mode = drawmode::TRIANGLES;
	triParams.flatColors = m_bFlatColors;
	triParams.verticesCount = _numVtx;
	triParams.vertices = m_dmaVertices.data();
	triParams.combiner = currentCombiner();
	g_debugger.addTriangles(triParams);
	m_dmaVerticesNum = 0;
	m_statistics.drawnTris += _numVtx / 3;

	if (config.frameBufferEmulation.enable != 0) {
		f32 maxY;
		if (config.generalEmulation.enableClipping != 0) {
			maxY = renderAndDrawTriangles(m_dmaVertices.data(), nullptr, _numVtx, m_bFlatColors, m_statistics);
		}
		else {
			gfxContext.drawTriangles(triParams);
			maxY = renderTriangles(m_dmaVertices.data(), nullptr, _numVtx);
		}
		frameBufferList().setBufferChanged(maxY);
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdSoftwareRender &&
			gDP.otherMode.depthUpdate != 0) {
			FrameBuffer * pCurrentDepthBuffer = frameBufferList().findBuffer(gDP.depthImageAddress);
			if (pCurrentDepthBuffer != nullptr)
				pCurrentDepthBuffer->setDirty();
		}
	} else {
		gfxContext.drawTriangles(triParams);
	}
	dropRenderState();
}

void GraphicsDrawer::_drawThickLine(u32 _v0, u32 _v1, float _width)
{
	if ((gSP.geometryMode & G_LIGHTING) == 0) {
		if ((gSP.geometryMode & G_SHADE) == 0) {
			SPVertex & vtx1 = triangles.vertices[_v0];
			vtx1.flat_r = gDP.primColor.r;
			vtx1.flat_g = gDP.primColor.g;
			vtx1.flat_b = gDP.primColor.b;
			vtx1.flat_a = gDP.primColor.a;
			SPVertex & vtx2 = triangles.vertices[_v1];
			vtx2.flat_r = gDP.primColor.r;
			vtx2.flat_g = gDP.primColor.g;
			vtx2.flat_b = gDP.primColor.b;
			vtx2.flat_a = gDP.primColor.a;
		}
		else if ((gSP.geometryMode & G_SHADING_SMOOTH) == 0) {
			// Flat shading
			SPVertex & vtx0 = triangles.vertices[_v0 + ((RSP.w1 >> 24) & 3)];
			SPVertex & vtx1 = triangles.vertices[_v0];
			vtx1.r = vtx1.flat_r = vtx0.r;
			vtx1.g = vtx1.flat_g = vtx0.g;
			vtx1.b = vtx1.flat_b = vtx0.b;
			vtx1.a = vtx1.flat_a = vtx0.a;
			SPVertex & vtx2 = triangles.vertices[_v1];
			vtx2.r = vtx2.flat_r = vtx0.r;
			vtx2.g = vtx2.flat_g = vtx0.g;
			vtx2.b = vtx2.flat_b = vtx0.b;
			vtx2.a = vtx2.flat_a = vtx0.a;
		}
	}

	setDMAVerticesSize(4);
	SPVertex * pVtx = getDMAVerticesData();
	const f32 ySign = GBI.isNegativeY() ? -1.0f : 1.0f;
	pVtx[0] = triangles.vertices[_v0];
	pVtx[0].x = pVtx[0].x / pVtx[0].w * gSP.viewport.vscale[0] + gSP.viewport.vtrans[0];
	pVtx[0].y = ySign * pVtx[0].y / pVtx[0].w * gSP.viewport.vscale[1] + gSP.viewport.vtrans[1];
	pVtx[0].z = pVtx[0].z / pVtx[0].w;
	pVtx[1] = pVtx[0];

	pVtx[2] = triangles.vertices[_v1];
	pVtx[2].x = pVtx[2].x / pVtx[2].w * gSP.viewport.vscale[0] + gSP.viewport.vtrans[0];
	pVtx[2].y = ySign * pVtx[2].y / pVtx[2].w * gSP.viewport.vscale[1] + gSP.viewport.vtrans[1];
	pVtx[2].z = pVtx[2].z / pVtx[2].w;
	pVtx[3] = pVtx[2];

	if (fabs(pVtx[0].y - pVtx[2].y) < 0.0001) {
		const f32 Y = pVtx[0].y;
		pVtx[0].y = pVtx[2].y = Y - _width;
		pVtx[1].y = pVtx[3].y = Y + _width;
	} else if (fabs(pVtx[0].x - pVtx[2].x) < 0.0001) {
		const f32 X = pVtx[0].x;
		pVtx[0].x = pVtx[2].x = X - _width;
		pVtx[1].x = pVtx[3].x = X + _width;
	} else {
		const f32 X0 = pVtx[0].x;
		const f32 Y0 = pVtx[0].y;
		const f32 X1 = pVtx[2].x;
		const f32 Y1 = pVtx[2].y;
		const f32 dx = X1 - X0;
		const f32 dy = Y1 - Y0;
		const f32 len = sqrtf(dx*dx + dy*dy);
		const f32 wx = dy * _width / len;
		const f32 wy = dx * _width / len;
		pVtx[0].x = X0 + wx;
		pVtx[0].y = Y0 - wy;
		pVtx[1].x = X0 - wx;
		pVtx[1].y = Y0 + wy;
		pVtx[2].x = X1 + wx;
		pVtx[2].y = Y1 - wy;
		pVtx[3].x = X1 - wx;
		pVtx[3].y = Y1 + wy;
	}
	drawScreenSpaceTriangle(4);
}

void GraphicsDrawer::drawLine(u32 _v0, u32 _v1, float _width)
{
	m_texrectDrawer.draw();
	m_statistics.lines++;

	if (!_canDraw())
		return;

	f32 lineWidth = _width;
	if (config.frameBufferEmulation.nativeResFactor == 0)
		lineWidth *= dwnd().getScaleX();
	else
		lineWidth *= config.frameBufferEmulation.nativeResFactor;
	if (lineWidth > m_maxLineWidth) {
		_drawThickLine(_v0, _v1, _width * 0.5f);
		return;
	}

	if ((triangles.vertices[_v0].modify & MODIFY_XY) != 0)
		gSP.changed &= ~CHANGED_VIEWPORT;
	if (gSP.changed || gDP.changed)
		_updateStates(DrawingState::Line);

	m_drawingState = DrawingState::Line;

	if ((triangles.vertices[_v0].modify & MODIFY_XY) != 0)
		_updateViewport();

	SPVertex vertexBuf[2] = { triangles.vertices[_v0], triangles.vertices[_v1] };
	gfxContext.drawLine(lineWidth, vertexBuf);
	dropRenderState();
}

void GraphicsDrawer::drawRect(int _ulx, int _uly, int _lrx, int _lry)
{
	m_texrectDrawer.draw();
	m_statistics.fillRects++;

	if (!_canDraw())
		return;

	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	gSP.changed &= ~CHANGED_VIEWPORT; // Don't update viewport
	if (gSP.changed || gDP.changed)
		_updateStates(DrawingState::Rect);

	m_drawingState = DrawingState::Rect;

	_updateViewport();

	gfxContext.enable(enable::CULL_FACE, false);

	const float Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f;
	const float W = 1.0f;
	m_rect[0].x = static_cast<f32>(_ulx);
	m_rect[0].y = static_cast<f32>(_uly);
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = static_cast<f32>(_lrx);
	m_rect[1].y = m_rect[0].y;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = static_cast<f32>(_lry);
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	m_rect[0].bc0 = 0.0f;
	m_rect[0].bc1 = 0.0f;
	m_rect[1].bc0 = 0.0f;
	m_rect[1].bc1 = 1.0f;
	m_rect[2].bc0 = 1.0f;
	m_rect[2].bc1 = 0.0f;
	m_rect[3].bc0 = 1.0f;
	m_rect[3].bc1 = 1.0f;

	DisplayWindow & wnd = dwnd();
	if (wnd.isAdjustScreen() && (gDP.colorImage.width > VI.width * 98 / 100) && (static_cast<u32>(_lrx - _ulx) < VI.width * 9 / 10)) {
		const float scale = wnd.getAdjustScale();
		const float offsetx = static_cast<f32>(gDP.colorImage.width) * (1.0f - scale) / 2.0f;
		for (u32 i = 0; i < 4; ++i) {
			m_rect[i].x *= scale;
			m_rect[i].x += offsetx;
		}
	}

	Context::DrawRectParameters rectParams;
	rectParams.mode = drawmode::TRIANGLE_STRIP;
	rectParams.texrect = false;
	rectParams.verticesCount = 4;
	rectParams.vertices = m_rect;
	rectParams.combiner = currentCombiner();
	gfxContext.drawRects(rectParams);
	g_debugger.addRects(rectParams);
	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
	dropRenderState();
}

static
bool texturedRectShadowMap(const GraphicsDrawer::TexturedRectParams &)
{
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	if (pCurrentBuffer != nullptr) {
		if (gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&
			gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*gDP.colorImage.width * 6 / 4)) {

			if (!Context::IntegerTextures)
				return true;

			pCurrentBuffer->m_pDepthBuffer->activateDepthBufferTexture(pCurrentBuffer);
			CombinerInfo::get().setDepthFogCombiner();
			// DepthFogCombiner does not support shader blending.
			dwnd().getDrawer().setBlendMode(true);
			return false;
		}
	}
	return false;
}

u32 rectDepthBufferCopyFrame = 0xFFFFFFFF;
static
bool texturedRectDepthBufferCopy(const GraphicsDrawer::TexturedRectParams & _params)
{
	// Copy one line from depth buffer into auxiliary color buffer with height = 1.
	// Data from depth buffer loaded into TMEM and then rendered to RDRAM by texrect.
	// Works only with depth buffer emulation enabled.
	// Load of arbitrary data to that area causes weird camera rotation in CBFD.
	if (_params.uly != 0.0f || std::min(_params.lry, gDP.scissor.lry) != 1.0f)
		return false;
	const gDPTile * pTile = gSP.textureTile[0];
	if (pTile->loadType == LOADTYPE_BLOCK && gDP.textureImage.size == 2 &&
		gDP.textureImage.address >= gDP.depthImageAddress &&
		gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*VI.height*2)) {
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdDisable)
			return true;
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer == nullptr)
			return true;
		pBuffer->m_cleared = true;
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdCopyFromVRam) {
			if (rectDepthBufferCopyFrame != dwnd().getBuffersSwapCount()) {
				rectDepthBufferCopyFrame = dwnd().getBuffersSwapCount();
				if (!FrameBuffer_CopyDepthBuffer(gDP.depthImageAddress))
					return true;
			}
			RDP_RepeatLastLoadBlock();
		}

		const u32 width = static_cast<u32>(_params.lrx - _params.ulx);
		const u32 ulx = static_cast<u32>(_params.ulx);
		u16 * pSrc = reinterpret_cast<u16*>(TMEM) + _params.s/32;
		u16 *pDst = reinterpret_cast<u16*>(RDRAM + gDP.colorImage.address);
		for (u32 x = 0; x < width; ++x)
			pDst[(ulx + x) ^ 1] = swapword(pSrc[x]);

		return true;
	}
	return false;
}

static
bool texturedRectCopyToItself(const GraphicsDrawer::TexturedRectParams & _params)
{
	FrameBuffer * pCurrent = frameBufferList().getCurrent();
	if (pCurrent != nullptr && pCurrent->m_size == G_IM_SIZ_8b && gSP.textureTile[0]->frameBufferAddress == pCurrent->m_startAddress)
		return true;
	return texturedRectDepthBufferCopy(_params);
}

static
bool texturedRectBGCopy(const GraphicsDrawer::TexturedRectParams & _params)
{
	if (gDP.colorImage.size > G_IM_SIZ_8b)
		return false;

	float flry = _params.lry;
	if (flry > gDP.scissor.lry)
		flry = gDP.scissor.lry;

	const u32 width = static_cast<u32>(_params.lrx - _params.ulx);
	const u32 tex_width = gSP.textureTile[0]->line << 3;
	const u32 uly = static_cast<u32>(_params.uly);
	const u32 lry = static_cast<u32>(flry);

	u8 * texaddr = RDRAM + gDP.loadInfo[gSP.textureTile[0]->tmem].texAddress + tex_width*_params.t/32 + _params.s/32;
	u8 * fbaddr = RDRAM + gDP.colorImage.address + static_cast<u32>(_params.ulx);
	//	LOG(LOG_VERBOSE, "memrect (%d, %d, %d, %d), ci_width: %d texaddr: 0x%08lx fbaddr: 0x%08lx\n", (u32)_params.ulx, uly, (u32)_params.lrx, lry, gDP.colorImage.width, gSP.textureTile[0]->imageAddress + tex_width*(u32)_params.ult + (u32)_params.uls, gDP.colorImage.address + (u32)_params.ulx);

	for (u32 y = uly; y < lry; ++y) {
		u8 *src = texaddr + (y - uly) * tex_width;
		u8 *dst = fbaddr + y * gDP.colorImage.width;
		memcpy(dst, src, width);
	}
	frameBufferList().removeBuffer(gDP.colorImage.address);
	return true;
}

static
bool texturedRectPaletteMod(const GraphicsDrawer::TexturedRectParams & _params)
{
	if (gDP.textureImage.address == 0x400) {
		// Paper Mario uses complex set of actions to prepare darkness texture.
		// It includes manipulations with texture formats and drawing buffer into itsels.
		// All that stuff is hardly possible to reproduce with GL, so I just use dirty hacks to emualte it.

		if (gDP.colorImage.address == 0x400 && gDP.colorImage.width == 64) {
			memcpy(RDRAM + 0x400, RDRAM + 0x14d500, 4096);
			return true;
		}

		if (gDP.textureImage.width == 64) {
			gDPTile & curTile = gDP.tiles[0];
			curTile.frameBufferAddress = 0;
			curTile.textureMode = TEXTUREMODE_NORMAL;
			textureCache().update(0);
			currentCombiner()->update(false);
		}
		return false;
	}

	// Modify palette for Paper Mario "2D lighting" effect
	if (gDP.scissor.lrx != 16 || gDP.scissor.lry != 1 || _params.lrx != 16 || _params.lry != 1)
		return false;
	u8 envr = static_cast<u8>(gDP.envColor.r * 31.0f);
	u8 envg = static_cast<u8>(gDP.envColor.g * 31.0f);
	u8 envb = static_cast<u8>(gDP.envColor.b * 31.0f);
	u16 env16 = static_cast<u16>((envr << 11) | (envg << 6) | (envb << 1) | 1);
	u8 prmr = static_cast<u8>(gDP.primColor.r * 31.0f);
	u8 prmg = static_cast<u8>(gDP.primColor.g * 31.0f);
	u8 prmb = static_cast<u8>(gDP.primColor.b * 31.0f);
	u16 prim16 = static_cast<u16>((prmr << 11) | (prmg << 6) | (prmb << 1) | 1);
	u16 * src = reinterpret_cast<u16*>(&TMEM[256]);
	u16 * dst = reinterpret_cast<u16*>(RDRAM + gDP.colorImage.address);
	for (u32 i = 0; i < 16; ++i)
		dst[i ^ 1] = (src[i << 2] & 0x100) ? prim16 : env16;
	return true;
}

// Special processing of textured rect.
// Return true if actuial rendering is not necessary
static bool(*texturedRectSpecial)(const GraphicsDrawer::TexturedRectParams & _params) = nullptr;

void GraphicsDrawer::drawTexturedRect(const TexturedRectParams & _params)
{
	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	m_drawingState = DrawingState::TexRect;
	m_statistics.texRects++;

	if (m_texrectDrawer.canContinue()) {
		CombinerInfo & cmbInfo = CombinerInfo::get();
		cmbInfo.setPolygonMode(DrawingState::TexRect);
		cmbInfo.update();
		_updateTextures();
		cmbInfo.updateParameters();
	} else {
		if (!m_texrectDrawer.isEmpty())
			m_texrectDrawer.draw();
		gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
		gSP.changed &= ~CHANGED_VIEWPORT; // Don't update viewport
		if (_params.texrectCmd && (gSP.changed | gDP.changed) != 0)
			_updateStates(DrawingState::TexRect);
		gfxContext.enable(enable::CULL_FACE, false);

		if (_params.texrectCmd && texturedRectSpecial != nullptr && texturedRectSpecial(_params)) {
			gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
			return;
		}

		if (_params.texrectCmd && !_canDraw())
			return;
	}

	CombinerProgram * pCurrentCombiner = currentCombiner();
	const FrameBuffer * pCurrentBuffer = _params.pBuffer;
	DisplayWindow & wnd = dwnd();
	TextureCache & cache = textureCache();
	const bool bUseBilinear = gDP.otherMode.textureFilter != 0;
	bool bUseFbTexture = false;
	bool bUseHdTexture = false;
	for (u32 i = 0; i < 2; ++i) {
		if (pCurrentCombiner->usesTile(i) && cache.current[i] != nullptr) {
			bUseFbTexture |= cache.current[i]->frameBufferTexture != CachedTexture::fbNone;
			bUseHdTexture |= cache.current[i]->bHDTexture;
		}
	}
	const bool bUseTexrectDrawer = m_bBGMode || ((config.graphics2D.enableNativeResTexrects != 0)
		&& bUseBilinear
		&& pCurrentCombiner->usesTexture()
		&& (pCurrentBuffer == nullptr || !pCurrentBuffer->m_cfb)
		&& (cache.current[0] != nullptr)
		//		&& (cache.current[0] == nullptr || cache.current[0]->format == G_IM_FMT_RGBA || cache.current[0]->format == G_IM_FMT_CI)
		&& !bUseFbTexture && !bUseHdTexture);
	const float Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : 0.0f;
	const float W = 1.0f;
	const f32 ulx = _params.ulx;
	const f32 uly = _params.uly;
	const f32 lrx = _params.lrx;
	const f32 lry = _params.lry;

	m_rect[0].x = ulx;
	m_rect[0].y = uly;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = lrx;
	m_rect[1].y = uly;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = ulx;
	m_rect[2].y = lry;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = lrx;
	m_rect[3].y = lry;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	struct
	{
		float s0, t0, s1, t1;
	} texST[2] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }; //struct for texture coordinates

	float offsetX, offsetY;
	if (_params.flip) {
		offsetX = (_params.lry - _params.uly) * _params.dsdx;
		offsetY = (_params.lrx - _params.ulx) * _params.dtdy;
	} else {
		offsetX = (_params.lrx - _params.ulx) * _params.dsdx;
		offsetY = (_params.lry - _params.uly) * _params.dtdy;
	}

	if (config.generalEmulation.enableInaccurateTextureCoordinates == 0u) {
		// Accurate texture path
		texST[0].s0 = _FIXED2FLOAT(_params.s, 5);
		texST[0].s1 = texST[0].s0 + offsetX;
		texST[0].t0 = _FIXED2FLOAT(_params.t, 5);
		texST[0].t1 = texST[0].t0 + offsetY;
	} else {
		// Fast texture path
		for (u32 t = 0; t < 2; ++t) {
			if (pCurrentCombiner->usesTile(t) && cache.current[t] && gSP.textureTile[t]) {
				f32 shiftScaleS = 1.0f;
				f32 shiftScaleT = 1.0f;

				s16 S = _params.s;
				shiftScaleS = calcShiftScaleS(*gSP.textureTile[t], &S);
				const f32 uls = _FIXED2FLOAT(S, 5);
				const f32 lrs = uls + offsetX * shiftScaleS;

				s16 T = _params.t;
				shiftScaleT = calcShiftScaleT(*gSP.textureTile[t], &T);
				const f32 ult = _FIXED2FLOAT(T, 5);
				const f32 lrt = ult + offsetY * shiftScaleT;

				texST[t].s0 = uls - gSP.textureTile[t]->fuls;
				texST[t].s1 = lrs - gSP.textureTile[t]->fuls;
				texST[t].t0 = ult - gSP.textureTile[t]->fult;
				texST[t].t1 = lrt - gSP.textureTile[t]->fult;

				if (cache.current[t]->frameBufferTexture != CachedTexture::fbNone) {
					texST[t].s0 = cache.current[t]->offsetS + texST[t].s0;
					texST[t].t0 = cache.current[t]->offsetT + texST[t].t0;
					texST[t].s1 = cache.current[t]->offsetS + texST[t].s1;
					texST[t].t1 = cache.current[t]->offsetT + texST[t].t1;
				}

				texST[t].s0 *= cache.current[t]->scaleS;
				texST[t].t0 *= cache.current[t]->scaleT;
				texST[t].s1 *= cache.current[t]->scaleS;
				texST[t].t1 *= cache.current[t]->scaleT;
			}
		}
	}

	if (gDP.otherMode.cycleType == G_CYC_COPY && cache.current[0]->frameBufferTexture != CachedTexture::fbMultiSample) {
		Context::TexParameters texParams;
		texParams.handle = cache.current[0]->name;
		texParams.target = textureTarget::TEXTURE_2D;
		texParams.textureUnitIndex = textureIndices::Tex[0];
		texParams.minFilter = textureParameters::FILTER_NEAREST;
		texParams.magFilter = textureParameters::FILTER_NEAREST;
		gfxContext.setTextureParameters(texParams);
	}

	m_rect[0].s0 = texST[0].s0;
	m_rect[0].t0 = texST[0].t0;
	m_rect[0].s1 = texST[1].s0;
	m_rect[0].t1 = texST[1].t0;

	m_rect[3].s0 = texST[0].s1;
	m_rect[3].t0 = texST[0].t1;
	m_rect[3].s1 = texST[1].s1;
	m_rect[3].t1 = texST[1].t1;

	if (_params.flip) {
		m_rect[1].s0 = texST[0].s0;
		m_rect[1].t0 = texST[0].t1;
		m_rect[1].s1 = texST[1].s0;
		m_rect[1].t1 = texST[1].t1;

		m_rect[2].s0 = texST[0].s1;
		m_rect[2].t0 = texST[0].t0;
		m_rect[2].s1 = texST[1].s1;
		m_rect[2].t1 = texST[1].t0;
	} else {
		m_rect[1].s0 = texST[0].s1;
		m_rect[1].t0 = texST[0].t0;
		m_rect[1].s1 = texST[1].s1;
		m_rect[1].t1 = texST[1].t0;

		m_rect[2].s0 = texST[0].s0;
		m_rect[2].t0 = texST[0].t1;
		m_rect[2].s1 = texST[1].s0;
		m_rect[2].t1 = texST[1].t1;
	}

	if (wnd.isAdjustScreen() && !bUseFbTexture &&
		(_params.forceAjustScale ||
		((gDP.colorImage.width > VI.width * 98 / 100) && (static_cast<u32>(_params.lrx - _params.ulx) < VI.width * 9 / 10))))
	{
		const float scale = wnd.getAdjustScale();
		const float offsetx = static_cast<f32>(gDP.colorImage.width) * (1.0f - scale) / 2.0f;
		for (u32 i = 0; i < 4; ++i) {
			m_rect[i].x *= scale;
			m_rect[i].x += offsetx;
		}
	}

	m_rect[0].bc0 = 0.0f;
	m_rect[0].bc1 = 0.0f;
	m_rect[1].bc0 = 0.0f;
	m_rect[1].bc1 = 1.0f;
	m_rect[2].bc0 = 1.0f;
	m_rect[2].bc1 = 0.0f;
	m_rect[3].bc0 = 1.0f;
	m_rect[3].bc1 = 1.0f;


	if (bUseTexrectDrawer) {
		if (m_bBGMode) {
			m_texrectDrawer.addBackgroundRect();
			return;
		}
		if (m_texrectDrawer.addRect())
			return;
	}

	_updateViewport(_params.pBuffer);
	Context::DrawRectParameters rectParams;
	rectParams.mode = drawmode::TRIANGLE_STRIP;
	rectParams.verticesCount = 4;
	rectParams.vertices = m_rect;
	rectParams.combiner = currentCombiner();
	gfxContext.drawRects(rectParams);
	if (g_debugger.isCaptureMode()) {
		g_debugger.addRects(rectParams);
	}

	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
	dropRenderState();
}

void GraphicsDrawer::correctTexturedRectParams(TexturedRectParams & _params)
{
	if (config.graphics2D.correctTexrectCoords == Config::tcSmart) {
		if (_params.ulx == m_texrectParams.ulx && _params.lrx == m_texrectParams.lrx) {
			if (fabsf(_params.uly - m_texrectParams.lry) < 0.51f)
				_params.uly = m_texrectParams.lry;
			else if (fabsf(_params.lry - m_texrectParams.uly) < 0.51f)
				_params.lry = m_texrectParams.uly;
		}
		else if (_params.uly == m_texrectParams.uly && _params.lry == m_texrectParams.lry) {
			if (fabsf(_params.ulx - m_texrectParams.lrx) < 0.51f)
				_params.ulx = m_texrectParams.lrx;
			else if (fabsf(_params.lrx - m_texrectParams.ulx) < 0.51f)
				_params.lrx = m_texrectParams.ulx;
		}
	}
	else if (config.graphics2D.correctTexrectCoords == Config::tcForce) {
		_params.lrx += 0.25f;
		_params.lry += 0.25f;
	}

	m_texrectParams = _params;
}

void GraphicsDrawer::drawText(const char *_pText, float x, float y)
{
	m_drawingState = DrawingState::Non;
	g_textDrawer.drawText(_pText, x, y);
}

void GraphicsDrawer::Statistics::clear()
{
	fillRects = 0;
	texRects = 0;
	clippedTris = 0;
	rejectedTris = 0;
	culledTris = 0;
	drawnTris = 0;
	lines = 0;
}

void GraphicsDrawer::_drawOSD(const char *_pText, float _x, float & _y)
{
	float tW, tH;
	g_textDrawer.getTextSize(_pText, tW, tH);

	const bool top = (config.posTop & config.onScreenDisplay.pos) != 0;
	const bool right = (config.onScreenDisplay.pos == Config::posTopRight) || (config.onScreenDisplay.pos == Config::posBottomRight);
	const bool center = (config.onScreenDisplay.pos == Config::posTopCenter) || (config.onScreenDisplay.pos == Config::posBottomCenter);

	if (center)
		_x = -tW * 0.5f;
	else if (right)
		_x -= tW;

	if (top)
		_y -= tH;

	drawText(_pText, _x, _y);

	if (top)
		_y -= tH * 0.5f;
	else
		_y += tH * 1.5f;
}

void GraphicsDrawer::drawOSD()
{
	if ((config.onScreenDisplay.fps |
		config.onScreenDisplay.vis |
		config.onScreenDisplay.percent |
		config.onScreenDisplay.internalResolution |
		config.onScreenDisplay.renderingResolution |
		config.onScreenDisplay.statistics
		) == 0 &&
		m_osdMessages.empty())
		return;

	gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, ObjectHandle::defaultFramebuffer);

	DisplayWindow & wnd = DisplayWindow::get();
	const s32 X = (wnd.getScreenWidth() - wnd.getWidth()) / 2;
	const s32 Y = static_cast<s32>(wnd.getHeightOffset());
	const s32 W = static_cast<s32>(wnd.getWidth());
	const s32 H = static_cast<s32>(wnd.getHeight());

	gfxContext.setViewport(X, Y, W, H);
	gfxContext.setScissor(X, Y, W, H);

	gSP.changed |= CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_SCISSOR;


	const bool bottom = (config.posBottom & config.onScreenDisplay.pos) != 0;
	const bool left = (config.onScreenDisplay.pos == Config::posTopLeft) || (config.onScreenDisplay.pos == Config::posBottomLeft);

	const float hp = left ? -1.0f : 1.0f;
	const float vp = bottom ? -1.0f : 1.0f;

	float hShift, vShift;
	g_textDrawer.getTextSize("0", hShift, vShift);
	hShift *= 0.5f;
	vShift *= 0.5f;
	const float x = hp - hShift * hp;
	float y = vp - vShift * vp;
	char buf[256];

	if (config.onScreenDisplay.fps) {
		sprintf(buf, "%d FPS", int(perf.getFps()));
		_drawOSD(buf, x, y);
	}

	if (config.onScreenDisplay.vis) {
		sprintf(buf, "%d VI/S", int(perf.getVIs()));
		_drawOSD(buf, x, y);
	}

	if (config.onScreenDisplay.percent) {
		sprintf(buf, "%d %%", int(perf.getPercent()));
		_drawOSD(buf, x, y);
	}

	if (config.onScreenDisplay.renderingResolution) {
		sprintf(buf, "Rendering Resolution %ux%u", wnd.getScreenWidth(), wnd.getScreenHeight());
		_drawOSD(buf, x, y);
	}

	if (config.onScreenDisplay.internalResolution) {
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer != nullptr && VI.width != 0) {
			const float aspect = float(VI.height) / float(VI.width);
			const u32 height = u32(pBuffer->m_width * aspect);
			sprintf(buf, "Internal Resolution %ux%u", pBuffer->m_width, height);
			_drawOSD(buf, x, y);
		}
	}

	if (config.onScreenDisplay.statistics) {
		if (RSP.LLE)
			sprintf(buf, "fill rects: %3u | tex rects: %3u | triangles: %5u",
				m_statistics.fillRects, m_statistics.texRects, m_statistics.drawnTris);
		else
			sprintf(buf, "fill rects: %3u | tex rects: %3u | lines: %4u | tris drawn: %4u | clipped: %4u | culled: %4u | total: %5u",
				m_statistics.fillRects, m_statistics.texRects, m_statistics.lines,
				m_statistics.drawnTris, m_statistics.clippedTris, m_statistics.culledTris,
				m_statistics.drawnTris + m_statistics.clippedTris + m_statistics.culledTris);
		_drawOSD(buf, x, y);
	}


	for (const std::string & m : m_osdMessages) {
		_drawOSD(m.c_str(), x, y);
	}
}

void GraphicsDrawer::showMessage(std::string _message, Milliseconds _interval)
{
	m_osdMessages.emplace_back(_message);
	std::thread t(&GraphicsDrawer::_removeOSDMessage, this, std::prev(m_osdMessages.end()), _interval);
	t.detach();
}

void GraphicsDrawer::_removeOSDMessage(OSDMessages::iterator _iter, Milliseconds _interval)
{
	std::this_thread::sleep_for(_interval);
	m_osdMessages.erase(_iter);
}

void GraphicsDrawer::clearDepthBuffer()
{
	if (!_canDraw())
		return;

	depthBufferList().clearBuffer();

	_updateDepthUpdate();
}

void GraphicsDrawer::clearColorBuffer(float *_pColor)
{
	if (_pColor != nullptr)
		gfxContext.clearColorBuffer(_pColor[0], _pColor[1], _pColor[2], _pColor[3]);
	else
		gfxContext.clearColorBuffer(0.0f, 0.0f, 0.0f, 0.0f);
}

bool GraphicsDrawer::isClipped(u32 _v0, u32 _v1, u32 _v2) const
{
	if ((triangles.vertices[_v0].clip & triangles.vertices[_v1].clip & triangles.vertices[_v2].clip) != 0) {
		m_statistics.clippedTris++;
		return true;
	}
	return false;
}

bool GraphicsDrawer::isRejected(u32 _v0, u32 _v1, u32 _v2) const
{
	if (!GBI.isRej() || gSP.clipRatio < 2)
		return false;

	static gDPScissor rejectBox;
	if ((gDP.changed & CHANGED_REJECT_BOX) != 0) {
		const f32 scissorWidth2 = (gDP.scissor.lrx - gDP.scissor.ulx) * (gSP.clipRatio - 1) * 0.5f;
		const f32 scissorHeight2 = (gDP.scissor.lry - gDP.scissor.uly) * (gSP.clipRatio - 1) * 0.5f;
		rejectBox.ulx = gDP.scissor.ulx - scissorWidth2;
		rejectBox.lrx = gDP.scissor.lrx + scissorWidth2;
		rejectBox.uly = gDP.scissor.uly - scissorHeight2;
		rejectBox.lry = gDP.scissor.lry + scissorHeight2;
		gDP.changed ^= CHANGED_REJECT_BOX;
	}
	u32 verts[3] = { _v0, _v1, _v2 };
	const f32 ySign = GBI.isNegativeY() ? -1.0f : 1.0f;
	for (u32 i = 0; i < 3; ++i) {
		const SPVertex & v = triangles.vertices[verts[i]];
		if ((v.modify & MODIFY_XY) != 0)
			continue;
		const f32 sx = gSP.viewport.vtrans[0] + (v.x / v.w) * gSP.viewport.vscale[0];
		if (sx < rejectBox.ulx) {
			m_statistics.rejectedTris++;
			return true;
		}
		if (sx > rejectBox.lrx) {
			m_statistics.rejectedTris++;
			return true;
		}
		const f32 sy = gSP.viewport.vtrans[1] + (v.y / v.w) * gSP.viewport.vscale[1] * ySign;
		if (sy < rejectBox.uly) {
			m_statistics.rejectedTris++;
			return true;
		}
		if (sy > rejectBox.lry) {
			m_statistics.rejectedTris++;
			return true;
		}
	}
	return false;
}

void GraphicsDrawer::copyTexturedRect(const CopyRectParams & _params)
{
	m_drawingState = DrawingState::Non;

	const float scaleX = 1.0f / _params.dstWidth;
	const float scaleY = 1.0f / _params.dstHeight;
	const float Z = 0.0f;
	const float W = 1.0f;
	float X0 = _params.dstX0 * (2.0f * scaleX) - 1.0f;
	float Y0 = _params.dstY0 * (2.0f * scaleY) - 1.0f;
	float X1 = _params.dstX1 * (2.0f * scaleX) - 1.0f;
	float Y1 = _params.dstY1 * (2.0f * scaleY) - 1.0f;
	if (_params.invertX) {
		X0 = -X0;
		X1 = -X1;
	}
	if (_params.invertY) {
		Y0 = -Y0;
		Y1 = -Y1;
	}

	m_rect[0].x = X0;
	m_rect[0].y = Y0;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = X1;
	m_rect[1].y = Y0;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = X0;
	m_rect[2].y = Y1;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = X1;
	m_rect[3].y = Y1;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	const float scaleS = 1.0f / _params.srcWidth;
	const float scaleT = 1.0f / _params.srcHeight;

	const float S0 = _params.srcX0 * scaleS;
	const float S1 = _params.srcX1 * scaleS;
	const float T0 = _params.srcY0 * scaleT;
	const float T1 = _params.srcY1 * scaleT;

	m_rect[0].s0 = S0;
	m_rect[0].t0 = T0;
	m_rect[1].s0 = S1;
	m_rect[1].t0 = T0;
	m_rect[2].s0 = S0;
	m_rect[2].t0 = T1;
	m_rect[3].s0 = S1;
	m_rect[3].t0 = T1;

	for (u32 i = 0; i < 2; ++i) {
		CachedTexture * tex = _params.tex[i];
		if (tex == nullptr)
			continue;

		Context::TexParameters texParams;
		texParams.handle = tex->name;
		texParams.textureUnitIndex = textureIndices::Tex[i];
		if (tex->frameBufferTexture == CachedTexture::fbMultiSample)
			texParams.target = textureTarget::TEXTURE_2D_MULTISAMPLE;
		else {
			texParams.target = textureTarget::TEXTURE_2D;
			texParams.minFilter = _params.filter;
			texParams.magFilter = _params.filter;
			texParams.wrapS = textureParameters::WRAP_CLAMP_TO_EDGE;
			texParams.wrapT = textureParameters::WRAP_CLAMP_TO_EDGE;
		}
		gfxContext.setTextureParameters(texParams);
	}

	gfxContext.setViewport(0, 0, static_cast<s32>(_params.dstWidth), static_cast<s32>(_params.dstHeight));
	gfxContext.enable(enable::CULL_FACE, false);
	gfxContext.enable(enable::BLEND, false);

	if (config.frameBufferEmulation.copyDepthToMainDepthBuffer == 0 || _params.tex[1] == nullptr) {
		gfxContext.enable(enable::DEPTH_TEST, false);
		gfxContext.enableDepthWrite(false);
	} else {
		gfxContext.setDepthCompare(compare::ALWAYS);
		gfxContext.enableDepthWrite(true);
		gfxContext.enable(enable::DEPTH_TEST, true);
	}

	Context::DrawRectParameters rectParams;
	rectParams.mode = drawmode::TRIANGLE_STRIP;
	rectParams.verticesCount = 4;
	rectParams.vertices = m_rect;
	rectParams.combiner = _params.combiner;
	_params.combiner->activate();
	gfxContext.enable(enable::SCISSOR_TEST, false);
	gfxContext.drawRects(rectParams);
	gfxContext.enable(enable::SCISSOR_TEST, true);

	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_RENDERMODE | CHANGED_TILE | CHANGED_COMBINE;
}

void GraphicsDrawer::blitOrCopyTexturedRect(const BlitOrCopyRectParams & _params)
{
	Context::BlitFramebuffersParams blitParams;
	blitParams.readBuffer = _params.readBuffer;
	blitParams.drawBuffer = _params.drawBuffer;
	blitParams.srcX0 = _params.srcX0;
	blitParams.srcX1 = _params.srcX1;
	blitParams.dstX0 = _params.dstX0;
	blitParams.dstX1 = _params.dstX1;
	blitParams.srcY0 = _params.srcY0;
	blitParams.srcY1 = _params.srcY1;
	blitParams.dstY0 = _params.dstY0;
	blitParams.dstY1 = _params.dstY1;
	blitParams.mask = _params.mask;
	blitParams.filter = _params.filter;
	if (_params.invertX) {
		std::swap(blitParams.dstX0, blitParams.dstX1);
	}
	if (_params.invertY) {
		std::swap(blitParams.dstY0, blitParams.dstY1);
	}

	if (gfxContext.blitFramebuffers(blitParams))
		return;

	gfxContext.bindFramebuffer(bufferTarget::READ_FRAMEBUFFER, _params.readBuffer);
	gfxContext.bindFramebuffer(bufferTarget::DRAW_FRAMEBUFFER, _params.drawBuffer);
	copyTexturedRect(_params);
}

void GraphicsDrawer::_initStates()
{
	gfxContext.enable(enable::CULL_FACE, false);
	gfxContext.enable(enable::SCISSOR_TEST, true);
	gfxContext.enableDepthWrite(false);
	gfxContext.setDepthCompare(compare::ALWAYS);

	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
		gfxContext.enable(enable::DEPTH_TEST, false);
		gfxContext.enable(enable::POLYGON_OFFSET_FILL, false);
	}
	else {
		gfxContext.enable(enable::DEPTH_TEST, true);
#if defined(OS_ANDROID) || defined(OS_IOS)
		if (config.generalEmulation.forcePolygonOffset != 0)
			gfxContext.setPolygonOffset(config.generalEmulation.polygonOffsetFactor, config.generalEmulation.polygonOffsetUnits);
		else
#endif
			gfxContext.setPolygonOffset(-3.0f, -3.0f);
	}

	DisplayWindow & wnd = DisplayWindow::get();
	gfxContext.setViewport(0, static_cast<s32>(wnd.getHeightOffset()),
						   static_cast<s32>(wnd.getScreenWidth()), static_cast<s32>(wnd.getScreenHeight()));

	gfxContext.clearColorBuffer(0.0f, 0.0f, 0.0f, 0.0f);

	srand(static_cast<u32>(time(nullptr)));

	wnd.swapBuffers();
}

void GraphicsDrawer::_setSpecialTexrect() const
{
	std::string name(RSP.romname);
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);
#define FOUND(romname) name.find(romname) != std::string::npos

	if (FOUND("BEETLE") || FOUND("HSV") || FOUND("DUCK DODGERS") || FOUND("DAFFY DUCK"))
		texturedRectSpecial = texturedRectShadowMap;
	else if (FOUND("PERFECT DARK") || FOUND("TUROK_DINOSAUR_HUNTE"))
		texturedRectSpecial = texturedRectDepthBufferCopy; // See comments to that function!
	else if (FOUND("CONKER BFD"))
		texturedRectSpecial = texturedRectCopyToItself;
	else if (FOUND("YOSHI STORY"))
		texturedRectSpecial = texturedRectBGCopy;
	else if (FOUND("PAPER MARIO") || FOUND("MARIO STORY"))
		texturedRectSpecial = texturedRectPaletteMod;
	else
		texturedRectSpecial = nullptr;
}

void GraphicsDrawer::_initData()
{
	_initStates();
	_setSpecialTexrect();

	textureCache().init();
	g_textDrawer.init();
	DepthBuffer_Init();
	FrameBuffer_Init();
	Combiner_Init();
	TFH.init();
	PostProcessor::get().init();
	g_zlutTexture.init();
	g_paletteTexture.init();
	perf.reset();
	FBInfo::fbInfo.reset();
	m_texrectDrawer.init();
	m_drawingState = DrawingState::Non;
	m_maxLineWidth = gfxContext.getMaxLineWidth();

	gSP.changed = gDP.changed = 0xFFFFFFFF;

	memset(triangles.vertices.data(), 0, triangles.vertices.size() * sizeof(SPVertex));
	triangles.elements.fill(0);
	for (auto vtx : triangles.vertices)
		vtx.w = 1.0f;
	triangles.num = 0;
	m_dmaVerticesNum = 0;
}

void GraphicsDrawer::_destroyData()
{
	m_drawingState = DrawingState::Non;
	m_texrectDrawer.destroy();
	g_paletteTexture.destroy();
	g_zlutTexture.destroy();
	PostProcessor::get().destroy();
	if (TFH.optionsChanged())
		TFH.shutdown();
	Combiner_Destroy();
	FrameBuffer_Destroy();
	DepthBuffer_Destroy();
	g_textDrawer.destroy();
	textureCache().destroy();
}
