#include <Types.h>
#include "opengl_Utilis.h"
#include "GLFunctions.h"

bool opengl::isExtensionSupported(const char *extension)
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
