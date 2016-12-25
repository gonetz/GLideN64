#include <Graphics/Parameters.h>
#include "GLFunctions.h"

namespace graphics {

	namespace color {
		Parameter RGBA(GL_RGBA);
		Parameter RG(GL_RG);
		Parameter RED(GL_RED);
		Parameter DEPTH(GL_DEPTH_COMPONENT);
	}

	namespace internalcolor {
		Parameter RGBA(GL_RGBA);
		Parameter RG(GL_RG);
		Parameter RED(GL_RED);
		Parameter DEPTH(GL_DEPTH_COMPONENT);
	}

	namespace type {
		Parameter UNSIGNED_BYTE(GL_UNSIGNED_BYTE);
		Parameter UNSIGNED_SHORT(GL_UNSIGNED_SHORT);
		Parameter UNSIGNED_INT(GL_UNSIGNED_INT);
		Parameter FLOAT(GL_FLOAT);
	}

}
