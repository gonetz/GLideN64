#include "ColorBufferToRDRAM.h"

#include "inc/ARB_buffer_storage.h"


class ColorBufferToRDRAM_BufferStorageExt : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAM_BufferStorageExt();
	~ColorBufferToRDRAM_BufferStorageExt() = default;

private:
	bool _readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)  override;
	void _cleanUp()  override;
	void _initBuffers(void) override;
	virtual void _destroyBuffers(void) override;
	static const int _numPBO = 2;
	GLuint m_PBO[_numPBO];
	void* m_PBOData[_numPBO];
	u32 m_curIndex;
	GLsync m_fence[_numPBO];
#ifdef GLESX
	PFNGLBUFFERSTORAGEPROC glBufferStorage;
#endif
};
