#include <Config.h>
#include <CRC.h>
#include <memory>
#include <algorithm>
#include "GLFunctions.h"
#include "opengl_Attributes.h"
#include "opengl_BufferedDrawer.h"
#include "opengl_Wrapper.h"

using namespace graphics;
using namespace opengl;

const u32 BufferedDrawer::m_bufMaxSize = 4*1024*1024;
#ifndef GL_DEBUG
const GLbitfield BufferedDrawer::m_bufAccessBits = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
const GLbitfield BufferedDrawer::m_bufMapBits = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
#else
const GLbitfield BufferedDrawer::m_bufAccessBits = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
const GLbitfield BufferedDrawer::m_bufMapBits = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
#endif

BufferedDrawer::BufferedDrawer(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray, CachedBindBuffer * _bindBuffer)
: m_glInfo(_glinfo)
, m_cachedAttribArray(_cachedAttribArray)
, m_bindBuffer(_bindBuffer)
{
	m_vertices.resize(VERTBUFF_SIZE);
	/* Init buffers for rects */
	FunctionWrapper::glGenVertexArrays(1, &m_rectsBuffers.vao);
	FunctionWrapper::glBindVertexArray(m_rectsBuffers.vao);
	_initBuffer(m_rectsBuffers.vbo, m_bufMaxSize);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, true);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, true);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, true);

	FunctionWrapper::glVertexAttribPointerBuffered(rectAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(RectVertex), offsetof(RectVertex, x));
	FunctionWrapper::glVertexAttribPointerBuffered(rectAttrib::texcoord0, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), offsetof(RectVertex, s0));
	FunctionWrapper::glVertexAttribPointerBuffered(rectAttrib::texcoord1, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), offsetof(RectVertex, s1));

	/* Init buffers for triangles */
	FunctionWrapper::glGenVertexArrays(1, &m_trisBuffers.vao);
	FunctionWrapper::glBindVertexArray(m_trisBuffers.vao);
	_initBuffer(m_trisBuffers.vbo, m_bufMaxSize);
	_initBuffer(m_trisBuffers.ebo, m_bufMaxSize);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	FunctionWrapper::glVertexAttribPointerBuffered(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, x));
	FunctionWrapper::glVertexAttribPointerBuffered(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, r));
	FunctionWrapper::glVertexAttribPointerBuffered(triangleAttrib::texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, s));
	FunctionWrapper::glVertexAttribPointerBuffered(triangleAttrib::modify, 4, GL_BYTE, GL_TRUE, sizeof(Vertex), offsetof(Vertex, modify));
}

void BufferedDrawer::_initBuffer(Buffer & _buffer, GLuint _bufSize)
{
	_buffer.size = _bufSize;
	FunctionWrapper::glGenBuffers(1, &_buffer.handle);
	m_bindBuffer->bind(Parameter(_buffer.type), ObjectHandle(_buffer.handle));
	if (m_glInfo.bufferStorage) {
		FunctionWrapper::glBufferStorage(_buffer.type, _bufSize, std::move(std::unique_ptr<u8[]>(nullptr)), m_bufAccessBits);
		_buffer.data = (GLubyte*)FunctionWrapper::glMapBufferRange(_buffer.type, 0, _bufSize, m_bufMapBits);
	} else {
		FunctionWrapper::glBufferData(_buffer.type, _bufSize, std::move(std::unique_ptr<u8[]>(nullptr)), GL_DYNAMIC_DRAW);
	}
}

BufferedDrawer::~BufferedDrawer()
{
	m_bindBuffer->bind(Parameter(GL_ARRAY_BUFFER), ObjectHandle::null);
	m_bindBuffer->bind(Parameter(GL_ELEMENT_ARRAY_BUFFER), ObjectHandle::null);

	int numDeleteBuffers = 3;
	std::unique_ptr<GLuint[]> buffers(new GLuint[numDeleteBuffers]);
	buffers[0] = m_rectsBuffers.vbo.handle;
	buffers[1] = m_trisBuffers.vbo.handle;
	buffers[2] = m_trisBuffers.ebo.handle;

	FunctionWrapper::glDeleteBuffers(numDeleteBuffers, std::move(buffers));
	FunctionWrapper::glBindVertexArray(0);

	int numVertexBuffers = 2;
	auto vertexBuffers = std::unique_ptr<GLuint[]>(new GLuint[numVertexBuffers]);
	vertexBuffers[0] = m_rectsBuffers.vao;
	vertexBuffers[1] = m_trisBuffers.vao;
	FunctionWrapper::glDeleteVertexArrays(numVertexBuffers, std::move(vertexBuffers));
}

void BufferedDrawer::_updateBuffer(Buffer & _buffer, u32 _count, u32 _dataSize, const void * _data)
{
	if (_buffer.offset + _dataSize >= _buffer.size) {
		_buffer.offset = 0;
		_buffer.pos = 0;
	}

	if (m_glInfo.bufferStorage) {
		memcpy(&_buffer.data[_buffer.offset], _data, _dataSize);
#ifdef GL_DEBUG
		m_bindBuffer->bind(Parameter(_buffer.type), ObjectHandle(_buffer.handle));
		FunctionWrapper::glFlushMappedBufferRange(_buffer.type, _buffer.offset, _dataSize);
#endif
	} else {
		std::unique_ptr<u8[]> data((new u8[_dataSize]));
		std::copy_n(reinterpret_cast<const u8*>(_data), _dataSize, data.get());
		FunctionWrapper::glMapBufferRangeWriteAsync(_buffer.type, _buffer.handle, _buffer.offset, _dataSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT, std::move(data));
	}

	_buffer.offset += _dataSize;
	_buffer.pos += _count;
}

void BufferedDrawer::_updateRectBuffer(const graphics::Context::DrawRectParameters & _params)
{
	const BuffersType type = BuffersType::rects;
	if (m_type != type) {
		FunctionWrapper::glBindVertexArray(m_rectsBuffers.vao);
		m_type = type;
	}

	Buffer & buffer = m_rectsBuffers.vbo;
	const u32 dataSize = _params.verticesCount * static_cast<u32>(sizeof(RectVertex));

	if (m_glInfo.bufferStorage) {
		_updateBuffer(buffer, _params.verticesCount, dataSize, _params.vertices);
		return;
	}

	const u32 crc = CRC_Calculate(0xFFFFFFFF, _params.vertices, dataSize);
	auto iter = m_rectBufferOffsets.find(crc);
	if (iter != m_rectBufferOffsets.end()) {
		buffer.pos = iter->second;
		return;
	}

	const GLintptr prevOffset = buffer.offset;
	_updateBuffer(buffer, _params.verticesCount, dataSize, _params.vertices);
	if (buffer.offset < prevOffset)
		m_rectBufferOffsets.clear();

	buffer.pos = static_cast<GLint>(buffer.offset / sizeof(RectVertex));
	m_rectBufferOffsets[crc] = buffer.pos;
}


void BufferedDrawer::drawRects(const graphics::Context::DrawRectParameters & _params)
{
	_updateRectBuffer(_params);

	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, _params.texrect);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, _params.texrect);

	FunctionWrapper::glDrawArrays(GLenum(_params.mode), m_rectsBuffers.vbo.pos - _params.verticesCount, _params.verticesCount);
}

void BufferedDrawer::_convertFromSPVertex(bool _flatColors, u32 _count, const SPVertex * _data)
{
	if (_count > m_vertices.size())
		m_vertices.resize(_count);

	for (u32 i = 0; i < _count; ++i) {
		const SPVertex & src = _data[i];
		Vertex & dst = m_vertices[i];
		dst.x = src.x;
		dst.y = src.y;
		dst.z = src.z;
		dst.w = src.w;
		if (_flatColors) {
			dst.r = src.flat_r;
			dst.g = src.flat_g;
			dst.b = src.flat_b;
			dst.a = src.flat_a;
		} else {
			dst.r = src.r;
			dst.g = src.g;
			dst.b = src.b;
			dst.a = src.a;
		}
		dst.s = src.s;
		dst.t = src.t;
		dst.modify = src.modify;
	}
}

void BufferedDrawer::_updateTrianglesBuffers(const graphics::Context::DrawTriangleParameters & _params)
{
	const BuffersType type = BuffersType::triangles;

	if (m_type != type) {
		FunctionWrapper::glBindVertexArray(m_trisBuffers.vao);
		m_type = type;
	}

	_convertFromSPVertex(_params.flatColors, _params.verticesCount, _params.vertices);
	const u32 vboDataSize = _params.verticesCount * static_cast<u32>(sizeof(Vertex));
	Buffer & vboBuffer = m_trisBuffers.vbo;
	_updateBuffer(vboBuffer, _params.verticesCount, vboDataSize, m_vertices.data());

	if (_params.elements == nullptr)
		return;

	const u32 eboDataSize = static_cast<u32>(sizeof(GLushort)) * _params.elementsCount;
	Buffer & eboBuffer = m_trisBuffers.ebo;
	_updateBuffer(eboBuffer, _params.elementsCount, eboDataSize, _params.elements);
}

void BufferedDrawer::drawTriangles(const graphics::Context::DrawTriangleParameters & _params)
{
	_updateTrianglesBuffers(_params);

	if (isHWLightingAllowed())
		FunctionWrapper::glVertexAttrib1f(triangleAttrib::numlights, GLfloat(_params.vertices[0].HWLight));

	if (_params.elements == nullptr) {
		FunctionWrapper::glDrawArrays(GLenum(_params.mode), m_trisBuffers.vbo.pos - _params.verticesCount, _params.verticesCount);
		return;
	}

	u16* indices = (u16*)nullptr + m_trisBuffers.ebo.pos - _params.elementsCount;
	FunctionWrapper::glDrawRangeElementsBaseVertex(GLenum(_params.mode), 0, _params.verticesCount - 1, _params.elementsCount, GL_UNSIGNED_SHORT,
		indices, m_trisBuffers.vbo.pos - _params.verticesCount);
}

void BufferedDrawer::drawLine(f32 _width, SPVertex * _vertices)
{
	const BuffersType type = BuffersType::triangles;

	if (m_type != type) {
		FunctionWrapper::glBindVertexArray(m_trisBuffers.vao);
		m_type = type;
	}

	_convertFromSPVertex(false, 2, _vertices);
	const GLsizeiptr vboDataSize = 2 * sizeof(Vertex);
	Buffer & vboBuffer = m_trisBuffers.vbo;
	_updateBuffer(vboBuffer, 2, vboDataSize, m_vertices.data());

	FunctionWrapper::glLineWidth(_width);
	FunctionWrapper::glDrawArrays(GL_LINES, m_trisBuffers.vbo.pos - 2, 2);
}
