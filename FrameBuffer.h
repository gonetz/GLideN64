#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <list>

#include "Types.h"
#include "Textures.h"
struct gDPTile;
struct DepthBuffer;

struct FrameBuffer
{
	FrameBuffer();
	FrameBuffer(FrameBuffer && _other);
	~FrameBuffer();

	u32 m_startAddress, m_endAddress;
	u32 m_size, m_width, m_height, m_fillcolor;
	float m_scaleX, m_scaleY;
	bool m_cleared;

	GLuint m_FBO;
	gDPTile *m_pLoadTile;
	CachedTexture *m_pTexture;
	DepthBuffer *m_pDepthBuffer;
};

class FrameBufferList
{
public:
	void init();
	void destroy();
	void saveBuffer( u32 _address, u16 _format, u16 _size, u16 _width, u16 _height );
	void removeBuffer( u32 _address );
	void attachDepthBuffer();
	FrameBuffer * findBuffer(u32 _address);
	FrameBuffer * findTmpBuffer(u32 _address);
	FrameBuffer * getCurrent() const {return m_pCurrent;}
	void renderBuffer(u32 _address);
	bool isFboMode() const {return m_drawBuffer == GL_FRAMEBUFFER;}

	static FrameBufferList & get()
	{
		static FrameBufferList frameBufferList;
		return frameBufferList;
	}

private:
	FrameBufferList() : m_pCurrent(NULL), m_drawBuffer(GL_BACK) {}
	FrameBufferList(const FrameBufferList &);

	void _initDepthTexture();

	typedef std::list<FrameBuffer> FrameBuffers;
	FrameBuffers m_list;
	FrameBuffer * m_pCurrent;
	GLenum m_drawBuffer;
};

inline
FrameBufferList & frameBufferList()
{
	return FrameBufferList::get();
}

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_CopyToRDRAM( u32 address, bool bSync );
void FrameBuffer_CopyFromRDRAM( u32 address, bool bUseAlpha );
void FrameBuffer_CopyDepthBuffer( u32 address );
void FrameBuffer_ActivateBufferTexture(s16 t, FrameBuffer *pBuffer);
void FrameBuffer_ActivateBufferTextureBG(s16 t, FrameBuffer *pBuffer);

#endif
