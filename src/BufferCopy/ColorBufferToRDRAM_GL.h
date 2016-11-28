#include "ColorBufferToRDRAM.h"

#include <Textures.h>
#include <FBOTextureFormats.h>

class ColorBufferToRDRAM_GL : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAM_GL();
	~ColorBufferToRDRAM_GL() {};

private:
	void _init() override;
	void _destroyBuffers() override;
	bool _readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)  override;
	void _cleanUp()  override;
	void _initBuffers(void) override;
	static const int _numPBO = 3;
	GLuint m_PBO[_numPBO];
	u32 m_curIndex;
};
