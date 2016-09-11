#ifndef DEPTHBUFFER_H
#define DEPTHBUFFER_H

#include "Types.h"
#include "Textures.h"

struct FrameBuffer;
struct DepthBuffer
{
	DepthBuffer();
	DepthBuffer(DepthBuffer && _other);
	~DepthBuffer();
	void initDepthImageTexture(FrameBuffer * _pBuffer);
	void initDepthBufferTexture(FrameBuffer * _pBuffer);
	CachedTexture * resolveDepthBufferTexture(FrameBuffer * _pBuffer);
	CachedTexture * copyDepthBufferTexture(FrameBuffer * _pBuffer);

	void setDepthAttachment(GLenum _target);
	void activateDepthBufferTexture(FrameBuffer * _pBuffer);

	void bindDepthImageTexture();

	u32 m_address, m_width;
	u32 m_ulx, m_uly, m_lrx, m_lry; // Parameters of fillrect command.
	GLuint m_depthImageFBO;
	CachedTexture *m_pDepthImageTexture;
	CachedTexture *m_pDepthBufferTexture;
	GLuint m_depthRenderbuffer;
	u32 m_depthRenderbufferWidth;
	bool m_cleared;
	// multisampling
	CachedTexture *m_pResolveDepthBufferTexture;
	bool m_resolved;
	// render to depth buffer
	GLuint m_copyFBO;
	CachedTexture *m_pDepthBufferCopyTexture;
	bool m_copied;

private:
	void _initDepthBufferTexture(FrameBuffer * _pBuffer, CachedTexture *_pTexture, bool _multisample);
	void _initDepthBufferRenderbuffer(FrameBuffer * _pBuffer);
	void _DepthBufferTexture(FrameBuffer * _pBuffer);
};

class DepthBufferList
{
public:
	void init();
	void destroy();
	void saveBuffer(u32 _address);
	void removeBuffer(u32 _address);
	void clearBuffer(u32 _ulx, u32 _uly, u32 _lrx, u32 _lry);
	void setNotCleared();
	DepthBuffer *findBuffer(u32 _address);
	DepthBuffer * getCurrent() const {return m_pCurrent;}

	static DepthBufferList & get();

	const u16 * const getZLUT() const {return m_pzLUT;}

private:
	DepthBufferList();
	DepthBufferList(const DepthBufferList &);
	~DepthBufferList();

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
