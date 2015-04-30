#ifndef OPENGL_H
#define OPENGL_H

#include <vector>
#ifdef OS_WINDOWS
#include <windows.h>
#include <GL/gl.h>
#include "glext.h"
#include "windows/GLFunctions.h"
#define GL_IMAGE_TEXTURES_SUPPORT
#else
#include "winlnxdefs.h"
#ifdef GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define GL_DRAW_FRAMEBUFFER GL_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER GL_FRAMEBUFFER
#else
#define GL_GLEXT_PROTOTYPES
#if defined(OS_MAC_OS_X)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif defined(OS_LINUX)
#include <GL/gl.h>
#include <GL/glext.h>
#define GL_IMAGE_TEXTURES_SUPPORT
#endif // OS_MAC_OS_X
#endif // GLES2
#ifdef USE_SDL
#include <SDL.h>
#endif // USE_SDL
#endif // OS_WINDOWS

#include "glState.h"
#include "gSP.h"

#define INDEXMAP_SIZE 80U
#define VERTBUFF_SIZE 256U
#define ELEMBUFF_SIZE 1024U

class OGLRender
{
public:
	void addTriangle(int _v0, int _v1, int _v2);
	void drawTriangles();
	void drawLLETriangle(u32 _numVtx);
	void drawDMATriangles(u32 _numVtx);
	void drawLine(int _v0, int _v1, float _width);
	void drawRect(int _ulx, int _uly, int _lrx, int _lry, float * _pColor);
	struct TexturedRectParams
	{
		float ulx, uly, lrx, lry;
		float uls, ult, lrs, lrt;
		bool flip;
		TexturedRectParams(float _ulx, float _uly, float _lrx, float _lry, float _uls, float _ult, float _lrs, float _lrt, bool _flip) :
			ulx(_ulx), uly(_uly), lrx(_lrx), lry(_lry), uls(_uls), ult(_ult), lrs(_lrs), lrt(_lrt), flip(_flip)
		{}
	};
	void drawTexturedRect(const TexturedRectParams & _params);
	void drawText(const char *_pText, float x, float y);
	void clearDepthBuffer(u32 _uly, u32 _lry);
	void clearColorBuffer( float * _pColor );

	int getTrianglesCount() const {return triangles.num;}
	bool isClipped(s32 _v0, s32 _v1, s32 _v2) const
	{
		return (triangles.vertices[_v0].clip & triangles.vertices[_v1].clip & triangles.vertices[_v2].clip) != 0;
	}
	bool isImageTexturesSupported() const {return m_bImageTexture;}
	SPVertex & getVertex(u32 _v) {return triangles.vertices[_v];}
	void setDMAVerticesSize(u32 _size) { if (triangles.dmaVertices.size() < _size) triangles.dmaVertices.resize(_size); }
	SPVertex * getDMAVerticesData() { return triangles.dmaVertices.data(); }
	void updateScissor(FrameBuffer * _pBuffer) const;

	enum RENDER_STATE {
		rsNone = 0,
		rsTriangle = 1,
		rsRect = 2,
		rsTexRect = 3,
		rsLine = 4
	};
	RENDER_STATE getRenderState() const {return m_renderState;}

	void dropRenderState() {m_renderState = rsNone;}

private:
	OGLRender() : m_bImageTexture(false), m_bFlatColors(false) {}
	OGLRender(const OGLRender &);
	friend class OGLVideo;

	void _initExtensions();
	void _initStates();
	void _initData();
	void _destroyData();

	void _setSpecialTexrect() const;

	void _setColorArray() const;
	void _setTexCoordArrays() const;
	void _setBlendMode() const;
	void _updateCullFace() const;
	void _updateViewport() const;
	void _updateDepthUpdate() const;
	void _updateStates(RENDER_STATE _renderState) const;
	void _prepareDrawTriangle(bool _dma);

	struct {
		SPVertex vertices[VERTBUFF_SIZE];
		std::vector<SPVertex> dmaVertices;
		GLubyte elements[ELEMBUFF_SIZE];
		int num;
		u32 indexmap[INDEXMAP_SIZE];
		u32 indexmapinv[VERTBUFF_SIZE];
		u32 indexmap_prev;
		u32 indexmap_nomap;
	} triangles;

	struct GLVertex
	{
		float x, y, z, w;
		float s0, t0, s1, t1;
	};

	RENDER_STATE m_renderState;
	GLVertex m_rect[4];
	bool m_bImageTexture;
	bool m_bFlatColors;
};

class OGLVideo
{
public:
	void start();
	void stop();
	void restart();
	void swapBuffers();
	void saveScreenshot();
	bool changeWindow();
	bool resizeWindow();
	void setWindowSize(u32 _width, u32 _height);
	void setCaptureScreen(const char * const _strDirectory);
	void setToggleFullscreen() {m_bToggleFullscreen = true;}
	void readScreen(void **_pDest, long *_pWidth, long *_pHeight );
	void readScreen2(void * _dest, int * _width, int * _height, int _front);

	void updateScale();
	f32 getScaleX() const {return m_scaleX;}
	f32 getScaleY() const {return m_scaleY;}
	f32 getAdjustScale() const {return m_adjustScale;}
	u32 getWidth() const { return m_width; }
	u32 getHeight() const {return m_height;}
	u32 getScreenWidth() const {return m_screenWidth;}
	u32 getScreenHeight() const {return m_screenHeight;}
	u32 getHeightOffset() const {return m_heightOffset;}
	bool isFullscreen() const {return m_bFullscreen;}
	bool isAdjustScreen() const {return m_bAdjustScreen;}

	OGLRender & getRender() {return m_render;}

	static OGLVideo & get();

protected:
	OGLVideo() :
		m_bCaptureScreen(false), m_bToggleFullscreen(false), m_bResizeWindow(false), m_bFullscreen(false), m_bAdjustScreen(false),
		m_width(0), m_height(0), m_heightOffset(0),
		m_screenWidth(0), m_screenHeight(0), m_resizeWidth(0), m_resizeHeight(0),
		m_scaleX(0), m_scaleY(0), m_adjustScale(0)
	{}

	void _setBufferSize();

	bool m_bCaptureScreen;
	bool m_bToggleFullscreen;
	bool m_bResizeWindow;
	bool m_bFullscreen;
	bool m_bAdjustScreen;

	u32 m_width, m_height, m_heightOffset;
	u32 m_screenWidth, m_screenHeight;
	u32 m_resizeWidth, m_resizeHeight;
	f32 m_scaleX, m_scaleY;
	f32 m_adjustScale;

	wchar_t m_strScreenDirectory[PLUGIN_PATH_SIZE];

private:
	OGLRender m_render;

	virtual bool _start() = 0;
	virtual void _stop() = 0;
	virtual void _swapBuffers() = 0;
	virtual void _saveScreenshot() = 0;
	virtual void _changeWindow() = 0;
	virtual bool _resizeWindow() = 0;
};

inline
OGLVideo & video()
{
	return OGLVideo::get();
}

class TextureFilterHandler
{
public:
	TextureFilterHandler() : m_inited(0) {}
	// It's not safe to call shutdown() in destructor, because texture filter has its own static objects, which can be destroyed first.
	~TextureFilterHandler() { m_inited = 0; }
	void init();
	void shutdown();
	bool isInited() const { return m_inited != 0; }
private:
	u32 m_inited;
};

extern TextureFilterHandler TFH;

void initGLFunctions();
bool checkFBO();
bool isGLError();

void displayLoadProgress(const wchar_t *format, ...);

#endif
