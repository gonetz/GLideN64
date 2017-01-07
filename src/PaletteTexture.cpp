#include "Graphics/Context.h"
#include "Graphics/Parameters.h"
#include "N64.h"
#include "gDP.h"
#include "VI.h"
#include "Textures.h"
#include "FBOTextureFormats.h"
#include "PaletteTexture.h"
#include "DepthBuffer.h"


PaletteTexture g_paletteTexture;

PaletteTexture::PaletteTexture()
: m_pTexture(nullptr)
, m_paletteCRC256(0)
{
}

void PaletteTexture::init()
{
	m_paletteCRC256 = 0;
	m_pTexture = textureCache().addFrameBufferTexture(false);
	m_pTexture->format = G_IM_FMT_IA;
	m_pTexture->clampS = 1;
	m_pTexture->clampT = 1;
	m_pTexture->frameBufferTexture = CachedTexture::fbOneSample;
	m_pTexture->maskS = 0;
	m_pTexture->maskT = 0;
	m_pTexture->mirrorS = 0;
	m_pTexture->mirrorT = 0;
	m_pTexture->realWidth = 256;
	m_pTexture->realHeight = 1;
	m_pTexture->textureBytes = m_pTexture->realWidth * m_pTexture->realHeight;
#ifdef GLESX
	m_pTexture->textureBytes *= sizeof(u32);
#else
	m_pTexture->textureBytes *= sizeof(u16);
#endif
	textureCache().addFrameBufferTextureSize(m_pTexture->textureBytes);

	graphics::Context::InitTextureParams initParams;
	initParams.handle = graphics::ObjectHandle(m_pTexture->glName);
	initParams.ImageUnit = graphics::textureImageUnits::Tlut;
	initParams.width = m_pTexture->realWidth;
	initParams.height = m_pTexture->realHeight;
	initParams.internalFormat = fboFormats.lutInternalFormat;
	initParams.format = fboFormats.lutFormat;
	initParams.dataType = fboFormats.lutType;
	gfxContext.init2DTexture(initParams);

	graphics::Context::TexParameters setParams;
	setParams.handle = graphics::ObjectHandle(m_pTexture->glName);
	setParams.target = graphics::target::TEXTURE_2D;
	setParams.textureUnitIndex = graphics::textureIndices::PaletteTex;
	setParams.minFilter = graphics::textureParameters::FILTER_NEAREST;
	setParams.magFilter = graphics::textureParameters::FILTER_NEAREST;
	setParams.wrapS = graphics::textureParameters::WRAP_CLAMP_TO_EDGE;
	setParams.wrapT = graphics::textureParameters::WRAP_CLAMP_TO_EDGE;
	gfxContext.setTextureParameters(setParams);

	// Generate Pixel Buffer Object. Initialize it with max buffer size.
	m_pbuf.reset(gfxContext.createPixelWriteBuffer(m_pTexture->textureBytes));
	isGLError();
}

void PaletteTexture::destroy()
{
	glBindImageTexture(TlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);
	textureCache().removeFrameBufferTexture(m_pTexture);
	m_pTexture = nullptr;
	m_pbuf.reset();
}

void PaletteTexture::update()
{
	if (m_paletteCRC256 == gDP.paletteCRC256)
		return;
	
	m_paletteCRC256 = gDP.paletteCRC256;

	graphics::PixelBufferBinder<graphics::PixelWriteBuffer> binder(m_pbuf.get());
	GLubyte* ptr = (GLubyte*)m_pbuf->getWriteBuffer(m_pTexture->textureBytes);
#ifdef GLESX
	u32 * palette = (u32*)ptr;
#else
	u16 * palette = (u16*)ptr;
#endif
	u16 *src = (u16*)&TMEM[256];
	for (int i = 0; i < 256; ++i)
		palette[i] = swapword(src[i * 4]);
	m_pbuf->closeWriteBuffer();

	graphics::Context::UpdateTextureDataParams params;
	params.handle = graphics::ObjectHandle(m_pTexture->glName);
	params.ImageUnit = graphics::textureImageUnits::Tlut;
	params.textureUnitIndex = graphics::textureIndices::PaletteTex;
	params.width = m_pTexture->realWidth;
	params.height = m_pTexture->realHeight;
	params.format = fboFormats.lutFormat;
	params.internalFormat = fboFormats.lutInternalFormat;
	params.dataType = fboFormats.lutType;
	params.data = m_pbuf->getData();
	glBindImageTexture(TlutImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, fboFormats.lutInternalFormat);
	gfxContext.update2DTexture(params);
}
