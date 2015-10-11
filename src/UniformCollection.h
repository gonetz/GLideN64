#ifndef UNIFORM_COLLECTION_H
#define UNIFORM_COLLECTION_H

#include "GLSLCombiner.h"

class UniformCollection {
public:
	enum TextureUniforms {
		tuTexScale,
		tuTexOffset,
		tuCacheScale,
		tuCacheOffset,
		tuCacheShiftScale,
		tuCacheFrameBuffer,
		tuTotal
	};

	enum ColorUniforms {
		cuFogColor,
		cuCenterColor,
		cuScaleColor,
		cuBlendColor,
		cuEnvColor,
		cuPrimColor,
		cuPrimLod,
		cuK4,
		cuK5,
		cuTotal
	};

	enum LightUniforms {
		luLightDirection,
		luLightColor,
		luTotal
	};

	virtual ~UniformCollection() {}

	virtual void bindWithShaderCombiner(ShaderCombiner * _pCombiner) = 0;
	virtual void setColorData(ColorUniforms _index, u32 _dataSize, const void * _data) = 0;
	virtual void updateTextureParameters() = 0;
	virtual void updateLightParameters() = 0;
	virtual void updateUniforms(ShaderCombiner * _pCombiner, OGLRender::RENDER_STATE _renderState) = 0;
};

UniformCollection * createUniformCollection();

#endif // UNIFORM_COLLECTION_H