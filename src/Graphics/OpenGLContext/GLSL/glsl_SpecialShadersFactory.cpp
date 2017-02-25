#include <assert.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Parameters.h>
#include <PaletteTexture.h>
#include <ZlutTexture.h>
#include <gDP.h>
#include <Config.h>
#include <Graphics/ObjectHandle.h>
#include <Graphics/OpenGLContext/opengl_CachedFunctions.h>
#include "glsl_SpecialShadersFactory.h"
#include "glsl_ShaderPart.h"
#include "glsl_Utils.h"

namespace glsl {

	/*---------------VertexShaderPart-------------*/

	class VertexShaderRectNocolor : public ShaderPart
	{
	public:
		VertexShaderRectNocolor(const opengl::GLInfo & _glinfo)
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

	class VertexShaderTexturedRect : public ShaderPart
	{
	public:
		VertexShaderTexturedRect(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN highp vec4 aRectPosition;	\n"
				"IN highp vec2 aTexCoord0;		\n"
				"OUT mediump vec2 vTexCoord0;	\n"
				"void main()					\n"
				"{								\n"
				"  gl_Position = aRectPosition;	\n"
				"  vTexCoord0 = aTexCoord0;		\n"
				"}								\n"
			;
		}
	};

	/*---------------ShadowMapShaderPart-------------*/

	class ShadowMapFragmentShader : public ShaderPart
	{
	public:
		ShadowMapFragmentShader(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"layout(binding = 0, r32ui) highp uniform readonly uimage2D uZlutImage;\n"
				"layout(binding = 1, r32ui) highp uniform readonly uimage2D uTlutImage;\n"
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

	/*---------------MonochromeShaderPart-------------*/

	class MonochromeFragmentShader : public ShaderPart
	{
	public:
		MonochromeFragmentShader(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"uniform sampler2D uColorImage;									\n"
					"uniform mediump vec2 uScreenSize;								\n"
					"void main()													\n"
					"{																\n"
					"  mediump vec2 coord = gl_FragCoord.xy/uScreenSize;			\n"
					"  lowp vec4 tex = texture2D(uColorImage, coord);				\n"
					"  lowp float c = dot(vec4(0.2126, 0.7152, 0.0722, 0.0), tex);	\n"
					"  gl_FragColor = vec4(c, c, c, 1.0);							\n"
					"}																\n"
					;
			} else {
				if (config.video.multisampling > 0) {
					m_part =
						"uniform lowp sampler2DMS uColorImage;					\n"
						"uniform lowp int uMSAASamples;							\n"
						"OUT lowp vec4 fragColor;								\n"
						"lowp vec4 sampleMS()									\n"
						"{														\n"
						"  mediump ivec2 coord = ivec2(gl_FragCoord.xy);		\n"
						"  lowp vec4 texel = vec4(0.0);							\n"
						"  for (int i = 0; i < uMSAASamples; ++i)				\n"
						"    texel += texelFetch(uColorImage, coord, i);		\n"
						"  return texel / float(uMSAASamples);					\n"
						"}														\n"
						"														\n"
						"void main()											\n"
						"{														\n"
						"  lowp vec4 tex = sampleMS();							\n"
						//"  lowp float c = (tex.r + tex.g + tex.b) / 3.0f;		\n"
						"  lowp float c = dot(vec4(0.2126, 0.7152, 0.0722, 0.0), tex);\n"
						"  fragColor = vec4(c, c, c, 1.0);						\n"
						"}														\n"
						;
				} else {
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
			}
		}
	};

	/*---------------TexrectDrawerShaderPart-------------*/

	class TexrectDrawerTex3PointFilter : public ShaderPart
	{
	public:
		TexrectDrawerTex3PointFilter(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"#if (__VERSION__ > 120)																						\n"
					"# define IN in																									\n"
					"# define OUT out																								\n"
					"#else																											\n"
					"# define IN varying																							\n"
					"# define OUT																									\n"
					"#endif // __VERSION __																							\n"
					"uniform mediump vec4 uTextureBounds;																			\n"
					"uniform mediump vec2 uTextureSize;																				\n"
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					"#define TEX_OFFSET(off) texture2D(tex, texCoord - (off)/texSize)												\n"
					"lowp vec4 texFilter(in sampler2D tex, in mediump vec2 texCoord)												\n"
					"{																												\n"
					"  mediump vec2 texSize = uTextureSize;																			\n"
					"  mediump vec2 texelSize = vec2(1.0) / texSize;																\n"
					"  lowp vec4 c = texture2D(tex, texCoord);		 																\n"
					"  if (abs(texCoord.s - uTextureBounds[0]) < texelSize.x || abs(texCoord.s - uTextureBounds[2]) < texelSize.x) return c;	\n"
					"  if (abs(texCoord.t - uTextureBounds[1]) < texelSize.y || abs(texCoord.t - uTextureBounds[3]) < texelSize.y) return c;	\n"
					"																												\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\n"
					"  offset -= step(1.0, offset.x + offset.y);																	\n"
					"  lowp vec4 zero = vec4(0.0);					 																\n"
					"  lowp vec4 c0 = TEX_OFFSET(offset);																			\n"
					"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));										\n"
					"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));										\n"
					"  return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);													\n"
					"}																												\n"
					"																												\n"
				;
			} else {
				m_part =
					"uniform mediump vec4 uTextureBounds;																			\n"
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					"#define TEX_OFFSET(off, tex, texCoord, texSize) texture(tex, texCoord - (off)/texSize)							\n"
					"#define TEX_FILTER(name, tex, texCoord)																		\\\n"
					"{																												\\\n"
					"  mediump vec2 texSize = vec2(textureSize(tex,0));																\\\n"
					"  mediump vec2 texelSize = vec2(1.0) / texSize;																\\\n"
					"  lowp vec4 c = texture(tex, texCoord);		 																\\\n"
					"  if (abs(texCoord.s - uTextureBounds[0]) < texelSize.x || abs(texCoord.s - uTextureBounds[2]) < texelSize.x){	\\\n"
					"    name = c;												 													\\\n"
					"  } else if (abs(texCoord.t - uTextureBounds[1]) < texelSize.y || abs(texCoord.t - uTextureBounds[3]) < texelSize.y){	\\\n"
					"    name = c;												 													\\\n"
					"  } else {											 															\\\n"
					"    mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
					"    offset -= step(1.0, offset.x + offset.y);																	\\\n"
					"    lowp vec4 zero = vec4(0.0);					 															\\\n"
					"    lowp vec4 c0 = TEX_OFFSET(offset, tex, texCoord, texSize);													\\\n"
					"    lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, texSize);				\\\n"
					"    lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, texSize);				\\\n"
					"    name = c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);													\\\n"
					"  }																											\\\n"
					"}																												\\\n"
					"																											    \n"
					;
			}
		}
	};

	class TexrectDrawerTexBilinearFilter : public ShaderPart
	{
	public:
		TexrectDrawerTexBilinearFilter(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"#if (__VERSION__ > 120)																						\n"
					"# define IN in																									\n"
					"# define OUT out																								\n"
					"#else																											\n"
					"# define IN varying																							\n"
					"# define OUT																									\n"
					"#endif // __VERSION __																							\n"
					"uniform mediump vec4 uTextureBounds;																			\n"
					"uniform mediump vec2 uTextureSize;																				\n"
					"#define TEX_OFFSET(off) texture2D(tex, texCoord - (off)/texSize)												\n"
					"lowp vec4 texFilter(in sampler2D tex, in mediump vec2 texCoord)												\n"
					"{																												\n"
					"  mediump vec2 texSize = uTextureSize;																			\n"
					"  mediump vec2 texelSize = vec2(1.0) / texSize;																\n"
					"  lowp vec4 c = texture2D(tex, texCoord);																		\n"
					"  if (abs(texCoord.s - uTextureBounds[0]) < texelSize.x || abs(texCoord.s - uTextureBounds[2]) < texelSize.x) return c;	\n"
					"  if (abs(texCoord.t - uTextureBounds[1]) < texelSize.y || abs(texCoord.t - uTextureBounds[3]) < texelSize.y) return c;	\n"
					"																												\n"
					"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\n"
					"  offset -= step(1.0, offset.x + offset.y);																	\n"
					"  lowp vec4 zero = vec4(0.0);																					\n"
					"																												\n"
					"  lowp vec4 p0q0 = TEX_OFFSET(offset);																			\n"
					"  lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));										\n"
					"																												\n"
					"  lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));				                        \n"
					"  lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)));						\n"
					"																												\n"
					"  mediump vec2 interpolationFactor = abs(offset);																\n"
					"  lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); // Interpolates top row in X direction.		\n"
					"  lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); // Interpolates bottom row in X direction.	\n"
					"  return mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); // Interpolate in Y direction.					\n"
					"}																												\n"
					;
			}
			else {
				m_part =
					"uniform mediump vec4 uTextureBounds;																			\n"
					"#define TEX_OFFSET(off, tex, texCoord, texSize) texture(tex, texCoord - (off)/texSize)							\n"
					"#define TEX_FILTER(name, tex, texCoord)																		\\\n"
					"{																												\\\n"
					"  mediump vec2 texSize = vec2(textureSize(tex,0));																\\\n"
					"  mediump vec2 texelSize = vec2(1.0) / texSize;																\\\n"
					"  lowp vec4 c = texture(tex, texCoord);																		\\\n"
					"  if (abs(texCoord.s - uTextureBounds[0]) < texelSize.x || abs(texCoord.s - uTextureBounds[2]) < texelSize.x){	\\\n"
					"    name = c;												 													\\\n"
					"  } else if (abs(texCoord.t - uTextureBounds[1]) < texelSize.y || abs(texCoord.t - uTextureBounds[3]) < texelSize.y){	\\\n"
					"    name = c;												 													\\\n"
					"  } else {													 													\\\n"
					"    mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));													\\\n"
					"    offset -= step(1.0, offset.x + offset.y);																	\\\n"
					"    lowp vec4 zero = vec4(0.0);																				\\\n"
					"																												\\\n"
					"    lowp vec4 p0q0 = TEX_OFFSET(offset, tex, texCoord, texSize);												\\\n"
					"    lowp vec4 p1q0 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y), tex, texCoord, texSize);			\\\n"
					"																												\\\n"
					"    lowp vec4 p0q1 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)), tex, texCoord, texSize);			 \\\n"
					"    lowp vec4 p1q1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y - sign(offset.y)), tex, texCoord, texSize);	\\\n"
					"																												\\\n"
					"    mediump vec2 interpolationFactor = abs(offset);															\\\n"
					"    lowp vec4 pInterp_q0 = mix( p0q0, p1q0, interpolationFactor.x ); 											\\\n" // Interpolates top row in X direction.
					"    lowp vec4 pInterp_q1 = mix( p0q1, p1q1, interpolationFactor.x ); 											\\\n" // Interpolates bottom row in X direction.
					"    name = mix( pInterp_q0, pInterp_q1, interpolationFactor.y ); 												\\\n" // Interpolate in Y direction.
					"  }																											\\\n"
					"}																												\\\n"
					"																												\n"
				;
			}
		}
	};

	class TexrectDrawerFragmentDraw : public ShaderPart
	{
	public:
		TexrectDrawerFragmentDraw(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"uniform sampler2D uTex0;													\n"
					"uniform lowp int uEnableAlphaTest;											\n"
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);			\n"
					"IN mediump vec2 vTexCoord0;												\n"
					"OUT lowp vec4 fragColor;													\n"
					"void main()																\n"
					"{																			\n"
					"  fragColor = texFilter(uTex0, vTexCoord0);								\n"
					"  if (fragColor == uTestColor) discard;									\n"
					"  if (uEnableAlphaTest != 0 && !(fragColor.a > 0.0)) discard;				\n"
					"  gl_FragColor = fragColor;												\n"
					"}																			\n"
				;
			} else {
				m_part =
					"uniform sampler2D uTex0;																						\n"
					"uniform lowp int uEnableAlphaTest;																				\n"
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);												\n"
					"in mediump vec2 vTexCoord0;																					\n"
					"out lowp vec4 fragColor;																						\n"
					"void main()																									\n"
					"{																												\n"
					"  TEX_FILTER(fragColor, uTex0, vTexCoord0);																	\n"
					"  if (fragColor == uTestColor) discard;																		\n"
					"  if (uEnableAlphaTest == 1 && !(fragColor.a > 0.0)) discard;													\n"
					"}																												\n"
				;
			}
		}
	};

	class TexrectDrawerFragmentClear : public ShaderPart
	{
	public:
		TexrectDrawerFragmentClear(const opengl::GLInfo & _glinfo)
		{
			if (_glinfo.isGLES2) {
				m_part =
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);	\n"
					"void main()														\n"
					"{																	\n"
					"  gl_FragColor = uTestColor;										\n"
					"}																	\n"
				;
			} else {
				m_part =
					"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);	\n"
					"out lowp vec4 fragColor;													\n"
					"void main()																\n"
					"{																			\n"
					"  fragColor = uTestColor;													\n"
					"}																			\n"
				;
			}
		}
	};

	/*---------------TexrectCopyShaderPart-------------*/

	class TexrectCopy : public ShaderPart
	{
	public:
		TexrectCopy(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;							\n"
				"uniform sampler2D uTex0;								\n"
				"OUT lowp vec4 fragColor;								\n"
				"														\n"
				"void main()											\n"
				"{														\n"
				"	fragColor = texture2D(uTex0, vTexCoord0);			\n"
			;
		}
	};

	/*---------------PostProcessorShaderPart-------------*/

	class GammaCorrection : public ShaderPart
	{
	public:
		GammaCorrection(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;													\n"
				"uniform sampler2D uTex0;													\n"
				"uniform lowp float uGammaCorrectionLevel;									\n"
				"OUT lowp vec4 fragColor;													\n"
				"void main()																\n"
				"{																			\n"
				"    fragColor = texture2D(uTex0, vTexCoord0);								\n"
				"    fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / uGammaCorrectionLevel));	\n"
				;
		}
	};

	class OrientationCorrection : public ShaderPart
	{
	public:
		OrientationCorrection(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;													\n"
				"uniform sampler2D uTex0;													\n"
				"OUT lowp vec4 fragColor;													\n"
				"void main()																\n"
				"{																			\n"
				"    fragColor = texture2D(uTex0, vec2(1.0 - vTexCoord0.x, 1.0 - vTexCoord0.y));       \n"
			;
		}
	};

	/*---------------TextDrawerShaderPart-------------*/

	class TextDraw : public ShaderPart
	{
	public:
		TextDraw(const opengl::GLInfo & _glinfo)
		{
			m_part =
				"IN mediump vec2 vTexCoord0;							\n"
				"uniform sampler2D uTex0;								\n"
				"uniform lowp vec4 uColor;								\n"
				"OUT lowp vec4 fragColor;								\n"
				"														\n"
				"void main()											\n"
				"{														\n"
				"  fragColor = texture2D(uTex0, vTexCoord0).r * uColor;		\n"
			;
		}
	};

	/*---------------SpecialShader-------------*/

	template<class VertexBody, class FragmentBody>
	class SpecialShader : public graphics::ShaderProgram
	{
	public:
		SpecialShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd = nullptr)
			: m_program(0)
			, m_useProgram(_useProgram)
		{
			VertexBody vertexBody(_glinfo);
			FragmentBody fragmentBody(_glinfo);

			std::stringstream ssVertexShader;
			_vertexHeader->write(ssVertexShader);
			vertexBody.write(ssVertexShader);

			std::stringstream ssFragmentShader;
			_fragmentHeader->write(ssFragmentShader);
			fragmentBody.write(ssFragmentShader);
			if (_fragmentEnd != nullptr)
				_fragmentEnd->write(ssFragmentShader);

			m_program =
				graphics::ObjectHandle(Utils::createRectShaderProgram(ssVertexShader.str().data(), ssFragmentShader.str().data()));
		}

		~SpecialShader()
		{
			m_useProgram->useProgram(graphics::ObjectHandle::null);
			glDeleteProgram(GLuint(m_program));
		}

		void activate() override {
			m_useProgram->useProgram(m_program);
			gDP.changed |= CHANGED_COMBINE;
		}

	protected:
		graphics::ObjectHandle m_program;
		opengl::CachedUseProgram * m_useProgram;
	};

	/*---------------ShadowMapShader-------------*/

	typedef SpecialShader<VertexShaderRectNocolor, ShadowMapFragmentShader> ShadowMapShaderBase;

	class ShadowMapShader : public ShadowMapShaderBase
	{
	public:
		ShadowMapShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: ShadowMapShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader)
			, m_loc(-1)
		{
			m_useProgram->useProgram(m_program);
			m_loc = glGetUniformLocation(GLuint(m_program), "uFogColor");
			m_useProgram->useProgram(graphics::ObjectHandle::null);
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

	typedef SpecialShader<VertexShaderRectNocolor, MonochromeFragmentShader> MonochromeShaderBase;

	class MonochromeShader : public MonochromeShaderBase
	{
	public:
		MonochromeShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: MonochromeShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uColorImage");
			if (config.video.multisampling > 0) {
				glUniform1i(texLoc, u32(graphics::textureIndices::MSTex[0]));
				const int samplesLoc = glGetUniformLocation(GLuint(m_program), "uMSAASamples");
				glUniform1i(samplesLoc, config.video.multisampling);
			} else
				glUniform1i(texLoc, u32(graphics::textureIndices::Tex[0]));
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	/*---------------TexrectDrawerShader-------------*/

	class TexrectDrawerShaderDraw : public graphics::TexrectDrawerShaderProgram
	{
	public:
		TexrectDrawerShaderDraw(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader)
			: m_program(0)
			, m_useProgram(_useProgram)
		{
			VertexShaderTexturedRect vertexBody(_glinfo);
			std::stringstream ssVertexShader;
			_vertexHeader->write(ssVertexShader);
			vertexBody.write(ssVertexShader);

			std::stringstream ssFragmentShader;
			_fragmentHeader->write(ssFragmentShader);

			if (config.texture.bilinearMode == BILINEAR_STANDARD) {
				TexrectDrawerTexBilinearFilter filter(_glinfo);
				filter.write(ssFragmentShader);
			} else {
				TexrectDrawerTex3PointFilter filter(_glinfo);
				filter.write(ssFragmentShader);
			}

			TexrectDrawerFragmentDraw fragmentMain(_glinfo);
			fragmentMain.write(ssFragmentShader);

			m_program =
				graphics::ObjectHandle(Utils::createRectShaderProgram(ssVertexShader.str().data(), ssFragmentShader.str().data()));

			m_useProgram->useProgram(m_program);
			GLint loc = glGetUniformLocation(GLuint(m_program), "uTex0");
			assert(loc >= 0);
			glUniform1i(loc, 0);
			m_textureSizeLoc = glGetUniformLocation(GLuint(m_program), "uTextureSize");
			m_textureBoundsLoc = glGetUniformLocation(GLuint(m_program), "uTextureBounds");
			assert(m_textureBoundsLoc >= 0);
			m_enableAlphaTestLoc = glGetUniformLocation(GLuint(m_program), "uEnableAlphaTest");
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}

		~TexrectDrawerShaderDraw()
		{
			m_useProgram->useProgram(graphics::ObjectHandle::null);
			glDeleteProgram(GLuint(m_program));
		}

		void activate() override
		{
			m_useProgram->useProgram(m_program);
			gDP.changed |= CHANGED_COMBINE;
		}

		void setTextureSize(u32 _width, u32 _height) override
		{
			if (m_textureSizeLoc < 0)
				return;
			m_useProgram->useProgram(m_program);
			glUniform2f(m_textureSizeLoc, (GLfloat)_width, (GLfloat)_height);
			gDP.changed |= CHANGED_COMBINE;
		}

		void setTextureBounds(float _texBounds[4])  override
		{
			m_useProgram->useProgram(m_program);
			glUniform4fv(m_textureBoundsLoc, 1, _texBounds);
			gDP.changed |= CHANGED_COMBINE;
		}

		void setEnableAlphaTest(int _enable) override
		{
			m_useProgram->useProgram(m_program);
			glUniform1i(m_enableAlphaTestLoc, _enable);
			gDP.changed |= CHANGED_COMBINE;
		}

	protected:
		graphics::ObjectHandle m_program;
		opengl::CachedUseProgram * m_useProgram;
		GLint m_enableAlphaTestLoc;
		GLint m_textureSizeLoc;
		GLint m_textureBoundsLoc;
	};

	typedef SpecialShader<VertexShaderTexturedRect, TexrectDrawerFragmentClear> TexrectDrawerShaderClear;

	/*---------------TexrectCopyShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, TexrectCopy> TexrectCopyShaderBase;

	class TexrectCopyShader : public TexrectCopyShaderBase
	{
	public:
		TexrectCopyShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TexrectCopyShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	/*---------------PostProcessorShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, GammaCorrection> GammaCorrectionShaderBase;

	class GammaCorrectionShader : public GammaCorrectionShaderBase
	{
	public:
		GammaCorrectionShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: GammaCorrectionShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			const int levelLoc = glGetUniformLocation(GLuint(m_program), "uGammaCorrectionLevel");
			assert(levelLoc >= 0);
			const f32 gammaLevel = (config.gammaCorrection.force != 0) ? config.gammaCorrection.level : 2.0f;
			glUniform1f(levelLoc, gammaLevel);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	typedef SpecialShader<VertexShaderTexturedRect, OrientationCorrection> OrientationCorrectionShaderBase;

	class OrientationCorrectionShader : public OrientationCorrectionShaderBase
	{
	public:
		OrientationCorrectionShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: OrientationCorrectionShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	/*---------------TexrectDrawerShader-------------*/

	typedef SpecialShader<VertexShaderTexturedRect, TextDraw> TextDrawerShaderBase;

	class TextDrawerShader : public TextDrawerShaderBase
	{
	public:
		TextDrawerShader(const opengl::GLInfo & _glinfo,
			opengl::CachedUseProgram * _useProgram,
			const ShaderPart * _vertexHeader,
			const ShaderPart * _fragmentHeader,
			const ShaderPart * _fragmentEnd)
			: TextDrawerShaderBase(_glinfo, _useProgram, _vertexHeader, _fragmentHeader, _fragmentEnd)
		{
			m_useProgram->useProgram(m_program);
			const int texLoc = glGetUniformLocation(GLuint(m_program), "uTex0");
			glUniform1i(texLoc, 0);
			const int colorLoc = glGetUniformLocation(GLuint(m_program), "uColor");
			glUniform4fv(colorLoc, 1, config.font.colorf);
			m_useProgram->useProgram(graphics::ObjectHandle::null);
		}
	};

	/*---------------SpecialShadersFactory-------------*/

	SpecialShadersFactory::SpecialShadersFactory(const opengl::GLInfo & _glinfo,
												opengl::CachedUseProgram * _useProgram,
												const ShaderPart * _vertexHeader,
												const ShaderPart * _fragmentHeader,
												const ShaderPart * _fragmentEnd)
		: m_glinfo(_glinfo)
		, m_vertexHeader(_vertexHeader)
		, m_fragmentHeader(_fragmentHeader)
		, m_fragmentEnd(_fragmentEnd)
		, m_useProgram(_useProgram)
	{
	}

	graphics::ShaderProgram * SpecialShadersFactory::createShadowMapShader() const
	{
		if (!m_glinfo.imageTextures)
			return nullptr;

		return new ShadowMapShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createMonochromeShader() const
	{
		return new MonochromeShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::TexrectDrawerShaderProgram * SpecialShadersFactory::createTexrectDrawerDrawShader() const
	{
		return new TexrectDrawerShaderDraw(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectDrawerClearShader() const
	{
		return new TexrectDrawerShaderClear(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTexrectCopyShader() const
	{
		return new TexrectCopyShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createGammaCorrectionShader() const
	{
		return new GammaCorrectionShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createOrientationCorrectionShader() const
	{
		return new OrientationCorrectionShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

	graphics::ShaderProgram * SpecialShadersFactory::createTextDrawerShader() const
	{
		return new TextDrawerShader(m_glinfo, m_useProgram, m_vertexHeader, m_fragmentHeader, m_fragmentEnd);
	}

}
