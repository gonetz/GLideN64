#include <Config.h>
#include <CRC.h>
#include "GLFunctions.h"
#include "opengl_Attributes.h"
#include "opengl_BufferedDrawer.h"

using namespace graphics;
using namespace opengl;

const u32 BufferedDrawer::m_bufMaxSize = 4194304;

BufferedDrawer::BufferedDrawer(const GLInfo & _glinfo, CachedVertexAttribArray * _cachedAttribArray, CachedBindBuffer * _bindBuffer)
: m_glInfo(_glinfo)
, m_cachedAttribArray(_cachedAttribArray)
, m_bindBuffer(_bindBuffer)
{
	/* Init buffers for rects */
	glGenVertexArrays(1, &m_rectsBuffers.vao);
	glBindVertexArray(m_rectsBuffers.vao);
	_initBuffer(m_rectsBuffers.vbo, m_bufMaxSize);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::position, true);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord0, true);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::texcoord1, true);
	m_cachedAttribArray->enableVertexAttribArray(rectAttrib::color, false);
	glVertexAttribPointer(rectAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(RectVertex), (const GLvoid *)(offsetof(RectVertex, x)));
	glVertexAttribPointer(rectAttrib::texcoord0, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), (const GLvoid *)(offsetof(RectVertex, s0)));
	glVertexAttribPointer(rectAttrib::texcoord1, 2, GL_FLOAT, GL_FALSE, sizeof(RectVertex), (const GLvoid *)(offsetof(RectVertex, s1)));

	/* Init buffers for triangles */
	glGenVertexArrays(1, &m_trisBuffers.vao);
	glBindVertexArray(m_trisBuffers.vao);
	_initBuffer(m_trisBuffers.vbo, m_bufMaxSize);
	_initBuffer(m_trisBuffers.ebo, m_bufMaxSize);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::position, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::color, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::texcoord, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::modify, true);
	m_cachedAttribArray->enableVertexAttribArray(triangleAttrib::numlights, false);
	glVertexAttribPointer(triangleAttrib::position, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)(offsetof(Vertex, x)));
	glVertexAttribPointer(triangleAttrib::color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)(offsetof(Vertex, r)));
	glVertexAttribPointer(triangleAttrib::texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)(offsetof(Vertex, s)));
	//glVertexAttribPointer(SC_NUMLIGHTS, 1, GL_BYTE, GL_FALSE, sizeof(Vertex), (const GLvoid *)(offsetof(Vertex, HWLight)));
	glVertexAttribPointer(triangleAttrib::modify, 4, GL_BYTE, GL_TRUE, sizeof(Vertex), (const GLvoid *)(offsetof(Vertex, modify)));
}

void BufferedDrawer::_initBuffer(Buffer & _buffer, GLuint _bufSize)
{
	_buffer.size = _bufSize;
	glGenBuffers(1, &_buffer.handle);
	m_bindBuffer->bind(Parameter(_buffer.type), ObjectHandle(_buffer.handle));
	if (m_glInfo.bufferStorage) {
		glBufferStorage(_buffer.type, _bufSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		_buffer.data = (GLubyte*)glMapBufferRange(_buffer.type, 0, _bufSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	} else {
		glBufferData(_buffer.type, _bufSize, nullptr, GL_DYNAMIC_DRAW);
	}
}

BufferedDrawer::~BufferedDrawer()
{
	m_bindBuffer->bind(Parameter(GL_ARRAY_BUFFER), ObjectHandle());
	m_bindBuffer->bind(Parameter(GL_ELEMENT_ARRAY_BUFFER), ObjectHandle());
	GLuint buffers[3] = { m_rectsBuffers.vbo.handle, m_trisBuffers.vbo.handle, m_trisBuffers.ebo.handle };
	glDeleteBuffers(3, buffers);
	glBindVertexArray(0);
	GLuint arrays[2] = { m_rectsBuffers.vao, m_trisBuffers.vao };
	glDeleteVertexArrays(2, arrays);
}

void BufferedDrawer::_updateBuffer(Buffer & _buffer, u32 _dataSize, const void * _data)
{
	if (_buffer.offset + _dataSize > _buffer.size) {
		_buffer.offset = 0;
		_buffer.pos = 0;
	}

	if (m_glInfo.bufferStorage) {
		memcpy(&_buffer.data[_buffer.offset], _data, _dataSize);
	}
	else {
		m_bindBuffer->bind(Parameter(GL_ARRAY_BUFFER), ObjectHandle(_buffer.handle));
		void* buffer_pointer = glMapBufferRange(GL_ARRAY_BUFFER, _buffer.offset, _dataSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		memcpy(buffer_pointer, _data, _dataSize);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
}

void BufferedDrawer::_updateRectBuffer(const graphics::Context::DrawRectParameters & _params)
{
	const BuffersType type = BuffersType::rects;
	if (m_type != type) {
		glBindVertexArray(m_rectsBuffers.vao);
		m_type = type;
	}

	Buffer & buffer = m_rectsBuffers.vbo;
	const size_t dataSize = _params.verticesCount * sizeof(RectVertex);
	const u32 crc = CRC_Calculate(0xFFFFFFFF, _params.vertices, dataSize);

	auto iter = m_rectBufferOffsets.find(crc);
	if (iter != m_rectBufferOffsets.end()) {
		buffer.pos = iter->second;
		return;
	}

	_updateBuffer(buffer, dataSize, _params.vertices);
	if (buffer.pos == 0)
		m_rectBufferOffsets.clear();

	buffer.offset += dataSize;
	buffer.pos = buffer.offset / sizeof(RectVertex);
	m_rectBufferOffsets[crc] = buffer.pos;
}


void BufferedDrawer::drawRects(const graphics::Context::DrawRectParameters & _params)
{
	_updateRectBuffer(_params);

	glVertexAttrib4fv(rectAttrib::color, _params.rectColor.data());

	glDrawArrays(GLenum(_params.mode), m_rectsBuffers.vbo.pos - _params.verticesCount, _params.verticesCount);
}

void BufferedDrawer::_convertFromSPVertex(bool _flatColors, u32 _count, const SPVertex * _data)
{
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
		glBindVertexArray(m_trisBuffers.vao);
		m_type = type;
	}

	_convertFromSPVertex(_params.flatColors, _params.verticesCount, _params.vertices);
	const GLsizeiptr vboDataSize = _params.verticesCount * sizeof(Vertex);
	Buffer & vboBuffer = m_trisBuffers.vbo;
	_updateBuffer(vboBuffer, vboDataSize, m_vertices);
	vboBuffer.offset += vboDataSize;
	vboBuffer.pos += _params.verticesCount;

	if (_params.elements == nullptr)
		return;

	const GLsizeiptr eboDataSize = sizeof(GLubyte) * _params.elementsCount;
	Buffer & eboBuffer = m_trisBuffers.ebo;
	_updateBuffer(eboBuffer, eboDataSize, _params.elements);
	eboBuffer.offset += eboDataSize;
	eboBuffer.pos += _params.elementsCount;
}

void BufferedDrawer::drawTriangles(const graphics::Context::DrawTriangleParameters & _params)
{
	_updateTrianglesBuffers(_params);

	if (config.generalEmulation.enableHWLighting != 0)
		glVertexAttrib1f(triangleAttrib::numlights, GLfloat(_params.vertices[0].HWLight));

	if (_params.elements == nullptr) {
		glDrawArrays(GLenum(_params.mode), m_trisBuffers.vbo.pos - _params.verticesCount, _params.verticesCount);
		return;
	}

	glDrawElementsBaseVertex(GLenum(_params.mode), _params.elementsCount, GL_UNSIGNED_BYTE,
		(char*)nullptr + m_trisBuffers.ebo.pos - _params.elementsCount, m_trisBuffers.vbo.pos - _params.verticesCount);
}

void BufferedDrawer::drawLine(f32 _width, SPVertex * _vertices)
{
	const BuffersType type = BuffersType::triangles;

	if (m_type != type) {
		glBindVertexArray(m_trisBuffers.vao);
		m_type = type;
	}

	_convertFromSPVertex(false, 2, _vertices);
	const GLsizeiptr vboDataSize = 2 * sizeof(Vertex);
	Buffer & vboBuffer = m_trisBuffers.vbo;
	_updateBuffer(vboBuffer, vboDataSize, m_vertices);
	vboBuffer.offset += vboDataSize;
	vboBuffer.pos += 2;

	glDrawArrays(GL_LINES, m_trisBuffers.vbo.pos - 2, 2);
}
