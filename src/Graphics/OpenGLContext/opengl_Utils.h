#pragma once

namespace opengl {

	struct Utils
	{
		static bool isExtensionSupported(const char * extension);
		static bool isGLError();
	};

}
