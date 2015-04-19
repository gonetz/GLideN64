#include "OpenGL.h"

#ifdef GLSTATE_H

void GLState::reset()
{
	cached_ActiveTexture_texture = 0;
	cached_BlendFunc_sfactor = 0;
	cached_BlendFunc_dfactor = 0;
	cached_ReadBufferMode = 0;
	cached_glPixelStorei_target = 0;
	cached_glPixelStorei_param = 0;
	cached_ClearColor_red = 0;
	cached_ClearColor_green = 0;
	cached_ClearColor_blue = 0;
	cached_ClearColor_alpha = 0;
	cached_CullFace_mode = 0;
	cached_DepthFunc_func = 0;
	cached_DepthMask_flag = 0;
	cached_BLEND = false;
	cached_CULL_FACE = false;
	cached_DEPTH_TEST = false;
	cached_DEPTH_CLAMP = false;
	cached_CLIP_DISTANCE0 = false;
	cached_DITHER = false;
	cached_POLYGON_OFFSET_FILL = false;
	cached_SAMPLE_ALPHA_TO_COVERAGE = false;
	cached_SAMPLE_COVERAGE = false;
	cached_SCISSOR_TEST = false;
	cached_PolygonOffset_factor = 0;
	cached_PolygonOffset_units = 0;
	cached_Scissor_x = 0;
	cached_Scissor_y = 0;
	cached_Scissor_width = 0;
	cached_Scissor_height = 0;
	cached_UseProgram_program = -1;
	cached_Viewport_x = 0;
	cached_Viewport_y = 0;
	cached_Viewport_width = 0;
	cached_Viewport_height = 0;
}

GLState glState;

#endif // GLSTATE_H
