#include <algorithm>
#include "ColorBufferToRDRAM.h"

#include <Textures.h>
#include <FBOTextureFormats.h>

#include <ui/GraphicBuffer.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) (GLenum target, EGLImageKHR image);
typedef void (GL_APIENTRY* PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) (GLenum target, EGLImageKHR image);

using namespace android;

class ColorBufferToRDRAM_GLES : public ColorBufferToRDRAM
{
public:
	ColorBufferToRDRAM_GLES();
	~ColorBufferToRDRAM_GLES() = default;

private:
	void _init() override;
	void _destroy() override;
	bool _readPixels(GLint _x0, GLint _y0, GLsizei _width, GLsizei _height, u32 _size, bool _sync)  override;
	void _cleanUp()  override;
	void _initBuffers(void) override;

	GraphicBuffer* m_window;
	EGLImageKHR m_image;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES;
};
