#pragma once
#include <Graphics/OpenGLContext/GLSL/glsl_CombinerProgramBuilderCommon.h>

namespace glsl {

class CombinerProgramUniformFactory;

class CombinerProgramBuilderFast : public glsl::CombinerProgramBuilderCommon
{
public:
	CombinerProgramBuilderFast(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram);

private:

	void _writeFragmentGlobalVariablesTex(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderReadMSTex(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderReadTex(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderReadTexCopyMode(std::stringstream& ssShader) const override;
	void _writeFragmentHeaderClampWrapMirrorEngine(std::stringstream& ssShader) const override;
	void _writeFragmentClampWrapMirrorEngineTex0(std::stringstream& ssShader) const override;
	void _writeFragmentClampWrapMirrorEngineTex1(std::stringstream& ssShader) const override;
	void _writeFragmentReadTex0(std::stringstream& ssShader) const override;
	void _writeFragmentReadTex1(std::stringstream& ssShader) const override;
	void _writeFragmentReadTexCopyMode(std::stringstream& ssShader) const override;
	void _writeShaderClampWrapMirrorEngine(std::stringstream& ssShader) const override;
	void _writeShaderMipmap(std::stringstream& ssShader) const override;
	void _writeShaderReadtex(std::stringstream& ssShader) const override;
	void _writeShaderReadtexCopyMode(std::stringstream& ssShader) const override;

	ShaderPartPtr m_fragmentGlobalVariablesTex;
	ShaderPartPtr m_fragmentHeaderClampWrapMirror;
	ShaderPartPtr m_fragmentHeaderReadMSTex;
	ShaderPartPtr m_fragmentHeaderReadTex;
	ShaderPartPtr m_fragmentHeaderReadTexCopyMode;
	ShaderPartPtr m_fragmentReadTex0;
	ShaderPartPtr m_fragmentReadTex1;
	ShaderPartPtr m_fragmentClampWrapMirrorTex0;
	ShaderPartPtr m_fragmentClampWrapMirrorTex1;
	ShaderPartPtr m_fragmentReadTexCopyMode;
	ShaderPartPtr m_shaderMipmap;
	ShaderPartPtr m_shaderReadtex;
	ShaderPartPtr m_shaderReadtexCopyMode;
	ShaderPartPtr m_shaderClampWrapMirror;
};

}

