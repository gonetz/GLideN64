#ifndef RDRAMtoColorBuffer_H
#define RDRAMtoColorBuffer_H

#include <vector>
#include <memory>
#include <Graphics/ObjectHandle.h>

namespace graphics {
	class PixelWriteBuffer;
}

struct CachedTexture;
struct FrameBuffer;

class RDRAMtoColorBuffer
{
public:
	void init();
	void destroy();

	void addAddress(u32 _address, u32 _size);
	void copyFromRDRAM(u32 _address, bool _bCFB);

	static RDRAMtoColorBuffer & get();

private:
	RDRAMtoColorBuffer();
	RDRAMtoColorBuffer(const RDRAMtoColorBuffer &);

	void reset();

	class Cleaner
	{
	public:
		Cleaner(RDRAMtoColorBuffer * _p) : m_p(_p) {}
		~Cleaner()
		{
			m_p->reset();
		}
	private:
		RDRAMtoColorBuffer * m_p;
	};

	FrameBuffer * m_pCurBuffer;
	CachedTexture * m_pTexture;
	std::vector<u32> m_vecAddress;
	std::unique_ptr<graphics::PixelWriteBuffer> m_pbuf;
};

#endif // RDRAMtoColorBuffer_H
