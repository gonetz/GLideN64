#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include "Types.h"
#include "OpenGL.h"
#include "Textures.h"

class PostProcessor {
public:
	void init();
	void destroy();

	void process(FrameBuffer * _pBuffer);

	static PostProcessor & get();

private:
	PostProcessor() :
		m_extractBloomProgram(0), m_seperableBlurProgram(0), m_glowProgram(0), m_bloomProgram(0), m_copyProgram(0),
		m_FBO_original(0), m_FBO_glowMap(0), m_FBO_blur(0),
		m_pTextureOriginal(NULL), m_pTextureGlowMap(NULL), m_pTextureBlur(NULL) {}
	PostProcessor(const PostProcessor & _other);

	GLuint m_extractBloomProgram;
	GLuint m_seperableBlurProgram;
	GLuint m_glowProgram;
	GLuint m_bloomProgram;
	GLuint m_copyProgram;

	GLuint m_FBO_original;
	GLuint m_FBO_glowMap;
	GLuint m_FBO_blur;

	CachedTexture * m_pTextureOriginal;
	CachedTexture * m_pTextureGlowMap;
	CachedTexture * m_pTextureBlur;
};

#endif // POST_PROCESSOR_H
