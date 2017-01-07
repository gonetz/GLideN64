#include <Types.h>
#include <Log.h>
#include "opengl_Utils.h"
#include "GLFunctions.h"

using namespace opengl;

bool Utils::isExtensionSupported(const char *extension)
{
#ifdef GL_NUM_EXTENSIONS
	GLint count = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);
	for (u32 i = 0; i < count; ++i) {
		const char* name = (const char*)glGetStringi(GL_EXTENSIONS, i);
		if (name == nullptr)
			continue;
		if (strcmp(extension, name) == 0)
			return true;
	}
	return false;
#else
	GLubyte *where = (GLubyte *)strchr(extension, ' ');
	if (where || *extension == '\0')
		return false;

	const GLubyte *extensions = glGetString(GL_EXTENSIONS);

	const GLubyte *start = extensions;
	for (;;) {
		where = (GLubyte *)strstr((const char *)start, extension);
		if (where == nullptr)
			break;

		GLubyte *terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
		if (*terminator == ' ' || *terminator == '\0')
			return true;

		start = terminator;
	}

	return false;
#endif // GL_NUM_EXTENSIONS
}


static
const char* GLErrorString(GLenum errorCode)
{
	static const struct {
		GLenum code;
		const char *string;
	} errors[] =
	{
		/* GL */
		{ GL_NO_ERROR, "no error" },
		{ GL_INVALID_ENUM, "invalid enumerant" },
		{ GL_INVALID_VALUE, "invalid value" },
		{ GL_INVALID_OPERATION, "invalid operation" },
#ifndef GLESX
		{ GL_STACK_OVERFLOW, "stack overflow" },
		{ GL_STACK_UNDERFLOW, "stack underflow" },
#endif
		{ GL_OUT_OF_MEMORY, "out of memory" },

		{ 0, nullptr }
	};

	int i;

	for (i = 0; errors[i].string; i++)
	{
		if (errors[i].code == errorCode)
		{
			return errors[i].string;
		}
	}

	return nullptr;
}

bool Utils::isGLError()
{
	GLenum errCode;
	const char* errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = GLErrorString(errCode);
		if (errString != nullptr) {
			LOG(LOG_ERROR, "OpenGL Error: %s (%x)", errString, errCode);
		}
		else {
			LOG(LOG_ERROR, "OpenGL Error: %x", errCode);
		}

		return true;
	}
	return false;
}
