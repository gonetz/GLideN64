#include "Graphics/Context.h"
#include "Graphics/Parameters.h"
#include "DepthBuffer.h"
#include "Config.h"
#include "Textures.h"
#include "ZlutTexture.h"


ZlutTexture g_zlutTexture;

ZlutTexture::ZlutTexture()
: m_pTexture(nullptr)
{
}

void ZlutTexture::init()
{
// TODO make GL independent
#ifdef GLESX
	std::vector<u32> vecZLUT(0x40000);
	const u16 * const zLUT16 = depthBufferList().getZLUT();
	for (u32 i = 0; i < 0x40000; ++i)
		vecZLUT[i] = zLUT16[i];
	const u32 * zLUT = vecZLUT.data();
#else
	const u16 * const zLUT = depthBufferList().getZLUT();
#endif

	m_pTexture = textureCache().addFrameBufferTexture(false);
	m_pTexture->format = G_IM_FMT_IA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 512;
	m_pTexture->realHeight = 512;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight * sizeof(zLUT[0]);
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);

	const graphics::FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();
	graphics::Context::InitTextureParams initParams;
	initParams.handle = graphics::ObjectHandle(m_pTexture->glName);
	initParams.ImageUnit = graphics::textureImageUnits::Zlut;
	initParams.width = m_pTexture->realWidth;
	initParams.height = m_pTexture->realHeight;
	initParams.internalFormat = fbTexFormats.lutInternalFormat;
	initParams.format = fbTexFormats.lutFormat;
	initParams.dataType = fbTexFormats.lutType;
	initParams.data = zLUT;
	gfxContext.init2DTexture(initParams);

	graphics::Context::TexParameters setParams;
	setParams.handle = graphics::ObjectHandle(m_pTexture->glName);
	setParams.target = graphics::target::TEXTURE_2D;
	setParams.textureUnitIndex = graphics::textureIndices::ZLUTTex;
	setParams.minFilter = graphics::textureParameters::FILTER_NEAREST;
	setParams.magFilter = graphics::textureParameters::FILTER_NEAREST;
	setParams.wrapS = graphics::textureParameters::WRAP_CLAMP_TO_EDGE;
	setParams.wrapT = graphics::textureParameters::WRAP_CLAMP_TO_EDGE;
	gfxContext.setTextureParameters(setParams);
}

void ZlutTexture::destroy() {
	const graphics::FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();
	glBindImageTexture(ZlutImageUnit, 0, 0, GL_FALSE, GL_FALSE, GL_READ_ONLY, GLenum(fbTexFormats.lutInternalFormat));
	textureCache().removeFrameBufferTexture(m_pTexture);
	m_pTexture = nullptr;
}
