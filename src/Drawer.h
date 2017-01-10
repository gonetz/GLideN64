#pragma once
#include <array>
#include <vector>
#include "gSP.h"
#include "Graphics/Parameter.h"

struct CachedTexture;
struct FrameBuffer;

#define VERTBUFF_SIZE 256U
#define ELEMBUFF_SIZE 1024U

enum class DrawingState
{
	None = 0,
	Line = 1,
	Triangle = 2,
	Rect = 3,
	TexRect = 4,
};

class Drawer
{
public:
	void addTriangle(int _v0, int _v1, int _v2);

	void drawTriangles();

	void drawScreenSpaceTriangle(u32 _numVtx);

	void drawDMATriangles(u32 _numVtx);

	void drawLine(int _v0, int _v1, float _width);

	void drawRect(int _ulx, int _uly, int _lrx, int _lry, float * _pColor);

	struct TexturedRectParams
	{
		float ulx, uly, lrx, lry;
		float uls, ult, lrs, lrt;
		float dsdx, dtdy;
		bool flip, forceAjustScale, texrectCmd;
		const FrameBuffer * pBuffer;
		const CachedTexture * pInputTexture;
		const CachedTexture * pOutputTexture;
		TexturedRectParams(float _ulx, float _uly, float _lrx, float _lry,
			float _uls, float _ult, float _lrs, float _lrt,
			float _dsdx, float _dtdy,
			bool _flip, bool _forceAjustScale, bool _texrectCmd,
			const FrameBuffer * _pBuffer,
			const CachedTexture * _pInputTexture = 0,
			const CachedTexture * _pOutputTexture = 0
			) :
			ulx(_ulx), uly(_uly), lrx(_lrx), lry(_lry),
			uls(_uls), ult(_ult), lrs(_lrs), lrt(_lrt),
			dsdx(_dsdx), dtdy(_dtdy),
			flip(_flip), forceAjustScale(_forceAjustScale), texrectCmd(_texrectCmd),
			pBuffer(_pBuffer), pInputTexture(_pInputTexture), pOutputTexture(_pOutputTexture)
		{}
	private:
		friend class Drawer;
		TexturedRectParams() :
			ulx(0), uly(0), lrx(0), lry(0)
		{};
	};

	void correctTexturedRectParams(TexturedRectParams & _params);

	void drawTexturedRect(const TexturedRectParams & _params);

	void copyTexturedRect(u32 _srcX0, u32 _srcY0, u32 _srcX1, u32 _srcY1,
		u32 _srcWidth, u32 _srcHeight, u32 _srcTex,
		s32 _dstX0, s32 _dstY0, s32 _dstX1, s32 _dstY1,
		u32 _dstWidth, u32 _dstHeight, graphics::Parameter _filter);

	void drawText(const char *_pText, float x, float y);

	void drawOSD();

	void clearDepthBuffer(u32 _ulx, u32 _uly, u32 _lrx, u32 _lry);

	void clearColorBuffer(float * _pColor);

	int getTrianglesCount() const { return triangles.num; }

	bool isClipped(s32 _v0, s32 _v1, s32 _v2) const
	{
		return (triangles.vertices[_v0].clip & triangles.vertices[_v1].clip & triangles.vertices[_v2].clip) != 0;
	}

	bool isImageTexturesSupported() const { return m_bImageTexture; }

	SPVertex & getVertex(u32 _v) { return triangles.vertices[_v]; }

	void setDMAVerticesSize(u32 _size) { if (m_dmaVertices.size() < _size) m_dmaVertices.resize(_size); }

	SPVertex * getDMAVerticesData() { return m_dmaVertices.data(); }

	void updateScissor(FrameBuffer * _pBuffer) const;

	DrawingState getDrawingState() const { return m_drawingState; }

	void dropRenderState() { m_drawingState = DrawingState::None; }

private:
	Drawer()
		: m_modifyVertices(0)
		, m_bImageTexture(false)
		, m_bFlatColors(false) {
	}
	Drawer(const Drawer &);

	void _initExtensions();
	void _initStates();
	void _initData();
	void _destroyData();

	void _setSpecialTexrect() const;

	void _setBlendMode() const;
	void _legacySetBlendMode() const;
	void _updateCullFace() const;
	void _updateViewport() const;
	void _updateScreenCoordsViewport() const;
	void _updateDepthUpdate() const;
	void _updateDepthCompare() const;
	void _updateTextures() const;
	void _updateStates(DrawingState _drawingState) const;
	void _prepareDrawTriangle(bool _dma);
	bool _canDraw() const;
	void _drawThickLine(int _v0, int _v1, float _width);

	void _getTextSize(const char *_pText, float & _w, float & _h) const;
	void _drawOSD(const char *_pText, float _x, float & _y);

	struct RectVertex
	{
		float x, y, z, w;
		float s0, t0, s1, t1;
	};

	DrawingState m_drawingState;
	TexturedRectParams m_texrectParams;

	struct {
		std::array<SPVertex, VERTBUFF_SIZE> vertices;
		std::array<u8, ELEMBUFF_SIZE> elements;
		u32 num;
		int maxElement;
	} triangles;

	std::vector<SPVertex> m_dmaVertices;

	RectVertex m_rect[4];

	u32 m_modifyVertices;
	f32 m_maxLineWidth;
	bool m_bImageTexture;
	bool m_bFlatColors;
	bool m_bDmaVertices;

	//GLuint m_programCopyTex;
};
