#pragma once

namespace graphics {

	class PixelWriteBuffer
	{
	public:
		virtual ~PixelWriteBuffer() {}
		virtual void * getWriteBuffer(size_t _size) = 0;
		virtual void closeWriteBuffer() = 0;
		virtual void * getData() = 0;
		virtual void bind() = 0;
		virtual void unbind() = 0;
	};

	template<class T>
	class PixelBufferBinder
	{
	public:
		PixelBufferBinder(T * _buffer)
			: m_buffer(_buffer) {
			m_buffer->bind();
		}

		~PixelBufferBinder() {
			m_buffer->unbind();
			m_buffer = nullptr;
		}
	private:
		T * m_buffer;
	};

}
