#include "UniformBlock.h"
#include "../Config.h"
#include "../Textures.h"

static
const char * strTextureUniforms[UniformBlock::tuTotal] = {
	"uTexScale",
	"uTexOffset",
	"uCacheScale",
	"uCacheOffset",
	"uCacheShiftScale",
	"uCacheFrameBuffer"
};

static
const char * strColorUniforms[UniformBlock::cuTotal] = {
	"uFogColor",
	"uCenterColor",
	"uScaleColor",
	"uBlendColor",
	"uEnvColor",
	"uPrimColor",
	"uPrimLod",
	"uK4",
	"uK5"
};

static
const char * strLightUniforms[UniformBlock::luTotal] = {
	"uLightDirection",
	"uLightColor"
};

UniformBlock::UniformBlock() : m_currentBuffer(0)
{
}

UniformBlock::~UniformBlock()
{
}

void UniformBlock::_initTextureBuffer(GLuint _program)
{
	const GLint blockSize = m_textureBlock.initBuffer(_program, "TextureBlock", strTextureUniforms);
	if (blockSize == 0)
		return;
	m_textureBlockData.resize(blockSize);
	GLbyte * pData = m_textureBlockData.data();
	memset(pData, 0, blockSize);
	glBindBuffer(GL_UNIFORM_BUFFER, m_textureBlock.m_buffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_textureBlock.m_blockBindingPoint, m_textureBlock.m_buffer);
	updateTextureParameters();
}

void UniformBlock::_initColorsBuffer(GLuint _program)
{
	const GLint blockSize = m_colorsBlock.initBuffer(_program, "ColorsBlock", strColorUniforms);
	if (blockSize == 0)
		return;
	m_colorsBlockData.resize(blockSize);
	GLbyte * pData = m_colorsBlockData.data();
	memset(pData, 0, blockSize);
	memcpy(pData + m_colorsBlock.m_offsets[cuFogColor], &gDP.fogColor.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuCenterColor], &gDP.key.center.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuScaleColor], &gDP.key.scale.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuEnvColor], &gDP.envColor.r, sizeof(f32)* 4);
	memcpy(pData + m_colorsBlock.m_offsets[cuPrimColor], &gDP.primColor.r, sizeof(f32)* 4);
	*(f32*)(pData + m_colorsBlock.m_offsets[cuPrimLod]) = gDP.primColor.l;
	*(f32*)(pData + m_colorsBlock.m_offsets[cuK4]) = gDP.convert.k4*0.0039215689f;
	*(f32*)(pData + m_colorsBlock.m_offsets[cuK5]) = gDP.convert.k5*0.0039215689f;

	glBindBuffer(GL_UNIFORM_BUFFER, m_colorsBlock.m_buffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, pData, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_colorsBlock.m_blockBindingPoint, m_colorsBlock.m_buffer);
	m_currentBuffer = m_colorsBlock.m_buffer;
}

void UniformBlock::_initLightBuffer(GLuint _program)
{
	const GLint blockSize = m_lightBlock.initBuffer(_program, "LightBlock", strLightUniforms);
	if (blockSize == 0)
		return;
	m_lightBlockData.resize(blockSize);
	GLbyte * pData = m_lightBlockData.data();
	memset(pData, 0, blockSize);
	glBindBuffer(GL_UNIFORM_BUFFER, m_lightBlock.m_buffer);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, m_lightBlock.m_blockBindingPoint, m_lightBlock.m_buffer);
	updateLightParameters();
}

bool UniformBlock::_isDataChanged(void * _pBuffer, const void * _pData, u32 _dataSize)
{
	u32 * pSrc = (u32*)_pData;
	u32 * pDst = (u32*)_pBuffer;
	u32 cnt = _dataSize / 4;
	for (u32 i = 0; i < cnt; ++i) {
		if (pSrc[i] != pDst[i]) {
			memcpy(_pBuffer, _pData, _dataSize);
			return true;
		}
	}
	return false;
}

void UniformBlock::bindWithShaderCombiner(ShaderCombiner * _pCombiner)
{
	const GLuint program = _pCombiner->m_program;
	if (_pCombiner->usesTexture()) {
		if (m_textureBlock.m_buffer == 0)
			_initTextureBuffer(program);
		else {
			const GLint blockIndex = glGetUniformBlockIndex(program, "TextureBlock");
			if (blockIndex != GL_INVALID_INDEX)
				glUniformBlockBinding(program, blockIndex, m_textureBlock.m_blockBindingPoint);
		}
	}

	if (m_colorsBlock.m_buffer == 0)
		_initColorsBuffer(program);
	else {
		const GLint blockIndex = glGetUniformBlockIndex(program, "ColorsBlock");
		if (blockIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(program, blockIndex, m_colorsBlock.m_blockBindingPoint);
	}

	if (_pCombiner->usesHwLighting()) {
		if (m_lightBlock.m_buffer == 0)
			_initLightBuffer(program);
		else {
			const GLint blockIndex = glGetUniformBlockIndex(program, "LightBlock");
			if (blockIndex != GL_INVALID_INDEX)
				glUniformBlockBinding(program, blockIndex, m_lightBlock.m_blockBindingPoint);
		}
	}
}

void UniformBlock::setColorData(ColorUniforms _index, u32 _dataSize, const void * _data)
{
	if (m_colorsBlock.m_buffer == 0)
		return;
	if (!_isDataChanged(m_colorsBlockData.data() + m_colorsBlock.m_offsets[_index], _data, _dataSize))
		return;

	if (m_currentBuffer != m_colorsBlock.m_buffer) {
		m_currentBuffer = m_colorsBlock.m_buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_colorsBlock.m_buffer);
	}

	glBufferSubData(GL_UNIFORM_BUFFER, m_colorsBlock.m_offsets[_index], _dataSize, _data);
}

void UniformBlock::updateTextureParameters()
{
	if (m_textureBlock.m_buffer == 0)
		return;

	std::vector<GLbyte> temp(m_textureBlockData.size(), 0);
	GLbyte * pData = temp.data();
	f32 texScale[4] = { gSP.texture.scales, gSP.texture.scalet, 0, 0 };
	memcpy(pData + m_textureBlock.m_offsets[tuTexScale], texScale, m_textureBlock.m_offsets[tuTexOffset] - m_textureBlock.m_offsets[tuTexScale]);

	f32 texOffset[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	if (gSP.textureTile[0] != nullptr) {
		if (gSP.textureTile[0]->textureMode != TEXTUREMODE_BGIMAGE && gSP.textureTile[0]->textureMode != TEXTUREMODE_FRAMEBUFFER_BG) {
			texOffset[0] = gSP.textureTile[0]->fuls;
			texOffset[1] = gSP.textureTile[0]->fult;
			FrameBuffer * pBuffer = gSP.textureTile[0]->frameBuffer;
			if (pBuffer != nullptr) {
				if (gSP.textureTile[0]->masks > 0 && gSP.textureTile[0]->clamps == 0)
					texOffset[0] = float(gSP.textureTile[0]->uls % (1 << gSP.textureTile[0]->masks));
				if (gSP.textureTile[0]->maskt > 0 && gSP.textureTile[0]->clampt == 0)
					texOffset[1] = float(gSP.textureTile[0]->ult % (1 << gSP.textureTile[0]->maskt));
			}
		}
	}
	if (gSP.textureTile[1] != 0) {
		texOffset[4] = gSP.textureTile[1]->fuls;
		texOffset[5] = gSP.textureTile[1]->fult;
		FrameBuffer * pBuffer = gSP.textureTile[1]->frameBuffer;
		if (pBuffer != nullptr) {
			if (gSP.textureTile[1]->masks > 0 && gSP.textureTile[1]->clamps == 0)
				texOffset[4] = float(gSP.textureTile[1]->uls % (1 << gSP.textureTile[1]->masks));
			if (gSP.textureTile[1]->maskt > 0 && gSP.textureTile[1]->clampt == 0)
				texOffset[5] = float(gSP.textureTile[1]->ult % (1 << gSP.textureTile[1]->maskt));
		}
	}
	memcpy(pData + m_textureBlock.m_offsets[tuTexOffset], texOffset, m_textureBlock.m_offsets[tuCacheScale] - m_textureBlock.m_offsets[tuTexOffset]);

	f32 texCacheScale[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	f32 texCacheOffset[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	f32 texCacheShiftScale[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	GLint texCacheFrameBuffer[4] = { 0, 0, 0, 0 };
	TextureCache & cache = textureCache();
	if (cache.current[0]) {
		texCacheScale[0] = cache.current[0]->scaleS;
		texCacheScale[1] = cache.current[0]->scaleT;
		texCacheOffset[0] = cache.current[0]->offsetS;
		texCacheOffset[1] = cache.current[0]->offsetT;

		f32 shiftScaleS = 1.0f;
		f32 shiftScaleT = 1.0f;
		getTextureShiftScale(0, cache, shiftScaleS, shiftScaleT);
		texCacheShiftScale[0] = shiftScaleS;
		texCacheShiftScale[1] = shiftScaleT;
		texCacheFrameBuffer[0] = cache.current[0]->frameBufferTexture;
	}
	if (cache.current[1]) {
		texCacheScale[4] = cache.current[1]->scaleS;
		texCacheScale[5] = cache.current[1]->scaleT;
		texCacheOffset[4] = cache.current[1]->offsetS;
		texCacheOffset[5] = cache.current[1]->offsetT;

		f32 shiftScaleS = 1.0f;
		f32 shiftScaleT = 1.0f;
		getTextureShiftScale(1, cache, shiftScaleS, shiftScaleT);
		texCacheShiftScale[4] = shiftScaleS;
		texCacheShiftScale[5] = shiftScaleT;
		texCacheFrameBuffer[1] = cache.current[1]->frameBufferTexture;
	}
	memcpy(pData + m_textureBlock.m_offsets[tuCacheScale], texCacheScale, m_textureBlock.m_offsets[tuCacheOffset] - m_textureBlock.m_offsets[tuCacheScale]);
	memcpy(pData + m_textureBlock.m_offsets[tuCacheOffset], texCacheOffset, m_textureBlock.m_offsets[tuCacheShiftScale] - m_textureBlock.m_offsets[tuCacheOffset]);
	memcpy(pData + m_textureBlock.m_offsets[tuCacheShiftScale], texCacheShiftScale, m_textureBlock.m_offsets[tuCacheFrameBuffer] - m_textureBlock.m_offsets[tuCacheShiftScale]);
	memcpy(pData + m_textureBlock.m_offsets[tuCacheFrameBuffer], texCacheFrameBuffer, m_textureBlockData.size() - m_textureBlock.m_offsets[tuCacheFrameBuffer]);

	if (m_currentBuffer != m_textureBlock.m_buffer) {
		m_currentBuffer = m_textureBlock.m_buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_textureBlock.m_buffer);
	}

	if(temp != m_textureBlockData) {
		m_textureBlockData = temp;
		glBufferSubData(GL_UNIFORM_BUFFER, m_textureBlock.m_offsets[tuTexScale], m_textureBlockData.size(), pData);
	}
}

void UniformBlock::updateLightParameters()
{
	if (m_lightBlock.m_buffer == 0)
		return;

	GLbyte * pData = m_lightBlockData.data();
	const u32 arraySize = m_lightBlock.m_offsets[luLightColor] / 8;
	for (s32 i = 0; i <= gSP.numLights; ++i) {
		memcpy(pData + m_lightBlock.m_offsets[luLightDirection] + arraySize*i, &gSP.lights[i].ix, arraySize);
		memcpy(pData + m_lightBlock.m_offsets[luLightColor] + arraySize*i, &gSP.lights[i].r, arraySize);
	}
	if (m_currentBuffer != m_lightBlock.m_buffer) {
		m_currentBuffer = m_lightBlock.m_buffer;
		glBindBuffer(GL_UNIFORM_BUFFER, m_lightBlock.m_buffer);
	}

	glBufferSubData(GL_UNIFORM_BUFFER, m_lightBlock.m_offsets[luLightDirection], m_lightBlockData.size(), pData);
}

UniformCollection * createUniformCollection()
{
	return new UniformBlock();
}
