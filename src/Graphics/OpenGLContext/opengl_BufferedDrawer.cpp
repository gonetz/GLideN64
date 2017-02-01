#include <Graphics/Context.h>
#include <Config.h>
#include "GLFunctions.h"
#include "opengl_Attributes.h"
#include "opengl_CachedFunctions.h"
#include "opengl_BufferedDrawer.h"

using namespace graphics;
using namespace opengl;

BufferedDrawer::BufferedDrawer(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray, CachedBindBuffer * _bindBuffer)
: m_glInfo(_glinfo)
, m_cachedAttribArray(_cachedAttribArray)
, m_bindBuffer(_bindBuffer)
{
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	int fake;
	m_attribsData.fill(&fake);
	m_bufMaxSize = 4194304;
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glGenBuffers(BO_COUNT, m_bufObj);
	for (u32 i = 0; i < BO_COUNT; ++i) {
		if (i == IBO) {
			m_bufType[i] = GL_ELEMENT_ARRAY_BUFFER;
			m_bufFormatSize[i] = sizeof(GLubyte);
		} else if (i == TRI_VBO) {
			m_bufType[i] = GL_ARRAY_BUFFER;
			m_bufFormatSize[i] = sizeof(SPVertex);
		} else if (i == RECT_VBO) {
			m_bufType[i] = GL_ARRAY_BUFFER;
			m_bufFormatSize[i] = sizeof(RectVertex);
		}
		m_bindBuffer->bind(Parameter(m_bufType[i]), ObjectHandle(m_bufObj[i]));
		if (m_glInfo.bufferStorage) {
			glBufferStorage(m_bufType[i], m_bufMaxSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
			m_bufData[i] = (char*)glMapBufferRange(m_bufType[i], 0, m_bufMaxSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		}
		else
			glBufferData(m_bufType[i], m_bufMaxSize, nullptr, GL_DYNAMIC_DRAW);
		m_bufOffset[i] = 0;
	}
}

BufferedDrawer::~BufferedDrawer()
{
	m_bindBuffer->bind(Parameter(GL_ARRAY_BUFFER), ObjectHandle());
	m_bindBuffer->bind(Parameter(GL_ELEMENT_ARRAY_BUFFER), ObjectHandle());
	glDeleteBuffers(BO_COUNT, m_bufObj);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &m_vao);
}

bool BufferedDrawer::_updateAttribPointer(u32 _type, u32 _index, const void * _ptr)
{
	if (m_attribsData[_index] == _ptr)
		return false;

	m_bindBuffer->bind(Parameter(m_bufType[_type]), ObjectHandle(m_bufObj[_type]));
	m_attribsData[_index] = _ptr;
	return true;
}

void BufferedDrawer::_updateBuffer(u32 _type, u32 _length, const void * _data)
{
	if (m_bufOffset[_type] * m_bufFormatSize[_type] + _length > m_bufMaxSize)
		m_bufOffset[_type] = 0;
	if (m_glInfo.bufferStorage) {
		memcpy(&m_bufData[_type][m_bufOffset[_type] * m_bufFormatSize[_type]], _data, _length);
	} else {
		m_bindBuffer->bind(Parameter(m_bufType[_type]), ObjectHandle(m_bufObj[_type]));
		void* buffer_pointer = glMapBufferRange(m_bufType[_type], m_bufOffset[_type] * m_bufFormatSize[_type], _length, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		memcpy(buffer_pointer, _data, _length);
		glUnmapBuffer(m_bufType[_type]);
	}
}

void BufferedDrawer::drawTriangles(const graphics::Context::DrawTriangleParameters & _params)
{
	_updateBuffer(TRI_VBO, sizeof(SPVertex) * _params.verticesCount, _params.vertices);

	if (m_glInfo.imageTextures && config.frameBufferEmulation.N64DepthCompare != 0)
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
		const void * ptr = (const void *)offsetof(SPVertex, x);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::position, ptr))
			glVertexAttribPointer(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}

	if (_params.combiner->usesShade()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
		const void * ptr = _params.flatColors ? (const void *)offsetof(SPVertex, flat_r) : (const void *)offsetof(SPVertex, r);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::color, ptr))
			glVertexAttribPointer(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}
	else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);

	if (_params.combiner->usesTexture()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, true);
		const void * ptr = (const void *)offsetof(SPVertex, s);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::texcoord, ptr))
			glVertexAttribPointer(triangleAttrib::texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);

	if (_params.combiner->usesHwLighting()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, true);
		const void * ptr = (const void *)offsetof(SPVertex, HWLight);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::numlights, ptr))
			glVertexAttribPointer(triangleAttrib::numlights, 1, GL_BYTE, GL_FALSE, sizeof(SPVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, true);
		const void * ptr = (const void *)offsetof(SPVertex, modify);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::modify, ptr))
			glVertexAttribPointer(triangleAttrib::modify, 4, GL_BYTE, GL_FALSE, sizeof(SPVertex), ptr);
	}

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	if (_params.elements == nullptr) {
		glDrawArrays(GLenum(_params.mode), m_bufOffset[TRI_VBO], _params.verticesCount);
		m_bufOffset[TRI_VBO] += _params.verticesCount;
		return;
	}

	_updateBuffer(IBO, sizeof(GLubyte) * _params.elementsCount, _params.elements);
	glDrawElementsBaseVertex(GLenum(_params.mode), _params.elementsCount, GL_UNSIGNED_BYTE, (char*)nullptr + (m_bufOffset[IBO] * m_bufFormatSize[IBO]), m_bufOffset[TRI_VBO]);
	m_bufOffset[TRI_VBO] += _params.verticesCount;
	m_bufOffset[IBO] += _params.elementsCount;
}

void BufferedDrawer::drawRects(const graphics::Context::DrawRectParameters & _params)
{
	_updateBuffer(RECT_VBO, sizeof(RectVertex) * _params.verticesCount, _params.vertices);

	{
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, true);
		const void * ptr = (const void *)offsetof(RectVertex, x);
		if (_updateAttribPointer(RECT_VBO, rectAttrib::position, ptr))
			glVertexAttribPointer(rectAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(RectVertex), ptr);
	}

	glVertexAttrib4fv(rectAttrib::color, _params.rectColor.data());

	if (_params.combiner->usesTile(0)) {
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, true);
		const void * ptr = (const void *)offsetof(RectVertex, s0);
		if (_updateAttribPointer(RECT_VBO, rectAttrib::texcoord0, ptr))
			glVertexAttribPointer(rectAttrib::texcoord0, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);

	if (_params.combiner->usesTile(1)) {
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, true);
		const void * ptr = (const void *)offsetof(RectVertex, s1);
		if (_updateAttribPointer(RECT_VBO, rectAttrib::texcoord1, ptr))
			glVertexAttribPointer(rectAttrib::texcoord1, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), ptr);
	} else
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	glDrawArrays(GLenum(_params.mode), m_bufOffset[RECT_VBO], _params.verticesCount);
	m_bufOffset[RECT_VBO] += _params.verticesCount;
}

void BufferedDrawer::drawLine(f32 _width, SPVertex * _vertices)
{
	_updateBuffer(TRI_VBO, sizeof(SPVertex) * 2, _vertices);

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
		const void * ptr = (const void *)offsetof(SPVertex, x);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::position, ptr))
			glVertexAttribPointer(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
		const void * ptr = (const void *)offsetof(SPVertex, r);
		if (_updateAttribPointer(TRI_VBO, triangleAttrib::color, ptr))
			glVertexAttribPointer(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), ptr);
	}

	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	glLineWidth(_width);
	glDrawArrays(GL_LINES, m_bufOffset[TRI_VBO], 2);
	m_bufOffset[TRI_VBO] += 2;
}
