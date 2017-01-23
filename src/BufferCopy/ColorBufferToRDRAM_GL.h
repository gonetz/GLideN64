#include "ColorBufferToRDRAM.h"

#include <Textures.h>

class ColorBufferToRDRAM_GL : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAM_GL();
	~ColorBufferToRDRAM_GL() {};

private:
	void _init() override;
	void _destroyBuffers() override;
	bool _readPixels(s32 _x0, s32 _y0, u32 _width, u32 _height, u32 _size, bool _sync)  override;
	void _cleanUp()  override;
	void _initBuffers(void) override;
	static const int _numPBO = 3;
	GLuint m_PBO[_numPBO];
	u32 m_curIndex;
};
