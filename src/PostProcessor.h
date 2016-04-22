#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include "Types.h"
#include "OpenGL.h"
#include "Textures.h"

class PostProcessor {
public:
	void init();
	void destroy();

	FrameBuffer * doBlur(FrameBuffer * _pBuffer);
	FrameBuffer * doGammaCorrection(FrameBuffer * _pBuffer);

	static PostProcessor & get();

private:
	PostProcessor();
	PostProcessor(const PostProcessor & _other);

	void _initCommon();
	void _destroyCommon();
	void _initGammaCorrection();
	void _destroyGammaCorrection();
	void _initBlur();
	void _destroyBlur();
	void _setGLState();
	void _preDraw(FrameBuffer * _pBuffer);
	void _postDraw();

	GLuint m_extractBloomProgram;
	GLuint m_seperableBlurProgram;
	GLuint m_glowProgram;
	GLuint m_bloomProgram;

	GLuint m_gammaCorrectionProgram;

	FrameBuffer * m_pResultBuffer;

	GLuint m_FBO_glowMap;
	GLuint m_FBO_blur;

	CachedTexture * m_pTextureOriginal;
	CachedTexture * m_pTextureGlowMap;
	CachedTexture * m_pTextureBlur;

#ifdef ANDROID
	static PostProcessor processor;
#endif
};

#endif // POST_PROCESSOR_H
