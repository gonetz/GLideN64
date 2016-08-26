#ifndef OPENGL_H
#define OPENGL_H

#include <vector>
#include <stddef.h>

#ifdef OS_WINDOWS
#define NOMINMAX
#include <windows.h>
#else
#include "winlnxdefs.h"
#endif

#ifdef __LIBRETRO__
#include <glsm/glsmsym.h>
#include <GLideN64_libretro.h>
#elif GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define GL_DRAW_FRAMEBUFFER GL_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER GL_FRAMEBUFFER
#define NO_BLIT_BUFFER_COPY
#define GLESX
#ifdef PANDORA
typedef char GLchar;
#endif
#elif defined(GLES3)
#include <GLES3/gl3.h>
#define GLESX
#elif defined(GLES3_1)
#include <GLES3/gl31.h>
#define GLESX
#define GL_IMAGE_TEXTURES_SUPPORT
#define GL_MULTISAMPLING_SUPPORT
#elif defined(EGL)
#include <GL/glcorearb.h>
#include "common/GLFunctions.h"
#include <GL/glext.h>
#define GL_IMAGE_TEXTURES_SUPPORT
#define GL_MULTISAMPLING_SUPPORT
#else
#if defined(OS_MAC_OS_X)
#define GL_GLEXT_PROTOTYPES
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif defined(OS_LINUX)
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#define GL_IMAGE_TEXTURES_SUPPORT
#define GL_MULTISAMPLING_SUPPORT
#elif defined(OS_WINDOWS)
#include <GL/gl.h>
#include "glext.h"
#include "common/GLFunctions.h"
#define GL_IMAGE_TEXTURES_SUPPORT
#define GL_MULTISAMPLING_SUPPORT
#endif // OS_MAC_OS_X
#endif // GLES2

#ifdef GLESX
#define GET_PROGRAM_BINARY_EXTENSION "GL_OES_get_program_binary"
#else
#define GET_PROGRAM_BINARY_EXTENSION "GL_ARB_get_program_binary"
#endif

#ifdef USE_SDL
#include <SDL.h>
#endif // USE_SDL

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef __LIBRETRO__
#include "glState.h"
#endif
#include "gSP.h"

#define INDEXMAP_SIZE 80U
#define VERTBUFF_SIZE 256U
#define ELEMBUFF_SIZE 1024U

extern const char * strTexrectDrawerVertexShader;
extern const char * strTexrectDrawerTex3PointFilter;
extern const char * strTexrectDrawerTexBilinearFilter;
extern const char * strTexrectDrawerFragmentShaderTex;
extern const char * strTexrectDrawerFragmentShaderClean;
extern const char * strTextureCopyShader;

struct CachedTexture;
class OGLRender
{
public:
	void updateVBO(GLuint* type, GLsizeiptr length, void *pointer);
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
		friend class OGLRender;
		TexturedRectParams() :
			ulx(0), uly(0), lrx(0), lry(0)
		{};
	};
	void correctTexturedRectParams(TexturedRectParams & _params);
	void drawTexturedRect(const TexturedRectParams & _params);
	void copyTexturedRect(GLint _srcX0, GLint _srcY0, GLint _srcX1, GLint _srcY1,
						  GLuint _srcWidth, GLuint _srcHeight, GLuint _srcTex,
						  GLint _dstX0, GLint _dstY0, GLint _dstX1, GLint _dstY1,
						  GLuint _dstWidth, GLuint _dstHeight, GLenum _filter);
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
	void flush() { m_texrectDrawer.draw(); }

	enum RENDER_STATE {
		rsNone = 0,
		rsLine = 1,
		rsTriangle = 2,
		rsRect = 3,
		rsTexRect = 4,
	};
	RENDER_STATE getRenderState() const {return m_renderState;}

	enum OGL_RENDERER {
		glrOther,
		glrAdreno
	};
	OGL_RENDERER getRenderer() const { return m_oglRenderer; }

	void dropRenderState() {m_renderState = rsNone;}

private:
	OGLRender()
		: m_oglRenderer(glrOther)
		, m_modifyVertices(0)
		, m_bImageTexture(false)
		, m_bFlatColors(false) {
	}
	OGLRender(const OGLRender &);
	friend class OGLVideo;

	void _initExtensions();
	void _initVBO();
	void _initStates();
	void _initData();
	void _destroyVBO();
	void _destroyData();

	void _setSpecialTexrect() const;

	void _setColorArray() const;
	void _setTexCoordArrays() const;
	void _setBlendMode() const;
	void _legacySetBlendMode() const;
	void _updateCullFace() const;
	void _updateViewport() const;
	void _updateScreenCoordsViewport() const;
	void _updateDepthUpdate() const;
	void _updateDepthCompare() const;
	void _updateTextures(RENDER_STATE _renderState) const;
	void _updateStates(RENDER_STATE _renderState) const;
	void _prepareDrawTriangle(int _type, u32 numUpdate);
	bool _canDraw() const;

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

	class TexrectDrawer
	{
	public:
		TexrectDrawer();
		void init();
		void destroy();
		void add();
		bool draw();
		bool isEmpty();
	private:
		u32 m_numRects;
		u64 m_otherMode;
		u64 m_mux;
		f32 m_ulx, m_lrx, m_uly, m_lry, m_Z;
		f32 m_max_lrx, m_max_lry;
		GLuint m_FBO;
		GLuint m_programTex;
		GLuint m_programClean;
		GLint m_enableAlphaTestLoc;
		GLint m_textureBoundsLoc;
		gDPScissor m_scissor;
		CachedTexture * m_pTexture;
		FrameBuffer * m_pBuffer;

		struct RectCoords {
			f32 x, y;
		};
		std::vector<RectCoords> m_vecRectCoords;
	};

	RENDER_STATE m_renderState;
	OGL_RENDERER m_oglRenderer;
	TexturedRectParams m_texrectParams;
	GLVertex m_rect[4];
	u32 m_modifyVertices;
	bool m_bImageTexture;
	bool m_bFlatColors;
	GLuint tri_vbo, rect_vbo, vao;
	u32 tri_offset, rect_offset, tri_size, rect_size, vbo_max_size;

	TexrectDrawer m_texrectDrawer;

	GLuint m_programCopyTex;
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
	u32 getBuffersSwapCount() const {return m_buffersSwapCount;}
	u32 getWidth() const { return m_width; }
	u32 getHeight() const {return m_height;}
	u32 getScreenWidth() const {return m_screenWidth;}
	u32 getScreenHeight() const {return m_screenHeight;}
	u32 getHeightOffset() const {return m_heightOffset;}
	bool isFullscreen() const {return m_bFullscreen;}
	bool isAdjustScreen() const {return m_bAdjustScreen;}
	bool isResizeWindow() const {return m_bResizeWindow;}

	OGLRender & getRender() {return m_render;}

	static OGLVideo & get();
	static bool isExtensionSupported(const char * extension);

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

	u32 m_buffersSwapCount;
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

void initGLFunctions();
bool checkFBO();
bool isGLError();

void displayLoadProgress(const wchar_t *format, ...);

#endif
