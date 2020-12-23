#include "opengl_Attributes.h"

namespace opengl {

	namespace triangleAttrib {
		const GLuint position = 0U;
		const GLuint color = 1U;
		const GLuint texcoord = 2U;
		const GLuint numlights = 3U;
		const GLuint modify = 4U;
		const GLuint barycoords = 5U;
	}

	// Rect attributes
	namespace rectAttrib {
		const GLuint position = 6U;
		const GLuint texcoord0 = 7U;
		const GLuint texcoord1 = 8U;
		const GLuint barycoords = 9U;
	}
}
