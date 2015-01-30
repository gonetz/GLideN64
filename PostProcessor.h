#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include "Types.h"
#include "OpenGL.h"
#include "Textures.h"

class PostProcessor {
public:
	void init();
	void destroy();

	void processTexture(CachedTexture * _pTexture);

	static PostProcessor & get();

private:
	PostProcessor() : m_bloomProgram(0), m_copyProgram(0), m_FBO(0), m_pTexture(NULL) {};
	PostProcessor(const PostProcessor & _other);

	GLuint m_bloomProgram;
	GLuint m_copyProgram;

	GLuint m_FBO;
	CachedTexture * m_pTexture;
};

#endif // POST_PROCESSOR_H
