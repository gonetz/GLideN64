#include "OpenGLContextImpl.h"
#include "GLFunctions.h"

using namespace opengl;

OpenGLContextImpl::OpenGLContextImpl()
{
}


OpenGLContextImpl::~OpenGLContextImpl()
{
}

void OpenGLContextImpl::init()
{
	initGLFunctions();
}

void OpenGLContextImpl::destroy()
{

}

graphics::ObjectName OpenGLContextImpl::createTexture() const
{
	GLuint glName;
	glGenTextures(1, &glName);
	return graphics::ObjectName(static_cast<u32>(glName));
}

void OpenGLContextImpl::deleteTexture(graphics::ObjectName _name) const
{
	u32 glName(_name);
	glDeleteTextures(1, &glName);
}
