#ifndef UNIFORM_BLOCK_H
#define UNIFORM_BLOCK_H

#include "../UniformCollection.h"

class UniformBlock : public UniformCollection
{
public:
	UniformBlock();
	~UniformBlock();

	virtual void bindWithShaderCombiner(ShaderCombiner * _pCombiner);
	virtual void setColorData(ColorUniforms _index, u32 _dataSize, const void * _data);
	virtual void updateTextureParameters();
	virtual void updateLightParameters();
	virtual void updateUniforms(ShaderCombiner * /*_pCombiner*/, OGLRender::RENDER_STATE /*_renderState*/) {}

private:
	void _initTextureBuffer(GLuint _program);
	void _initColorsBuffer(GLuint _program);
	void _initLightBuffer(GLuint _program);

	bool _isDataChanged(void * _pBuffer, const void * _pData, u32 _dataSize);

	template <u32 _numUniforms, u32 _bindingPoint>
	struct UniformBlockData
	{
		UniformBlockData() : m_buffer(0), m_blockBindingPoint(_bindingPoint)
		{
			memset(m_indices, 0, sizeof(m_indices));
			memset(m_offsets, 0, sizeof(m_offsets));
		}
		~UniformBlockData()
		{
			if (m_buffer != 0) {
				glDeleteBuffers(1, &m_buffer);
				m_buffer = 0;
			}
		}

		GLint initBuffer(GLuint _program, const char * _strBlockName, const char ** _strUniformNames)
		{
			GLuint blockIndex = glGetUniformBlockIndex(_program, _strBlockName);
			if (blockIndex == GL_INVALID_INDEX)
				return 0;

			GLint blockSize, numUniforms;
			glGetActiveUniformBlockiv(_program, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
			glGetActiveUniformBlockiv(_program, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms);

			glGetUniformIndices(_program, numUniforms, _strUniformNames, m_indices);
			glGetActiveUniformsiv(_program, numUniforms, m_indices, GL_UNIFORM_OFFSET, m_offsets);

			glUniformBlockBinding(_program, blockIndex, m_blockBindingPoint);
			glGenBuffers(1, &m_buffer);
			return blockSize;
		}

		GLuint m_buffer;
		GLuint m_blockBindingPoint;
		GLuint m_indices[_numUniforms];
		GLint m_offsets[_numUniforms];
	};

	GLuint m_currentBuffer;

	UniformBlockData<tuTotal, 1> m_textureBlock;
	UniformBlockData<cuTotal, 2> m_colorsBlock;
	UniformBlockData<luTotal, 3> m_lightBlock;

	std::vector<GLbyte> m_textureBlockData;
	std::vector<GLbyte> m_colorsBlockData;
	std::vector<GLbyte> m_lightBlockData;
};

#endif // UNIFORM_BLOCK_H
