#pragma once
#include <memory>
#include <array>
#include <vector>
#include <list>
#include <chrono>
#include <string>
#include "gSP.h"
#include "TexrectDrawer.h"
#include "Graphics/ObjectHandle.h"
#include "Graphics/Parameters.h"

namespace graphics {
	class CombinerProgram;
}

struct CachedTexture;
struct FrameBuffer;

constexpr u32 VERTBUFF_SIZE = 256U;
constexpr u32 VERTBUFF_MASK = VERTBUFF_SIZE - 1;
constexpr u32 ELEMBUFF_SIZE = 1024U;

constexpr f32 SCREEN_SIZE_DIM = 640.0f;
constexpr u32 MIPMAP_TILE_WIDTH = 256u;

enum class DrawingState
{
	Non,
	Line,
	Triangle,
	ScreenSpaceTriangle,
	Rect,
	TexRect
};

struct RectVertex
{
	float x, y, z, w;
	float s0, t0, s1, t1;
	float bc0, bc1;
};

typedef std::chrono::milliseconds Milliseconds;

class GraphicsDrawer
{
public:
	void addTriangle(u32 _v0, u32 _v1, u32 _v2);

	void drawTriangles();

	void drawScreenSpaceTriangle(u32 _numVtx, graphics::DrawModeParam _mode = graphics::drawmode::TRIANGLE_STRIP);

	void drawDMATriangles(u32 _numVtx);

	void drawLine(u32 _v0, u32 _v1, float _width, u32 _flag);

	void drawRect(int _ulx, int _uly, int _lrx, int _lry);

	struct TexturedRectParams
	{
		float ulx, uly, lrx, lry;
		float dsdx, dtdy;
		s16 s, t;
		bool flip, forceAjustScale, texrectCmd;
		const FrameBuffer * pBuffer;
		TexturedRectParams(float _ulx, float _uly, float _lrx, float _lry,
			float _dsdx, float _dtdy,
			s16 _s, s16 _t,
			bool _flip, bool _forceAjustScale, bool _texrectCmd,
			const FrameBuffer * _pBuffer
			) :
			ulx(_ulx), uly(_uly), lrx(_lrx), lry(_lry),
			dsdx(_dsdx), dtdy(_dtdy),
			s(_s), t(_t),
			flip(_flip), forceAjustScale(_forceAjustScale), texrectCmd(_texrectCmd),
			pBuffer(_pBuffer)
		{}
	private:
		friend class GraphicsDrawer;
		TexturedRectParams() :
			ulx(0), uly(0), lrx(0), lry(0)
		{};
	};

	void correctTexturedRectParams(TexturedRectParams & _params);

	void drawTexturedRect(const TexturedRectParams & _params);

	struct CopyRectParams
	{
		s32 srcX0 = 0;
		s32 srcY0 = 0;
		s32 srcX1;
		s32 srcY1;
		u32 srcWidth;
		u32 srcHeight;
		s32 dstX0 = 0;
		s32 dstY0 = 0;
		s32 dstX1;
		s32 dstY1;
		u32 dstWidth;
		u32 dstHeight;
		bool invertX = false;
		bool invertY = false;
		typedef std::array<CachedTexture *, 2> Textures;
		Textures tex = Textures{ { nullptr, nullptr } };
		graphics::CombinerProgram * combiner = nullptr;
		graphics::TextureParam filter;
	};

	void copyTexturedRect(const CopyRectParams & _params);

	struct BlitOrCopyRectParams : public CopyRectParams
	{
		graphics::ObjectHandle readBuffer;
		graphics::ObjectHandle drawBuffer;
		graphics::BlitMaskParam mask;
	};

	void blitOrCopyTexturedRect(const BlitOrCopyRectParams & _params);

	void drawText(const char *_pText, float x, float y);

	void drawOSD();

	void showMessage(std::string _message, Milliseconds _interval);

	void clearDepthBuffer();

	void clearColorBuffer(float * _pColor);

	int getTrianglesCount() const { return triangles.num; }

	bool isClipped(u32 _v0, u32 _v1, u32 _v2) const;

	bool isRejected(u32 _v0, u32 _v1, u32 _v2) const;

	SPVertex & getVertex(u32 _v) { return triangles.vertices[_v&VERTBUFF_MASK]; }

	SPVertex * getVertexPtr(u32 _v) { return triangles.vertices.data() + _v; }

	void setDMAVerticesSize(u32 _size) { if (m_dmaVertices.size() < _size) m_dmaVertices.resize(_size); }

	SPVertex * getDMAVerticesData() { return m_dmaVertices.data(); }

	SPVertex & getCurrentDMAVertex();
	u32 getDMAVerticesCount() const { return m_dmaVerticesNum; }

	void updateScissor(FrameBuffer * _pBuffer) const;

	DrawingState getDrawingState() const { return m_drawingState; }

	void dropRenderState() { m_drawingState = DrawingState::Non; }

	void flush() { m_texrectDrawer.draw(); }

	bool isTexrectDrawerMode() const { return !m_texrectDrawer.isEmpty(); }

	void setBackgroundDrawingMode(bool _mode) { m_bBGMode = _mode; }

	void setBlendMode(bool _forceLegacyBlending = false) const;

	void clearStatistics() { m_statistics.clear(); }

	enum class BgDepthCopyMode {
		eNone = 0,
		eCopyDone,
		eBg1cyc,
		eBgCopy
	};

	void setBgDepthCopyMode(BgDepthCopyMode mode);
	BgDepthCopyMode getBgDepthCopyMode() const;

	struct Statistics {
		u32 fillRects = 0;
		u32 texRects = 0;
		u32 clippedTris = 0;
		u32 rejectedTris = 0;
		u32 culledTris = 0;
		u32 drawnTris = 0;
		u32 lines = 0;
		void clear();
	};

private:
	friend class DisplayWindow;
	friend TexrectDrawer;

	GraphicsDrawer();
	~GraphicsDrawer();

	GraphicsDrawer(const GraphicsDrawer &) = delete;

	void _initStates();
	void _initData();
	void _destroyData();

	void _setSpecialTexrect() const;

	void _legacyBlending() const;
	void _ordinaryBlending() const;
	void _dualSourceBlending() const;
	void _updateCullFace() const;
	void _updateViewport(const FrameBuffer * _pBuffer = nullptr, const f32 scale = 0.0f) const;
	void _updateDepthUpdate() const;
	void _updateDepthCompare() const;
	void _updateTextures() const;
	void _updateStates(DrawingState _drawingState) const;
	void _prepareDrawTriangle(DrawingState _drawingState);
	bool _canDraw() const;
	void _drawThickLine(u32 _v0, u32 _v1, float _width, u32 _flag);

	void _drawOSD(const char *_pText, float _x, float & _y);

	typedef std::list<std::string> OSDMessages;
	void _removeOSDMessage(OSDMessages::iterator _iter, Milliseconds _interval);

	DrawingState m_drawingState{ DrawingState::Non };
	TexturedRectParams m_texrectParams;

	struct {
		std::array<SPVertex, VERTBUFF_SIZE> vertices;
		std::array<u16, ELEMBUFF_SIZE> elements;
		u32 num = 0;
		u32 maxElement = 0;
	} triangles;

	std::vector<SPVertex> m_dmaVertices;
	u32 m_dmaVerticesNum{ 0u };

	RectVertex m_rect[4];

	u32 m_modifyVertices{ 0u };
	f32 m_maxLineWidth{ 1.0f };
	bool m_bFlatColors{ false };
	bool m_bBGMode{ false };
	BgDepthCopyMode m_depthCopyMode{ BgDepthCopyMode::eNone };
	TexrectDrawer m_texrectDrawer;
	OSDMessages m_osdMessages;
	mutable Statistics m_statistics;
};
