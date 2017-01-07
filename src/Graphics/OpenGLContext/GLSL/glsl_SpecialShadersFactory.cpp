#include <Types.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Parameters.h>
#include <PaletteTexture.h>
#include <ZlutTexture.h>
#include <gDP.h>
#include "glsl_SpecialShadersFactory.h"
#include "glsl_ShaderPart.h"
#include "glsl_Utils.h"
#include "Textures.h"

namespace glsl {

	/*---------------ShadowMapShader-------------*/

	class VertexShaderRectNocolor : public ShaderPart
	{
	public:
		VertexShaderRectNocolor()
		{
			m_part =
				"IN highp vec4 aRectPosition;									\n"
				"void main()                                                    \n"
				"{                                                              \n"
				"  gl_Position = aRectPosition;									\n"
				"}                                                              \n"
			;
		}
	};

	class ShadowMapFragmentShader : public ShaderPart
	{
	public:
		ShadowMapFragmentShader()
		{
			m_part =
#ifndef GLESX
				"layout(binding = 0, r16ui) uniform readonly uimage2D uZlutImage;\n"
				"layout(binding = 1, r16ui) uniform readonly uimage2D uTlutImage;\n"
#else
				"layout(binding = 0, r32ui) highp uniform readonly uimage2D uZlutImage;\n"
				"layout(binding = 1, r32ui) highp uniform readonly uimage2D uTlutImage;\n"
#endif
				"layout(binding = 0) uniform sampler2D uDepthImage;		\n"
				"uniform lowp vec4 uFogColor;								\n"
				"OUT lowp vec4 fragColor;									\n"
				"lowp float get_alpha()										\n"
				"{															\n"
				"  mediump ivec2 coord = ivec2(gl_FragCoord.xy);			\n"
				"  highp float bufZ = texelFetch(uDepthImage,coord, 0).r;	\n"
				"  highp int iZ = bufZ > 0.999 ? 262143 : int(floor(bufZ * 262143.0));\n"
				"  mediump int y0 = clamp(iZ/512, 0, 511);					\n"
				"  mediump int x0 = iZ - 512*y0;							\n"
				"  highp uint iN64z = imageLoad(uZlutImage,ivec2(x0,y0)).r;		\n"
				"  highp float n64z = clamp(float(iN64z)/65532.0, 0.0, 1.0);\n"
				"  highp int index = min(255, int(n64z*255.0));				\n"
				"  highp uint iAlpha = imageLoad(uTlutImage,ivec2(index,0)).r;\n"
				"  return float(iAlpha>>8)/255.0;							\n"
				"}															\n"
				"void main()												\n"
				"{															\n"
				"  fragColor = vec4(uFogColor.rgb, get_alpha());			\n"
				"}															\n"
				;
		}
	};

	class MonochromeFragmentShader : public ShaderPart
	{
	public:
		MonochromeFragmentShader()
		{
			m_part =
				"uniform sampler2D uColorImage;							\n"
				"OUT lowp vec4 fragColor;								\n"
				"void main()											\n"
				"{														\n"
				"  mediump ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
				"  lowp vec4 tex = texelFetch(uColorImage, coord, 0);		\n"
				//"  lowp float c = (tex.r + tex.g + tex.b) / 3.0f;		\n"
				"  lowp float c = dot(vec4(0.2126, 0.7152, 0.0722, 0.0), tex);\n"
				"  fragColor = vec4(c, c, c, 1.0);						\n"
				"}														\n"
				;
		}
	};


	/*---------------SpecialShader-------------*/

	template<class FragmentBody>
	class SpecialShader : public graphics::ShaderProgram
	{
	public:
		SpecialShader(const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: m_program(0)
		{
			VertexShaderRectNocolor vertexBody;
			FragmentBody fragmentBody;

			std::stringstream ssVertexShader;
			_vertexHeader->write(ssVertexShader);
			vertexBody.write(ssVertexShader);

			std::stringstream ssFragmentShader;
			_fragmentHeader->write(ssFragmentShader);
			fragmentBody.write(ssFragmentShader);

			m_program = Utils::createRectShaderProgram(ssVertexShader.str().data(), ssFragmentShader.str().data());
		}

		~SpecialShader()
		{
			glUseProgram(0);
			glDeleteProgram(m_program);
			m_program = 0;
		}

		void SpecialShader::activate() override {
			glUseProgram(m_program);
			gDP.changed |= CHANGED_COMBINE;
		}

	protected:
		GLuint m_program;
	};

	/*---------------ShadowMapShader-------------*/

	typedef SpecialShader<ShadowMapFragmentShader> ShadowMapShaderBase;

	class ShadowMapShader : public ShadowMapShaderBase
	{
	public:
		ShadowMapShader(const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: ShadowMapShaderBase(_vertexHeader, _fragmentHeader)
			, m_loc(-1)
		{
			glUseProgram(m_program);
			m_loc = glGetUniformLocation(m_program, "uFogColor");
			glUseProgram(0);
		}

		void activate() override {
			ShadowMapShaderBase::activate();
			glUniform4fv(m_loc, 1, &gDP.fogColor.r);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			g_paletteTexture.update();
		}

	private:
		int m_loc;
	};

	/*---------------MonochromeShader-------------*/

	typedef SpecialShader<MonochromeFragmentShader> MonochromeShaderBase;

	class MonochromeShader : public MonochromeShaderBase
	{
	public:
		MonochromeShader(const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: MonochromeShaderBase(_vertexHeader, _fragmentHeader)
		{
			glUseProgram(m_program);
			const int texLoc = glGetUniformLocation(m_program, "uColorImage");
			glUniform1i(texLoc, 0);
			glUseProgram(0);
		}
	};

	/*---------------SpecialShadersFactory-------------*/

	SpecialShadersFactory::SpecialShadersFactory(const opengl::GLInfo & _glinfo,
												const ShaderPart * _vertexHeader,
												const ShaderPart * _fragmentHeader)
		: m_glinfo(_glinfo)
		, m_vertexHeader(_vertexHeader)
		, m_fragmentHeader(_fragmentHeader)
	{
	}

	graphics::ShaderProgram * SpecialShadersFactory::createShadowMapShader() const
	{
		if (!m_glinfo.imageTextures)
			return nullptr;

		return new ShadowMapShader(m_vertexHeader, m_fragmentHeader);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createMonochromeShader() const
	{
		return new MonochromeShader(m_vertexHeader, m_fragmentHeader);
	}
}