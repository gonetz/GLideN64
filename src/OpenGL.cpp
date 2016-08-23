#include <algorithm>
#include <assert.h>
#include <math.h>
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */

#include "Types.h"
#include "GLideN64.h"
#include "OpenGL.h"
#include "RDP.h"
#include "RSP.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "FrameBufferInfo.h"
#include "VI.h"
#include "Config.h"
#include "Log.h"
#include "TextDrawer.h"
#include "PostProcessor.h"
#include "ShaderUtils.h"
#include "SoftwareRender.h"
#include "FBOTextureFormats.h"
#include "TextureFilterHandler.h"

using namespace std;

bool checkFBO() {
	GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (e) {
//		case GL_FRAMEBUFFER_UNDEFINED:
//			printf("FBO Undefined\n");
//			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Incomplete Attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Missing Attachment\n");
			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
//			printf("FBO Incomplete Draw Buffer\n");
//			break;
		case GL_FRAMEBUFFER_UNSUPPORTED :
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Unsupported\n");
			break;
		case GL_FRAMEBUFFER_COMPLETE:
			LOG(LOG_VERBOSE, "[gles2GlideN64]: FBO OK\n");
			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
//			printf("framebuffer FRAMEBUFFER_DIMENSIONS\n");
//			break;
//		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
//			printf("framebuffer INCOMPLETE_FORMATS\n");
//			break;
		default:
			LOG(LOG_ERROR, "[gles2GlideN64]: FBO Problem?\n");
	}
	return e == GL_FRAMEBUFFER_COMPLETE;
}

const char* GLErrorString(GLenum errorCode)
{
	static const struct {
		GLenum code;
		const char *string;
	} errors[]=
	{
		/* GL */
	{GL_NO_ERROR, "no error"},
	{GL_INVALID_ENUM, "invalid enumerant"},
	{GL_INVALID_VALUE, "invalid value"},
	{GL_INVALID_OPERATION, "invalid operation"},
#ifndef GLESX
	{GL_STACK_OVERFLOW, "stack overflow"},
	{GL_STACK_UNDERFLOW, "stack underflow"},
#endif
	{GL_OUT_OF_MEMORY, "out of memory"},

	{0, nullptr }
};

	int i;

	for (i=0; errors[i].string; i++)
	{
		if (errors[i].code == errorCode)
		{
			return errors[i].string;
		}
	}

	return nullptr;
}

bool isGLError()
{
	GLenum errCode;
	const char* errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = GLErrorString(errCode);
		if (errString != nullptr)
			fprintf (stderr, "OpenGL Error: %s\n", errString);
		return true;
	}
	return false;
}

bool OGLVideo::isExtensionSupported(const char *extension)
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

void OGLVideo::start()
{
	_start(); // TODO: process initialization error
	initGLFunctions();
	m_render._initData();
	m_buffersSwapCount = 0;
}

void OGLVideo::stop()
{
	m_render._destroyData();
	_stop();
}

void OGLVideo::restart()
{
	m_bResizeWindow = true;
}

void OGLVideo::swapBuffers()
{
	_swapBuffers();
	gDP.otherMode.l = 0;
	if ((config.generalEmulation.hacks & hack_doNotResetTLUTmode) == 0)
		gDPSetTextureLUT(G_TT_NONE);
	++m_buffersSwapCount;
}

void OGLVideo::setCaptureScreen(const char * const _strDirectory)
{
	::mbstowcs(m_strScreenDirectory, _strDirectory, PLUGIN_PATH_SIZE-1);
	m_bCaptureScreen = true;
}

void OGLVideo::saveScreenshot()
{
	if (!m_bCaptureScreen)
		return;
	_saveScreenshot();
	m_bCaptureScreen = false;
}

bool OGLVideo::changeWindow()
{
	if (!m_bToggleFullscreen)
		return false;
	m_render._destroyData();
	_changeWindow();
	updateScale();
	m_render._initData();
	m_bToggleFullscreen = false;
	return true;
}

void OGLVideo::setWindowSize(u32 _width, u32 _height)
{
	if (m_width != _width || m_height != _height) {
		m_resizeWidth = _width;
		m_resizeHeight = _height;
		m_bResizeWindow = true;
	}
}

bool OGLVideo::resizeWindow()
{
	if (!m_bResizeWindow)
		return false;
	m_render._destroyData();
	if (!_resizeWindow())
		_start();
	updateScale();
	m_render._initData();
	m_bResizeWindow = false;
	return true;
}

void OGLVideo::updateScale()
{
	if (VI.width == 0 || VI.height == 0)
		return;
	m_scaleX = m_width / (float)VI.width;
	m_scaleY = m_height / (float)VI.height;
}

void OGLVideo::_setBufferSize()
{
	m_bAdjustScreen = false;
	if (config.frameBufferEmulation.enable) {
		switch (config.frameBufferEmulation.aspect) {
		case Config::aStretch: // stretch
			m_width = m_screenWidth;
			m_height = m_screenHeight;
			break;
		case Config::a43: // force 4/3
			if (m_screenWidth * 3 / 4 > m_screenHeight) {
				m_height = m_screenHeight;
				m_width = m_screenHeight * 4 / 3;
			} else if (m_screenHeight * 4 / 3 > m_screenWidth) {
				m_width = m_screenWidth;
				m_height = m_screenWidth * 3 / 4;
			} else {
				m_width = m_screenWidth;
				m_height = m_screenHeight;
			}
			break;
		case Config::a169: // force 16/9
			if (m_screenWidth * 9 / 16 > m_screenHeight) {
				m_height = m_screenHeight;
				m_width = m_screenHeight * 16 / 9;
			} else if (m_screenHeight * 16 / 9 > m_screenWidth) {
				m_width = m_screenWidth;
				m_height = m_screenWidth * 9 / 16;
			} else {
				m_width = m_screenWidth;
				m_height = m_screenHeight;
			}
			break;
		case Config::aAdjust: // adjust
			m_width = m_screenWidth;
			m_height = m_screenHeight;
			if (m_screenWidth * 3 / 4 > m_screenHeight) {
				f32 width43 = m_screenHeight * 4.0f / 3.0f;
				m_adjustScale = width43 / m_screenWidth;
				m_bAdjustScreen = true;
			}
			break;
		default:
			assert(false && "Unknown aspect ratio");
			m_width = m_screenWidth;
			m_height = m_screenHeight;
		}
	} else {
		m_width = m_screenWidth;
		m_height = m_screenHeight;
		if (config.frameBufferEmulation.aspect == Config::aAdjust && (m_screenWidth * 3 / 4 > m_screenHeight)) {
			f32 width43 = m_screenHeight * 4.0f / 3.0f;
			m_adjustScale = width43 / m_screenWidth;
			m_bAdjustScreen = true;
		}
	}
}

void OGLVideo::readScreen(void **_pDest, long *_pWidth, long *_pHeight )
{
	*_pWidth = m_width;
	*_pHeight = m_height;

	*_pDest = malloc( m_height * m_width * 3 );
	if (*_pDest == nullptr)
		return;

#ifndef GLESX
	const GLenum format = GL_BGR_EXT;
	glReadBuffer( GL_FRONT );
#else
	const GLenum format = GL_RGB;
#endif
	glReadPixels( 0, m_heightOffset, m_width, m_height, format, GL_UNSIGNED_BYTE, *_pDest );
}

void OGLVideo::readScreen2(void * _dest, int * _width, int * _height, int _front)
{
	if (_width == nullptr || _height == nullptr)
		return;

	*_width = m_screenWidth;
	*_height = m_screenHeight;

	if (_dest == nullptr)
		return;

	u8 *pBufferData = (u8*)malloc((*_width)*(*_height) * 4);
	u8 *pDest = (u8*)_dest;

#ifndef GLES2
	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	if (_front != 0)
		glReadBuffer(GL_FRONT);
	else
		glReadBuffer(GL_BACK);
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pBufferData);
	glReadBuffer(oldMode);
#else
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pBufferData);
#endif

	//Convert RGBA to RGB
	for (u32 y = 0; y < *_height; ++y) {
		u8 *ptr = pBufferData + ((*_width) * 4 * y);
		for (u32 x = 0; x < *_width; ++x) {
			pDest[x * 3] = ptr[0]; // red
			pDest[x * 3 + 1] = ptr[1]; // green
			pDest[x * 3 + 2] = ptr[2]; // blue
			ptr += 4;
		}
		pDest += (*_width) * 3;
	}

	free(pBufferData);
}

/*---------------OGLRender::TexrectDrawer-------------*/

OGLRender::TexrectDrawer::TexrectDrawer()
	: m_numRects(0)
	, m_otherMode(0)
	, m_mux(0)
	, m_ulx(0)
	, m_lrx(0)
	, m_uly(0)
	, m_lry(0)
	, m_Z(0)
	, m_FBO(0)
	, m_programTex(0)
	, m_programClean(0)
	, m_pTexture(nullptr)
	, m_pBuffer(nullptr)
{}

void OGLRender::TexrectDrawer::init()
{
	// generate a framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glGenFramebuffers(1, &m_FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);

	m_pTexture = textureCache().addFrameBufferTexture();
	m_pTexture->format = G_IM_FMT_RGBA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 640;
	m_pTexture->realHeight = 580;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight * 4;
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);
	glBindTexture(GL_TEXTURE_2D, m_pTexture->glName);
	glTexImage2D(GL_TEXTURE_2D, 0, fboFormats.colorInternalFormat, m_pTexture->realWidth, m_pTexture->realHeight, 0, fboFormats.colorFormat, fboFormats.colorType, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pTexture->glName, 0);
	// check if everything is OK
	assert(checkFBO());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	std::string fragmentShader(config.texture.bilinearMode == BILINEAR_STANDARD ? strTexrectDrawerTexBilinearFilter : strTexrectDrawerTex3PointFilter);
	fragmentShader += strTexrectDrawerFragmentShaderTex;
	m_programTex = createShaderProgram(strTexrectDrawerVertexShader, fragmentShader.c_str());
	m_programClean = createShaderProgram(strTexrectDrawerVertexShader, strTexrectDrawerFragmentShaderClean);

	glUseProgram(m_programTex);
	GLint loc = glGetUniformLocation(m_programTex, "uTex0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(m_programTex, "uTextureSize");
	if (loc >= 0)
		glUniform2f(loc, m_pTexture->realWidth, m_pTexture->realHeight);

	m_textureBoundsLoc = glGetUniformLocation(m_programTex, "uTextureBounds");
	assert(m_textureBoundsLoc >= 0);
	m_enableAlphaTestLoc = glGetUniformLocation(m_programTex, "uEnableAlphaTest");
	assert(m_enableAlphaTestLoc >= 0);

	glUseProgram(0);

	m_vecRectCoords.reserve(256);
}

void OGLRender::TexrectDrawer::destroy()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (m_FBO != 0) {
		glDeleteFramebuffers(1, &m_FBO);
		m_FBO = 0;
	}
	if (m_pTexture != nullptr) {
		textureCache().removeFrameBufferTexture(m_pTexture);
		m_pTexture = nullptr;
	}
	if (m_programTex != 0)
		glDeleteProgram(m_programTex);
	m_programTex = 0;
	if (m_programClean != 0)
		glDeleteProgram(m_programClean);
	m_programClean = 0;
}

void OGLRender::TexrectDrawer::add()
{
	OGLVideo & ogl = video();
	OGLRender & render = ogl.getRender();
	GLVertex * pRect = render.m_rect;

	bool bDownUp = false;
	if (m_numRects != 0) {
		bool bContinue = false;
		if (m_otherMode == gDP.otherMode._u64 && m_mux == gDP.combine.mux) {
			const float scaleY = (m_pBuffer != nullptr ? m_pBuffer->m_height : VI.height) / 2.0f;
			if (m_ulx == pRect[0].x) {
				//			bContinue = m_lry == pRect[0].y;
				bContinue = fabs((m_lry - pRect[0].y) * scaleY) < 1.1f; // Fix for Mario Kart
				bDownUp = m_uly == pRect[3].y;
				bContinue |= bDownUp;
			} else {
				for (auto iter = m_vecRectCoords.crbegin(); iter != m_vecRectCoords.crend(); ++iter) {
					if (iter->x == pRect[0].x && iter->y == pRect[0].y) {
						bContinue = true;
						break;
					}
				}
			}
		}
		if (!bContinue) {
			GLVertex rect[4];
			memcpy(rect, pRect, sizeof(rect));
			draw();
			memcpy(pRect, rect, sizeof(rect));
			render._updateTextures(rsTexRect);
		}
	}

	if (m_numRects == 0) {
		m_pBuffer = frameBufferList().getCurrent();
		m_otherMode = gDP.otherMode._u64;
		m_mux = gDP.combine.mux;
		m_Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
		m_scissor = gDP.scissor;

		m_ulx = pRect[0].x;
		m_uly = pRect[0].y;
		m_lrx = m_max_lrx = pRect[3].x;
		m_lry = m_max_lry = pRect[3].y;

		CombinerInfo::get().update();
		glDisable(GL_DEPTH_TEST);
		glDepthMask(FALSE);
		glDisable(GL_BLEND);

		if (m_pBuffer == nullptr)
			glViewport(0, 0, VI.width, VI.height);
		else
			glViewport(0, 0, m_pBuffer->m_width, m_pBuffer->m_height);

		glScissor(gDP.scissor.ulx, gDP.scissor.uly, gDP.scissor.lrx - gDP.scissor.ulx, gDP.scissor.lry - gDP.scissor.uly);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	}

	if (bDownUp) {
		m_ulx = pRect[0].x;
		m_uly = pRect[0].y;
	} else {
		m_lrx = pRect[3].x;
		m_lry = pRect[3].y;
		m_max_lrx = max(m_max_lrx, m_lrx);
		m_max_lry = max(m_max_lry, m_lry);
	}

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	RectCoords coords;
	coords.x = pRect[1].x;
	coords.y = pRect[1].y;
	m_vecRectCoords.push_back(coords);
	coords.x = pRect[3].x;
	coords.y = pRect[3].y;
	m_vecRectCoords.push_back(coords);

	++m_numRects;
}

bool OGLRender::TexrectDrawer::draw()
{
	if (m_numRects == 0)
		return false;

	const u64 otherMode = gDP.otherMode._u64;
	const gDPScissor scissor = gDP.scissor;
	gDP.scissor = m_scissor;
	gDP.otherMode._u64 = m_otherMode;
	OGLVideo & ogl = video();
	OGLRender & render = ogl.getRender();
	render._setBlendMode();
	gDP.changed |= CHANGED_RENDERMODE;  // Force update of depth compare parameters
	render._updateDepthCompare();

	int enableAlphaTest = 0;
	switch (gDP.otherMode.cycleType) {
	case G_CYC_COPY:
		if (gDP.otherMode.alphaCompare & G_AC_THRESHOLD)
			enableAlphaTest = 1;
		break;
	case G_CYC_1CYCLE:
	case G_CYC_2CYCLE:
		if (((gDP.otherMode.alphaCompare & G_AC_THRESHOLD) != 0) && (gDP.otherMode.alphaCvgSel == 0) && (gDP.otherMode.forceBlender == 0 || gDP.blendColor.a > 0))
			enableAlphaTest = 1;
		else if ((gDP.otherMode.alphaCompare == G_AC_DITHER) && (gDP.otherMode.alphaCvgSel == 0))
			enableAlphaTest = 1;
		else if (gDP.otherMode.cvgXAlpha != 0)
			enableAlphaTest = 1;
		break;
	}

	m_lrx = m_max_lrx;
	m_lry = m_max_lry;

	GLVertex * rect = render.m_rect;

	const float scaleX = (m_pBuffer != nullptr ? 1.0f / m_pBuffer->m_width : VI.rwidth) * 2.0f;
	const float scaleY = (m_pBuffer != nullptr ? 1.0f / m_pBuffer->m_height : VI.rheight) * 2.0f;

	const float s0 = (m_ulx + 1.0f) / scaleX / (float)m_pTexture->realWidth;
	const float t1 = (m_uly + 1.0f) / scaleY / (float)m_pTexture->realHeight;
	const float s1 = (m_lrx + 1.0f) / scaleX / (float)m_pTexture->realWidth;
	const float t0 = (m_lry + 1.0f) / scaleY / (float)m_pTexture->realHeight;
	const float W = 1.0f;

	if (m_pBuffer == nullptr)
		glViewport(0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
	else
		glViewport(0, 0, m_pBuffer->m_width*m_pBuffer->m_scaleX, m_pBuffer->m_height*m_pBuffer->m_scaleY);

	textureCache().activateTexture(0, m_pTexture);
	// Disable filtering to avoid black outlines
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glUseProgram(m_programTex);
	glUniform1i(m_enableAlphaTestLoc, enableAlphaTest);
	float texBounds[4] = { s0, t0, s1, t1 };
	glUniform4fv(m_textureBoundsLoc, 1, texBounds);
	
	glEnableVertexAttribArray(SC_TEXCOORD0);

	rect[0].x = m_ulx;
	rect[0].y = -m_lry;
	rect[0].z = m_Z;
	rect[0].w = W;
	rect[0].s0 = s0;
	rect[0].t0 = t0;
	rect[1].x = m_lrx;
	rect[1].y = -m_lry;
	rect[1].z = m_Z;
	rect[1].w = W;
	rect[1].s0 = s1;
	rect[1].t0 = t0;
	rect[2].x = m_ulx;
	rect[2].y = -m_uly;
	rect[2].z = m_Z;
	rect[2].w = W;
	rect[2].s0 = s0;
	rect[2].t0 = t1;
	rect[3].x = m_lrx;
	rect[3].y = -m_uly;
	rect[3].z = m_Z;
	rect[3].w = W;
	rect[3].s0 = s1;
	rect[3].t0 = t1;

	render.updateScissor(m_pBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pBuffer != nullptr ? m_pBuffer->m_FBO : 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
	glUseProgram(m_programClean);
	rect[0].y = m_uly;
	rect[1].y = m_uly;
	rect[2].y = m_lry;
	rect[3].y = m_lry;

	if (m_pBuffer == nullptr)
		glViewport(0, 0, VI.width, VI.height);
	else
		glViewport(0, 0, m_pBuffer->m_width, m_pBuffer->m_height);

	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_SCISSOR_TEST);

	m_pBuffer = frameBufferList().getCurrent();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pBuffer != nullptr ? m_pBuffer->m_FBO : 0);

	m_numRects = 0;
	m_vecRectCoords.clear();
	gDP.otherMode._u64 = otherMode;
	gDP.scissor = scissor;
	gDP.changed |= CHANGED_COMBINE | CHANGED_SCISSOR | CHANGED_RENDERMODE;
	gSP.changed |= CHANGED_VIEWPORT | CHANGED_TEXTURE;
	return true;
}

bool OGLRender::TexrectDrawer::isEmpty() {
	return m_numRects == 0;
}

/*---------------OGLRender-------------*/

void OGLRender::addTriangle(int _v0, int _v1, int _v2)
{
	const u32 firstIndex = triangles.num;
	triangles.elements[triangles.num++] = _v0;
	triangles.elements[triangles.num++] = _v1;
	triangles.elements[triangles.num++] = _v2;
	m_modifyVertices |= triangles.vertices[_v0].modify |
		triangles.vertices[_v1].modify |
		triangles.vertices[_v2].modify;

	if ((gSP.geometryMode & G_LIGHTING) == 0) {
		if ((gSP.geometryMode & G_SHADE) == 0) {
			// Prim shading
			for (u32 i = firstIndex; i < triangles.num; ++i) {
				SPVertex & vtx = triangles.vertices[triangles.elements[i]];
				vtx.flat_r = gDP.primColor.r;
				vtx.flat_g = gDP.primColor.g;
				vtx.flat_b = gDP.primColor.b;
				vtx.flat_a = gDP.primColor.a;
			}
		} else if ((gSP.geometryMode & G_SHADING_SMOOTH) == 0) {
			// Flat shading
			SPVertex & vtx0 = triangles.vertices[triangles.elements[firstIndex + ((RSP.w1 >> 24) & 3)]];
			for (u32 i = firstIndex; i < triangles.num; ++i) {
				SPVertex & vtx = triangles.vertices[triangles.elements[i]];
				vtx.r = vtx.flat_r = vtx0.r;
				vtx.g = vtx.flat_g = vtx0.g;
				vtx.b = vtx.flat_b = vtx0.b;
				vtx.a = vtx.flat_a = vtx0.a;
				vtx.a = vtx0.a;
			}
		}
	}

	if (gDP.otherMode.depthSource == G_ZS_PRIM) {
		for (u32 i = firstIndex; i < triangles.num; ++i) {
			SPVertex & vtx = triangles.vertices[triangles.elements[i]];
			vtx.z = gDP.primDepth.z * vtx.w;
		}
	}

#ifdef GLESX
	if (GBI.isNoN() && gDP.otherMode.depthCompare == 0 && gDP.otherMode.depthUpdate == 0) {
		for (u32 i = firstIndex; i < triangles.num; ++i) {
			SPVertex & vtx = triangles.vertices[triangles.elements[i]];
			vtx.z = 0.0f;
		}
	}
#endif
}

void OGLRender::_legacySetBlendMode() const
{
	const u32 blendmode = gDP.otherMode.l >> 16;
	// 0x7000 = CVG_X_ALPHA|ALPHA_CVG_SEL|FORCE_BL
	if (gDP.otherMode.alphaCvgSel != 0 && (gDP.otherMode.l & 0x7000) != 0x7000) {
		switch (blendmode) {
		case 0x4055: // Mario Golf
		case 0x5055: // Paper Mario intro clr_mem * a_in + clr_mem * a_mem
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_ONE);
			break;
		default:
			glDisable(GL_BLEND);
		}
		return;
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		glEnable(GL_BLEND);

		switch (blendmode)
		{
			// Mace objects
		case 0x0382:
			// Mace special blend mode, see GLSLCombiner.cpp
		case 0x0091:
			// 1080 Sky
		case 0x0C08:
			// Used LOTS of places
		case 0x0F0A:
			//DK64 blue prints
		case 0x0302:
			// Bomberman 2 special blend mode, see GLSLCombiner.cpp
		case 0xA500:
			//Sin and Punishment
		case 0xCB02:
			// Battlezone
			// clr_in * a + clr_in * (1-a)
		case 0xC800:
			// Conker BFD
			// clr_in * a_fog + clr_fog * (1-a)
			// clr_in * 0 + clr_in * 1
		case 0x07C2:
		case 0x00C0:
			//ISS64
		case 0xC302:
			// Donald Duck
		case 0xC702:
			glBlendFunc(GL_ONE, GL_ZERO);
			break;

		case 0x55f0:
			// Bust-A-Move 3 DX
			// CLR_MEM * A_FOG + CLR_FOG * 1MA
			glBlendFunc(GL_ONE, GL_SRC_ALPHA);
			break;

		case 0x0F1A:
			if (gDP.otherMode.cycleType == G_CYC_1CYCLE)
				glBlendFunc(GL_ONE, GL_ZERO);
			else
				glBlendFunc(GL_ZERO, GL_ONE);
			break;

			//Space Invaders
		case 0x0448: // Add
		case 0x055A:
			glBlendFunc(GL_ONE, GL_ONE);
			break;

		case 0xc712: // Pokemon Stadium?
		case 0xAF50: // LOT in Zelda: MM
		case 0x0F5A: // LOT in Zelda: MM
		case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
		case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
			//clr_in * 0 + clr_mem * 1
			glBlendFunc(GL_ZERO, GL_ONE);
			break;

		case 0x5F50: //clr_mem * 0 + clr_mem * (1-a)
			glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case 0xF550: //clr_fog * a_fog + clr_mem * (1-a)
		case 0x0150: // spiderman
		case 0x0550: // bomberman 64
		case 0x0D18: //clr_in * a_fog + clr_mem * (1-a)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case 0xC912: //40 winks, clr_in * a_fog + clr_mem * 1
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;

		case 0x0040: // Fzero
		case 0xC810: // Blends fog
		case 0x0C18: // Standard interpolated blend
		case 0x0050: // Standard interpolated blend
		case 0x0051: // Standard interpolated blend
		case 0x0055: // Used for antialiasing
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case 0x0C19: // Used for antialiasing
		case 0xC811: // Blends fog
			glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
			break;

		case 0x5000: // V8 explosions
			glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
			break;

		case 0xFA00: // Bomberman second attack
			glBlendFunc(GL_ONE, GL_ZERO);
			break;

		default:
			//LOG(LOG_VERBOSE, "Unhandled blend mode=%x", gDP.otherMode.l >> 16);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		}
	}
	else if ((config.generalEmulation.hacks & hack_pilotWings) != 0 && (gDP.otherMode.l & 0x80) != 0) { //CLR_ON_CVG without FORCE_BL
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	}
	else if ((config.generalEmulation.hacks & hack_blastCorps) != 0 && gDP.otherMode.cycleType < G_CYC_COPY && gSP.texture.on == 0 && currentCombiner()->usesTexture()) { // Blast Corps
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	}
	else {
		glDisable(GL_BLEND);
	}
}

void OGLRender::_setBlendMode() const
{
	if (config.generalEmulation.enableLegacyBlending != 0) {
		_legacySetBlendMode();
		return;
	}

	if (gDP.otherMode.forceBlender != 0 && gDP.otherMode.cycleType < G_CYC_COPY) {
		GLenum srcFactor = GL_ONE;
		GLenum dstFactor = GL_ZERO;
		u32 memFactorSource = 2, muxA, muxB;
		if (gDP.otherMode.cycleType == G_CYC_2CYCLE) {
			muxA = gDP.otherMode.c2_m1b;
			muxB = gDP.otherMode.c2_m2b;
			if (gDP.otherMode.c2_m1a == 1) {
				if (gDP.otherMode.c2_m2a == 1) {
					glEnable(GL_BLEND);
					glBlendFunc(GL_ZERO, GL_ONE);
					return;
				}
				memFactorSource = 0;
			} else if (gDP.otherMode.c2_m2a == 1) {
				memFactorSource = 1;
			}
			if (gDP.otherMode.c2_m2a == 0 && gDP.otherMode.c2_m2b == 1) {
				// c_in * a_mem
				srcFactor = GL_DST_ALPHA;
			}
		} else {
			muxA = gDP.otherMode.c1_m1b;
			muxB = gDP.otherMode.c1_m2b;
			if (gDP.otherMode.c1_m1a == 1) {
				if (gDP.otherMode.c1_m2a == 1) {
					glEnable(GL_BLEND);
					glBlendFunc(GL_ZERO, GL_ONE);
					return;
				}
				memFactorSource = 0;
			} else if (gDP.otherMode.c1_m2a == 1) {
				memFactorSource = 1;
			}
			if (gDP.otherMode.c1_m2a == 0 && gDP.otherMode.c1_m2b == 1) {
				// c_pixel * a_mem
				srcFactor = GL_DST_ALPHA;
			}
		}
		switch (memFactorSource) {
		case 0:
			switch (muxA) {
			case 0:
				dstFactor = GL_SRC_ALPHA;
				break;
			case 1:
				glBlendColor(gDP.fogColor.r, gDP.fogColor.g, gDP.fogColor.b, gDP.fogColor.a);
				dstFactor = GL_CONSTANT_ALPHA;
				break;
			case 2:
				assert(false); // shade alpha
				dstFactor = GL_SRC_ALPHA;
				break;
			case 3:
				dstFactor = GL_ZERO;
				break;
			}
			break;
		case 1:
			switch (muxB) {
			case 0:
				// 1.0 - muxA
				switch (muxA) {
				case 0:
					dstFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case 1:
					glBlendColor(gDP.fogColor.r, gDP.fogColor.g, gDP.fogColor.b, gDP.fogColor.a);
					dstFactor = GL_ONE_MINUS_CONSTANT_ALPHA;
					break;
				case 2:
					assert(false); // shade alpha
					dstFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case 3:
					dstFactor = GL_ONE;
					break;
				}
				break;
			case 1:
				dstFactor = GL_DST_ALPHA;
				break;
			case 2:
				dstFactor = GL_ONE;
				break;
			case 3:
				dstFactor = GL_ZERO;
				break;
			}
			break;
		default:
			dstFactor = GL_ZERO;
		}
		glEnable( GL_BLEND );
		glBlendFunc(srcFactor, dstFactor);
	} else if ((config.generalEmulation.hacks & hack_pilotWings) != 0 && gDP.otherMode.clearOnCvg != 0) { //CLR_ON_CVG without FORCE_BL
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	} else if ((config.generalEmulation.hacks & hack_blastCorps) != 0 && gDP.otherMode.cycleType < G_CYC_COPY && gSP.texture.on == 0 && currentCombiner()->usesTexture()) { // Blast Corps
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE);
	} else if ((gDP.otherMode.forceBlender == 0 && gDP.otherMode.cycleType < G_CYC_COPY)) {
		if (gDP.otherMode.c1_m1a == 1 && gDP.otherMode.c1_m2a == 1) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_ONE);
		} else {
			glDisable(GL_BLEND);
		}
	} else {
		glDisable( GL_BLEND );
	}
}

void OGLRender::_updateCullFace() const
{
	if (gSP.geometryMode & G_CULL_BOTH) {
		glEnable( GL_CULL_FACE );

		if (gSP.geometryMode & G_CULL_BACK)
			glCullFace(GL_BACK);
		else
			glCullFace(GL_FRONT);
	}
	else
		glDisable( GL_CULL_FACE );
}

inline
float _adjustViewportX(f32 _X0)
{
		const float halfX = gDP.colorImage.width / 2.0f;
		const float halfVP = gSP.viewport.width / 2.0f;
		return (_X0 + halfVP - halfX) * video().getAdjustScale() + halfX - halfVP;
}

void OGLRender::_updateViewport() const
{
	OGLVideo & ogl = video();
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	if (pCurrentBuffer == nullptr) {
		const f32 scaleX = ogl.getScaleX();
		const f32 scaleY = ogl.getScaleY();
		float Xf = gSP.viewport.vscale[0] < 0 ? (gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) : gSP.viewport.x;
		if (ogl.isAdjustScreen() && gSP.viewport.width < gDP.colorImage.width && gDP.colorImage.width > VI.width * 98 / 100)
			Xf = _adjustViewportX(Xf);
		const GLint X = (GLint)(Xf * scaleX);
		const GLint Y = gSP.viewport.vscale[1] < 0 ? (GLint)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * scaleY) : (GLint)((VI.height - (gSP.viewport.y + gSP.viewport.height)) * scaleY);
		glViewport(X, Y + ogl.getHeightOffset(),
			max((GLint)(gSP.viewport.width * scaleX), 0), max((GLint)(gSP.viewport.height * scaleY), 0));
	} else {
		const f32 scaleX = pCurrentBuffer->m_scaleX;
		const f32 scaleY = pCurrentBuffer->m_scaleY;
		float Xf = gSP.viewport.vscale[0] < 0 ? (gSP.viewport.x + gSP.viewport.vscale[0] * 2.0f) : gSP.viewport.x;
		if (ogl.isAdjustScreen() && gSP.viewport.width < gDP.colorImage.width && gDP.colorImage.width > VI.width * 98 / 100)
			Xf = _adjustViewportX(Xf);
		const GLint X = (GLint)(Xf * scaleX);
		const GLint Y = gSP.viewport.vscale[1] < 0 ? (GLint)((gSP.viewport.y + gSP.viewport.vscale[1] * 2.0f) * scaleY) : (GLint)((pCurrentBuffer->m_height - (gSP.viewport.y + gSP.viewport.height)) * scaleY);
		glViewport(X, Y,
			max((GLint)(gSP.viewport.width * scaleX), 0), max((GLint)(gSP.viewport.height * scaleY), 0));
	}
	gSP.changed &= ~CHANGED_VIEWPORT;
}

void OGLRender::_updateScreenCoordsViewport() const
{
	OGLVideo & ogl = video();
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	if (pCurrentBuffer == nullptr)
		glViewport(0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
	else
		glViewport(0, 0, pCurrentBuffer->m_width*pCurrentBuffer->m_scaleX, pCurrentBuffer->m_height*pCurrentBuffer->m_scaleY);
	gSP.changed |= CHANGED_VIEWPORT;
}

inline
void _adjustScissorX(f32 & _X0, f32 & _X1, float _scale)
{
	const float halfX = gDP.colorImage.width / 2.0f;
	_X0 = (_X0 - halfX) * _scale + halfX;
	_X1 = (_X1 - halfX) * _scale + halfX;
}

void OGLRender::updateScissor(FrameBuffer * _pBuffer) const
{
	OGLVideo & ogl = video();
	f32 scaleX, scaleY;
	u32 heightOffset, screenHeight;
	if (_pBuffer == nullptr) {
		scaleX = ogl.getScaleX();
		scaleY = ogl.getScaleY();
		heightOffset = ogl.getHeightOffset();
		screenHeight = VI.height;
	}
	else {
		scaleX = _pBuffer->m_scaleX;
		scaleY = _pBuffer->m_scaleY;
		heightOffset = 0;
		screenHeight = (_pBuffer->m_height == 0) ? VI.height : _pBuffer->m_height;
	}

	float SX0 = gDP.scissor.ulx;
	float SX1 = gDP.scissor.lrx;
	if (ogl.isAdjustScreen() && gSP.viewport.width < gDP.colorImage.width && gDP.colorImage.width > VI.width * 98 / 100)
		_adjustScissorX(SX0, SX1, ogl.getAdjustScale());

	glScissor((GLint)(SX0 * scaleX), (GLint)((screenHeight - gDP.scissor.lry) * scaleY + heightOffset),
		max((GLint)((SX1 - SX0) * scaleX), 0), max((GLint)((gDP.scissor.lry - gDP.scissor.uly) * scaleY), 0));
	gDP.changed &= ~CHANGED_SCISSOR;
}

void OGLRender::_updateDepthUpdate() const
{
	if (gDP.otherMode.depthUpdate != 0)
		glDepthMask( TRUE );
	else
		glDepthMask( FALSE );
}

void OGLRender::_updateDepthCompare() const
{
	if (config.frameBufferEmulation.N64DepthCompare) {
		glDisable( GL_DEPTH_TEST );
		glDepthMask( FALSE );
	} else if ((gDP.changed & (CHANGED_RENDERMODE | CHANGED_CYCLETYPE)) != 0) {
		if (((gSP.geometryMode & G_ZBUFFER) || gDP.otherMode.depthSource == G_ZS_PRIM) && gDP.otherMode.cycleType <= G_CYC_2CYCLE) {
			if (gDP.otherMode.depthCompare != 0) {
				switch (gDP.otherMode.depthMode) {
					case ZMODE_INTER:
					glDisable(GL_POLYGON_OFFSET_FILL);
					glDepthFunc(GL_LEQUAL);
					break;
					case ZMODE_OPA:
					case ZMODE_XLU:
					// Max || Infront;
					glDisable(GL_POLYGON_OFFSET_FILL);
					if (gDP.otherMode.depthSource == G_ZS_PRIM && gDP.primDepth.z == 1.0f)
						// Max
						glDepthFunc(GL_LEQUAL);
					else
						// Infront
						glDepthFunc(GL_LESS);
					break;
					case ZMODE_DEC:
					glEnable(GL_POLYGON_OFFSET_FILL);
					glDepthFunc(GL_LEQUAL);
					break;
				}
			} else {
				glDisable(GL_POLYGON_OFFSET_FILL);
				glDepthFunc(GL_ALWAYS);
			}

			_updateDepthUpdate();

			glEnable(GL_DEPTH_TEST);
#ifndef GLESX
			if (!GBI.isNoN())
				glDisable(GL_DEPTH_CLAMP);
#endif
		} else {
			glDisable(GL_DEPTH_TEST);
#ifndef GLESX
			if (!GBI.isNoN())
				glEnable(GL_DEPTH_CLAMP);
#endif
		}
	}
}

void OGLRender::_updateTextures(RENDER_STATE _renderState) const
{
	//For some reason updating the texture cache on the first frame of LOZ:OOT causes a nullptr Pointer exception...
	CombinerInfo & cmbInfo = CombinerInfo::get();
	ShaderCombiner * pCurrentCombiner = cmbInfo.getCurrent();
	if (pCurrentCombiner != nullptr) {
		for (u32 t = 0; t < 2; ++t) {
			if (pCurrentCombiner->usesTile(t))
				textureCache().update(t);
			else
				textureCache().activateDummy(t);
		}
		pCurrentCombiner->updateFrameBufferInfo();
	}
	if (pCurrentCombiner->usesTexture() && (_renderState == rsTriangle || _renderState == rsLine))
		cmbInfo.updateTextureParameters();
	gDP.changed &= ~(CHANGED_TILE | CHANGED_TMEM);
	gSP.changed &= ~(CHANGED_TEXTURE);
}


void OGLRender::_updateStates(RENDER_STATE _renderState) const
{
	OGLVideo & ogl = video();

	CombinerInfo & cmbInfo = CombinerInfo::get();
	cmbInfo.update();

	if (gSP.changed & CHANGED_GEOMETRYMODE) {
		_updateCullFace();
		gSP.changed &= ~CHANGED_GEOMETRYMODE;
	}

	_updateDepthCompare();

	if (gDP.changed & CHANGED_SCISSOR)
		updateScissor(frameBufferList().getCurrent());

	if (gSP.changed & CHANGED_VIEWPORT)
		_updateViewport();

	if (gSP.changed & CHANGED_LIGHT)
		cmbInfo.updateLightParameters();

	if ((gSP.changed & CHANGED_TEXTURE) ||
		(gDP.changed & (CHANGED_TILE|CHANGED_TMEM)) ||
		cmbInfo.isChanged() ||
		_renderState == rsTexRect) {
		_updateTextures(_renderState);
	}

	if ((gDP.changed & (CHANGED_RENDERMODE | CHANGED_CYCLETYPE))) {
		_setBlendMode();
		gDP.changed &= ~(CHANGED_RENDERMODE | CHANGED_CYCLETYPE);
	}

	cmbInfo.updateParameters(_renderState);

#ifndef GLES2
	if (gDP.colorImage.address == gDP.depthImageAddress &&
		gDP.otherMode.cycleType != G_CYC_FILL &&
		(config.generalEmulation.hacks & hack_ZeldaMM) == 0
	) {
		FrameBuffer * pCurBuf = frameBufferList().getCurrent();
		if (pCurBuf != nullptr && pCurBuf->m_pDepthBuffer != nullptr) {
			if (gDP.otherMode.depthCompare != 0) {
				CachedTexture * pDepthTexture = pCurBuf->m_pDepthBuffer->copyDepthBufferTexture(pCurBuf);
				if (pDepthTexture == nullptr)
					return;
				glActiveTexture(GL_TEXTURE0 + g_depthTexIndex);
				glBindTexture(GL_TEXTURE_2D, pDepthTexture->glName);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);
			glDepthMask(TRUE);
			gDP.changed |= CHANGED_RENDERMODE;
		}
	}
#endif
}

void OGLRender::_setColorArray() const
{
	if (currentCombiner()->usesShade() || gDP.otherMode.c1_m1b == 2)
		// combiner uses shade or blender uses shade alpha
		glEnableVertexAttribArray(SC_COLOR);
	else
		glDisableVertexAttribArray(SC_COLOR);
}

void OGLRender::_setTexCoordArrays() const
{
	if (m_renderState == rsTriangle) {
		glDisableVertexAttribArray(SC_TEXCOORD1);
		if (currentCombiner()->usesTexture())
			glEnableVertexAttribArray(SC_TEXCOORD0);
		else
			glDisableVertexAttribArray(SC_TEXCOORD0);
	} else {
		if (currentCombiner()->usesTile(0))
			glEnableVertexAttribArray(SC_TEXCOORD0);
		else
			glDisableVertexAttribArray(SC_TEXCOORD0);

		if (currentCombiner()->usesTile(1))
			glEnableVertexAttribArray(SC_TEXCOORD1);
		else
			glDisableVertexAttribArray(SC_TEXCOORD1);
	}
}

void OGLRender::_prepareDrawTriangle(bool _dma)
{
	m_texrectDrawer.draw();

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	if (m_bImageTexture && config.frameBufferEmulation.N64DepthCompare != 0)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#endif // GL_IMAGE_TEXTURES_SUPPORT

	if ((m_modifyVertices & MODIFY_XY) != 0)
		gSP.changed &= ~CHANGED_VIEWPORT;

	if (gSP.changed || gDP.changed)
		_updateStates(rsTriangle);

	const bool updateArrays = m_renderState != rsTriangle;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		m_renderState = rsTriangle;
		_setColorArray();
		_setTexCoordArrays();
	}
	currentCombiner()->updateRenderState();

	bool bFlatColors = false;
	if (!RSP.bLLE && (gSP.geometryMode & G_LIGHTING) == 0) {
		bFlatColors = (gSP.geometryMode & G_SHADE) == 0;
		bFlatColors |= (gSP.geometryMode & G_SHADING_SMOOTH) == 0;
	}

	const bool updateColorArrays = m_bFlatColors != bFlatColors;
	m_bFlatColors = bFlatColors;

	if (updateArrays) {
		SPVertex * pVtx = _dma ? triangles.dmaVertices.data() : &triangles.vertices[0];
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->x);
		if (m_bFlatColors)
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->flat_r);
		else
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->r);
		glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->s);
		if (config.generalEmulation.enableHWLighting) {
			glEnableVertexAttribArray(SC_NUMLIGHTS);
			glVertexAttribPointer(SC_NUMLIGHTS, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), &pVtx->HWLight);
		}
		glEnableVertexAttribArray(SC_MODIFY);
		glVertexAttribPointer(SC_MODIFY, 4, GL_BYTE, GL_FALSE, sizeof(SPVertex), &pVtx->modify);
	} else if (updateColorArrays) {
		SPVertex * pVtx = _dma ? triangles.dmaVertices.data() : &triangles.vertices[0];
		if (m_bFlatColors)
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->flat_r);
		else
			glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &pVtx->r);
	}

	if ((m_modifyVertices & MODIFY_XY) != 0)
		_updateScreenCoordsViewport();
	m_modifyVertices = 0;
}

bool OGLRender::_canDraw() const
{
	return config.frameBufferEmulation.enable == 0 || frameBufferList().getCurrent() != nullptr;
}

void OGLRender::drawLLETriangle(u32 _numVtx)
{
	if (_numVtx == 0 || !_canDraw())
		return;

	for (u32 i = 0; i < _numVtx; ++i) {
		SPVertex & vtx = triangles.vertices[i];
		vtx.modify = MODIFY_ALL;
	}
	m_modifyVertices = MODIFY_ALL;

	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	_prepareDrawTriangle(false);
	glDisable(GL_CULL_FACE);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVtx);
	triangles.num = 0;

	frameBufferList().setBufferChanged();
	gSP.changed |= CHANGED_GEOMETRYMODE;
}

void OGLRender::drawDMATriangles(u32 _numVtx)
{
	if (_numVtx == 0 || !_canDraw())
		return;
	_prepareDrawTriangle(true);
	glDrawArrays(GL_TRIANGLES, 0, _numVtx);
}

void OGLRender::drawTriangles()
{
	if (triangles.num == 0 || !_canDraw()) {
		triangles.num = 0;
		return;
	}

	_prepareDrawTriangle(false);
	glDrawElements(GL_TRIANGLES, triangles.num, GL_UNSIGNED_BYTE, triangles.elements);
//	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdSoftwareRender && gDP.otherMode.depthUpdate != 0) {
		renderTriangles(triangles.vertices, triangles.elements, triangles.num);
		FrameBuffer * pCurrentDepthBuffer = frameBufferList().findBuffer(gDP.depthImageAddress);
		if (pCurrentDepthBuffer != nullptr)
			pCurrentDepthBuffer->m_cleared = false;
	}

	triangles.num = 0;
}

void OGLRender::drawLine(int _v0, int _v1, float _width)
{
	if (!_canDraw())
		return;

	if ((triangles.vertices[_v0].modify & MODIFY_XY) != 0)
		gSP.changed &= ~CHANGED_VIEWPORT;
	if (gSP.changed || gDP.changed)
		_updateStates(rsLine);

	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();

	if (m_renderState != rsLine || CombinerInfo::get().isChanged()) {
		_setColorArray();
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].x);
		glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].r);
		glEnableVertexAttribArray(SC_MODIFY);
		glVertexAttribPointer(SC_MODIFY, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), &triangles.vertices[0].modify);

		m_renderState = rsLine;
		currentCombiner()->updateRenderState();
	}

	if ((triangles.vertices[_v0].modify & MODIFY_XY) != 0)
		_updateScreenCoordsViewport();

	unsigned short elem[2];
	elem[0] = _v0;
	elem[1] = _v1;
	if (config.frameBufferEmulation.nativeResFactor == 0)
		glLineWidth(_width * video().getScaleX());
	else
		glLineWidth(_width * config.frameBufferEmulation.nativeResFactor);
	glDrawElements(GL_LINES, 2, GL_UNSIGNED_SHORT, elem);
}

void OGLRender::drawRect(int _ulx, int _uly, int _lrx, int _lry, float *_pColor)
{
	m_texrectDrawer.draw();
	if (!_canDraw())
		return;
	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	if (gSP.changed || gDP.changed)
		_updateStates(rsRect);

	const bool updateArrays = m_renderState != rsRect;
	if (updateArrays || CombinerInfo::get().isChanged()) {
		m_renderState = rsRect;
		glDisableVertexAttribArray(SC_COLOR);
		glDisableVertexAttribArray(SC_TEXCOORD0);
		glDisableVertexAttribArray(SC_TEXCOORD1);
		glDisableVertexAttribArray(SC_NUMLIGHTS);
		glDisableVertexAttribArray(SC_MODIFY);
	}

	if (updateArrays)
		glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
	currentCombiner()->updateRenderState();

	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	OGLVideo & ogl = video();
	if (pCurrentBuffer == nullptr)
		glViewport( 0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
	else {
		glViewport(0, 0, pCurrentBuffer->m_width*pCurrentBuffer->m_scaleX, pCurrentBuffer->m_height*pCurrentBuffer->m_scaleY);
	}
	glDisable(GL_CULL_FACE);

	const float scaleX = pCurrentBuffer != nullptr ? 1.0f / pCurrentBuffer->m_width : VI.rwidth;
	const float scaleY = pCurrentBuffer != nullptr ? 1.0f / pCurrentBuffer->m_height : VI.rheight;
	const float Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
	const float W = 1.0f;
	m_rect[0].x = (float)_ulx * (2.0f * scaleX) - 1.0;
	m_rect[0].y = (float)_uly * (-2.0f * scaleY) + 1.0;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = (float)_lrx * (2.0f * scaleX) - 1.0;
	m_rect[1].y = m_rect[0].y;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = (float)_lry * (-2.0f * scaleY) + 1.0;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	if (ogl.isAdjustScreen() && (gDP.colorImage.width > VI.width * 98 / 100) && (_lrx - _ulx < VI.width * 9 / 10)) {
		const float scale = ogl.getAdjustScale();
		for (u32 i = 0; i < 4; ++i)
			m_rect[i].x *= scale;
	}

	if (gDP.otherMode.cycleType == G_CYC_FILL)
		glVertexAttrib4fv(SC_COLOR, _pColor);
	else
		glVertexAttrib4f(SC_COLOR, 0.0f, 0.0f, 0.0f, 0.0f);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
}

static
bool texturedRectShadowMap(const OGLRender::TexturedRectParams &)
{
	FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
	if (pCurrentBuffer != nullptr) {
		if (gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*gDP.colorImage.width * 6 / 4)) {
#ifdef GL_IMAGE_TEXTURES_SUPPORT
			pCurrentBuffer->m_pDepthBuffer->activateDepthBufferTexture(pCurrentBuffer);
			SetDepthFogCombiner();
#else
			return true;
#endif
		}
	}
	return false;
}

u32 rectDepthBufferCopyFrame = 0xFFFFFFFF;
static
bool texturedRectDepthBufferCopy(const OGLRender::TexturedRectParams & _params)
{
	// Copy one line from depth buffer into auxiliary color buffer with height = 1.
	// Data from depth buffer loaded into TMEM and then rendered to RDRAM by texrect.
	// Works only with depth buffer emulation enabled.
	// Load of arbitrary data to that area causes weird camera rotation in CBFD.
	const gDPTile * pTile = gSP.textureTile[0];
	if (pTile->loadType == LOADTYPE_BLOCK && gDP.textureImage.size == 2 && gDP.textureImage.address >= gDP.depthImageAddress &&  gDP.textureImage.address < (gDP.depthImageAddress + gDP.colorImage.width*gDP.colorImage.width * 6 / 4)) {
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdDisable)
			return true;
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer == nullptr)
			return true;
		pBuffer->m_cleared = true;
		if (config.frameBufferEmulation.copyDepthToRDRAM == Config::cdCopyFromVRam) {
			if (rectDepthBufferCopyFrame != video().getBuffersSwapCount()) {
				rectDepthBufferCopyFrame = video().getBuffersSwapCount();
				if (!FrameBuffer_CopyDepthBuffer(gDP.colorImage.address))
					return true;
			}
			RDP_RepeatLastLoadBlock();
		}

		const u32 width = (u32)(_params.lrx - _params.ulx);
		const u32 ulx = (u32)_params.ulx;
		u16 * pSrc = ((u16*)TMEM) + (u32)floorf(_params.uls + 0.5f);
		u16 *pDst = (u16*)(RDRAM + gDP.colorImage.address);
		for (u32 x = 0; x < width; ++x)
			pDst[(ulx + x) ^ 1] = swapword(pSrc[x]);

		return true;
	}
	return false;
}

static
bool texturedRectCopyToItself(const OGLRender::TexturedRectParams & _params)
{
	FrameBuffer * pCurrent = frameBufferList().getCurrent();
	if (pCurrent != nullptr && pCurrent->m_size == G_IM_SIZ_8b && gSP.textureTile[0]->frameBuffer == pCurrent)
		return true;
	return texturedRectDepthBufferCopy(_params);
}

static
bool texturedRectBGCopy(const OGLRender::TexturedRectParams & _params)
{
	if (GBI.getMicrocodeType() != S2DEX)
		return false;

	float flry = _params.lry;
	if (flry > gDP.scissor.lry)
		flry = gDP.scissor.lry;

	const u32 width = (u32)(_params.lrx - _params.ulx);
	const u32 tex_width = gSP.textureTile[0]->line << 3;
	const u32 uly = (u32)_params.uly;
	const u32 lry = flry;

	u8 * texaddr = RDRAM + gDP.loadInfo[gSP.textureTile[0]->tmem].texAddress + tex_width*(u32)_params.ult + (u32)_params.uls;
	u8 * fbaddr = RDRAM + gDP.colorImage.address + (u32)_params.ulx;
//	LOG(LOG_VERBOSE, "memrect (%d, %d, %d, %d), ci_width: %d texaddr: 0x%08lx fbaddr: 0x%08lx\n", (u32)_params.ulx, uly, (u32)_params.lrx, lry, gDP.colorImage.width, gSP.textureTile[0]->imageAddress + tex_width*(u32)_params.ult + (u32)_params.uls, gDP.colorImage.address + (u32)_params.ulx);

	for (u32 y = uly; y < lry; ++y) {
		u8 *src = texaddr + (y - uly) * tex_width;
		u8 *dst = fbaddr + y * gDP.colorImage.width;
		memcpy(dst, src, width);
	}
	frameBufferList().removeBuffer(gDP.colorImage.address);
	return true;
}

static
bool texturedRectPaletteMod(const OGLRender::TexturedRectParams & _params)
{
	if (gDP.textureImage.address == 0x400) {
		// Paper Mario uses complex set of actions to prepare darkness texture.
		// It includes manipulations with texture formats and drawing buffer into itsels.
		// All that stuff is hardly possible to reproduce with GL, so I just use dirty hacks to emualte it.

		if (gDP.colorImage.address == 0x400 && gDP.colorImage.width == 64) {
			memcpy(RDRAM + 0x400, RDRAM + 0x14d500, 4096);
			return true;
		}

		if (gDP.textureImage.width == 64) {
			gDPTile & curTile = gDP.tiles[0];
			curTile.frameBuffer = nullptr;
			curTile.textureMode = TEXTUREMODE_NORMAL;
			textureCache().update(0);
			currentCombiner()->updateFrameBufferInfo();
		}
		return false;
	}

	// Modify palette for Paper Mario "2D lighting" effect
	if (gDP.scissor.lrx != 16 || gDP.scissor.lry != 1 || _params.lrx != 16 || _params.lry != 1)
		return false;
	u8 envr = (u8)(gDP.envColor.r * 31.0f);
	u8 envg = (u8)(gDP.envColor.g * 31.0f);
	u8 envb = (u8)(gDP.envColor.b * 31.0f);
	u16 env16 = (u16)((envr << 11) | (envg << 6) | (envb << 1) | 1);
	u8 prmr = (u8)(gDP.primColor.r * 31.0f);
	u8 prmg = (u8)(gDP.primColor.g * 31.0f);
	u8 prmb = (u8)(gDP.primColor.b * 31.0f);
	u16 prim16 = (u16)((prmr << 11) | (prmg << 6) | (prmb << 1) | 1);
	u16 * src = (u16*)&TMEM[256];
	u16 * dst = (u16*)(RDRAM + gDP.colorImage.address);
	for (u32 i = 0; i < 16; ++i)
		dst[i ^ 1] = (src[i<<2] & 0x100) ? prim16 : env16;
	return true;
}

static
bool texturedRectMonochromeBackground(const OGLRender::TexturedRectParams & _params)
{
	if (gDP.textureImage.address >= gDP.colorImage.address && gDP.textureImage.address <= (gDP.colorImage.address + gDP.colorImage.width*gDP.colorImage.height * 2)) {
#ifdef GL_IMAGE_TEXTURES_SUPPORT
		FrameBuffer * pCurrentBuffer = frameBufferList().getCurrent();
		if (pCurrentBuffer != nullptr) {
			FrameBuffer_ActivateBufferTexture(0, pCurrentBuffer);
			SetMonochromeCombiner();
			return false;
		} else
#endif
			return true;
	}
	return false;
}

// Special processing of textured rect.
// Return true if actuial rendering is not necessary
bool(*texturedRectSpecial)(const OGLRender::TexturedRectParams & _params) = nullptr;

void OGLRender::drawTexturedRect(const TexturedRectParams & _params)
{
	gSP.changed &= ~CHANGED_GEOMETRYMODE; // Don't update cull mode
	if (!m_texrectDrawer.isEmpty()) {
		CombinerInfo::get().update();
		currentCombiner()->updateRenderState();
		_updateTextures(rsTexRect);
		if (CombinerInfo::get().isChanged())
			_setTexCoordArrays();
	} else {
		if (_params.texrectCmd && (gSP.changed | gDP.changed) != 0)
			_updateStates(rsTexRect);
		glDisable(GL_CULL_FACE);

		const bool updateArrays = m_renderState != rsTexRect;
		if (updateArrays || CombinerInfo::get().isChanged()) {
			m_renderState = rsTexRect;
			glDisableVertexAttribArray(SC_COLOR);
			_setTexCoordArrays();

			GLfloat alpha = 0.0f;
			if (currentCombiner()->usesShade()) {
				gDPCombine combine;
				combine.mux = currentCombiner()->getKey();
				if (combine.mA0 == G_ACMUX_0 && combine.aA0 == G_ACMUX_SHADE)
					alpha = 1.0f;
			}
			glVertexAttrib4f(SC_COLOR, 0, 0, 0, alpha);
		}

		if (updateArrays) {
#ifdef RENDERSTATE_TEST
			StateChanges++;
#endif
			glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
			glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s0);
			glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s1);
			glDisableVertexAttribArray(SC_NUMLIGHTS);
			glDisableVertexAttribArray(SC_MODIFY);
		}
		currentCombiner()->updateRenderState();

		if (_params.texrectCmd && texturedRectSpecial != nullptr && texturedRectSpecial(_params)) {
			gSP.changed |= CHANGED_GEOMETRYMODE;
			return;
		}

		if (!_canDraw())
			return;
	}

	ShaderCombiner * pCurrentCombiner = currentCombiner();
	const FrameBuffer * pCurrentBuffer = _params.pBuffer;
	OGLVideo & ogl = video();
	TextureCache & cache = textureCache();
	const bool bUseBilinear = (gDP.otherMode.textureFilter | (gSP.objRendermode&G_OBJRM_BILERP)) != 0;
	const bool bUseTexrectDrawer = config.generalEmulation.enableNativeResTexrects != 0
		&& bUseBilinear
		&& pCurrentCombiner->usesTexture()
		&& (pCurrentBuffer == nullptr || !pCurrentBuffer->m_cfb)
		&& (cache.current[0] != nullptr)
//		&& (cache.current[0] == nullptr || cache.current[0]->format == G_IM_FMT_RGBA || cache.current[0]->format == G_IM_FMT_CI)
		&& ((cache.current[0]->frameBufferTexture == CachedTexture::fbNone && !cache.current[0]->bHDTexture))
		&& (cache.current[1] == nullptr || (cache.current[1]->frameBufferTexture == CachedTexture::fbNone && !cache.current[1]->bHDTexture));

	const float scaleX = pCurrentBuffer != nullptr ? 1.0f / pCurrentBuffer->m_width : VI.rwidth;
	const float scaleY = pCurrentBuffer != nullptr ? 1.0f / pCurrentBuffer->m_height : VI.rheight;
	const float Z = (gDP.otherMode.depthSource == G_ZS_PRIM) ? gDP.primDepth.z : gSP.viewport.nearz;
	const float W = 1.0f;
	f32 uly, lry;
	if (bUseTexrectDrawer) {
		uly = (float)_params.uly * (2.0f * scaleY) - 1.0f;
		lry = (float)_params.lry * (2.0f * scaleY) - 1.0f;
	} else {
		uly = (float)_params.uly * (-2.0f * scaleY) + 1.0f;
		lry = (float)(_params.lry) * (-2.0f * scaleY) + 1.0f;
		// Flush text drawer
		if (m_texrectDrawer.draw())
			_updateStates(rsTexRect);
	}
	m_rect[0].x = (float)_params.ulx * (2.0f * scaleX) - 1.0f;
	m_rect[0].y = uly;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = (float)(_params.lrx) * (2.0f * scaleX) - 1.0f;
	m_rect[1].y = m_rect[0].y;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = m_rect[0].x;
	m_rect[2].y = lry;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = m_rect[1].x;
	m_rect[3].y = m_rect[2].y;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	struct
	{
		float s0, t0, s1, t1;
	} texST[2] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }; //struct for texture coordinates

	for (u32 t = 0; t < 2; ++t) {
		if (pCurrentCombiner->usesTile(t) && cache.current[t] && gSP.textureTile[t]) {
			f32 shiftScaleS = 1.0f;
			f32 shiftScaleT = 1.0f;
			getTextureShiftScale(t, cache, shiftScaleS, shiftScaleT);
			if (_params.uls > _params.lrs) {
				texST[t].s0 = (_params.uls + _params.dsdx) * shiftScaleS - gSP.textureTile[t]->fuls;
				texST[t].s1 = _params.lrs * shiftScaleS - gSP.textureTile[t]->fuls;
			} else {
				texST[t].s0 = _params.uls * shiftScaleS - gSP.textureTile[t]->fuls;
				texST[t].s1 = (_params.lrs + _params.dsdx) * shiftScaleS - gSP.textureTile[t]->fuls;
			}
			if (_params.ult > _params.lrt) {
				texST[t].t0 = (_params.ult + _params.dtdy) * shiftScaleT - gSP.textureTile[t]->fult;
				texST[t].t1 = _params.lrt * shiftScaleT - gSP.textureTile[t]->fult;
			} else {
				texST[t].t0 = _params.ult * shiftScaleT - gSP.textureTile[t]->fult;
				texST[t].t1 = (_params.lrt + _params.dtdy) * shiftScaleT - gSP.textureTile[t]->fult;
			}

			if (cache.current[t]->frameBufferTexture != CachedTexture::fbNone) {
				texST[t].s0 = cache.current[t]->offsetS + texST[t].s0;
				texST[t].t0 = cache.current[t]->offsetT - texST[t].t0;
				texST[t].s1 = cache.current[t]->offsetS + texST[t].s1;
				texST[t].t1 = cache.current[t]->offsetT - texST[t].t1;
			}

			glActiveTexture(GL_TEXTURE0 + t);

			if ((cache.current[t]->mirrorS == 0 && cache.current[t]->maskS == 0 &&
				(texST[t].s0 < texST[t].s1 ?
				texST[t].s0 >= 0.0 && texST[t].s1 <= (float)cache.current[t]->width :
				texST[t].s1 >= 0.0 && texST[t].s0 <= (float)cache.current[t]->width))
				|| (cache.current[t]->maskS == 0 && (texST[t].s0 < -1024.0f || texST[t].s1 > 1023.99f)))
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

			if (cache.current[t]->mirrorT == 0 &&
				(texST[t].t0 < texST[t].t1 ?
				texST[t].t0 >= 0.0f && texST[t].t1 <= (float)cache.current[t]->height :
				texST[t].t1 >= 0.0f && texST[t].t0 <= (float)cache.current[t]->height))
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			texST[t].s0 *= cache.current[t]->scaleS;
			texST[t].t0 *= cache.current[t]->scaleT;
			texST[t].s1 *= cache.current[t]->scaleS;
			texST[t].t1 *= cache.current[t]->scaleT;
		}
	}

	if (gDP.otherMode.cycleType == G_CYC_COPY) {
		glActiveTexture( GL_TEXTURE0 );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	m_rect[0].s0 = texST[0].s0;
	m_rect[0].t0 = texST[0].t0;
	m_rect[0].s1 = texST[1].s0;
	m_rect[0].t1 = texST[1].t0;

	m_rect[3].s0 = texST[0].s1;
	m_rect[3].t0 = texST[0].t1;
	m_rect[3].s1 = texST[1].s1;
	m_rect[3].t1 = texST[1].t1;

	if (_params.flip) {
		m_rect[1].s0 = texST[0].s0;
		m_rect[1].t0 = texST[0].t1;
		m_rect[1].s1 = texST[1].s0;
		m_rect[1].t1 = texST[1].t1;

		m_rect[2].s0 = texST[0].s1;
		m_rect[2].t0 = texST[0].t0;
		m_rect[2].s1 = texST[1].s1;
		m_rect[2].t1 = texST[1].t0;
	} else {
		m_rect[1].s0 = texST[0].s1;
		m_rect[1].t0 = texST[0].t0;
		m_rect[1].s1 = texST[1].s1;
		m_rect[1].t1 = texST[1].t0;

		m_rect[2].s0 = texST[0].s0;
		m_rect[2].t0 = texST[0].t1;
		m_rect[2].s1 = texST[1].s0;
		m_rect[2].t1 = texST[1].t1;
	}

	if (ogl.isAdjustScreen() &&
		(_params.forceAjustScale ||
		((gDP.colorImage.width > VI.width * 98 / 100) && (_params.lrx - _params.ulx < VI.width * 9 / 10))))
	{
		const float scale = ogl.getAdjustScale();
		for (u32 i = 0; i < 4; ++i)
			m_rect[i].x *= scale;
	}

	if (bUseTexrectDrawer)
		m_texrectDrawer.add();
	else {
		if (pCurrentBuffer == nullptr)
			glViewport(0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());
		else
			glViewport(0, 0, pCurrentBuffer->m_width*pCurrentBuffer->m_scaleX, pCurrentBuffer->m_height*pCurrentBuffer->m_scaleY);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
	}
}

void OGLRender::correctTexturedRectParams(TexturedRectParams & _params)
{
    if (config.generalEmulation.correctTexrectCoords == Config::tcSmart) {
        if (_params.ulx == m_texrectParams.ulx && _params.lrx == m_texrectParams.lrx) {
            if (fabsf(_params.uly - m_texrectParams.lry) < 0.51f)
                _params.uly = m_texrectParams.lry;
            else if (fabsf(_params.lry - m_texrectParams.uly) < 0.51f)
                _params.lry = m_texrectParams.uly;
        } else if (_params.uly == m_texrectParams.uly && _params.lry == m_texrectParams.lry) {
            if (fabsf(_params.ulx - m_texrectParams.lrx) < 0.51f)
                _params.ulx = m_texrectParams.lrx;
            else if (fabsf(_params.lrx - m_texrectParams.ulx) < 0.51f)
                _params.lrx = m_texrectParams.ulx;
        }
    } else if (config.generalEmulation.correctTexrectCoords == Config::tcForce) {
        _params.lrx += 0.25f;
        _params.lry += 0.25f;
    }

    m_texrectParams = _params;
}

void OGLRender::drawText(const char *_pText, float x, float y)
{
	m_renderState = rsNone;
	TextDrawer::get().renderText(_pText, x, y);
}

void OGLRender::clearDepthBuffer(u32 _uly, u32 _lry)
{
	if (!_canDraw())
		return;

	depthBufferList().clearBuffer(_uly, _lry);

	glDisable( GL_SCISSOR_TEST );

#ifdef ANDROID
	glDepthMask( FALSE );
	glClear( GL_DEPTH_BUFFER_BIT );
#endif

	glDepthMask( TRUE );
	glClear( GL_DEPTH_BUFFER_BIT );

	_updateDepthUpdate();

	glEnable( GL_SCISSOR_TEST );
}

void OGLRender::clearColorBuffer(float *_pColor )
{
	glDisable(GL_SCISSOR_TEST);

	if (_pColor != nullptr)
		glClearColor( _pColor[0], _pColor[1], _pColor[2], _pColor[3] );
	else
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	glEnable( GL_SCISSOR_TEST );
}

void OGLRender::_initExtensions()
{
	LOG(LOG_VERBOSE, "OpenGL version string: %s\n", glGetString(GL_VERSION));
	LOG(LOG_VERBOSE, "OpenGL vendor: %s\n", glGetString(GL_VENDOR));
	const GLubyte * strRenderer = glGetString(GL_RENDERER);
	if (strstr((const char*)strRenderer, "Adreno") != nullptr)
		m_oglRenderer = glrAdreno;
	LOG(LOG_VERBOSE, "OpenGL renderer: %s\n", strRenderer);

	fboFormats.init();

#ifndef GLES2
	GLint majorVersion = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	LOG(LOG_VERBOSE, "OpenGL major version: %d\n", majorVersion);
	assert(majorVersion >= 3 && "Plugin requires GL version 3 or higher.");
#endif

#ifdef GL_IMAGE_TEXTURES_SUPPORT
	GLint minorVersion = 0;
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	LOG(LOG_VERBOSE, "OpenGL minor version: %d\n", minorVersion);
#ifndef GLESX
	m_bImageTexture = (majorVersion >= 4) && (minorVersion >= 3) && (glBindImageTexture != nullptr);
#elif defined(GLES3_1)
	m_bImageTexture = (majorVersion >= 3) && (minorVersion >= 1) && (glBindImageTexture != nullptr);
#else
	m_bImageTexture = false;
#endif
#else
	m_bImageTexture = false;
#endif
	LOG(LOG_VERBOSE, "ImageTexture support: %s\n", m_bImageTexture ? "yes" : "no");
	if (!m_bImageTexture)
		LOG(LOG_WARNING, "N64 depth compare and depth based fog will not work without Image Textures support provided in OpenGL >= 4.3 or GLES >= 3.1\n");

	if (config.texture.maxAnisotropy != 0 && OGLVideo::isExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &config.texture.maxAnisotropyF);
		config.texture.maxAnisotropyF = min(config.texture.maxAnisotropyF, (f32)config.texture.maxAnisotropy);
	} else
		config.texture.maxAnisotropyF = 0.0f;
	LOG(LOG_VERBOSE, "Max Anisotropy: %f\n", config.texture.maxAnisotropyF);
}

void OGLRender::_initStates()
{
	glDisable(GL_CULL_FACE);
	glEnableVertexAttribArray(SC_POSITION);
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_ALWAYS );
	glDepthMask( GL_FALSE );
	glEnable( GL_SCISSOR_TEST );

	if (config.frameBufferEmulation.N64DepthCompare != 0) {
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_POLYGON_OFFSET_FILL );
		glDepthFunc( GL_ALWAYS );
		glDepthMask( FALSE );
	} else {
#ifdef ANDROID
		if(config.generalEmulation.forcePolygonOffset != 0)
			glPolygonOffset(config.generalEmulation.polygonOffsetFactor, config.generalEmulation.polygonOffsetUnits);
		else
#endif
			glPolygonOffset(-3.0f, -3.0f);
	}

	OGLVideo & ogl = video();
	glViewport(0, ogl.getHeightOffset(), ogl.getScreenWidth(), ogl.getScreenHeight());

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	srand( time(nullptr) );

	ogl.swapBuffers();
}

void OGLRender::_initData()
{
	glState.reset();
	_initExtensions();
	_initStates();
	_setSpecialTexrect();

	textureCache().init();
	DepthBuffer_Init();
	FrameBuffer_Init();
	Combiner_Init();
	TextDrawer::get().init();
	TFH.init();
	PostProcessor::get().init();
	FBInfo::fbInfo.reset();
	m_texrectDrawer.init();
	m_renderState = rsNone;

	gSP.changed = gDP.changed = 0xFFFFFFFF;

	memset(triangles.vertices, 0, VERTBUFF_SIZE * sizeof(SPVertex));
	memset(triangles.elements, 0, ELEMBUFF_SIZE * sizeof(GLubyte));
	for (u32 i = 0; i < VERTBUFF_SIZE; ++i)
		triangles.vertices[i].w = 1.0f;
	triangles.num = 0;

#ifdef NO_BLIT_BUFFER_COPY
	m_programCopyTex = createShaderProgram(strTexrectDrawerVertexShader, strTextureCopyShader);
	glUseProgram(m_programCopyTex);
	GLint loc = glGetUniformLocation(m_programCopyTex, "uTex0");
	assert(loc >= 0);
	glUniform1i(loc, 0);
	glUseProgram(0);
#endif
}

void OGLRender::_destroyData()
{
#ifdef NO_BLIT_BUFFER_COPY
	glDeleteProgram(m_programCopyTex);
	m_programCopyTex = 0;
#endif

	m_renderState = rsNone;
	m_texrectDrawer.destroy();
	if (config.bloomFilter.enable != 0)
		PostProcessor::get().destroy();
	if (TFH.optionsChanged())
		TFH.shutdown();
	TextDrawer::get().destroy();
	Combiner_Destroy();
	FrameBuffer_Destroy();
	DepthBuffer_Destroy();
	textureCache().destroy();
}

void OGLRender::_setSpecialTexrect() const
{
	const char * name = RSP.romname;
	if (strstr(name, (const char *)"Beetle") || strstr(name, (const char *)"BEETLE") || strstr(name, (const char *)"HSV")
		|| strstr(name, (const char *)"DUCK DODGERS") || strstr(name, (const char *)"DAFFY DUCK"))
		texturedRectSpecial = texturedRectShadowMap;
	else if (strstr(name, (const char *)"Perfect Dark") || strstr(name, (const char *)"PERFECT DARK"))
		texturedRectSpecial = texturedRectDepthBufferCopy; // See comments to that function!
	else if (strstr(name, (const char *)"CONKER BFD"))
		texturedRectSpecial = texturedRectCopyToItself;
	else if (strstr(name, (const char *)"YOSHI STORY"))
		texturedRectSpecial = texturedRectBGCopy;
	else if (strstr(name, (const char *)"PAPER MARIO") || strstr(name, (const char *)"MARIO STORY"))
		texturedRectSpecial = texturedRectPaletteMod;
	else if (strstr(name, (const char *)"ZELDA"))
		texturedRectSpecial = texturedRectMonochromeBackground;
	else
		texturedRectSpecial = nullptr;
}

#ifdef NO_BLIT_BUFFER_COPY
void OGLRender::copyTexturedRect(GLint _srcX0, GLint _srcY0, GLint _srcX1, GLint _srcY1,
								 GLuint _srcWidth, GLuint _srcHeight, GLuint _srcTex,
								 GLint _dstX0, GLint _dstY0, GLint _dstX1, GLint _dstY1,
								 GLuint _dstWidth, GLuint _dstHeight, GLenum _filter)
{
	glDisableVertexAttribArray(SC_COLOR);
	glDisableVertexAttribArray(SC_TEXCOORD1);
	glDisableVertexAttribArray(SC_NUMLIGHTS);
	glDisableVertexAttribArray(SC_MODIFY);
	glEnableVertexAttribArray(SC_TEXCOORD0);
	glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].x);
	glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &m_rect[0].s0);
	glUseProgram(m_programCopyTex);

	const float scaleX = 1.0f / _dstWidth;
	const float scaleY = 1.0f / _dstHeight;
	const float X0 = _dstX0 * (2.0f * scaleX) - 1.0f;
	const float Y0 = _dstY0 * (2.0f * scaleY) - 1.0f;
	const float X1 = _dstX1 * (2.0f * scaleX) - 1.0f;
	const float Y1 = _dstY1 * (2.0f * scaleY) - 1.0f;
	const float Z = 1.0f;
	const float W = 1.0f;

	m_rect[0].x = X0;
	m_rect[0].y = Y0;
	m_rect[0].z = Z;
	m_rect[0].w = W;
	m_rect[1].x = X1;
	m_rect[1].y = Y0;
	m_rect[1].z = Z;
	m_rect[1].w = W;
	m_rect[2].x = X0;
	m_rect[2].y = Y1;
	m_rect[2].z = Z;
	m_rect[2].w = W;
	m_rect[3].x = X1;
	m_rect[3].y = Y1;
	m_rect[3].z = Z;
	m_rect[3].w = W;

	const float scaleS = 1.0f / _srcWidth;
	const float scaleT = 1.0f / _srcHeight;

	const float S0 = _srcX0 * scaleS;
	const float S1 = _srcX1 * scaleS;
	const float T0 = _srcY0 * scaleT;
	const float T1 = _srcY1 * scaleT;

	m_rect[0].s0 = S0;
	m_rect[0].t0 = T0;
	m_rect[1].s0 = S1;
	m_rect[1].t0 = T0;
	m_rect[2].s0 = S0;
	m_rect[2].t0 = T1;
	m_rect[3].s0 = S1;
	m_rect[3].t0 = T1;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _srcTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glViewport(0, 0, _dstWidth, _dstHeight);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(FALSE);
	glDisable(GL_SCISSOR_TEST);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_SCISSOR_TEST);

	gSP.changed |= CHANGED_GEOMETRYMODE | CHANGED_VIEWPORT;
	gDP.changed |= CHANGED_RENDERMODE | CHANGED_TEXTURE;
	m_renderState = rsNone;
}
#else
void OGLRender::copyTexturedRect(GLint _srcX0, GLint _srcY0, GLint _srcX1, GLint _srcY1,
								 GLuint _srcWidth, GLuint _srcHeight, GLuint _srcTex,
								 GLint _dstX0, GLint _dstY0, GLint _dstX1, GLint _dstY1,
								 GLuint _dstWidth, GLuint _dstHeight, GLenum _filter)
{
	glDisable(GL_SCISSOR_TEST);
	glBlitFramebuffer(
		_srcX0, _srcY0, _srcX1, _srcY1,
		_dstX0, _dstY0, _dstX1, _dstY1,
		GL_COLOR_BUFFER_BIT, _filter
		);
	glEnable(GL_SCISSOR_TEST);
}
#endif
