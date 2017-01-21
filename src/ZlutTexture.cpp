#include "Graphics/Context.h"
#include "Graphics/Parameters.h"
#include "DepthBuffer.h"
#include "Config.h"
#include "Textures.h"
#include "ZlutTexture.h"

using namespace graphics;

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

	const FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();
	Context::InitTextureParams initParams;
	initParams.handle = ObjectHandle(m_pTexture->glName);
	initParams.ImageUnit = textureImageUnits::Zlut;
	initParams.width = m_pTexture->realWidth;
	initParams.height = m_pTexture->realHeight;
	initParams.internalFormat = fbTexFormats.lutInternalFormat;
	initParams.format = fbTexFormats.lutFormat;
	initParams.dataType = fbTexFormats.lutType;
	initParams.data = zLUT;
	gfxContext.init2DTexture(initParams);

	Context::TexParameters setParams;
	setParams.handle = ObjectHandle(m_pTexture->glName);
	setParams.target = target::TEXTURE_2D;
	setParams.textureUnitIndex = textureIndices::ZLUTTex;
	setParams.minFilter = textureParameters::FILTER_NEAREST;
	setParams.magFilter = textureParameters::FILTER_NEAREST;
	setParams.wrapS = textureParameters::WRAP_CLAMP_TO_EDGE;
	setParams.wrapT = textureParameters::WRAP_CLAMP_TO_EDGE;
	gfxContext.setTextureParameters(setParams);
}

void ZlutTexture::destroy() {
	const FramebufferTextureFormats & fbTexFormats = gfxContext.getFramebufferTextureFormats();

	Context::BindImageTextureParameters bindParams;
	bindParams.imageUnit = textureImageUnits::Zlut;
	bindParams.texture = ObjectHandle();
	bindParams.accessMode = textureImageAccessMode::READ_ONLY;
	bindParams.textureFormat = fbTexFormats.lutInternalFormat;

	gfxContext.bindImageTexture(bindParams);

	textureCache().removeFrameBufferTexture(m_pTexture);
	m_pTexture = nullptr;
}
