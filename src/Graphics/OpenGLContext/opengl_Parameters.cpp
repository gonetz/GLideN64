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
		Parameter RGBA(GL_RGBA8);
		Parameter RG(GL_RG8);
		Parameter RED(GL_R8);
		Parameter DEPTH(GL_DEPTH_COMPONENT);
		Parameter RG32F(GL_RG32F);
	}

	namespace datatype {
		Parameter UNSIGNED_BYTE(GL_UNSIGNED_BYTE);
		Parameter UNSIGNED_SHORT(GL_UNSIGNED_SHORT);
		Parameter UNSIGNED_INT(GL_UNSIGNED_INT);
		Parameter FLOAT(GL_FLOAT);
	}

	namespace target {
		Parameter TEXTURE_2D(GL_TEXTURE_2D);
		Parameter TEXTURE_2D_MULTISAMPLE(GL_TEXTURE_2D_MULTISAMPLE);
		Parameter RENDERBUFFER(GL_RENDERBUFFER);
	}

	namespace bufferTarget {
		Parameter FRAMEBUFFER(GL_FRAMEBUFFER);
		Parameter DRAW_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
		Parameter READ_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
	}

	namespace bufferAttachment {
		Parameter COLOR_ATTACHMENT0(GL_COLOR_ATTACHMENT0);
		Parameter DEPTH_ATTACHMENT(GL_DEPTH_ATTACHMENT);
	}

	namespace enable {
		Parameter BLEND(GL_BLEND);
		Parameter CULL_FACE(GL_CULL_FACE);
		Parameter DEPTH_TEST(GL_DEPTH_TEST);
		Parameter DEPTH_CLAMP(GL_DEPTH_CLAMP);
		Parameter CLIP_DISTANCE0(GL_CLIP_DISTANCE0);
		Parameter DITHER(GL_DITHER);
		Parameter POLYGON_OFFSET_FILL(GL_POLYGON_OFFSET_FILL);
		Parameter SCISSOR_TEST(GL_SCISSOR_TEST);
	}

	namespace textureIndices {
		Parameter Tex[2] = {0U, 1U};
		Parameter NoiseTex(2U);
		Parameter DepthTex(3U);
		Parameter ZLUTTex(4U);
		Parameter PaletteTex(5U);
		Parameter MSTex[2] = { 6U, 7U };
	}

	namespace textureImageUnits {
		Parameter Zlut(0U);
		Parameter Tlut(1U);
		Parameter Depth(2U);
	}

	namespace textureImageAccessMode {
		Parameter READ_ONLY(GL_READ_ONLY);
		Parameter WRITE_ONLY(GL_WRITE_ONLY);
		Parameter READ_WRITE(GL_READ_WRITE);
	}

	namespace textureParameters {
		Parameter FILTER_NEAREST(GL_NEAREST);
		Parameter FILTER_LINEAR(GL_LINEAR);
		Parameter FILTER_NEAREST_MIPMAP_NEAREST(GL_NEAREST_MIPMAP_NEAREST);
		Parameter FILTER_LINEAR_MIPMAP_NEAREST(GL_LINEAR_MIPMAP_NEAREST);
		Parameter WRAP_CLAMP_TO_EDGE(GL_CLAMP_TO_EDGE);
		Parameter WRAP_REPEAT(GL_REPEAT);
		Parameter WRAP_MIRRORED_REPEAT(GL_MIRRORED_REPEAT);
	}

	namespace cullMode {
		Parameter FRONT(GL_FRONT);
		Parameter BACK(GL_BACK);
	}

	namespace compare {
		Parameter LEQUAL(GL_LEQUAL);
		Parameter LESS(GL_LESS);
		Parameter ALWAYS(GL_ALWAYS);
	}

	namespace blend {
		Parameter ZERO(GL_ZERO);
		Parameter ONE(GL_ONE);
		Parameter SRC_ALPHA(GL_SRC_ALPHA);
		Parameter DST_ALPHA(GL_DST_ALPHA);
		Parameter ONE_MINUS_SRC_ALPHA(GL_ONE_MINUS_SRC_ALPHA);
		Parameter CONSTANT_ALPHA(GL_CONSTANT_ALPHA);
		Parameter ONE_MINUS_CONSTANT_ALPHA(GL_ONE_MINUS_CONSTANT_ALPHA);
	}

	namespace drawmode {
		Parameter TRIANGLES(GL_TRIANGLES);
		Parameter TRIANGLE_STRIP(GL_TRIANGLE_STRIP);
		Parameter LINES(GL_LINES);
	}

	namespace blitMask {
		Parameter COLOR_BUFFER(GL_COLOR_BUFFER_BIT);
		Parameter DEPTH_BUFFER(GL_DEPTH_BUFFER_BIT);
		Parameter STENCIL_BUFFER(GL_STENCIL_BUFFER_BIT);
	}
}
