#ifndef GLSTATE_H
#define GLSTATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct GLState {
	GLState() { reset(); }
	void reset();

	GLenum cached_ActiveTexture_texture;

	GLenum cached_BlendFunc_sfactor;
	GLenum cached_BlendFunc_dfactor;

	GLenum cached_ReadBufferMode;

	GLenum cached_glPixelStorei_target;
	GLint cached_glPixelStorei_param;

	GLclampf cached_ClearColor_red;
	GLclampf cached_ClearColor_green;
	GLclampf cached_ClearColor_blue;
	GLclampf cached_ClearColor_alpha;

	GLenum cached_DepthFunc_func;
	GLboolean cached_DepthMask_flag;

	bool cached_BLEND;
	bool cached_CULL_FACE;
	bool cached_DEPTH_TEST;
	bool cached_DEPTH_CLAMP;
	bool cached_CLIP_DISTANCE0;
	bool cached_DITHER;
	bool cached_POLYGON_OFFSET_FILL;
	bool cached_SAMPLE_ALPHA_TO_COVERAGE;
	bool cached_SAMPLE_COVERAGE;
	bool cached_SCISSOR_TEST;

	GLenum cached_FrontFace_mode;
	GLenum cached_CullFace_mode;

	GLfloat cached_PolygonOffset_factor;
	GLfloat cached_PolygonOffset_units;

	GLint cached_Scissor_x;
	GLint cached_Scissor_y;
	GLsizei cached_Scissor_width;
	GLsizei cached_Scissor_height;

	GLuint cached_UseProgram_program;
	GLint cached_Viewport_x;
	GLint cached_Viewport_y;
	GLsizei cached_Viewport_width;
	GLsizei cached_Viewport_height;
};

extern GLState glState;

void inline cache_glActiveTexture (GLenum texture)
{
	if (texture != glState.cached_ActiveTexture_texture) {
		glActiveTexture(texture);
		glState.cached_ActiveTexture_texture = texture;
	}
}
#define glActiveTexture(texture) cache_glActiveTexture(texture)

void inline cache_glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	if (sfactor != glState.cached_BlendFunc_sfactor || dfactor != glState.cached_BlendFunc_dfactor)	{
		glBlendFunc(sfactor, dfactor);
		glState.cached_BlendFunc_sfactor = sfactor;
		glState.cached_BlendFunc_dfactor = dfactor;
	}
}
#define glBlendFunc(sfactor, dfactor) cache_glBlendFunc(sfactor, dfactor)

void inline cache_glReadBuffer(GLenum mode)
{
	if (mode != glState.cached_ReadBufferMode)
		glState.cached_ReadBufferMode = mode;
}
#define glReadBuffer(mode) cache_glReadBuffer(mode)

void inline cache_glPixelStorei(GLenum target, GLint param)
{
	if (target != glState.cached_glPixelStorei_target || param != glState.cached_glPixelStorei_param) {
		glPixelStorei(target, param);
		glState.cached_glPixelStorei_target = target;
		glState.cached_glPixelStorei_param = param;
	}
}
#define glPixelStorei(target, param) cache_glPixelStorei(target, param)

void inline cache_glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	if (red != glState.cached_ClearColor_red || green != glState.cached_ClearColor_green || blue != glState.cached_ClearColor_blue || alpha != glState.cached_ClearColor_alpha)	{
		glClearColor(red, green, blue, alpha);
		glState.cached_ClearColor_red = red;
		glState.cached_ClearColor_green = green;
		glState.cached_ClearColor_blue = blue;
		glState.cached_ClearColor_alpha = alpha;
	}
}
#define glClearColor(red, green, blue, alpha) cache_glClearColor(red, green, blue, alpha)

void inline cache_glCullFace (GLenum mode)
{
	if (mode != glState.cached_CullFace_mode) {
		glCullFace(mode);
		glState.cached_CullFace_mode = mode;
	}
}
#define glCullFace(mode) cache_glCullFace(mode)

void inline cache_glDepthFunc (GLenum func)
{
	if (func != glState.cached_DepthFunc_func) {
		glDepthFunc(func);
		glState.cached_DepthFunc_func = func;
	}
}
#define glDepthFunc(func) cache_glDepthFunc(func)

void inline cache_glDepthMask (GLboolean flag)
{
	if (flag != glState.cached_DepthMask_flag) {
		glDepthMask(flag);
		glState.cached_DepthMask_flag = flag;
	}
}
#define glDepthMask(flag) cache_glDepthMask(flag)

void inline cache_glDisable (GLenum cap)
{
	switch (cap) {
	case GL_BLEND:
		if (glState.cached_BLEND) {
			glDisable(GL_BLEND);
			glState.cached_BLEND = false;
		}
		break;
	case GL_CULL_FACE:
		if (glState.cached_CULL_FACE) {
			glDisable(GL_CULL_FACE);
			glState.cached_CULL_FACE = false;
		}
		break;
	case GL_DEPTH_TEST:
		if (glState.cached_DEPTH_TEST) {
			glDisable(GL_DEPTH_TEST);
			glState.cached_DEPTH_TEST = false;
		}
		break;
#ifndef GLESX
	case GL_DEPTH_CLAMP:
		if (glState.cached_DEPTH_CLAMP) {
			glDisable(GL_DEPTH_CLAMP);
			glState.cached_DEPTH_CLAMP = false;
		}
		break;
	case GL_CLIP_DISTANCE0:
		if (glState.cached_CLIP_DISTANCE0) {
			glDisable(GL_CLIP_DISTANCE0);
			glState.cached_CLIP_DISTANCE0 = false;
		}
		break;
#endif
	case GL_DITHER:
		if (glState.cached_DITHER) {
			glDisable(GL_DITHER);
			glState.cached_DITHER = false;
		}
		break;
	case GL_POLYGON_OFFSET_FILL:
		if (glState.cached_POLYGON_OFFSET_FILL) {
			glDisable(GL_POLYGON_OFFSET_FILL);
			glState.cached_POLYGON_OFFSET_FILL = false;
		}
		break;
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
		if (glState.cached_SAMPLE_ALPHA_TO_COVERAGE) {
			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glState.cached_SAMPLE_ALPHA_TO_COVERAGE = false;
		}
		break;
	case GL_SAMPLE_COVERAGE:
		if (glState.cached_SAMPLE_COVERAGE) {
			glDisable(GL_SAMPLE_COVERAGE);
			glState.cached_SAMPLE_COVERAGE = false;
		}
		break;
	case GL_SCISSOR_TEST:
		if (glState.cached_SCISSOR_TEST) {
			glDisable(GL_SCISSOR_TEST);
			glState.cached_SCISSOR_TEST = false;
		}
		break;
	default:
		glDisable(cap);
		break;
	}
}
#define glDisable(cap) cache_glDisable(cap)

void inline cache_glEnable(GLenum cap)
{
	switch (cap) {
	case GL_BLEND:
		if (!glState.cached_BLEND) {
			glEnable(GL_BLEND);
			glState.cached_BLEND = true;
		}
		break;
	case GL_CULL_FACE:
		if (!glState.cached_CULL_FACE) {
			glEnable(GL_CULL_FACE);
			glState.cached_CULL_FACE = true;
		}
		break;
	case GL_DEPTH_TEST:
		if (!glState.cached_DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glState.cached_DEPTH_TEST = true;
		}
		break;
#ifndef GLESX
	case GL_DEPTH_CLAMP:
		if (!glState.cached_DEPTH_CLAMP) {
			glEnable(GL_DEPTH_CLAMP);
			glState.cached_DEPTH_CLAMP = true;
		}
		break;
	case GL_CLIP_DISTANCE0:
		if (!glState.cached_CLIP_DISTANCE0) {
			glEnable(GL_CLIP_DISTANCE0);
			glState.cached_CLIP_DISTANCE0 = true;
		}
		break;
#endif
	case GL_DITHER:
		if (!glState.cached_DITHER) {
			glEnable(GL_DITHER);
			glState.cached_DITHER = true;
		}
		break;
	case GL_POLYGON_OFFSET_FILL:
		if (!glState.cached_POLYGON_OFFSET_FILL) {
			glEnable(GL_POLYGON_OFFSET_FILL);
			glState.cached_POLYGON_OFFSET_FILL = true;
		}
		break;
	case GL_SAMPLE_ALPHA_TO_COVERAGE:
		if (!glState.cached_SAMPLE_ALPHA_TO_COVERAGE) {
			glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
			glState.cached_SAMPLE_ALPHA_TO_COVERAGE = true;
		}
		break;
	case GL_SAMPLE_COVERAGE:
		if (!glState.cached_SAMPLE_COVERAGE) {
			glEnable(GL_SAMPLE_COVERAGE);
			glState.cached_SAMPLE_COVERAGE = true;
		}
		break;
	case GL_SCISSOR_TEST:
		if (!glState.cached_SCISSOR_TEST) {
			glEnable(GL_SCISSOR_TEST);
			glState.cached_SCISSOR_TEST = true;
		}
		break;
	default:
		glEnable(cap);
		break;
	}
}
#define glEnable(cap) cache_glEnable(cap)

void inline cache_glPolygonOffset (GLfloat factor, GLfloat units)
{
	if (factor != glState.cached_PolygonOffset_factor || units != glState.cached_PolygonOffset_units) {
		glPolygonOffset(factor, units);
		glState.cached_PolygonOffset_factor = factor;
		glState.cached_PolygonOffset_units = units;
	}
}
#define glPolygonOffset(factor, units) cache_glPolygonOffset(factor, units)

void inline cache_glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (x != glState.cached_Scissor_x || y != glState.cached_Scissor_y || width != glState.cached_Scissor_width || height != glState.cached_Scissor_height) {
		glScissor(x, y, width, height);
		glState.cached_Scissor_x = x;
		glState.cached_Scissor_y = y;
		glState.cached_Scissor_width = width;
		glState.cached_Scissor_height = height;
	}
}
#define glScissor(x, y, width, height) cache_glScissor(x, y, width, height)

void inline cache_glUseProgram (GLuint program)
{
	if (program != glState.cached_UseProgram_program) {
		glUseProgram(program);
		glState.cached_UseProgram_program = program;
	}
}
#define glUseProgram(program) cache_glUseProgram(program)

void inline cache_glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (x != glState.cached_Viewport_x || y != glState.cached_Viewport_y || width != glState.cached_Viewport_width || height != glState.cached_Viewport_height) {
		glViewport(x, y, width, height);
		glState.cached_Viewport_x = x;
		glState.cached_Viewport_y = y;
		glState.cached_Viewport_width = width;
		glState.cached_Viewport_height = height;
	}
}
#define glViewport(x, y, width, height) cache_glViewport(x, y, width, height)

#ifdef __cplusplus
}
#endif

#endif //GLSTATE_H
