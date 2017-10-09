#include <Config.h>
#include "GLFunctions.h"
#include "opengl_Attributes.h"
#include "opengl_CachedFunctions.h"
#include "opengl_UnbufferedDrawerThreadSafe.h"
#include "opengl_Wrapper.h"
#include <algorithm>

using namespace opengl;

UnbufferedDrawerThreadSafe::UnbufferedDrawerThreadSafe(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray)
: m_glInfo(_glinfo)
, m_cachedAttribArray(_cachedAttribArray)
{
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	m_attribsData.fill(nullptr);
}

UnbufferedDrawerThreadSafe::~UnbufferedDrawerThreadSafe()
{
}

bool UnbufferedDrawerThreadSafe::_updateAttribPointer(u32 _index, const void * _ptr)
{
	if (m_attribsData[_index] == _ptr)
		return false;

	m_attribsData[_index] = _ptr;
	return true;
}

void UnbufferedDrawerThreadSafe::drawTriangles(const graphics::Context::DrawTriangleParameters & _params)
{
	const char* verticesRawData = reinterpret_cast<char*>(_params.vertices);
	auto verticesCopy = std::unique_ptr<std::vector<char>>(new std::vector<char>(verticesRawData,
		verticesRawData + _params.verticesCount*sizeof(SPVertex)));

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);

		const void * ptr = &_params.vertices->x;
		if (_updateAttribPointer(triangleAttrib::position, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE,
				sizeof(SPVertex), offsetof(SPVertex, x));
		}
	}

	if (_params.combiner->usesShade()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
		std::size_t offset = _params.flatColors ? offsetof(SPVertex, flat_r) : offsetof(SPVertex, r);
		const void * ptr = _params.flatColors ? &_params.vertices->flat_r : &_params.vertices->r;
		if (_updateAttribPointer(triangleAttrib::color, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE,
				sizeof(SPVertex), offset);
		}
	}
	else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);

	if (_params.combiner->usesTexture()) {
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, true);
		const void * ptr = &_params.vertices->s;
		if (_updateAttribPointer(triangleAttrib::texcoord, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(triangleAttrib::texcoord, 2, GL_FLOAT, GL_FALSE,
				sizeof(SPVertex), offsetof(SPVertex, s));
		}
	} else
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, true);
		const void * ptr = &_params.vertices->modify;
		if (_updateAttribPointer(triangleAttrib::modify, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(triangleAttrib::modify, 4, GL_BYTE, GL_FALSE,
				sizeof(SPVertex), offsetof(SPVertex, modify));
		}
	}

	if (isHWLightingAllowed())
		FunctionWrapper::glVertexAttrib1f(triangleAttrib::numlights, GLfloat(_params.vertices[0].HWLight));

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	if (_params.elements == nullptr) {
		FunctionWrapper::glDrawArraysUnbuffered(GLenum(_params.mode), 0, _params.verticesCount, std::move(verticesCopy));
		return;
	}

	std::unique_ptr<u16[]> elementsCopy(new u16[_params.elementsCount]);
	std::copy_n(reinterpret_cast<u16*>(_params.elements), _params.elementsCount, elementsCopy.get());

	FunctionWrapper::glDrawElementsUnbuffered(GLenum(_params.mode), _params.elementsCount, GL_UNSIGNED_SHORT,
		std::move(elementsCopy), std::move(verticesCopy));
}

void UnbufferedDrawerThreadSafe::drawRects(const graphics::Context::DrawRectParameters & _params)
{
	const char* verticesRawData = reinterpret_cast<char*>(_params.vertices);
	auto verticesCopy = std::unique_ptr<std::vector<char>>(new std::vector<char>(verticesRawData,
		verticesRawData + _params.verticesCount*sizeof(RectVertex)));

	{
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, true);
		const void * ptr = &_params.vertices->x;
		if (_updateAttribPointer(rectAttrib::position, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(rectAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(RectVertex),
				offsetof(RectVertex, x));
		}
	}

	if (_params.texrect && _params.combiner->usesTile(0)) {
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, true);
		const void * ptr = &_params.vertices->s0;
		if (_updateAttribPointer(rectAttrib::texcoord0, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(rectAttrib::texcoord0, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex),
				offsetof(RectVertex, s0));
		}
	} else
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);

	if (_params.texrect && _params.combiner->usesTile(1)) {
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, true);
		const void * ptr = &_params.vertices->s1;
		if (_updateAttribPointer(rectAttrib::texcoord1, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(rectAttrib::texcoord1, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex),
				offsetof(RectVertex, s1));
		}
	} else
		m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	FunctionWrapper::glDrawArraysUnbuffered(GLenum(_params.mode), 0, _params.verticesCount, std::move(verticesCopy));
}

void UnbufferedDrawerThreadSafe::drawLine(f32 _width, SPVertex * _vertices)
{
	const char* verticesRawData = reinterpret_cast<char*>(_vertices);
	auto verticesCopy = std::unique_ptr<std::vector<char>>(new std::vector<char>(verticesRawData,
		verticesRawData + 2*sizeof(SPVertex)));

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
		const void * ptr = &_vertices->x;
		if (_updateAttribPointer(triangleAttrib::position, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex),
				offsetof(SPVertex, x));
		}
	}

	{
		m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
		const void * ptr = &_vertices->r;
		if (_updateAttribPointer(triangleAttrib::color, ptr)) {
			FunctionWrapper::glVertexAttribPointerUnbuffered(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex),
				offsetof(SPVertex, r));
		}
	}

	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, false);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, false);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, false);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, false);

	FunctionWrapper::glLineWidth(_width);
	FunctionWrapper::glDrawArraysUnbuffered(GL_LINES, 0, 2, std::move(verticesCopy));
}
