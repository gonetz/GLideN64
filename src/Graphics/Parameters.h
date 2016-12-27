#pragma once
#include "Parameter.h"

namespace graphics {

	namespace color {
		extern Parameter RGBA;
		extern Parameter RG;
		extern Parameter RED;
		extern Parameter DEPTH;
	}

	namespace internalcolor {
		extern Parameter RGBA;
		extern Parameter RG;
		extern Parameter RED;
		extern Parameter DEPTH;
	}

	namespace type {
		extern Parameter UNSIGNED_BYTE;
		extern Parameter UNSIGNED_SHORT;
		extern Parameter UNSIGNED_INT;
		extern Parameter FLOAT;
	}

	namespace target {
		extern Parameter TEXTURE_2D;
		extern Parameter TEXTURE_2D_MULTISAMPLE;
	}

	namespace enable {
		extern Parameter BLEND;
		extern Parameter CULL_FACE;
		extern Parameter DEPTH_TEST;
		extern Parameter DEPTH_CLAMP;
		extern Parameter CLIP_DISTANCE0;
		extern Parameter DITHER;
		extern Parameter POLYGON_OFFSET_FILL;
		extern Parameter SCISSOR_TEST;
	}
}
