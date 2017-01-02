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
		extern Parameter RG32F;
	}

	namespace datatype {
		extern Parameter UNSIGNED_BYTE;
		extern Parameter UNSIGNED_SHORT;
		extern Parameter UNSIGNED_INT;
		extern Parameter FLOAT;
	}

	namespace target {
		extern Parameter TEXTURE_2D;
		extern Parameter TEXTURE_2D_MULTISAMPLE;
		extern Parameter RENDERBUFFER;
	}

	namespace bufferTarget {
		extern Parameter FRAMEBUFFER;
		extern Parameter DRAW_FRAMEBUFFER;
		extern Parameter READ_FRAMEBUFFER;
	}

	namespace bufferAttachment {
		extern Parameter COLOR_ATTACHMENT0;
		extern Parameter DEPTH_ATTACHMENT;
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

	namespace textureParameters {
		extern Parameter FILTER_NEAREST;
		extern Parameter FILTER_LINEAR;
		extern Parameter FILTER_NEAREST_MIPMAP_NEAREST;
		extern Parameter FILTER_LINEAR_MIPMAP_NEAREST;
		extern Parameter WRAP_CLAMP_TO_EDGE;
		extern Parameter WRAP_REPEAT;
		extern Parameter WRAP_MIRRORED_REPEAT;
	}
}
