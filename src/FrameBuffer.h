#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <list>
#include <vector>

#include "Types.h"
#include "Textures.h"
struct gDPTile;
struct DepthBuffer;

struct FrameBuffer
{
	FrameBuffer();
	FrameBuffer(FrameBuffer && _other);
	~FrameBuffer();
	void init(u32 _address, u32 _endAddress, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb);
	void reinit(u16 _height);
	void resolveMultisampledTexture();
	CachedTexture * getTexture();
	void copyRdram();
	bool isValid() const;
	bool _isMarioTennisScoreboard() const;

	u32 m_startAddress, m_endAddress;
	u32 m_size, m_width, m_height, m_fillcolor, m_validityChecked;
	float m_scaleX, m_scaleY;
	bool m_copiedToRdram;
	bool m_cleared;
	bool m_changed;
	bool m_cfb;
	bool m_isDepthBuffer;
	bool m_isPauseScreen;
	bool m_isOBScreen;
	bool m_needHeightCorrection;
	bool m_postProcessed;

	GLuint m_FBO;
	gDPTile *m_pLoadTile;
	CachedTexture *m_pTexture;
	DepthBuffer *m_pDepthBuffer;
	// multisampling
	CachedTexture *m_pResolveTexture;
	GLuint m_resolveFBO;
	bool m_resolved;

	std::vector<u8> m_RdramCopy;

private:
	void _initTexture(u16 _format, u16 _size, CachedTexture *_pTexture);
	void _setAndAttachTexture(u16 _size, CachedTexture *_pTexture);
};

class FrameBufferList
{
public:
	void init();
	void destroy();
	void saveBuffer(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb);
	void removeBuffer(u32 _address);
	void removeBuffers(u32 _width);
	void attachDepthBuffer();
	FrameBuffer * findBuffer(u32 _startAddress);
	FrameBuffer * findTmpBuffer(u32 _address);
	FrameBuffer * getCurrent() const {return m_pCurrent;}
	void renderBuffer(u32 _address);
	void setBufferChanged();
	void correctHeight();
	void clearBuffersChanged();

	static FrameBufferList & get();

private:
	FrameBufferList() : m_pCurrent(NULL) {}
	FrameBufferList(const FrameBufferList &);

	FrameBuffer * _findBuffer(u32 _startAddress, u32 _endAddress, u32 _width);

	typedef std::list<FrameBuffer> FrameBuffers;
	FrameBuffers m_list;
	FrameBuffer * m_pCurrent;
};

struct PBOBinder {
#ifndef GLES2
	PBOBinder(GLenum _target, GLuint _PBO) : m_target(_target)
	{
		glBindBuffer(m_target, _PBO);
	}
	~PBOBinder() {
		glBindBuffer(m_target, 0);
	}
	GLenum m_target;
#else
	PBOBinder(GLubyte* _ptr) : ptr(_ptr) {}
	~PBOBinder() { free(ptr); }
	GLubyte* ptr;
#endif
};

inline
FrameBufferList & frameBufferList()
{
	return FrameBufferList::get();
}

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_CopyToRDRAM( u32 _address );
void FrameBuffer_CopyFromRDRAM( u32 address, bool bUseAlpha );
bool FrameBuffer_CopyDepthBuffer( u32 address );
void FrameBuffer_ActivateBufferTexture(s16 t, FrameBuffer *pBuffer);
void FrameBuffer_ActivateBufferTextureBG(s16 t, FrameBuffer *pBuffer);

#endif
