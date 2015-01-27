#ifndef DEPTHBUFFER_H
#define DEPTHBUFFER_H

#include "Types.h"
#include "Textures.h"

struct DepthBuffer
{
	DepthBuffer();
	DepthBuffer(DepthBuffer && _other);
	~DepthBuffer();
	void initDepthImageTexture(FrameBuffer * _pBuffer);
	void initDepthBufferTexture(FrameBuffer * _pBuffer);

	void setDepthAttachment();
	void activateDepthBufferTexture();

	void bindDepthImageTexture();

	u32 m_address, m_width;
	GLuint m_FBO;
	CachedTexture *m_pDepthImageTexture;
	CachedTexture *m_pDepthBufferTexture;
};

class DepthBufferList
{
public:
	void init();
	void destroy();
	void saveBuffer(u32 _address);
	void removeBuffer(u32 _address);
	void clearBuffer();
	DepthBuffer *findBuffer(u32 _address);
	DepthBuffer * getCurrent() const {return m_pCurrent;}

	static DepthBufferList & get()
	{
		static DepthBufferList depthBufferList;
		return depthBufferList;
	}

	const u16 * const getZLUT() const {return m_pzLUT;}

private:
	DepthBufferList() : m_pCurrent(NULL), m_pzLUT(NULL) {}
	DepthBufferList(const FrameBufferList &);

	typedef std::list<DepthBuffer> DepthBuffers;
	DepthBuffers m_list;
	DepthBuffer *m_pCurrent;
	u16 * m_pzLUT;
};

inline
DepthBufferList & depthBufferList()
{
	return DepthBufferList::get();
}

extern const GLuint ZlutImageUnit;
extern const GLuint TlutImageUnit;
extern const GLuint depthImageUnit;

void DepthBuffer_Init();
void DepthBuffer_Destroy();
#endif
