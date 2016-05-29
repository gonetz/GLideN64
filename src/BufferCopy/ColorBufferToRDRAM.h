#ifndef ColorBufferToRDRAM_H
#define ColorBufferToRDRAM_H

#include <OpenGL.h>

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

protected:
	ColorBufferToRDRAM();
	ColorBufferToRDRAM(const ColorBufferToRDRAM &);
	virtual ~ColorBufferToRDRAM();

	CachedTexture * m_pTexture;

private:
	virtual void _init() = 0;
	virtual void _destroy() = 0;
	virtual GLubyte* _getPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync) = 0;
	virtual void _cleanUpPixels(GLubyte* pixelData) = 0;

	union RGBA {
		struct {
			u8 r, g, b, a;
		};
		u32 raw;
	};

	bool _prepareCopy(u32 _startAddress);

	void _copy(u32 _startAddress, u32 _endAddress, bool _sync);

	// Convert pixel from video memory to N64 buffer format.
	static u8 _RGBAtoR8(u8 _c);
	static u16 _RGBAtoRGBA16(u32 _c);
	static u32 _RGBAtoRGBA32(u32 _c);

	GLuint m_FBO;
	FrameBuffer * m_pCurFrameBuffer;
	u32 m_frameCount;
	u32 m_startAddress;
};

void copyWhiteToRDRAM(FrameBuffer * _pBuffer);

#endif // ColorBufferToRDRAM
