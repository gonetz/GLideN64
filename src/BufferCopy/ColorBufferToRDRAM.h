#ifndef ColorBufferToRDRAM_H
#define ColorBufferToRDRAM_H

#include <OpenGL.h>

#ifdef ANDROID
#include "ui/GraphicBuffer.h"
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, EGLImageKHR image);
typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) (GLenum target, EGLImageKHR image);

using namespace android;
#endif

struct CachedTexture;
struct FrameBuffer;

class ColorBufferToRDRAM
{
public:
	void init();
	void destroy();

	void copyToRDRAM(u32 _address, bool _sync);
	void copyChunkToRDRAM(u32 _address);

	static ColorBufferToRDRAM & get();

private:
	ColorBufferToRDRAM();
	ColorBufferToRDRAM(const ColorBufferToRDRAM &);
	~ColorBufferToRDRAM();

	union RGBA {
		struct {
			u8 r, g, b, a;
		};
		u32 raw;
	};

	bool _prepareCopy(u32 _startAddress);

	GLubyte* getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, bool _sync);
	void cleanUpPixels(GLubyte* pixelData);
	void _copy(u32 _startAddress, u32 _endAddress, bool _sync);

	// Convert pixel from video memory to N64 buffer format.
	static u8 _RGBAtoR8(u8 _c);
	static u16 _RGBAtoRGBA16(u32 _c);
	static u32 _RGBAtoRGBA32(u32 _c);

	GLuint m_FBO;
	CachedTexture * m_pTexture;
	FrameBuffer * m_pCurFrameBuffer;
	u32 m_curIndex;
	u32 m_frameCount;
	u32 m_startAddress;

	GLuint m_PBO[3];

#ifdef ANDROID
	GraphicBuffer* m_window;
	EGLImageKHR m_image;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES;
#endif
};

void copyWhiteToRDRAM(FrameBuffer * _pBuffer);

#endif // ColorBufferToRDRAM
