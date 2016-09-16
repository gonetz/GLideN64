#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <list>
#include <vector>

#include "Types.h"
#include "Textures.h"
struct gDPTile;
struct DepthBuffer;

const int fingerprint[4] = { 2, 6, 4, 3 };

struct FrameBuffer
{
	FrameBuffer();
	FrameBuffer(FrameBuffer && _other);
	~FrameBuffer();
	void init(u32 _address, u32 _endAddress, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb);
	void reinit(u16 _height);
	void resolveMultisampledTexture(bool _bForce = false);
	CachedTexture * getTexture(u32 _t);
	CachedTexture * getTextureBG(u32 _t);
	void copyRdram();
	bool isValid() const;
	bool _isMarioTennisScoreboard() const;
	bool isAuxiliary() const;

	u32 m_startAddress, m_endAddress;
	u32 m_size, m_width, m_height, m_fillcolor;
	float m_scaleX, m_scaleY;
	bool m_copiedToRdram;
	bool m_fingerprint;
	bool m_cleared;
	bool m_changed;
	bool m_cfb;
	bool m_isDepthBuffer;
	bool m_isPauseScreen;
	bool m_isOBScreen;
	bool m_needHeightCorrection;

	struct {
		u32 uls, ult;
	} m_loadTileOrigin;
	u32 m_loadType;

	GLuint m_FBO;
	CachedTexture *m_pTexture;
	DepthBuffer *m_pDepthBuffer;

	// multisampling
	GLuint m_resolveFBO;
	CachedTexture *m_pResolveTexture;
	bool m_resolved;

	// subtexture
	GLuint m_SubFBO;
	CachedTexture *m_pSubTexture;

	std::vector<u8> m_RdramCopy;

private:
	void _initTexture(u16 _width, u16 _height, u16 _format, u16 _size, CachedTexture *_pTexture);
	void _setAndAttachTexture(u16 _size, CachedTexture *_pTexture);
	bool _initSubTexture(u32 _t);
	CachedTexture * _getSubTexture(u32 _t);
	mutable u32 m_validityChecked;
};

class FrameBufferList
{
public:
	void init();
	void destroy();
	void saveBuffer(u32 _address, u16 _format, u16 _size, u16 _width, u16 _height, bool _cfb);
	void removeAux();
	void copyAux();
	void removeBuffer(u32 _address);
	void removeBuffers(u32 _width);
	void attachDepthBuffer();
	void clearDepthBuffer(DepthBuffer * _pDepthBuffer);
	FrameBuffer * findBuffer(u32 _startAddress);
	FrameBuffer * findTmpBuffer(u32 _address);
	FrameBuffer * getCurrent() const {return m_pCurrent;}
	void renderBuffer(u32 _address);
	void setBufferChanged();
	void correctHeight();
	void clearBuffersChanged();
	void setCurrentDrawBuffer() const;

	FrameBuffer * getCopyBuffer() const { return m_pCopy; }
	void setCopyBuffer(FrameBuffer * _pBuffer) { m_pCopy = _pBuffer; }

	void fillBufferInfo(void * _pinfo, u32 _size);

	static FrameBufferList & get();

private:
	FrameBufferList() : m_pCurrent(nullptr), m_pCopy(nullptr) {}
	FrameBufferList(const FrameBufferList &);

	FrameBuffer * _findBuffer(u32 _startAddress, u32 _endAddress, u32 _width);

	typedef std::list<FrameBuffer> FrameBuffers;
	FrameBuffers m_list;
	FrameBuffer * m_pCurrent;
	FrameBuffer * m_pCopy;
	u32 m_prevColorImageHeight;
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

u32 cutHeight(u32 _address, u32 _height, u32 _stride);

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_CopyToRDRAM( u32 _address , bool _sync );
void FrameBuffer_CopyChunkToRDRAM(u32 _address);
void FrameBuffer_CopyFromRDRAM(u32 address, bool bUseAlpha);
void FrameBuffer_AddAddress(u32 address, u32 _size);
bool FrameBuffer_CopyDepthBuffer(u32 address);
bool FrameBuffer_CopyDepthBufferChunk(u32 address);
void FrameBuffer_ActivateBufferTexture(u32 t, FrameBuffer *pBuffer);
void FrameBuffer_ActivateBufferTextureBG(u32 t, FrameBuffer *pBuffer);

#endif
