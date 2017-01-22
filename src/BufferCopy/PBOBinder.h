#pragma once
#include <OpenGL.h>

struct PBOBinder {
#ifndef GLES2
	PBOBinder(GLenum _target, GLuint _PBO) : m_target(_target)
	{
		glBindBuffer(m_target, _PBO);
	}
	~PBOBinder() {
		glBindBuffer(m_target, 0);
	}
	GLenum m_target;
#else
	PBOBinder(GLubyte* _ptr) : ptr(_ptr) {}
	~PBOBinder() { free(ptr); }
	GLubyte* ptr;
#endif
};
