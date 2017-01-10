#include <algorithm>
#include <assert.h>
#include "Graphics/Context.h"
#include "Graphics/Parameters.h"
#include "Config.h"
#include "RSP.h"
#include "VI.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "DisplayWindow.h"
#include "SoftwareRender.h"
#include "Drawer.h"

using namespace graphics;

void Drawer::addTriangle(int _v0, int _v1, int _v2)
{
	const u32 firstIndex = triangles.num;
	triangles.elements[triangles.num++] = _v0;
	triangles.elements[triangles.num++] = _v1;
	triangles.elements[triangles.num++] = _v2;
	triangles.maxElement = std::max(triangles.maxElement, _v0);
	triangles.maxElement = std::max(triangles.maxElement, _v1);
	triangles.maxElement = std::max(triangles.maxElement, _v2);

	m_modifyVertices |= triangles.vertices[_v0].modify |
		triangles.vertices[_v1].modify |
		triangles.vertices[_v2].modify;

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
			SPVertex & vtx0 = triangles.vertices[triangles.elements[firstIndex + ((RSP.w1 >> 24) & 3)]];
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

	if (!gfxContext.isSupported(SpecialFeatures::NearPlaneClipping)) {
		if (GBI.isNoN() && gDP.otherMode.depthCompare == 0 && gDP.otherMode.depthUpdate == 0) {
			for (u32 i = firstIndex; i < triangles.num; ++i) {
				SPVertex & vtx = triangles.vertices[triangles.elements[i]];
				vtx.z = 0.0f;
			}
		}
	}
}

void Drawer::_updateCullFace() const
{
	if (gSP.geometryMode & G_CULL_BOTH) {
		gfxContext.enable(enable::CULL_FACE, true);

		if (gSP.geometryMode & G_CULL_BACK)
			gfxContext.cullFace(cullMode::BACK);
		else
			gfxContext.cullFace(cullMode::FRONT);
	} else
		gfxContext.enable(enable::CULL_FACE, false);
}

void Drawer::_updateDepthUpdate() const
{
	gfxContext.enableDepthWrite(gDP.otherMode.depthUpdate != 0);
}

void Drawer::_updateDepthCompare() const
{
	if (config.frameBufferEmulation.N64DepthCompare != 0) {
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
				gfxContext.enable(enable::DEPTH_CLAMP, false);
		} else {
			gfxContext.enable(enable::DEPTH_TEST, false);
			if (!GBI.isNoN())
				gfxContext.enable(enable::DEPTH_CLAMP, true);
		}
	}
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

void Drawer::updateScissor(FrameBuffer * _pBuffer) const
{
	DisplayWindow & wnd = DisplayWindow::get();
	f32 scaleX, scaleY;
	u32 heightOffset, screenHeight;
	if (_pBuffer == nullptr) {
		scaleX = wnd.getScaleX();
		scaleY = wnd.getScaleY();
		heightOffset = wnd.getHeightOffset();
		screenHeight = VI.height;
	} else {
		scaleX = _pBuffer->m_scaleX;
		scaleY = _pBuffer->m_scaleY;
		heightOffset = 0;
		screenHeight = (_pBuffer->m_height == 0) ? VI.height : _pBuffer->m_height;
	}

	f32 SX0 = gDP.scissor.ulx;
	f32 SX1 = gDP.scissor.lrx;
	if (_needAdjustCoordinate(wnd))
		_adjustScissorX(SX0, SX1, wnd.getAdjustScale());

	gfxContext.setScissor((s32)(SX0 * scaleX), (s32)((screenHeight - gDP.scissor.lry) * scaleY + heightOffset),
		std::max((s32)((SX1 - SX0) * scaleX), 0), std::max((s32)((gDP.scissor.lry - gDP.scissor.uly) * scaleY), 0));
	gDP.changed &= ~CHANGED_SCISSOR;
}

inline
float _adjustViewportX(f32 _X0)
{
	const f32 halfX = gDP.colorImage.width / 2.0f;
	const f32 halfVP = gSP.viewport.width / 2.0f;
	return (_X0 + halfVP - halfX) * video().getAdjustScale() + halfX - halfVP;
}

void Drawer::_updateViewport() const
{
	DisplayWindow & wnd = DisplayWindow::get();
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	if (pCurrentBuffer == nullptr) {
		const f32 scaleX = wnd.getScaleX();
		const f32 scaleY = wnd.getScaleY();
		float Xf = gSP.viewport.vscale[0] < 0 ? (gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) : gSP.viewport.x;
		if (_needAdjustCoordinate(wnd))
			Xf = _adjustViewportX(Xf);
		const s32 X = (s32)(Xf * scaleX);
		const s32 Y = gSP.viewport.vscale[1] < 0 ? (s32)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * scaleY) : (s32)((VI.height - (gSP.viewport.y + gSP.viewport.height)) * scaleY);
		gfxContext.setViewport(X, Y + wnd.getHeightOffset(),
			std::max((s32)(gSP.viewport.width * scaleX), 0), std::max((s32)(gSP.viewport.height * scaleY), 0));
	} else {
		const f32 scaleX = pCurrentBuffer->m_scaleX;
		const f32 scaleY = pCurrentBuffer->m_scaleY;
		float Xf = gSP.viewport.vscale[0] < 0 ? (gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) : gSP.viewport.x;
		if (_needAdjustCoordinate(wnd))
			Xf = _adjustViewportX(Xf);
		const s32 X = (s32)(Xf * scaleX);
		const s32 Y = gSP.viewport.vscale[1] < 0 ? (s32)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * scaleY) : (s32)((pCurrentBuffer->m_height - (gSP.viewport.y + gSP.viewport.height)) * scaleY);
		gfxContext.setViewport(X, Y,
			std::max((s32)(gSP.viewport.width * scaleX), 0), std::max((s32)(gSP.viewport.height * scaleY), 0));
	}
	gSP.changed &= ~CHANGED_VIEWPORT;
}

void Drawer::_updateScreenCoordsViewport() const
{
	DisplayWindow & wnd = DisplayWindow::get();
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	if (pCurrentBuffer == nullptr)
		gfxContext.setViewport(0, wnd.getHeightOffset(), wnd.getScreenWidth(), wnd.getScreenHeight());
	else
		gfxContext.setViewport(0, 0, s32(pCurrentBuffer->m_width*pCurrentBuffer->m_scaleX), s32(pCurrentBuffer->m_height*pCurrentBuffer->m_scaleY));
	gSP.changed |= CHANGED_VIEWPORT;
}

void Drawer::_legacySetBlendMode() const
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
		Parameter sfactor, dfactor;

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
			}
			else {
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
	} else if ((config.generalEmulation.hacks & hack_pilotWings) != 0 && (gDP.otherMode.l & 0x80) != 0) { //CLR_ON_CVG without FORCE_BL
		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(blend::ZERO, blend::ONE);
	} else if ((config.generalEmulation.hacks & hack_blastCorps) != 0 && gDP.otherMode.cycleType < G_CYC_COPY && gSP.texture.on == 0 && currentCombiner()->usesTexture()) { // Blast Corps
		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(blend::ZERO, blend::ONE);
	} else {
		gfxContext.enable(enable::BLEND, false);
	}
}

void Drawer::_setBlendMode() const
{
	if (config.generalEmulation.enableLegacyBlending != 0) {
		_legacySetBlendMode();
		return;
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		Parameter srcFactor = blend::ONE;
		Parameter dstFactor = blend::ZERO;
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
			} else if (gDP.otherMode.c2_m2a == 1) {
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
			}
			else if (gDP.otherMode.c1_m2a == 1) {
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
	}
	else if ((config.generalEmulation.hacks & hack_pilotWings) != 0 && gDP.otherMode.clearOnCvg != 0) { //CLR_ON_CVG without FORCE_BL
		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(blend::ZERO, blend::ONE);
	}
	else if ((config.generalEmulation.hacks & hack_blastCorps) != 0 && gDP.otherMode.cycleType < G_CYC_COPY && gSP.texture.on == 0 && currentCombiner()->usesTexture()) { // Blast Corps
		gfxContext.enable(enable::BLEND, true);
		gfxContext.setBlending(blend::ZERO, blend::ONE);
	} else if ((gDP.otherMode.forceBlender == 0 && gDP.otherMode.cycleType < G_CYC_COPY)) {
		if (gDP.otherMode.c1_m1a == 1 && gDP.otherMode.c1_m2a == 1) {
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::ZERO, blend::ONE);
		} else {
			gfxContext.enable(enable::BLEND, false);
		}
	}
	else {
		gfxContext.enable(enable::BLEND, false);
	}
}

void Drawer::_updateTextures() const
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

void Drawer::_updateStates(DrawingState _drawingState) const
{
//	DisplayWindow & ogl = DisplayWindow::get();

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
		_setBlendMode();
		gDP.changed &= ~(CHANGED_RENDERMODE | CHANGED_CYCLETYPE);
	}

	cmbInfo.updateParameters();

	if (!gfxContext.isSupported(SpecialFeatures::FragmentDepthWrite))
		return;

	if (gDP.colorImage.address == gDP.depthImageAddress &&
		config.generalEmulation.enableFragmentDepthWrite != 0 &&
		config.frameBufferEmulation.N64DepthCompare == 0 &&
		(config.generalEmulation.hacks & hack_ZeldaMM) == 0
		) {
		// Current render target is depth buffer.
		// Shader will set gl_FragDepth to shader color, see ShaderCombiner ctor
		// Here we enable depth buffer write.
		if (gDP.otherMode.depthCompare != 0) {
			// Render to depth buffer with depth compare. Need to get copy of current depth buffer.
			FrameBuffer * pCurBuf = frameBufferList().getCurrent();
			if (pCurBuf != nullptr && pCurBuf->m_pDepthBuffer != nullptr) {
				CachedTexture * pDepthTexture = pCurBuf->m_pDepthBuffer->copyDepthBufferTexture(pCurBuf);
				if (pDepthTexture == nullptr)
					return;
				Context::TexParameters params;
				params.handle = graphics::ObjectHandle(pDepthTexture->glName);
				params.target = graphics::target::TEXTURE_2D;
				params.textureUnitIndex = graphics::textureIndices::DepthTex;
				params.maxMipmapLevel = 0;
				params.minFilter = graphics::textureParameters::FILTER_NEAREST;
				params.magFilter = graphics::textureParameters::FILTER_NEAREST;
				gfxContext.setTextureParameters(params);
			}
		}
		else if (frameBufferList().getCurrent() == nullptr) {
			gfxContext.enable(enable::BLEND, true);
			gfxContext.setBlending(blend::ZERO, blend::ONE);
		}
		gfxContext.enable(enable::DEPTH_TEST, true);
		gfxContext.setDepthCompare(compare::ALWAYS);
		gfxContext.enableDepthWrite(true);
		gDP.changed |= CHANGED_RENDERMODE;
	}
}

void Drawer::_prepareDrawTriangle(bool _dma)
{
#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (m_bImageTexture && config.frameBufferEmulation.N64DepthCompare != 0)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif // GL_IMAGE_TEXTURES_SUPPORT

	if ((m_modifyVertices & MODIFY_XY) != 0)
		gSP.changed &= ~CHANGED_VIEWPORT;

	if (gSP.changed || gDP.changed)
		_updateStates(DrawingState::Triangle);

	m_drawingState = DrawingState::Triangle;
	m_bDmaVertices = _dma;

	bool bFlatColors = false;
	if (!RSP.bLLE && (gSP.geometryMode & G_LIGHTING) == 0) {
		bFlatColors = (gSP.geometryMode & G_SHADE) == 0;
		bFlatColors |= (gSP.geometryMode & G_SHADING_SMOOTH) == 0;
	}
	m_bFlatColors = bFlatColors;

	if ((m_modifyVertices & MODIFY_XY) != 0)
		_updateScreenCoordsViewport();
	m_modifyVertices = 0;
}

bool Drawer::_canDraw() const
{
	return config.frameBufferEmulation.enable == 0 || frameBufferList().getCurrent() != nullptr;
}

void Drawer::drawTriangles()
{
	if (triangles.num == 0 || !_canDraw()) {
		triangles.num = 0;
		triangles.maxElement = 0;
		return;
	}

	_prepareDrawTriangle(false);

	/*
	const u32 count = static_cast<u32>(triangles.maxElement) + 1;
	m_vbo.setTrianglesBuffers(m_bFlatColors, count, triangles.vertices);
	const VerticesBuffers::BufferType bufferType = VerticesBuffers::BufferType::triangles;
	glDrawElementsBaseVertex(GL_TRIANGLES, triangles.num, GL_UNSIGNED_BYTE, triangles.elements, m_vbo.getPos(bufferType) - count);
	*/

	if (config.frameBufferEmulation.enable != 0 &&
		config.frameBufferEmulation.copyDepthToRDRAM == Config::cdSoftwareRender &&
		gDP.otherMode.depthUpdate != 0) {
		renderTriangles(triangles.vertices.data(), triangles.elements.data(), triangles.num);
		FrameBuffer * pCurrentDepthBuffer = frameBufferList().findBuffer(gDP.depthImageAddress);
		if (pCurrentDepthBuffer != nullptr)
			pCurrentDepthBuffer->m_cleared = false;
	}
	triangles.num = 0;
	triangles.maxElement = 0;
}


