#include <assert.h>
#include <Log.h>
#include <Config.h>
#include "glsl_Utils.h"
#include "glsl_ShaderPart.h"
#include "glsl_CombinerInputs.h"
#include "glsl_CombinerProgramImpl.h"
#include "glsl_CombinerProgramBuilder.h"
#include "glsl_CombinerProgramUniformFactory.h"

using namespace glsl;

class TextureConvert {
public:
	void setMode(u32 _mode) {
		m_mode = _mode;
	}

	bool getBilerp1() const {
		return (m_mode & 1) != 0;
	}

	bool getBilerp0() const {
		return (m_mode & 2) != 0;
	}

	bool useYUVCoversion() const {
		return (m_mode & 3) != 3;
	}

	bool useTextureFiltering() const {
		return (m_mode & 3) != 0;
	}

private:
	u32 m_mode;
};


u32 g_cycleType = G_CYC_1CYCLE;
TextureConvert g_textureConvert;

/*---------------_compileCombiner-------------*/

static
const char *ColorInput[] = {
	"combined_color.rgb",
	"readtex0.rgb",
	"readtex1.rgb",
	"uPrimColor.rgb",
	"vec_color.rgb",
	"uEnvColor.rgb",
	"uCenterColor.rgb",
	"uScaleColor.rgb",
	"combined_color.a",
	"vec3(readtex0.a)",
	"vec3(readtex1.a)",
	"vec3(uPrimColor.a)",
	"vec3(vec_color.a)",
	"vec3(uEnvColor.a)",
	"vec3(lod_frac)",
	"vec3(uPrimLod)",
	"vec3(0.5 + 0.5*snoise())",
	"vec3(uK4)",
	"vec3(uK5)",
	"vec3(1.0)",
	"vec3(0.0)",
	"vec3(0.5)"
};

static
const char *AlphaInput[] = {
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"uPrimColor.a",
	"vec_color.a",
	"uEnvColor.a",
	"uCenterColor.a",
	"uScaleColor.a",
	"combined_color.a",
	"readtex0.a",
	"readtex1.a",
	"uPrimColor.a",
	"vec_color.a",
	"uEnvColor.a",
	"lod_frac",
	"uPrimLod",
	"0.5 + 0.5*snoise()",
	"uK4",
	"uK5",
	"1.0",
	"0.0",
	"0.5"
};

inline
int correctFirstStageParam(int _param)
{
	switch (_param) {
	case G_GCI_TEXEL1:
		return G_GCI_TEXEL0;
	case G_GCI_TEXEL1_ALPHA:
		return G_GCI_TEXEL0_ALPHA;
	}
	return _param;
}

static
void _correctFirstStageParams(CombinerStage & _stage)
{
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctFirstStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctFirstStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctFirstStageParam(_stage.op[i].param3);
	}
}

inline
int correctFirstStageParam2Cyc(int _param)
{
	switch (_param) {
	case G_GCI_COMBINED:
		return G_GCI_HALF;
	}
	return _param;
}

static
void _correctFirstStageParams2Cyc(CombinerStage & _stage)
{
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctFirstStageParam2Cyc(_stage.op[i].param1);
		_stage.op[i].param2 = correctFirstStageParam2Cyc(_stage.op[i].param2);
		_stage.op[i].param3 = correctFirstStageParam2Cyc(_stage.op[i].param3);
	}
}

inline
int correctSecondStageParam(int _param)
{
	switch (_param) {
	case G_GCI_TEXEL0:
		return G_GCI_TEXEL1;
	case G_GCI_TEXEL1:
		return G_GCI_TEXEL0;
	case G_GCI_TEXEL0_ALPHA:
		return G_GCI_TEXEL1_ALPHA;
	case G_GCI_TEXEL1_ALPHA:
		return G_GCI_TEXEL0_ALPHA;
	}
	return _param;
}

static
void _correctSecondStageParams(CombinerStage & _stage) {
	for (int i = 0; i < _stage.numOps; ++i) {
		_stage.op[i].param1 = correctSecondStageParam(_stage.op[i].param1);
		_stage.op[i].param2 = correctSecondStageParam(_stage.op[i].param2);
		_stage.op[i].param3 = correctSecondStageParam(_stage.op[i].param3);
	}
}

static
CombinerInputs _compileCombiner(const CombinerStage & _stage, const char** _Input, std::stringstream & _strShader) {
	bool bBracketOpen = false;
	CombinerInputs inputs;
	for (int i = 0; i < _stage.numOps; ++i) {
		switch (_stage.op[i].op) {
		case LOAD:
			//			sprintf(buf, "(%s ", _Input[_stage.op[i].param1]);
			_strShader << "(" << _Input[_stage.op[i].param1] << " ";
			bBracketOpen = true;
			inputs.addInput(_stage.op[i].param1);
			break;
		case SUB:
			if (bBracketOpen) {
				//				sprintf(buf, "- %s)", _Input[_stage.op[i].param1]);
				_strShader << "- " << _Input[_stage.op[i].param1] << ")";
				bBracketOpen = false;
			}
			else
				//				sprintf(buf, "- %s", _Input[_stage.op[i].param1]);
				_strShader << "- " << _Input[_stage.op[i].param1];
			//			_strShader += buf;
			inputs.addInput(_stage.op[i].param1);
			break;
		case ADD:
			if (bBracketOpen) {
				//				sprintf(buf, "+ %s)", _Input[_stage.op[i].param1]);
				_strShader << "+ " << _Input[_stage.op[i].param1] << ")";
				bBracketOpen = false;
			}
			else
				//				sprintf(buf, "+ %s", _Input[_stage.op[i].param1]);
				_strShader << "+ " << _Input[_stage.op[i].param1];
			inputs.addInput(_stage.op[i].param1);
			break;
		case MUL:
			if (bBracketOpen) {
				//				sprintf(buf, ")*%s", _Input[_stage.op[i].param1]);
				_strShader << ")*" << _Input[_stage.op[i].param1];
				bBracketOpen = false;
			}
			else
				//				sprintf(buf, "*%s", _Input[_stage.op[i].param1]);
				_strShader << "*" << _Input[_stage.op[i].param1];
			inputs.addInput(_stage.op[i].param1);
			break;
		case INTER:
			//			sprintf(buf, "mix(%s, %s, %s)", _Input[_stage.op[0].param2], _Input[_stage.op[0].param1], _Input[_stage.op[0].param3]);
			_strShader << "mix(" <<
				_Input[_stage.op[0].param2] << "," <<
				_Input[_stage.op[0].param1] << "," <<
				_Input[_stage.op[0].param3] << ")";
			inputs.addInput(_stage.op[i].param1);
			inputs.addInput(_stage.op[i].param2);
			inputs.addInput(_stage.op[i].param3);
			break;

			//			default:
			//				assert(false);
		}
	}
	if (bBracketOpen)
		_strShader << ")";
	_strShader << ";" << std::endl;
	return inputs;
}

/*---------------ShaderParts-------------*/

class VertexShaderHeader : public ShaderPart
{
public:
	VertexShaderHeader(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part = "#version 100			\n";
			m_part +=
				"#if (__VERSION__ > 120)	\n"
				"# define IN in				\n"
				"# define OUT out			\n"
				"#else						\n"
				"# define IN attribute		\n"
				"# define OUT varying		\n"
				"#endif // __VERSION		\n"
				;
		}
		else if (_glinfo.isGLESX) {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 es " << std::endl;
			ss << "# define IN in" << std::endl << "# define OUT out" << std::endl;
			if (_glinfo.noPerspective) {
				ss << "#extension GL_NV_shader_noperspective_interpolation : enable" << std::endl
					<< "noperspective OUT highp float vZCoord;" << std::endl << "uniform lowp int uClampMode;" << std::endl;
			}
			m_part = ss.str();
		}
		else {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 core " << std::endl;
			ss << "# define IN in" << std::endl << "# define OUT out" << std::endl;
			m_part = ss.str();
		}
		m_part += "uniform lowp float uClipRatio; \n";
	}
};

class VertexShaderTexturedTriangle : public ShaderPart
{
public:
	VertexShaderTexturedTriangle(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aPosition;							\n"
			"IN lowp vec4 aColor;								\n"
			"IN highp vec2 aTexCoord;							\n"
			"IN lowp float aNumLights;							\n"
			"IN highp vec4 aModify;								\n"
			"													\n"
			"uniform int uTexturePersp;							\n"
			"uniform lowp int uTextureFilterMode;		\n"
			"													\n"
			"uniform lowp int uFogUsage;						\n"
			"uniform mediump vec2 uFogScale;					\n"
			"uniform mediump vec2 uScreenCoordsScale;			\n"
			"													\n"
			"uniform mediump vec2 uTexScale;					\n"
			"uniform mediump vec2 uTexOffset[2];				\n"
			"uniform mediump vec2 uCacheScale[2];				\n"
			"uniform mediump vec2 uCacheOffset[2];				\n"
			"uniform mediump vec2 uCacheShiftScale[2];			\n"
			"uniform lowp ivec2 uCacheFrameBuffer;				\n"
			"OUT highp vec2 vTexCoord0;							\n"
			"OUT highp vec2 vTexCoord1;							\n"
			"OUT mediump vec2 vLodTexCoord;						\n"
			"OUT lowp float vNumLights;							\n"
			"OUT lowp vec4 vShadeColor;							\n"
		;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"mediump vec2 calcTexCoord(in vec2 texCoord, in int idx)		\n"
			"{																\n"
			"    vec2 texCoordOut = texCoord*uCacheShiftScale[idx];			\n"
			"    texCoordOut -= uTexOffset[idx];							\n"
			"    texCoordOut += uCacheOffset[idx];							\n"
			"    if (uTextureFilterMode != 0 && uCacheFrameBuffer[idx] != 0) \n"
			"      texCoordOut -= vec2(0.5);								\n"
			"    return texCoordOut * uCacheScale[idx];						\n"
			"}																\n"
			"																\n"
			"void main()													\n"
			"{																\n"
			"  gl_Position = aPosition;										\n"
			"  vShadeColor = aColor;										\n"
			"  vShadeColorNoperspective = aColor;							\n"
			"  vec2 texCoord = aTexCoord;									\n"
			"  texCoord *= uTexScale;										\n"
			"  if (uTexturePersp == 0 && aModify[2] == 0.0) texCoord *= 0.5;\n"
			"  vTexCoord0 = calcTexCoord(texCoord, 0);						\n"
			"  vTexCoord1 = calcTexCoord(texCoord, 1);						\n"
			"  vLodTexCoord = texCoord;										\n"
			"  vNumLights = aNumLights;										\n"
			"  if (aModify != vec4(0.0)) {									\n"
			"    if ((aModify[0]) != 0.0) {									\n"
			"      gl_Position.xy = gl_Position.xy * uScreenCoordsScale + vec2(-1.0, 1.0);	\n"
			"      gl_Position.xy *= gl_Position.w;							\n"
			"    }															\n"
			"    if ((aModify[1]) != 0.0)									\n"
			"      gl_Position.z *= gl_Position.w;							\n"
			"    if ((aModify[3]) != 0.0)									\n"
			"      vNumLights = 0.0;										\n"
			"  }															\n"
			"  gl_Position.y = -gl_Position.y;								\n"
			"  if (uFogUsage > 0) {											\n"
			"    lowp float fp;												\n"
			"    if (aPosition.z < -aPosition.w && aModify[1] == 0.0)		\n"
			"      fp = -uFogScale.s + uFogScale.t;							\n"
			"    else														\n"
			"      fp = aPosition.z/aPosition.w*uFogScale.s + uFogScale.t;	\n"
			"    fp = clamp(fp, 0.0, 1.0);									\n"
			"    if (uFogUsage == 1)										\n"
			"      vShadeColor.a = fp;										\n"
			"    else														\n"
			"      vShadeColor.rgb = vec3(fp);								\n"
			"  }															\n"
			;
	}
};

class VertexShaderTriangle : public ShaderPart
{
public:
	VertexShaderTriangle(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aPosition;										\n"
			"IN lowp vec4 aColor;											\n"
			"IN lowp float aNumLights;										\n"
			"IN highp vec4 aModify;											\n"
			"																\n"
			"uniform lowp int uFogUsage;									\n"
			"uniform mediump vec2 uFogScale;								\n"
			"uniform mediump vec2 uScreenCoordsScale;						\n"
			"																\n"
			"OUT lowp float vNumLights;										\n"
			"OUT lowp vec4 vShadeColor;										\n"
		;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"																\n"
			"void main()													\n"
			"{																\n"
			"  gl_Position = aPosition;										\n"
			"  vShadeColor = aColor;										\n"
			"  vShadeColorNoperspective = aColor;							\n"
			"  vNumLights = aNumLights;										\n"
			"  if (aModify != vec4(0.0)) {									\n"
			"    if ((aModify[0]) != 0.0) {									\n"
			"      gl_Position.xy = gl_Position.xy * uScreenCoordsScale + vec2(-1.0, 1.0);	\n"
			"      gl_Position.xy *= gl_Position.w;							\n"
			"    }															\n"
			"    if ((aModify[1]) != 0.0) 									\n"
			"      gl_Position.z *= gl_Position.w;							\n"
			"    if ((aModify[3]) != 0.0)									\n"
			"      vNumLights = 0.0;										\n"
			"  }															\n"
			"  gl_Position.y = -gl_Position.y;								\n"
			"  if (uFogUsage > 0) {											\n"
			"    lowp float fp;												\n"
			"    if (aPosition.z < -aPosition.w && aModify[1] == 0.0)		\n"
			"      fp = -uFogScale.s + uFogScale.t;							\n"
			"    else														\n"
			"      fp = aPosition.z/aPosition.w*uFogScale.s + uFogScale.t;	\n"
			"    fp = clamp(fp, 0.0, 1.0);									\n"
			"    if (uFogUsage == 1)										\n"
			"      vShadeColor.a = fp;										\n"
			"    else														\n"
			"      vShadeColor.rgb = vec3(fp);								\n"
			"  }															\n"
			;
	}
};

class VertexShaderTexturedRect : public ShaderPart
{
public:
	VertexShaderTexturedRect(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aRectPosition;						\n"
			"IN highp vec2 aTexCoord0;							\n"
			"IN highp vec2 aTexCoord1;							\n"
			"													\n"
			"OUT highp vec2 vTexCoord0;							\n"
			"OUT highp vec2 vTexCoord1;							\n"
			"OUT lowp vec4 vShadeColor;							\n"
		;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"uniform lowp vec4 uRectColor;						\n"
			"void main()										\n"
			"{													\n"
			"  gl_Position = aRectPosition;						\n"
			"  vShadeColor = uRectColor;						\n"
			"  vShadeColorNoperspective = uRectColor;			\n"
			"  vTexCoord0 = aTexCoord0;							\n"
			"  vTexCoord1 = aTexCoord1;							\n"
			;
	}
};

class VertexShaderRect : public ShaderPart
{
public:
	VertexShaderRect(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"IN highp vec4 aRectPosition;						\n"
			"													\n"
			"OUT lowp vec4 vShadeColor;							\n"
			;
		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective OUT lowp vec4 vShadeColorNoperspective;\n";
		else
			m_part += "OUT lowp vec4 vShadeColorNoperspective;				\n";
		m_part +=
			"uniform lowp vec4 uRectColor;						\n"
			"void main()										\n"
			"{													\n"
			"  gl_Position = aRectPosition;						\n"
			"  vShadeColor = uRectColor;						\n"
			"  vShadeColorNoperspective = uRectColor;			\n"
			;
	}
};

class VertexShaderEnd : public ShaderPart
{
public:
	VertexShaderEnd(const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLESX) {
			m_part =
				"  gl_ClipDistance[0] = gl_Position.w - gl_Position.z;	\n"
				;
		} else if (config.generalEmulation.enableFragmentDepthWrite != 0 && _glinfo.noPerspective) {
				m_part =
					"  vZCoord = gl_Position.z / gl_Position.w;	\n"
					"  if (uClampMode > 0)	\n"
					"    gl_Position.z = 0.0;	\n"
					;
		}
		m_part +=
			" gl_Position.zw *= vec2(uClipRatio);	 \n"
			"} \n"
			;
	}
};

class FragmentShaderHeader : public ShaderPart
{
public:
	FragmentShaderHeader(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part = "#version 100 \n";
			if (config.generalEmulation.enableLOD) {
				m_part += "#extension GL_EXT_shader_texture_lod : enable \n";
				m_part += "#extension GL_OES_standard_derivatives : enable \n";
			}
			m_part +=
				"#if (__VERSION__ > 120)		\n"
				"# define IN in					\n"
				"# define OUT out				\n"
				"# define texture2D texture		\n"
				"#else							\n"
				"# define IN varying			\n"
				"# define OUT					\n"
				"#endif // __VERSION __			\n"
			;
		} else if (_glinfo.isGLESX) {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 es " << std::endl;
			if (_glinfo.noPerspective)
				ss << "#extension GL_NV_shader_noperspective_interpolation : enable" << std::endl;
			if (_glinfo.dual_source_blending)
				ss << "#extension GL_EXT_blend_func_extended : enable" << std::endl;
			if (config.frameBufferEmulation.N64DepthCompare == Config::dcFast) {
				if (_glinfo.imageTextures && _glinfo.fragment_interlockNV) {
					ss << "#extension GL_NV_fragment_shader_interlock : enable" << std::endl
						<< "layout(pixel_interlock_ordered) in;" << std::endl;
				} else if (_glinfo.ext_fetch)
					ss << "#extension GL_EXT_shader_framebuffer_fetch : enable" << std::endl;
			}
			ss << "# define IN in" << std::endl
				<< "# define OUT out" << std::endl
				<< "# define texture2D texture" << std::endl;
			m_part = ss.str();
		} else {
			std::stringstream ss;
			ss << "#version " << Utils::to_string(_glinfo.majorVersion) << Utils::to_string(_glinfo.minorVersion) << "0 core " << std::endl;
			if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
				if (_glinfo.imageTextures) {
					if (_glinfo.majorVersion * 10 + _glinfo.minorVersion < 42) {
						ss << "#extension GL_ARB_shader_image_load_store : enable" << std::endl
							<< "#extension GL_ARB_shading_language_420pack : enable" << std::endl;
					}
					if (_glinfo.fragment_interlock)
						ss << "#extension GL_ARB_fragment_shader_interlock : enable" << std::endl
							<< "layout(pixel_interlock_ordered) in;" << std::endl;
					else if (_glinfo.fragment_interlockNV)
						ss << "#extension GL_NV_fragment_shader_interlock : enable" << std::endl
							<< "layout(pixel_interlock_ordered) in;" << std::endl;
					else if (_glinfo.fragment_ordering)
						ss << "#extension GL_INTEL_fragment_shader_ordering : enable" << std::endl;
				} else if (_glinfo.ext_fetch)
					ss << "#extension GL_EXT_shader_framebuffer_fetch : enable" << std::endl;
			}
			ss << "# define IN in" << std::endl
				<< "# define OUT out" << std::endl
				<< "# define texture2D texture" << std::endl;
			m_part = ss.str();
		}
		m_part +=
			// Return the vector of the standard basis of R^4 with a 1 at position <pos> and 0 otherwise.
			"  #define STVEC(pos) (step(float(pos), vec4(0.5,1.5,2.5,3.5)) - step(float(pos), vec4(-0.5,0.5,1.5,2.5))) \n";

	}
};

class ShaderBlender1 : public ShaderPart
{
public:
	ShaderBlender1(const opengl::GLInfo & _glinfo)
	{
#if 1
		m_part =
			"  srcColor1 = vec4(0.0);									\n"
			"  dstFactor1 = 0.0;										\n"
			"  muxPM[0] = clampedColor;									\n"
			"  muxA[0] = clampedColor.a;								\n"
			"  muxa = MUXA(uBlendMux1[1]);								\n"
			"  muxB[0] = 1.0 - muxa;									\n"
			"  muxb = MUXB(uBlendMux1[3]);								\n"
			"  muxp = MUXPM(uBlendMux1[0]);								\n"
			"  muxm = MUXPM(uBlendMux1[2]);								\n"
			"  muxaf = MUXF(uBlendMux1[0]);								\n"
			"  muxbf = MUXF(uBlendMux1[2]);								\n"
			"  if (uForceBlendCycle1 != 0) {							\n"
			"    srcColor1 = muxp * muxa + muxm * muxb;					\n"
			"    dstFactor1 = muxaf * muxa + muxbf * muxb;				\n"
			"    srcColor1 = clamp(srcColor1, 0.0, 1.0);				\n"
			"  } else {													\n"
			"    srcColor1 = muxp;										\n"
			"    dstFactor1 = muxaf;									\n"
			"  }														\n"
			;
		if (_glinfo.dual_source_blending) {
			m_part +=
				"  fragColor = srcColor1;								\n"
				"  fragColor1 = vec4(dstFactor1);						\n"
				;
		} else {
			m_part +=
				"  fragColor = vec4(srcColor1.rgb, clampedColor.a);	\n"
				;
		}
#else
		// Keep old code for reference
		m_part =
			"  muxPM[0] = clampedColor;								\n"
			"  if (uForceBlendCycle1 != 0) {						\n"
			"    muxA[0] = clampedColor.a;							\n"
			"    muxB[0] = 1.0 - muxA[uBlendMux1[1]];				\n"
			"    lowp vec4 blend1 = (muxPM[uBlendMux1[0]] * muxA[uBlendMux1[1]]) + (muxPM[uBlendMux1[2]] * muxB[uBlendMux1[3]]);	\n"
			"    clampedColor.rgb = clamp(blend1.rgb, 0.0, 1.0);	\n"
			"  } else clampedColor.rgb = muxPM[uBlendMux1[0]].rgb;	\n"
			;
#endif
	}
};

class ShaderBlender2 : public ShaderPart
{
public:
	ShaderBlender2(const opengl::GLInfo & _glinfo)
	{
#if 1
		m_part =
			"  srcColor2 = vec4(0.0);									\n"
			"  dstFactor2 = 0.0;										\n"
			"  muxPM[0] = srcColor1;									\n"
			"  muxa = MUXA(uBlendMux2[1]);								\n"
			"  muxB[0] = 1.0 - muxa;									\n"
			"  muxb = MUXB(uBlendMux2[3]);								\n"
			"  muxp = MUXPM(uBlendMux2[0]);								\n"
			"  muxm = MUXPM(uBlendMux2[2]);								\n"
			"  muxF[0] = dstFactor1;									\n"
			"  muxaf = MUXF(uBlendMux2[0]);								\n"
			"  muxbf = MUXF(uBlendMux2[2]);								\n"
			"  if (uForceBlendCycle2 != 0) {							\n"
			"    srcColor2 = muxp * muxa + muxm * muxb;					\n"
			"    dstFactor2 = muxaf * muxa + muxbf * muxb;				\n"
			"    srcColor2 = clamp(srcColor2, 0.0, 1.0);				\n"
			"  } else {													\n"
			"    srcColor2 = muxp;										\n"
			"    dstFactor2 = muxaf;									\n"
			"  }														\n"
			;
		if (_glinfo.dual_source_blending) {
			m_part +=
				"  fragColor = srcColor2;								\n"
				"  fragColor1 = vec4(dstFactor2);						\n"
				;
		} else {
			m_part +=
				"  fragColor =  vec4(srcColor2.rgb, clampedColor.a);	\n"
				;
		}

#else
		// Keep old code for reference
		m_part =
			"  muxPM[0] = clampedColor;								\n"
			"  muxPM[1] = vec4(0.0);								\n"
			"  if (uForceBlendCycle2 != 0) {						\n"
			"    muxA[0] = clampedColor.a;							\n"
			"    muxB[0] = 1.0 - muxA[uBlendMux2[1]];				\n"
			"    lowp vec4 blend2 = muxPM[uBlendMux2[0]] * muxA[uBlendMux2[1]] + muxPM[uBlendMux2[2]] * muxB[uBlendMux2[3]];	\n"
			"    clampedColor.rgb = clamp(blend2.rgb, 0.0, 1.0);	\n"
			"  } else clampedColor.rgb = muxPM[uBlendMux2[0]].rgb;	\n"
			;
#endif
	}
};

class ShaderBlenderAlpha : public ShaderPart
{
public:
	ShaderBlenderAlpha(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.dual_source_blending)
		m_part +=
			"if (uBlendAlphaMode != 2) {							\n"
			"  lowp float cvg = clampedColor.a;						\n"
			"  lowp vec4 srcAlpha = vec4(cvg, cvg, 1.0, 0.0);		\n"
			"  lowp vec4 dstFactorAlpha = vec4(1.0, 1.0, 0.0, 1.0);	\n"
			"  if (uBlendAlphaMode == 0)							\n"
			"    dstFactorAlpha[0] = 0.0;							\n"
			"  fragColor.a = srcAlpha[uCvgDest];					\n"
			"  fragColor1.a = dstFactorAlpha[uCvgDest];				\n"
			"} else fragColor.a = clampedColor.a;					\n"
			;
	}
};

class ShaderLegacyBlender : public ShaderPart
{
public:
	ShaderLegacyBlender()
	{
		m_part =
			"  if (uFogUsage == 1) \n"
			"    fragColor.rgb = mix(fragColor.rgb, uFogColor.rgb, vShadeColor.a); \n"
			;
	}
};


/*
// N64 color wrap and clamp on floats
// See https://github.com/gonetz/GLideN64/issues/661 for reference
if (c < -1.0) return c + 2.0;

if (c < -0.5) return 1;

if (c < 0.0) return 0;

if (c > 2.0) return c - 2.0;

if (c > 1.5) return 0;

if (c > 1.0) return 1;

return c;
*/
class ShaderClamp : public ShaderPart
{
public:
	ShaderClamp()
	{
		m_part =
			"  lowp vec4 wrappedColor = WRAP(cmbRes, -0.51, 1.51); \n"
			"  lowp vec4 clampedColor = clamp(wrappedColor, 0.0, 1.0); \n"
			;
	}
};

/*
N64 sign-extension for C component of combiner
if (c > 1.0)
return c - 2.0;

return c;
*/
class ShaderSignExtendColorC : public ShaderPart
{
public:
	ShaderSignExtendColorC()
	{
		m_part =
			" color1 = WRAP(color1, -1.01, 1.01); \n"
			;
	}
};

class ShaderSignExtendAlphaC : public ShaderPart
{
public:
	ShaderSignExtendAlphaC()
	{
		m_part =
			" alpha1 = WRAP(alpha1, -1.01, 1.01); \n"
			;
	}
};

/*
N64 sign-extension for ABD components of combiner
if (c > 1.5)
return c - 2.0;

if (c < -0.5)
return c + 2.0;

return c;
*/
class ShaderSignExtendColorABD : public ShaderPart
{
public:
	ShaderSignExtendColorABD()
	{
		m_part =
			" color1 = WRAP(color1, -0.51, 1.51); \n"
			;
	}
};

class ShaderSignExtendAlphaABD : public ShaderPart
{
public:
	ShaderSignExtendAlphaABD()
	{
		m_part =
			"  alpha1 = WRAP(alpha1, -0.51,1.51); \n"
			;
	}
};

class ShaderAlphaTest : public ShaderPart
{
public:
	ShaderAlphaTest()
	{
		m_part =
			"  if (uEnableAlphaTest != 0) {							\n"
			"    lowp float alphaTestValue = (uAlphaCompareMode == 3) ? snoise() : uAlphaTestValue;	\n"
			"    lowp float alphaValue;								\n"
			"    if ((uAlphaCvgSel != 0) && (uCvgXAlpha == 0)) {	\n"
			"      alphaValue = 0.125;								\n"
			"    } else {											\n"
			"      alphaValue = clamp(alpha1, 0.0, 1.0);			\n"
			"    }													\n"
			"    if (alphaValue < alphaTestValue) discard;			\n"
			"  }													\n"
			;
	}
};

class ShaderDithering : public ShaderPart
{
public:
	ShaderDithering(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2)
			return;

		if (config.generalEmulation.enableDitheringPattern == 0) {
			m_part =
				"  if (uColorDitherMode == 2) {											\n"
				"    colorDither(snoiseRGB(), clampedColor.rgb);						\n"
				"  }																	\n"
				"  if (uAlphaDitherMode == 2) {											\n"
				"    alphaDither(snoiseA(), clampedColor.a);							\n"
				"  }																	\n"
				;
			return;
		}

		m_part +=
			"  lowp mat4 bayer = mat4( 0.0, 0.75, 0.1875, 0.9375,						\n"
			"                          0.5, 0.25, 0.6875, 0.4375,						\n"
			"                          0.125, 0.875, 0.0625, 0.8125,					\n"
			"                          0.625, 0.375, 0.5625, 0.3125);					\n"
			"  lowp mat4 mSquare = mat4( 0.0, 0.6875, 0.75, 0.4375,						\n"
			"                            0.875, 0.3125, 0.125, 0.5625, 					\n"
			"                            0.1875, 0.5, 0.9375, 0.25,						\n"
			"                            0.8125, 0.375, 0.0625, 0.625);					\n"
			// Try to keep dithering visible even at higher resolutions
			"  lowp float divider = 1.0 + step(3.0, uScreenScale.x);					\n"
			"  mediump ivec2 position = ivec2(mod((gl_FragCoord.xy - 0.5) / divider,4.0));\n"
			"  lowp vec4 posX = STVEC(position.x);										\n"
			"  lowp vec4 posY = STVEC(position.y);										\n"
			"  lowp float bayerThreshold = dot(bayer*posY, posX);						\n"
			"  lowp float mSquareThreshold = dot(mSquare*posY, posX);					\n"
			"  switch (uColorDitherMode) {												\n"
			"     case 0:																\n"
			"       colorDither(vec3(mSquareThreshold), clampedColor.rgb);				\n"
			"     break;																\n"
			"     case 1:																\n"
			"       colorDither(vec3(bayerThreshold), clampedColor.rgb);				\n"
			"       break;																\n"
			"     case 2:																\n"
			"       colorDither(snoiseRGB(), clampedColor.rgb);							\n"
			"       break;																\n"
			"     case 3:																\n"
			"       break;																\n"
			"  }																		\n"
			"  switch (uAlphaDitherMode) {												\n"
			"     case 0:																\n"
			"       switch (uColorDitherMode) {											\n"
			"         case 0:															\n"
			"         case 2:															\n"
			"           alphaDither(mSquareThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"         case 1:															\n"
			"         case 3:															\n"
			"           alphaDither(bayerThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"       }																	\n"
			"       break;																\n"
			"     case 1:																\n"
			"       switch (uColorDitherMode) {											\n"
			"         case 0:															\n"
			"         case 2:															\n"
			"           alphaDither(bayerThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"         case 1:															\n"
			"         case 3:															\n"
			"           alphaDither(mSquareThreshold, clampedColor.a);					\n"
			"           break;															\n"
			"       }																	\n"
			"       break;																\n"
			"     case 2:																\n"
			"       alphaDither(snoiseA(), clampedColor.a);								\n"
			"       break;																\n"
			"     case 3:																\n"
			"       break;																\n"
			"  }																		\n"
			;
	}
};

class ShaderFragmentGlobalVariablesTex : public ShaderPart
{
public:
	ShaderFragmentGlobalVariablesTex(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"uniform sampler2D uTex0;		\n"
			"uniform sampler2D uTex1;		\n"
			"uniform lowp vec4 uFogColor;	\n"
			"uniform lowp vec4 uCenterColor;\n"
			"uniform lowp vec4 uScaleColor;	\n"
			"uniform lowp vec4 uBlendColor;	\n"
			"uniform lowp vec4 uEnvColor;	\n"
			"uniform lowp vec4 uPrimColor;	\n"
			"uniform lowp float uPrimLod;	\n"
			"uniform lowp float uK4;		\n"
			"uniform lowp float uK5;		\n"
			"uniform lowp int uAlphaCompareMode;	\n"
			"uniform lowp ivec2 uFbMonochrome;		\n"
			"uniform lowp ivec2 uFbFixedAlpha;		\n"
			"uniform lowp int uEnableAlphaTest;		\n"
			"uniform lowp int uCvgXAlpha;			\n"
			"uniform lowp int uAlphaCvgSel;			\n"
			"uniform lowp float uAlphaTestValue;	\n"
			"uniform lowp int uDepthSource;			\n"
			"uniform highp float uPrimDepth;		\n"
			"uniform mediump vec2 uScreenScale;		\n"
			"uniform highp vec2 uTexClamp0;			\n"
			"uniform highp vec2 uTexClamp1;			\n"
			"uniform highp vec2 uTexWrap0;			\n"
			"uniform highp vec2 uTexWrap1;			\n"
			"uniform lowp vec2 uTexWrapEn0;			\n"
			"uniform lowp vec2 uTexWrapEn1;			\n"
			"uniform lowp vec2 uTexMirrorEn0;		\n"
			"uniform lowp vec2 uTexMirrorEn1;		\n"
			"uniform lowp vec2 uTexClampEn0;		\n"
			"uniform lowp vec2 uTexClampEn1;		\n"
			"uniform lowp int uScreenSpaceTriangle;	\n"
			"highp vec2 texCoord0;					\n"
			"highp vec2 texCoord1;					\n"
			"highp vec2 tcData0[5];					\n"
			"highp vec2 tcData1[5];					\n"
			"uniform lowp int uCvgDest;				\n"
			"uniform lowp int uBlendAlphaMode;		\n"
			;

		if (config.generalEmulation.enableLegacyBlending != 0) {
			m_part +=
				"uniform lowp int uFogUsage;		\n"
			;
		} else {
			m_part +=
				"uniform lowp ivec4 uBlendMux1;		\n"
				"uniform lowp int uForceBlendCycle1;\n"
			;
		}

		if (!_glinfo.isGLES2) {
			m_part +=
				"uniform sampler2D uDepthTex;		\n"
				"uniform lowp int uAlphaDitherMode;	\n"
				"uniform lowp int uColorDitherMode;	\n"
				"uniform lowp int uRenderTarget;	\n"
				"uniform mediump vec2 uDepthScale;	\n"
				;
			if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
				m_part +=
					"uniform lowp int uEnableDepthCompare;	\n"
					;
			}
		} else {
			m_part +=
				"lowp int nCurrentTile;			\n"
			;
		}

		if (config.video.multisampling > 0) {
			m_part +=
				"uniform lowp ivec2 uMSTexEnabled;	\n"
				"uniform lowp sampler2DMS uMSTex0;	\n"
				"uniform lowp sampler2DMS uMSTex1;	\n"
			;
		}

		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective IN lowp vec4 vShadeColorNoperspective;	\n";
		else
			m_part += "IN lowp vec4 vShadeColorNoperspective;				\n";

		m_part +=
			"IN lowp vec4 vShadeColor;		\n"
			"IN highp vec2 vTexCoord0;		\n"
			"IN highp vec2 vTexCoord1;		\n"
			"IN mediump vec2 vLodTexCoord;	\n"
			"IN lowp float vNumLights;		\n"
		;

		if (_glinfo.dual_source_blending) {
			m_part +=
				"layout(location = 0, index = 0) OUT lowp vec4 fragColor; 	\n"
				"layout(location = 0, index = 1) OUT lowp vec4 fragColor1;	\n"
			;
		} else {
			m_part +=
				"OUT lowp vec4 fragColor;	\n"
			;
		}

		if (config.frameBufferEmulation.N64DepthCompare == Config::dcFast && _glinfo.ext_fetch) {
			m_part +=
				"layout(location = 1) inout highp vec4 depthZ;	\n"
				"layout(location = 2) inout highp vec4 depthDeltaZ;	\n"
				;
		}
	}
};

class ShaderFragmentGlobalVariablesNotex : public ShaderPart
{
public:
	ShaderFragmentGlobalVariablesNotex(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"uniform lowp vec4 uFogColor;	\n"
			"uniform lowp vec4 uCenterColor;\n"
			"uniform lowp vec4 uScaleColor;	\n"
			"uniform lowp vec4 uBlendColor;	\n"
			"uniform lowp vec4 uEnvColor;	\n"
			"uniform lowp vec4 uPrimColor;	\n"
			"uniform lowp float uPrimLod;	\n"
			"uniform lowp float uK4;		\n"
			"uniform lowp float uK5;		\n"
			"uniform lowp int uAlphaCompareMode;	\n"
			"uniform lowp ivec2 uFbMonochrome;		\n"
			"uniform lowp ivec2 uFbFixedAlpha;		\n"
			"uniform lowp int uEnableAlphaTest;		\n"
			"uniform lowp int uCvgXAlpha;			\n"
			"uniform lowp int uAlphaCvgSel;			\n"
			"uniform lowp float uAlphaTestValue;	\n"
			"uniform lowp int uDepthSource;			\n"
			"uniform highp float uPrimDepth;		\n"
			"uniform mediump vec2 uScreenScale;		\n"
			"uniform lowp int uScreenSpaceTriangle;	\n"
			"uniform lowp int uCvgDest; \n"
			"uniform lowp int uBlendAlphaMode; \n"

		;

		if (config.generalEmulation.enableLegacyBlending != 0) {
			m_part +=
				"uniform lowp int uFogUsage;		\n"
			;
		} else {
			m_part +=
				"uniform lowp ivec4 uBlendMux1;		\n"
				"uniform lowp int uForceBlendCycle1;\n"
			;
		}

		if (!_glinfo.isGLES2) {
			m_part +=
				"uniform sampler2D uDepthTex;		\n"
				"uniform lowp int uAlphaDitherMode;	\n"
				"uniform lowp int uColorDitherMode;	\n"
				"uniform lowp int uRenderTarget;	\n"
				"uniform mediump vec2 uDepthScale;	\n"
				;
			if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
				m_part +=
					"uniform lowp int uEnableDepthCompare;	\n"
					;
			}
		} else {
			m_part +=
				"lowp int nCurrentTile;			\n"
				;
		}

		if (!_glinfo.isGLESX || _glinfo.noPerspective)
			m_part += "noperspective IN lowp vec4 vShadeColorNoperspective;	\n";
		else
			m_part += "IN lowp vec4 vShadeColorNoperspective;				\n";

		m_part +=
			"IN lowp vec4 vShadeColor;	\n"
			"IN lowp float vNumLights;	\n"
		;

		if (_glinfo.dual_source_blending) {
			m_part +=
				"layout(location = 0, index = 0) OUT lowp vec4 fragColor; 	\n"
				"layout(location = 0, index = 1) OUT lowp vec4 fragColor1;	\n"
			;
		} else {
			m_part +=
				"OUT lowp vec4 fragColor;	\n"
			;
		}

		if (config.frameBufferEmulation.N64DepthCompare == Config::dcFast && _glinfo.ext_fetch) {
			m_part +=
				"layout(location = 1) inout highp vec4 depthZ;	\n"
				"layout(location = 2) inout highp vec4 depthDeltaZ;	\n"
				;
		}
	}
};

class ShaderFragmentHeaderNoise : public ShaderPart
{
public:
	ShaderFragmentHeaderNoise(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"lowp float snoise();\n"
		;
	}
};

class ShaderFragmentHeaderWriteDepth : public ShaderPart
{
public:
	ShaderFragmentHeaderWriteDepth(const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			m_part =
				"highp float writeDepth();\n";
			;
			if (_glinfo.isGLESX &&  _glinfo.noPerspective) {
				m_part =
					"noperspective IN highp float vZCoord;	\n"
					"uniform lowp float uPolygonOffset;	\n"
					"uniform lowp int uClampMode;	\n"
					+ m_part
				;
			}
		}
	}
};

class ShaderFragmentHeaderCalcLight : public ShaderPart
{
public:
	ShaderFragmentHeaderCalcLight(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color);\n";
			;
	}
};

class ShaderFragmentHeaderMipMap : public ShaderPart
{
public:
	ShaderFragmentHeaderMipMap(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1);\n";
		;
	}
};

class ShaderFragmentHeaderTextureEngine : public ShaderPart
{
public:
	ShaderFragmentHeaderTextureEngine(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"highp vec2 clampWrapMirror(in highp vec2 vTexCoord,	\n"
			"	in highp vec2 vWrap, in highp vec2 vClamp,			\n"
			"	in lowp vec2 vClampEn, in lowp vec2 vMirrorEn );	\n"
			"void textureEngine0(in highp vec2 texCoord, out highp vec2 tcData[5]); \n"
			"void textureEngine1(in highp vec2 texCoord, out highp vec2 tcData[5]); \n"
			;
	}
};

class ShaderFragmentHeaderReadMSTex : public ShaderPart
{
public:
	ShaderFragmentHeaderReadMSTex(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		if (!m_glinfo.isGLES2 &&
			config.video.multisampling > 0 &&
			(g_cycleType == G_CYC_COPY || g_textureConvert.useTextureFiltering()))
		{
			shader <<
				"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha);\n";
		}
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentHeaderDither : public ShaderPart
{
public:
	ShaderFragmentHeaderDither(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2)
			return;

		m_part =
			"void colorDither(in lowp vec3 _threshold, inout lowp vec3 _color);\n"
			"void alphaDither(in lowp float _threshold, inout lowp float _alpha);\n"
			"lowp vec3 snoiseRGB();\n"
			"lowp float snoiseA();\n"
			;
	}
};

class ShaderFragmentHeaderDepthCompare : public ShaderPart
{
public:
	ShaderFragmentHeaderDepthCompare(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part =
				"bool depth_compare(highp float curZ);	\n"
				"bool depth_render(highp float Z, highp float curZ);	\n"
				;
			if (_glinfo.imageTextures) {
				m_part +=
					"layout(binding = 2, r32f) highp uniform restrict image2D uDepthImageZ;		\n"
					"layout(binding = 3, r32f) highp uniform restrict image2D uDepthImageDeltaZ;	\n"
					;
			}
		}
	}
};

class ShaderFragmentHeaderReadTex : public ShaderPart
{
public:
	ShaderFragmentHeaderReadTex(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;
		if (!m_glinfo.isGLES2) {

			if (g_textureConvert.useTextureFiltering()) {
				shaderPart += "uniform lowp int uTextureFilterMode;								\n";
				shaderPart += "#define TEX_NEAREST(name, tex, tcData)							\\\n"
					"{																			\\\n"
					" name = texelFetch(tex, ivec2(tcData[0]), 0); \\\n"
					"}																			\n"
					;
				switch (config.texture.bilinearMode + config.texture.enableHalosRemoval * 2) {
				case BILINEAR_3POINT:
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)												\\\n"
						"  {																					\\\n"
						"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);					\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0); \\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0); \\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0); \\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0); \\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);			\\\n"
						"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11); \\\n"
						"  name = c0 + bottomRightTri * (c1-c0); \\\n"
						"  }																					\n"
						;
				break;
				case BILINEAR_STANDARD:
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)																		\\\n"
						"{																											\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0); \\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0); \\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0); \\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0); \\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);						\\\n"
						"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);						\\\n"
						"  name = c0 + tcData[4].t * (c1-c0);									\\\n"
						"}																												\n"
						;
				break;
				case BILINEAR_3POINT_WITH_COLOR_BLEEDING:
					// 3 point texture filtering.
					// Original author: ArthurCarvalho
					// GLSL implementation: twinaphex, mupen64plus-libretro project.
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)												\\\n"
						"{																						\\\n"
						"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);					\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0);								\\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0);								\\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0);								\\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0);								\\\n"
						"  if(uEnableAlphaTest == 1 ){															\\\n" // Calculate premultiplied color values
						"    c00.rgb *= c00.a;																	\\\n"
						"    c01.rgb *= c01.a;																	\\\n"
						"    c10.rgb *= c10.a;																	\\\n"
						"    c11.rgb *= c11.a;																	\\\n"
						"  }																					\\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);				\\\n"
						"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11);	\\\n"
						"  name = c0 + bottomRightTri * (c1-c0); \\\n"
						"  if(uEnableAlphaTest == 1 ) name.rgb /= name.a;										\\\n" // Divide alpha to get actual color value
						"}																						\n"
						;
				break;
				case BILINEAR_STANDARD_WITH_COLOR_BLEEDING_AND_PREMULTIPLIED_ALPHA:
					shaderPart +=
						"#define TEX_FILTER(name, tex, tcData)																	\\\n"
						"{																										\\\n"
						"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]), 0);												\\\n"
						"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]), 0);												\\\n"
						"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]), 0);												\\\n"
						"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]), 0);												\\\n"
						"  if(uEnableAlphaTest == 1){																			\\\n" // Calculate premultiplied color values
						"    c00.rgb *= c00.a;																					\\\n"
						"    c01.rgb *= c01.a;																					\\\n"
						"    c10.rgb *= c10.a;																					\\\n"
						"    c11.rgb *= c11.a;																					\\\n"
						"  }																									\\\n"
						"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);														\\\n"
						"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);														\\\n"
						"  name = c0 + tcData[4].t * (c1-c0);																	\\\n"
						"  if(uEnableAlphaTest == 1)  name.rgb /= name.a;														\\\n"
						"}																										\n"
						;
				break;
				}
				shaderPart +=
					"#define READ_TEX(name, tex, tcData, fbMonochrome, fbFixedAlpha)	\\\n"
					"  {																\\\n"
					"  if (fbMonochrome == 3) {											\\\n"
					"    mediump ivec2 coord = ivec2(gl_FragCoord.xy);					\\\n"
					"    name = texelFetch(tex, coord, 0);								\\\n"
					"  } else {															\\\n"
					"    if (uTextureFilterMode == 0)									\\\n"
					"  {																\\\n"
					"    TEX_NEAREST(name, tex, tcData);								\\\n"
					"  }																\\\n"
					"    else TEX_FILTER(name, tex, tcData);			 				\\\n"
					"  }																\\\n"
					"  if (fbMonochrome == 1) name = vec4(name.r);						\\\n"
					"  else if (fbMonochrome == 2) 										\\\n"
					"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
					"  else if (fbMonochrome == 3) { 									\\\n"
					"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
					"    name.a = 0.0;													\\\n"
					"  }																\\\n"
					"  if (fbFixedAlpha == 1) name.a = 0.825;							\\\n"
					"  }																\n"
					;
			}

			if (g_textureConvert.useYUVCoversion()) {
				shaderPart +=
					"uniform lowp ivec2 uTextureFormat;									\n"
					"uniform lowp int uTextureConvert;									\n"
					"uniform mediump ivec4 uConvertParams;								\n"
					"#define YUVCONVERT(name, format)									\\\n"
					"  mediump ivec4 icolor = ivec4(name*255.0);						\\\n"
					"  if (format == 1)													\\\n"
					"    icolor.rg -= 128;												\\\n"
					"  mediump ivec4 iconvert;											\\\n"
					"  iconvert.r = icolor.b + (uConvertParams[0]*icolor.g + 128)/256;	\\\n"
					"  iconvert.g = icolor.b + (uConvertParams[1]*icolor.r + uConvertParams[2]*icolor.g + 128)/256;	\\\n"
					"  iconvert.b = icolor.b + (uConvertParams[3]*icolor.r + 128)/256;	\\\n"
					"  iconvert.a = icolor.b;											\\\n"
					"  name = vec4(iconvert)/255.0;										\n"
					"#define YUVCONVERT_TEX0(name, tex, tcData, format)				\\\n"
					"  {																\\\n"
					"  name = texelFetch(tex, ivec2(tcData[0]), 0);									\\\n"
					"  YUVCONVERT(name, format)											\\\n"
					"  }																\n"
					"#define YUVCONVERT_TEX1(name, tex, tcData, format, prev)			\\\n"
					"  {																\\\n"
					"  if (uTextureConvert != 0) name = prev;							\\\n"
					"  else name = texelFetch(tex, ivec2(tcData[0]), 0);								\\\n"
					"  YUVCONVERT(name, format)											\\\n"
					"  }																\n"
					;
			}

		} else {
			if (g_textureConvert.useTextureFiltering()) {
				shaderPart +=
					"uniform lowp int uTextureFilterMode;								\n"
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha);	\n"
					;
			}
			if (g_textureConvert.useYUVCoversion()) {
				shaderPart +=
					"uniform lowp ivec2 uTextureFormat;									\n"
					"uniform lowp int uTextureConvert;									\n"
					"uniform mediump ivec4 uConvertParams;								\n"
					"lowp vec4 YUV_Convert(in sampler2D tex, in highp vec2 tcData[5], in lowp int convert, in lowp int format, in lowp vec4 prev);	\n"
					;
			}
		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentHeaderReadTexCopyMode : public ShaderPart
{
public:
	ShaderFragmentHeaderReadTexCopyMode (const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			m_part =
				"#define READ_TEX(name, tex, tcData, fbMonochrome, fbFixedAlpha)	\\\n"
				"  {																\\\n"
				"  if (fbMonochrome == 3) {											\\\n"
				"    mediump ivec2 coord = ivec2(gl_FragCoord.xy);					\\\n"
				"    name = texelFetch(tex, coord, 0);								\\\n"
				"  } else {															\\\n"
				"    name = texelFetch(tex, ivec2(tcData[0]),0);					\\\n"
				"  }																\\\n"
				"  if (fbMonochrome == 1) name = vec4(name.r);						\\\n"
				"  else if (fbMonochrome == 2) 										\\\n"
				"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
				"  else if (fbMonochrome == 3) { 									\\\n"
				"    name.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), name.rgb));	\\\n"
				"    name.a = 0.0;													\\\n"
				"  }																\\\n"
				"  if (fbFixedAlpha == 1) name.a = 0.825;							\\\n"
				"  }																\n"
				;
		} else {
			m_part =
				"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha);	\n"
			;
		}
	}
};

class ShaderFragmentMain : public ShaderPart
{
public:
	ShaderFragmentMain(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"void main() \n"
			"{			 \n"
			;
		if (!_glinfo.isGLES2) {
			m_part +=
				"  highp float fragDepth = writeDepth();	\n"
				;
		}
		m_part +=
			"  lowp vec4 vec_color;				\n"
			"  lowp float alpha1;				\n"
			"  lowp vec3 color1, input_color;	\n"
			"  lowp vec4 shadeColor = uScreenSpaceTriangle == 0 ? vShadeColor : vShadeColorNoperspective;	\n"
		;
		m_part += "#define WRAP(x, low, high) mod((x)-(low), (high)-(low)) + (low) \n"; // Return wrapped value of x in interval [low, high)
		// m_part += "#define WRAP(x, low, high) (x) - ((high)-(low)) * floor(((x)-(low))/((high)-(low)))  \n"; // Perhaps more compatible?
		// m_part += "#define WRAP(x, low, high) (x) + ((high)-(low)) * (1.0-step(low,x)) - ((high)-(low)) * step(high,x) \n"; // Step based version. Only wraps correctly if input is in the range [low-(high-low), high + (high-low)). Similar to old code.
	}

};

class ShaderFragmentMain2Cycle : public ShaderPart
{
public:
	ShaderFragmentMain2Cycle(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"void main() \n"
			"{			 \n"
		;
		if (!_glinfo.isGLES2) {
			m_part +=
				"  highp float fragDepth = writeDepth(); \n"
			;
		}
		m_part +=
			"  lowp vec4 vec_color, combined_color;		\n"
			"  lowp float alpha1, alpha2;				\n"
			"  lowp vec3 color1, color2, input_color;	\n"
			"  lowp vec4 shadeColor = uScreenSpaceTriangle == 0 ? vShadeColor : vShadeColorNoperspective;	\n"
		;
		m_part += "#define WRAP(x, low, high) mod((x)-(low), (high)-(low)) + (low) \n"; // Return wrapped value of x in interval [low, high)
		// m_part += "#define WRAP(x, low, high) (x) - ((high)-(low)) * floor(((x)-(low))/((high)-(low)))  \n"; // Perhaps more compatible?
		// m_part += "#define WRAP(x, low, high) (x) + (2.0) * (1.0-step(low,x)) - (2.0) * step(high,x) \n"; // Step based version. Only wraps correctly if input is in the range [low-(high-low), high + (high-low)). Similar to old code.
	}
};

class ShaderFragmentBlendMux : public ShaderPart
{
public:
	ShaderFragmentBlendMux(const opengl::GLInfo & _glinfo)
	{
		if (config.generalEmulation.enableLegacyBlending == 0) {
			m_part =
				"  #define MUXA(pos) dot(muxA, STVEC(pos))									\n"
				"  #define MUXB(pos) dot(muxB, STVEC(pos))									\n"
				"  #define MUXPM(pos) muxPM*(STVEC(pos))									\n"
				"  #define MUXF(pos) dot(muxF, STVEC(pos))									\n"
				"  lowp mat4 muxPM = mat4(vec4(0.0), vec4(0.0), uBlendColor, uFogColor);	\n"
				"  lowp vec4 muxA = vec4(0.0, uFogColor.a, shadeColor.a, 0.0);				\n"
				"  lowp vec4 muxB = vec4(0.0, 1.0, 1.0, 0.0);								\n"
				"  lowp vec4 muxF = vec4(0.0, 1.0, 0.0, 0.0);								\n"
				"  lowp vec4 muxp, muxm, srcColor1, srcColor2;								\n"
				"  lowp float muxa, muxb, dstFactor1, dstFactor2, muxaf, muxbf;				\n"
			;
		}
	}
};

class ShaderFragmentReadTexMipmap : public ShaderPart
{
public:
	ShaderFragmentReadTexMipmap(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"  lowp vec4 readtex0, readtex1;						\n"
			"  lowp float lod_frac = mipmap(readtex0, readtex1);	\n"
		;
	}
};

class ShaderFragmentReadTexCopyMode : public ShaderPart
{
public:
	ShaderFragmentReadTexCopyMode(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"  nCurrentTile = 0; \n"
				"  lowp vec4 readtex0 = readTex(uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);		\n"
				;
		} else {
			if (config.video.multisampling > 0) {
				m_part =
					"  lowp vec4 readtex0;																	\n"
					"  if (uMSTexEnabled[0] == 0) {															\n"
					"      READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])		\n"
					"  } else readtex0 = readTexMS(uMSTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);\n"
					;
			} else {
				m_part =
					"  lowp vec4 readtex0;																	\n"
					"  READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])			\n"
					;
			}
		}
	}
};

class ShaderFragmentReadTex0 : public ShaderPart
{
public:
	ShaderFragmentReadTex0(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {

			shaderPart = "  nCurrentTile = 0; \n";
			if (g_textureConvert.getBilerp0()) {
				shaderPart += "  lowp vec4 readtex0 = readTex(uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);		\n";
			} else {
				shaderPart += "  lowp vec4 tmpTex = vec4(0.0);																\n"
							  "  lowp vec4 readtex0 = YUV_Convert(uTex0, tcData0, 0, uTextureFormat[0], tmpTex);			\n";
			}

		} else {

			if (!g_textureConvert.getBilerp0()) {
				shaderPart = "  lowp vec4 readtex0;																			\n"
							 "  YUVCONVERT_TEX0(readtex0, uTex0, tcData0, uTextureFormat[0])								\n";
			} else {
				if (config.video.multisampling > 0) {
					shaderPart =
						"  lowp vec4 readtex0;																				\n"
						"  if (uMSTexEnabled[0] == 0) {																		\n"
						"    READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])						\n"
						"  } else readtex0 = readTexMS(uMSTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0]);			\n";
				} else {
					shaderPart = "  lowp vec4 readtex0;																		\n"
								" READ_TEX(readtex0, uTex0, tcData0, uFbMonochrome[0], uFbFixedAlpha[0])				\n";
				}
			}

		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentReadTex1 : public ShaderPart
{
public:
	ShaderFragmentReadTex1(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {

			shaderPart = "  nCurrentTile = 1; \n";

			if (g_textureConvert.getBilerp1()) {
				shaderPart += "  lowp vec4 readtex1 = readTex(uTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1]);				\n";
			} else {
				shaderPart += "  lowp vec4 readtex1 = YUV_Convert(uTex1, tcData1, uTextureConvert, uTextureFormat[1], readtex0);	\n";
			}

		} else {

			if (!g_textureConvert.getBilerp1()) {
				shaderPart =
					"  lowp vec4 readtex1;																							\n"
					"    YUVCONVERT_TEX1(readtex1, uTex1, tcData1, uTextureFormat[1], readtex0)					\n";
			} else {
				if (config.video.multisampling > 0) {
					shaderPart =
						"  lowp vec4 readtex1;																						\n"
						"  if (uMSTexEnabled[1] == 0) {																				\n"
						"    READ_TEX(readtex1, uTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1])								\n"
						"  } else readtex1 = readTexMS(uMSTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1]);					\n";
				} else {
					shaderPart = "  lowp vec4 readtex1;																				\n"
								"  READ_TEX(readtex1, uTex1, tcData1, uFbMonochrome[1], uFbFixedAlpha[1])						\n";

				}
			}

		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderFragmentCallN64Depth : public ShaderPart
{
public:
	ShaderFragmentCallN64Depth(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part = "  bool should_discard = false;	\n";

			if (_glinfo.imageTextures) {
				if (_glinfo.fragment_interlock)
					m_part += "  beginInvocationInterlockARB();	\n";
				else if (_glinfo.fragment_interlockNV)
					m_part += "  beginInvocationInterlockNV();	\n";
				else if (_glinfo.fragment_ordering)
					m_part += "  beginFragmentShaderOrderingINTEL();	\n";
			}

			m_part +=
				"  if (uRenderTarget != 0) { if (!depth_render(fragColor.r, fragDepth)) should_discard = true; } \n"
				"  else if (!depth_compare(fragDepth)) should_discard = true; \n"
				;

			if (_glinfo.imageTextures) {
				if (_glinfo.fragment_interlock)
					m_part += "  endInvocationInterlockARB();	\n";
				else if (_glinfo.fragment_interlockNV)
					m_part += "  endInvocationInterlockNV();	\n";
			}

			m_part += "  if (should_discard) discard;	\n";

		}
	}
};

class ShaderFragmentRenderTarget : public ShaderPart
{
public:
	ShaderFragmentRenderTarget(const opengl::GLInfo & _glinfo)
	{
		if (config.generalEmulation.enableFragmentDepthWrite != 0) {
			m_part =
				"  if (uRenderTarget != 0) {					\n"
				"    if (uRenderTarget > 1) {					\n"
				"      ivec2 coord = ivec2(gl_FragCoord.xy);	\n"
				"      if (fragDepth >= texelFetch(uDepthTex, coord, 0).r) discard;	\n"
				"    }											\n"
				"    fragDepth = fragColor.r;				\n"
				"  }											\n"
				"  gl_FragDepth = fragDepth;	\n"
			;
		}
	}
};

class ShaderFragmentMainEnd : public ShaderPart
{
public:
	ShaderFragmentMainEnd(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"  gl_FragColor = fragColor; \n"
				"} \n\n"
				;
		} else {
			m_part =
				"} \n\n"
				;
		}
	}
};

class ShaderNoise : public ShaderPart
{
public:
	ShaderNoise(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"uniform sampler2D uTexNoise;							\n"
				"lowp float snoise()									\n"
				"{														\n"
				"  mediump vec2 texSize = vec2(640.0, 580.0);			\n"
				"  mediump vec2 coord = gl_FragCoord.xy/uScreenScale/texSize;	\n"
				"  return texture2D(uTexNoise, coord).r;				\n"
				"}														\n"
				;
		} else {
			m_part =
				"uniform sampler2D uTexNoise;							\n"
				"lowp float snoise()									\n"
				"{														\n"
				"  ivec2 coord = ivec2(gl_FragCoord.xy/uScreenScale);	\n"
				"  return texelFetch(uTexNoise, coord, 0).r;			\n"
				"}														\n"
				;
		}
	}
};

class ShaderDither : public ShaderPart
{
public:
	ShaderDither(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2)
			return;

		if (config.generalEmulation.enableDitheringQuantization != 0) {
			m_part =
				"void quantizeRGB(inout lowp vec3 _color)				\n"
				"{														\n"
				"     _color.rgb = round(_color.rgb * 32.0)/32.0;		\n"
				"}														\n"
				"void quantizeA(inout lowp float _alpha)				\n"
				"{														\n"
				"     _alpha = round(_alpha * 32.0)/32.0;				\n"
				"}														\n"
				;
		} else {
			m_part =
				"void quantizeRGB(inout lowp vec3 _color){}\n"
				"void quantizeA(inout lowp float _alpha){}\n"
				;
		}

		m_part +=
			"void colorDither(in lowp vec3 _noise, inout lowp vec3 _color)\n"
			"{															\n"
			"  mediump vec3 threshold = 7.0 / 255.0 * (_noise - 0.5);	\n"
			"  _color = clamp(_color + threshold,0.0,1.0);				\n"
			"  quantizeRGB(_color);										\n"
			"}															\n"
			"void alphaDither(in lowp float _noise, inout lowp float _alpha)\n"
			"{															\n"
			"  mediump float threshold = 7.0 / 255.0 * (_noise - 0.5);	\n"
			"  _alpha = clamp(_alpha + threshold,0.0,1.0);				\n"
			"  quantizeA(_alpha);										\n"
			"}															\n"
			"lowp vec3 snoiseRGB()									\n"
			"{														\n"
			"  mediump vec2 texSize = vec2(640.0, 580.0);			\n"
			;
		if (config.generalEmulation.enableHiresNoiseDithering != 0)
			// multiplier for higher res noise effect
			m_part +=
			"  lowp float mult = 1.0 + step(2.0, uScreenScale.x);	\n";
		else
			m_part +=
			"  lowp float mult = 1.0;								\n";
		m_part +=
			"	mediump vec2 coordR = mult * ((gl_FragCoord.xy)/uScreenScale/texSize);\n"
			"	mediump vec2 coordG = mult * ((gl_FragCoord.xy + vec2( 0.0, texSize.y / 2.0 ))/uScreenScale/texSize);\n"
			"	mediump vec2 coordB = mult * ((gl_FragCoord.xy + vec2( texSize.x / 2.0,  0.0))/uScreenScale/texSize);\n"
			// Only red channel of noise texture contains noise.
			"  lowp float r = texture(uTexNoise,coordR).r;			\n"
			"  lowp float g = texture(uTexNoise,coordG).r;			\n"
			"  lowp float b = texture(uTexNoise,coordB).r;			\n"
			"														\n"
			"  return vec3(r,g,b);									\n"
			"}														\n"
			"lowp float snoiseA()									\n"
			"{														\n"
			"  mediump vec2 texSize = vec2(640.0, 580.0);			\n"
			;
		if (config.generalEmulation.enableHiresNoiseDithering != 0)
			// multiplier for higher res noise effect
			m_part +=
			"  lowp float mult = 1.0 + step(2.0, uScreenScale.x);	\n";
		else
			m_part +=
			"  lowp float mult = 1.0;								\n";
		m_part +=
			"														\n"
			"	mediump vec2 coord = mult * ((gl_FragCoord.xy)/uScreenScale/texSize);\n"
			"														\n"
			// Only red channel of noise texture contains noise.
			"  return texture(uTexNoise,coord).r;					\n"
			"}														\n"
			;
	}
};

class ShaderWriteDepth : public ShaderPart
{
public:
	ShaderWriteDepth(const opengl::GLInfo & _glinfo)
	{
		if (!_glinfo.isGLES2) {
			if (config.generalEmulation.enableFragmentDepthWrite == 0 &&
				config.frameBufferEmulation.N64DepthCompare == Config::dcDisable) {
				// Dummy write depth
				m_part =
					"highp float writeDepth()	    \n"
					"{						\n"
					"  return 0.0;	\n"
					"}						\n"
				;
			} else {
				if ((config.generalEmulation.hacks & hack_RE2) != 0) {
					m_part =
						"uniform lowp usampler2D uZlutImage;\n"
						"highp float writeDepth()																		\n"
						"{																								\n"
						;
					if (_glinfo.isGLESX && _glinfo.noPerspective) {
						m_part +=
							"  if (uClampMode == 1 && (vZCoord > 1.0)) discard;	\n"
							"  highp float FragDepth = (uDepthSource != 0) ? uPrimDepth :								\n"
							"           clamp((vZCoord - uPolygonOffset) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
							;
					} else {
						m_part +=
							"  highp float FragDepth = (uDepthSource != 0) ? uPrimDepth :								\n"
							"            clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
						;
					}
					m_part +=
						"  highp int iZ = FragDepth > 0.999 ? 262143 : int(floor(FragDepth * 262143.0));				\n"
						"  mediump int y0 = clamp(iZ/512, 0, 511);															\n"
						"  mediump int x0 = iZ - 512*y0;																	\n"
						"  highp uint iN64z = texelFetch(uZlutImage,ivec2(x0,y0), 0).r;											\n"
						"  return clamp(float(iN64z)/65532.0, 0.0, 1.0);											\n"
						"}																									\n"
						;
				} else {
					if (_glinfo.isGLESX && _glinfo.noPerspective) {
						 m_part =
							"highp float writeDepth()																	\n"
							"{																							\n"
							"  if (uClampMode == 1 && (vZCoord > 1.0)) discard;											\n"
							"  if (uDepthSource != 0) return uPrimDepth;												\n"
							"  return clamp((vZCoord - uPolygonOffset) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);		\n"
							"}																							\n"
							;
					} else {
						m_part =
							"highp float writeDepth()						        									\n"
							"{																							\n"
							"  if (uDepthSource != 0) return uPrimDepth;												\n"
							"  return clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
							"}																							\n"
							;
					}
				}
			}
		}
	}
};

class ShaderMipmap : public ShaderPart
{
public:
	ShaderMipmap(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"uniform mediump vec2 uTextureSize[2];										\n"
				"lowp vec4 TextureMipMap(in sampler2D tex, in highp vec2 tcData[5], in lowp float lod)	\n"
				"{																					\n"
				"  mediump vec2 texSize;															\n"
				"  if (nCurrentTile == 0)															\n"
				"    texSize = uTextureSize[0];														\n"
				"  else																				\n"
				"    texSize = uTextureSize[1];														\n"
				"  lowp vec4 c00 = texture2DLodEXT(tex, (tcData[0] + 0.5)/texSize, lod);			\n"
				"  lowp vec4 c01 = texture2DLodEXT(tex, (tcData[1] + 0.5)/texSize, lod);			\n"
				"  lowp vec4 c10 = texture2DLodEXT(tex, (tcData[2] + 0.5)/texSize, lod);			\n"
				"  lowp vec4 c11 = texture2DLodEXT(tex, (tcData[3] + 0.5)/texSize, lod);			\n"
				"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);									\n"
				"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);									\n"
				"  return c0 + tcData[4].t * (c1-c0);												\n"
				"  }																				\n"
				;
			if (config.generalEmulation.enableLOD == 0) {
				// Fake mipmap
				m_part +=
					"uniform lowp int uMaxTile;			\n"
					"uniform mediump float uMinLod;		\n"
					"														\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  readtex0 = TextureMipMap(uTex0, tcData0, 0.0);				\n"
					"  readtex1 = TextureMipMap(uTex1, tcData1, 0.0);				\n"
					"  if (uMaxTile == 0) return 1.0;						\n"
					"  return uMinLod;										\n"
					"}														\n"
				;
			} else {
				m_part +=
					"uniform lowp int uEnableLod;		\n"
					"uniform mediump float uMinLod;		\n"
					"uniform lowp int uMaxTile;			\n"
					"uniform lowp int uTextureDetail;	\n"
					"														\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  readtex0 = TextureMipMap(uTex0, tcData0, 0.0);		\n"
					"  readtex1 = TextureMipMap(uTex1, tcData1, 0.0);		\n"
					"														\n"
					"  mediump float fMaxTile = float(uMaxTile);			\n"
					"  mediump vec2 dx = abs(dFdx(vLodTexCoord)) * uScreenScale;	\n"
					"  mediump vec2 dy = abs(dFdy(vLodTexCoord)) * uScreenScale;	\n"
					"  mediump float lod = max(dx.x + dx.y, dy.x + dy.y);	\n" /*LINEAR*/
					"  bool magnify = lod < 1.0;							\n"
					"  mediump float lod_tile = magnify ? 0.0 : floor(log2(floor(lod))); \n"
					"  bool distant = lod > 128.0 || lod_tile >= fMaxTile;	\n"
					"  mediump float lod_frac = fract(lod/pow(2.0, lod_tile));	\n"
					"  if (magnify) lod_frac = max(lod_frac, uMinLod);		\n"
					"  if (uTextureDetail == 0)	{							\n"
					"    if (distant) lod_frac = 1.0;						\n"
					"    else if (magnify) lod_frac = 0.0;					\n"
					"  }													\n"
					"  if (magnify && (uTextureDetail == 1 || uTextureDetail == 3))			\n"
					"      lod_frac = 1.0 - lod_frac;						\n"
					"  if (uMaxTile == 0) {									\n"
					"    if (uEnableLod != 0) {								\n"
					"      if (uTextureDetail < 2)	readtex1 = readtex0;	\n"
					"      else if (!magnify) readtex0 = readtex1;			\n"
					"    }													\n"
					"    return lod_frac;									\n"
					"  }													\n"
					"  if (uEnableLod == 0) return lod_frac;				\n"
					"														\n"
					"  lod_tile = min(lod_tile, fMaxTile);					\n"
					"  lowp float lod_tile_m1 = max(0.0, lod_tile - 1.0);	\n"
					"  lowp float lod_tile_p1 = min(fMaxTile - 1.0, lod_tile + 1.0);	\n"
					"  lowp vec4 lodT = TextureMipMap(uTex1, tcData1, lod_tile);	\n"
					"  lowp vec4 lodT_m1 = TextureMipMap(uTex1, tcData1, lod_tile_m1);	\n"
					"  lowp vec4 lodT_p1 = TextureMipMap(uTex1, tcData1, lod_tile_p1);	\n"
					"  if (lod_tile < 1.0) {								\n"
					"    if (magnify) {									\n"
					//     !sharpen && !detail
					"      if (uTextureDetail == 0) readtex1 = readtex0;	\n"
					"    } else {											\n"
					//     detail
					"      if (uTextureDetail > 1) {						\n"
					"        readtex0 = lodT;								\n"
					"        readtex1 = lodT_p1;							\n"
					"      }												\n"
					"    }													\n"
					"  } else {												\n"
					"    if (uTextureDetail > 1) {							\n"
					"      readtex0 = lodT;									\n"
					"      readtex1 = lodT_p1;								\n"
					"    } else {											\n"
					"      readtex0 = lodT_m1;								\n"
					"      readtex1 = lodT;									\n"
					"    }													\n"
					"  }													\n"
					"  return lod_frac;										\n"
					"}														\n"
				;
			}
		}
		else {
			if (config.texture.bilinearMode == BILINEAR_3POINT)
				m_part =
				"#define READ_TEX_MIPMAP(name, tex, tcData, lod)																		\\\n"
				"{																												\\\n"
				"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);					\\\n"
				"  lowp vec2 lod_scale = vec2(textureSize(tex,int(lod))) / vec2(textureSize(tex,0)); \\\n"
				"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);			\\\n"
				"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11); \\\n"
				"  name = c0 + bottomRightTri * (c1-c0); \\\n"
				"}																												\n"
				;
			else
				m_part =
				"#define READ_TEX_MIPMAP(name, tex, tcData, lod)																		\\\n"
				"{																												\\\n"
				"  lowp vec2 lod_scale = vec2(textureSize(tex,int(lod))) / vec2(textureSize(tex,0)); \\\n"
				"  lowp vec4 c00 = texelFetch(tex, ivec2(tcData[0]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c01 = texelFetch(tex, ivec2(tcData[1]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c10 = texelFetch(tex, ivec2(tcData[2]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c11 = texelFetch(tex, ivec2(tcData[3]*lod_scale), int(lod)); \\\n"
				"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);						\\\n"
				"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);						\\\n"
				"  name = c0 + tcData[4].t * (c1-c0);									\\\n"
				"}																												\n"
				;
			if (config.generalEmulation.enableLOD == 0) {
				// Fake mipmap
				m_part +=
					"uniform lowp int uMaxTile;			\n"
					"uniform mediump float uMinLod;		\n"
					"														\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  READ_TEX_MIPMAP(readtex0, uTex0, tcData0, 0);			\n"
					"  READ_TEX_MIPMAP(readtex1, uTex1, tcData1, 0);			\n"
					"  if (uMaxTile == 0) return 1.0;							\n"
					"  return uMinLod;											\n"
					"}															\n"
				;
			} else {
				m_part +=
					"uniform lowp int uEnableLod;		\n"
					"uniform mediump float uMinLod;		\n"
					"uniform lowp int uMaxTile;			\n"
					"uniform lowp int uTextureDetail;	\n"
					"																		\n"
					"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
					"  READ_TEX_MIPMAP(readtex0, uTex0, tcData0, 0);					\n"
					"  READ_TEX_MIPMAP(readtex1, uTex1, tcData1, 0);					\n"
					"																		\n"
					"  mediump float fMaxTile = float(uMaxTile);							\n"
					"  mediump vec2 dx = abs(dFdx(vLodTexCoord));							\n"
					"  dx *= uScreenScale;													\n"
					"  mediump float lod = max(dx.x, dx.y);									\n"
					"  bool magnify = lod < 1.0;											\n"
					"  mediump float lod_tile = magnify ? 0.0 : floor(log2(floor(lod)));	\n"
					"  bool distant = lod > 128.0 || lod_tile >= fMaxTile;					\n"
					"  mediump float lod_frac = fract(lod/pow(2.0, lod_tile));				\n"
					"  if (magnify) lod_frac = max(lod_frac, uMinLod);						\n"
					"  if (uTextureDetail == 0)	{											\n"
					"    if (distant) lod_frac = 1.0;										\n"
					"    else if (magnify) lod_frac = 0.0;									\n"
					"  }																	\n"
					"  if (magnify && ((uTextureDetail & 1) != 0))							\n"
					"      lod_frac = 1.0 - lod_frac;										\n"
					"  if (uMaxTile == 0) {													\n"
					"    if (uEnableLod != 0) {												\n"
					"      if ((uTextureDetail & 2) == 0) readtex1 = readtex0;				\n"
					"      else if (!magnify) readtex0 = readtex1;							\n"
					"    }																	\n"
					"    return lod_frac;													\n"
					"  }																	\n"
					"  if (uEnableLod == 0) return lod_frac;								\n"
					"																		\n"
					"  lod_tile = min(lod_tile, fMaxTile - 1.0);							\n"
					"  lowp float lod_tile_m1 = max(0.0, lod_tile - 1.0);					\n"
					"  lowp float lod_tile_p1 = min(fMaxTile - 1.0, lod_tile + 1.0);		\n"
					"  lowp vec4 lodT, lodT_m1, lodT_p1;									\n"
					"  READ_TEX_MIPMAP(lodT, uTex1, tcData1, lod_tile);						\n"
					"  READ_TEX_MIPMAP(lodT_m1, uTex1, tcData1, lod_tile_m1);				\n"
					"  READ_TEX_MIPMAP(lodT_p1, uTex1, tcData1, lod_tile_p1);				\n"
					"  if (lod_tile < 1.0) {												\n"
					"    if (magnify) {														\n"
					//     !sharpen && !detail
					"      if (uTextureDetail == 0) readtex1 = readtex0;					\n"
					"    } else {															\n"
					//     detail
					"      if ((uTextureDetail & 2) != 0 ) {								\n"
					"        readtex0 = lodT;												\n"
					"        readtex1 = lodT_p1;											\n"
					"      }																\n"
					"    }																	\n"
					"  } else {																\n"
					"    if ((uTextureDetail & 2) != 0 ) {									\n"
					"      readtex0 = lodT;													\n"
					"      readtex1 = lodT_p1;												\n"
					"    } else {															\n"
					"      readtex0 = lodT_m1;												\n"
					"      readtex1 = lodT;													\n"
					"    }																	\n"
					"  }																	\n"
					"  return lod_frac;														\n"
					"}																		\n"
				;
			}
		}
	}
};

class ShaderCalcLight : public ShaderPart
{
public:
	ShaderCalcLight(const opengl::GLInfo & _glinfo)
	{
		m_part =
			"uniform mediump vec3 uLightDirection[8];	\n"
			"uniform lowp vec3 uLightColor[8];			\n"
			"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color) {\n"
			"  output_color = input_color;									\n"
			"  lowp int nLights = int(floor(fLights + 0.5));				\n"
			"  if (nLights == 0)											\n"
			"     return;													\n"
			"  output_color = uLightColor[nLights];							\n"
			"  mediump float intensity;										\n"
			"  for (int i = 0; i < nLights; i++)	{						\n"
			"    intensity = max(dot(input_color, uLightDirection[i]), 0.0);\n"
			"    output_color += intensity*uLightColor[i];					\n"
			"  };															\n"
			"  output_color = clamp(output_color, 0.0, 1.0);				\n"
			"}																\n"
		;
	}
};

class ShaderReadtex : public ShaderPart
{
public:
	ShaderReadtex(const opengl::GLInfo & _glinfo) : m_glinfo(_glinfo)
	{
	}

	void write(std::stringstream & shader) const override
	{
		std::string shaderPart;

		if (m_glinfo.isGLES2) {
			shaderPart +=
				"uniform mediump vec2 uTextureSize[2];										\n"
				"lowp vec4 TextureNearest(in sampler2D tex, in highp vec2 tcData[5])		\n"
				"{																					\n"
				"  mediump vec2 texSize;															\n"
				"  if (nCurrentTile == 0)															\n"
				"    texSize = uTextureSize[0];														\n"
				"  else																				\n"
				"    texSize = uTextureSize[1];														\n"
				"  return texture2D(tex, (tcData[0] + 0.5) / texSize);								\n"
				"  }																				\n"
				;
			if (g_textureConvert.useYUVCoversion())
				shaderPart +=
				"lowp vec4 YUV_Convert(in sampler2D tex, in highp vec2 tcData[5], in lowp int convert, in lowp int format, in lowp vec4 prev)	\n"
				"{																	\n"
				"  lowp vec4 texColor;												\n"
				"  if (convert != 0) texColor = prev;								\n"
				"  else texColor = TextureNearest(tex, tcData);						\n"
				"  mediump ivec4 icolor = ivec4(texColor*255.0);					\n"
				"  if (format == 1)													\n"
				"    icolor.rg -= 128;												\n"
				"  mediump ivec4 iconvert;											\n"
				"  iconvert.r = icolor.b + (uConvertParams[0]*icolor.g + 128)/256;	\n"
				"  iconvert.g = icolor.b + (uConvertParams[1]*icolor.r + uConvertParams[2]*icolor.g + 128)/256;	\n"
				"  iconvert.b = icolor.b + (uConvertParams[3]*icolor.r + 128)/256;	\n"
				"  iconvert.a = icolor.b;											\n"
				"  return vec4(iconvert)/255.0;										\n"
				"  }																\n"
			;
			if (g_textureConvert.useTextureFiltering()) {
				if (config.texture.bilinearMode == BILINEAR_3POINT) {
					shaderPart +=
						// "uniform mediump vec2 uTextureSize[2];										\n" NOT NEEDED HERE?
						// 3 point texture filtering.
						// Original author: ArthurCarvalho
						// GLSL implementation: twinaphex, mupen64plus-libretro project.
						"lowp vec4 TextureFilter(in sampler2D tex, in highp vec2 tcData[5])		\n"
						"{																					\n"
						"  mediump vec2 texSize;															\n"
						"  if (nCurrentTile == 0)															\n"
						"    texSize = uTextureSize[0];														\n"
						"  else																				\n"
						"    texSize = uTextureSize[1];														\n"
						"  lowp float bottomRightTri = step(1.0, tcData[4].s + tcData[4].t);				\n"
						"  lowp vec4 c00 = texture2D(tex, (tcData[0] + 0.5)/texSize);						\n"
						"  lowp vec4 c01 = texture2D(tex, (tcData[1] + 0.5)/texSize);						\n"
						"  lowp vec4 c10 = texture2D(tex, (tcData[2] + 0.5)/texSize);						\n"
						"  lowp vec4 c11 = texture2D(tex, (tcData[3] + 0.5)/texSize);						\n"
						"  lowp vec4 c0 = c00 + tcData[4].s*(c10-c00) + tcData[4].t*(c01-c00);				\n"
						"  lowp vec4 c1 = c11 + (1.0-tcData[4].s)*(c01-c11) + (1.0-tcData[4].t)*(c10-c11);	\n"
						"  return c0 + bottomRightTri * (c1-c0);											\n"
						"  }																				\n"
						;
				} else {
					shaderPart +=
						// bilinear filtering.
						//"uniform mediump vec2 uTextureSize[2];										\n" NOT NEEDED HERE?
						"lowp vec4 TextureFilter(in sampler2D tex, in highp vec2 tcData[5])		\n"
						"{																					\n"
						"  mediump vec2 texSize;															\n"
						"  if (nCurrentTile == 0)															\n"
						"    texSize = uTextureSize[0];														\n"
						"  else																				\n"
						"    texSize = uTextureSize[1];														\n"
						"  lowp vec4 c00 = texture2D(tex, (tcData[0] + 0.5)/texSize);						\n"
						"  lowp vec4 c01 = texture2D(tex, (tcData[1] + 0.5)/texSize);						\n"
						"  lowp vec4 c10 = texture2D(tex, (tcData[2] + 0.5)/texSize);						\n"
						"  lowp vec4 c11 = texture2D(tex, (tcData[3] + 0.5)/texSize);						\n"
						"  lowp vec4 c0 = c00 + tcData[4].s * (c10-c00);									\n"
						"  lowp vec4 c1 = c01 + tcData[4].s * (c11-c01);									\n"
						"  return c0 + tcData[4].t * (c1-c0);												\n"
						"  }																				\n"
						;
				}
				shaderPart +=
					"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  lowp vec4 texColor;														\n"
					"  if (uTextureFilterMode == 0) texColor = TextureNearest(tex, tcData);		\n"
					"  else texColor = TextureFilter(tex, tcData);							\n"
					"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
					"  else if (fbMonochrome == 2) 												\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
					"  return texColor;															\n"
					"}																			\n"
					;
			}
		} else {
			if (config.video.multisampling > 0 && g_textureConvert.useTextureFiltering()) {
				shaderPart =
					"uniform lowp int uMSAASamples;												\n"
					"lowp vec4 sampleMS(in lowp sampler2DMS mstex, in mediump ivec2 ipos)		\n"
					"{																			\n"
					"  lowp vec4 texel = vec4(0.0);												\n"
					"  for (int i = 0; i < uMSAASamples; ++i)									\n"
					"    texel += texelFetch(mstex, ipos, i);									\n"
					"  return texel / float(uMSAASamples);										\n"
					"}																			\n"
					"																			\n"
					"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  mediump ivec2 itexCoord;													\n"
					"  if (fbMonochrome == 3) {													\n"
					"    itexCoord = ivec2(gl_FragCoord.xy);									\n"
					"  } else {																	\n"
					"    itexCoord = ivec2(tcData[0]);											\n"
					"  }																		\n"
					"  lowp vec4 texColor = sampleMS(mstex, itexCoord);							\n"
					"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
					"  else if (fbMonochrome == 2) 												\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"  else if (fbMonochrome == 3) { 											\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"    texColor.a = 0.0;														\n"
					"  }																		\n"
					"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
					"  return texColor;															\n"
					"}																			\n"
				;
			}
		}

		shader << shaderPart;
	}

private:
	const opengl::GLInfo& m_glinfo;
};

class ShaderReadtexCopyMode : public ShaderPart
{
public:
	ShaderReadtexCopyMode(const opengl::GLInfo & _glinfo)
	{
		if (_glinfo.isGLES2) {
			m_part =
				"uniform mediump vec2 uTextureSize[2];										\n"
				"lowp vec4 TextureNearest(in sampler2D tex, in highp vec2 tcData[5])		\n"
				"{																					\n"
				"  mediump vec2 texSize;															\n"
				"  if (nCurrentTile == 0)															\n"
				"    texSize = uTextureSize[0];														\n"
				"  else																				\n"
				"    texSize = uTextureSize[1];														\n"
				"  return texture2D(tex, (tcData[0] + 0.5) / texSize);								\n"
				"  }																				\n"
				"lowp vec4 readTex(in sampler2D tex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
				"{																			\n"
				"  lowp vec4 texColor = TextureNearest(tex, tcData);						\n"
				"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
				"  else if (fbMonochrome == 2) 												\n"
				"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
				"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
				"  return texColor;															\n"
				"}																			\n"
				;
		} else {
			if (config.video.multisampling > 0) {
				m_part =
					"uniform lowp int uMSAASamples;												\n"
					"lowp vec4 sampleMS(in lowp sampler2DMS mstex, in mediump ivec2 ipos)		\n"
					"{																			\n"
					"  lowp vec4 texel = vec4(0.0);												\n"
					"  for (int i = 0; i < uMSAASamples; ++i)									\n"
					"    texel += texelFetch(mstex, ipos, i);									\n"
					"  return texel / float(uMSAASamples);										\n"
					"}																			\n"
					"																			\n"
					"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in highp vec2 tcData[5], in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
					"{																			\n"
					"  mediump ivec2 itexCoord;													\n"
					"  if (fbMonochrome == 3) {													\n"
					"    itexCoord = ivec2(gl_FragCoord.xy);									\n"
					"  } else {																	\n"
					"    itexCoord = ivec2(tcData[0]);											\n"
					"  }																		\n"
					"  lowp vec4 texColor = sampleMS(mstex, itexCoord);							\n"
					"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
					"  else if (fbMonochrome == 2) 												\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"  else if (fbMonochrome == 3) { 											\n"
					"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
					"    texColor.a = 0.0;														\n"
					"  }																		\n"
					"  if (fbFixedAlpha == 1) texColor.a = 0.825;								\n"
					"  return texColor;															\n"
					"}																			\n"
				;
			}
		}
	}
};

class ShaderN64DepthCompare : public ShaderPart
{
public:
	ShaderN64DepthCompare(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part =
				"uniform lowp int uEnableDepth;							\n"
				"uniform lowp int uDepthMode;							\n"
				"uniform lowp int uEnableDepthUpdate;					\n"
				"uniform mediump float uDeltaZ;							\n"
				"bool depth_compare(highp float curZ)					\n"
				"{														\n"
				"  if (uEnableDepth == 0) return true;					\n"
				;
			if (_glinfo.imageTextures) {
				m_part +=
				"  ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
				"  highp vec4 depthZ = imageLoad(uDepthImageZ,coord);	\n"
				"  highp vec4 depthDeltaZ = imageLoad(uDepthImageDeltaZ,coord);\n"
					;
			}
			m_part +=
				"  highp float bufZ = depthZ.r;							\n"
				"  highp float dz, dzMin;								\n"
				"  if (uDepthSource == 1) {								\n"
				"     dzMin = dz = uDeltaZ;								\n"
				"  } else {												\n"
				"    dz = 4.0*fwidth(curZ);								\n"
				"    dzMin = min(dz, depthDeltaZ.r);					\n"
				"  }													\n"
				"  bool bInfront = curZ < bufZ;							\n"
				"  bool bFarther = (curZ + dzMin) >= bufZ;				\n"
				"  bool bNearer = (curZ - dzMin) <= bufZ;				\n"
				"  bool bMax = bufZ == 1.0;								\n"
				"  bool bRes = false;									\n"
				"  switch (uDepthMode) {								\n"
				"     case 1:											\n"
				"       bRes = bMax || bNearer;							\n"
				"       break;											\n"
				"     case 0:											\n"
				"     case 2:											\n"
				"       bRes = bMax || bInfront;						\n"
				"       break;											\n"
				"     case 3:											\n"
				"       bRes = bFarther && bNearer && !bMax;			\n"
				"       break;											\n"
				"  }													\n"
				"  bRes = bRes || (uEnableDepthCompare == 0);			\n"
				"  if (uEnableDepthUpdate != 0 && bRes) {				\n"
				;
			if (_glinfo.imageTextures) {
				m_part +=
				"    highp vec4 depthOutZ = vec4(curZ, 1.0, 1.0, 1.0);		\n"
				"    highp vec4 depthOutDeltaZ = vec4(dz, 1.0, 1.0, 1.0);	\n"
				"    imageStore(uDepthImageZ, coord, depthOutZ);			\n"
				"    imageStore(uDepthImageDeltaZ, coord, depthOutDeltaZ);	\n"
					;
			} else if (_glinfo.ext_fetch) {
				m_part +=
				"    depthZ.r = curZ;									\n"
				"    depthDeltaZ.r = dz;								\n"
					;
			}
			m_part +=
				"  }													\n"
				"  return bRes;											\n"
				"}														\n"
			;
		}
	}
};

class ShaderN64DepthRender : public ShaderPart
{
public:
	ShaderN64DepthRender(const opengl::GLInfo & _glinfo)
	{
		if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable) {
			m_part =
				"bool depth_render(highp float Z, highp float curZ)						\n"
				"{														\n"
				"  ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
				"  if (uEnableDepthCompare != 0) {						\n"
				;
			if (_glinfo.imageTextures) {
				m_part +=
					"    highp vec4 depthZ = imageLoad(uDepthImageZ,coord);	\n"
					;
			}
			m_part +=
				"    highp float bufZ = depthZ.r;						\n"
				"    if (curZ >= bufZ) return false;					\n"
				"  }													\n"
				;
			if (_glinfo.imageTextures) {
				m_part +=
					"  highp vec4 depthOutZ = vec4(Z, 1.0, 1.0, 1.0);		\n"
					"  highp vec4 depthOutDeltaZ = vec4(0.0, 1.0, 1.0, 1.0);\n"
					"  imageStore(uDepthImageZ,coord, depthOutZ);			\n"
					"  imageStore(uDepthImageDeltaZ,coord, depthOutDeltaZ);	\n"
					;
			} else if (_glinfo.ext_fetch) {
				m_part +=
					"  depthZ.r = Z;	\n"
					"  depthDeltaZ.r = 0.0;	\n"
					;
			}
			m_part +=
				"  return true;											\n"
				"}														\n"
				;
		}
	}
};


class ShaderTextureEngine : public ShaderPart
{
public:
	ShaderTextureEngine(const opengl::GLInfo _glinfo) {
		m_part =
			"highp vec2 clampWrapMirror(in highp vec2 vTexCoord, in highp vec2 vWrap,		\n"
			"	in highp vec2 vClamp, in lowp vec2 vWrapEn, in lowp vec2 vClampEn, in lowp vec2 vMirrorEn)		\n"
			"{																				\n"
			"	highp vec2 texCoord = vTexCoord;											\n"
			"	highp vec2 clampedCoord = clamp(texCoord, vec2(0.0), vClamp);				\n"
			"	texCoord += vClampEn*(clampedCoord-texCoord);								\n"
			"   lowp vec2 needMirror = step(vWrap, mod(texCoord, 2.0*vWrap));				\n"
			"	highp vec2 invertedCoord = mod(-texCoord-vec2(1.0), vWrap);	 				\n"
			"	texCoord += vMirrorEn*needMirror*(invertedCoord-texCoord);	 				\n"
			"   highp vec2 wrappedCoord = mod(texCoord,vWrap);								\n"
			"	texCoord += vWrapEn*(wrappedCoord-texCoord);								\n"
			"	return texCoord;															\n"
			"}																				\n"

			"highp vec2 wrap2D(in highp vec2 tc, in highp vec2 size)						\n"
			"{																				\n"
			"  highp float divs = floor(tc.s / size.s);										\n"
			"  highp float divt = floor((tc.t + divs) / size.t);							\n"
			"  return vec2(tc.s - divs * size.s, tc.t + divs - divt*size.t);				\n"
			"}																				\n"

			"uniform highp vec2 uTexSize0;		\n"
			"void textureEngine0(in highp vec2 texCoord, out highp vec2 tcData[5]) \n"
			"{  \n"
			"  mediump vec2 intPart = floor(texCoord); \n"
			"  highp vec2 tc00 = clampWrapMirror(intPart, uTexWrap0, uTexClamp0, uTexWrapEn0, uTexClampEn0, uTexMirrorEn0); \n"
			"  highp vec2 tc11 = clampWrapMirror(intPart + vec2(1.0,1.0), uTexWrap0, uTexClamp0, uTexWrapEn0, uTexClampEn0, uTexMirrorEn0); \n"
			"  tcData[0] = wrap2D(tc00, uTexSize0); \n"
			"  tcData[3] = wrap2D(tc11, uTexSize0); \n"
			"  tcData[1] = vec2(tcData[0].s, tcData[3].t); \n"
			"  tcData[2] = vec2(tcData[3].s, tcData[0].t); \n"
			"  tcData[4] = texCoord - intPart; \n"
			"}  \n"

			"uniform highp vec2 uTexSize1;		\n"
			"void textureEngine1(in highp vec2 texCoord, out highp vec2 tcData[5]) \n"
			"{  \n"
			"  mediump vec2 intPart = floor(texCoord); \n"
			"  highp vec2 tc00 = clampWrapMirror(intPart, uTexWrap1, uTexClamp1, uTexWrapEn1, uTexClampEn1, uTexMirrorEn1); \n"
			"  highp vec2 tc11 = clampWrapMirror(intPart + vec2(1.0,1.0), uTexWrap1, uTexClamp1, uTexWrapEn1, uTexClampEn1, uTexMirrorEn1); \n"
			"  tcData[0] = wrap2D(tc00, uTexSize1); \n"
			"  tcData[3] = wrap2D(tc11, uTexSize1); \n"
			"  tcData[1] = vec2(tcData[0].s, tcData[3].t); \n"
			"  tcData[2] = vec2(tcData[3].s, tcData[0].t); \n"
			"  tcData[4] = texCoord - intPart; \n"
			"}  \n"

			;
	}
};


class ShaderFragmentTextureEngineTex0 : public ShaderPart {
public:
	ShaderFragmentTextureEngineTex0(const opengl::GLInfo _glinfo)
	{
		m_part =
			"textureEngine0(vTexCoord0, tcData0); \n"
			;
	}
};

class ShaderFragmentTextureEngineTex1 : public ShaderPart {
public:
	ShaderFragmentTextureEngineTex1(const opengl::GLInfo _glinfo)
	{
		m_part =
			"textureEngine1(vTexCoord1, tcData1); \n"
			;
	}
};

/*---------------ShaderPartsEnd-------------*/

static
bool needClampColor() {
	return g_cycleType <= G_CYC_2CYCLE;
}

static
bool combinedColorC(const gDPCombine & _combine) {
	if (g_cycleType != G_CYC_2CYCLE)
		return false;
	return _combine.mRGB1 == G_CCMUX_COMBINED;
}

static
bool combinedAlphaC(const gDPCombine & _combine) {
	if (g_cycleType != G_CYC_2CYCLE)
		return false;
	return _combine.mA1 == G_ACMUX_COMBINED;
}

static
bool combinedColorABD(const gDPCombine & _combine) {
	if (g_cycleType != G_CYC_2CYCLE)
		return false;
	if (_combine.aRGB1 == G_CCMUX_COMBINED)
		return true;
	if (_combine.saRGB1 == G_CCMUX_COMBINED || _combine.sbRGB1 == G_CCMUX_COMBINED)
		return _combine.mRGB1 != G_CCMUX_0;
	return false;
}

static
bool combinedAlphaABD(const gDPCombine & _combine) {
	if (g_cycleType != G_CYC_2CYCLE)
		return false;
	if (_combine.aA1 == G_ACMUX_COMBINED)
		return true;
	if (_combine.saA1 == G_ACMUX_COMBINED || _combine.sbA1 == G_ACMUX_COMBINED)
		return _combine.mA1 != G_ACMUX_0;
	return false;
}

CombinerInputs CombinerProgramBuilder::compileCombiner(const CombinerKey & _key, Combiner & _color, Combiner & _alpha, std::string & _strShader)
{
	gDPCombine combine;
	combine.mux = _key.getMux();

	std::stringstream ssShader;

	if (g_cycleType != G_CYC_2CYCLE) {
		_correctFirstStageParams(_alpha.stage[0]);
		_correctFirstStageParams(_color.stage[0]);
	} else {
		_correctFirstStageParams2Cyc(_alpha.stage[0]);
		_correctFirstStageParams2Cyc(_color.stage[0]);
	}
	ssShader << "  alpha1 = ";
	CombinerInputs inputs = _compileCombiner(_alpha.stage[0], AlphaInput, ssShader);
	// Simulate N64 color sign-extend.
	if (combinedAlphaC(combine))
		m_signExtendAlphaC->write(ssShader);
	else if (combinedAlphaABD(combine))
		m_signExtendAlphaABD->write(ssShader);

	if (g_cycleType < G_CYC_FILL)
		m_alphaTest->write(ssShader);

	ssShader << "  color1 = ";
	inputs += _compileCombiner(_color.stage[0], ColorInput, ssShader);
	// Simulate N64 color sign-extend.
	if (combinedColorC(combine))
		m_signExtendColorC->write(ssShader);
	else if (combinedColorABD(combine))
		m_signExtendColorABD->write(ssShader);

	if (g_cycleType == G_CYC_2CYCLE) {

		ssShader << "  combined_color = vec4(color1, alpha1);" << std::endl;
		if (_alpha.numStages == 2) {
			ssShader << "  alpha2 = ";
			_correctSecondStageParams(_alpha.stage[1]);
			inputs += _compileCombiner(_alpha.stage[1], AlphaInput, ssShader);
		}
		else
			ssShader << "  alpha2 = alpha1;" << std::endl;

		ssShader << "  if (uCvgXAlpha != 0 && alpha2 < 0.125) discard;" << std::endl;

		if (_color.numStages == 2) {
			ssShader << "  color2 = ";
			_correctSecondStageParams(_color.stage[1]);
			inputs += _compileCombiner(_color.stage[1], ColorInput, ssShader);
		}
		else
			ssShader << "  color2 = color1;" << std::endl;

		ssShader << "  lowp vec4 cmbRes = vec4(color2, alpha2);" << std::endl;
	}
	else {
		if (g_cycleType < G_CYC_FILL)
			ssShader << "  if (uCvgXAlpha != 0 && alpha1 < 0.125) discard;" << std::endl;
		ssShader << "  lowp vec4 cmbRes = vec4(color1, alpha1);" << std::endl;
	}

	// Simulate N64 color clamp.
	if (needClampColor())
		m_clamp->write(ssShader);
	else
		ssShader << "  lowp vec4 clampedColor = clamp(cmbRes, 0.0, 1.0);" << std::endl;

	if (g_cycleType <= G_CYC_2CYCLE)
		m_callDither->write(ssShader);

	if (config.generalEmulation.enableLegacyBlending == 0) {
		if (g_cycleType <= G_CYC_2CYCLE) {
			m_blender1->write(ssShader);
			if (g_cycleType == G_CYC_2CYCLE)
				m_blender2->write(ssShader);
			m_blenderAlpha->write(ssShader);
		} else
			ssShader << "  fragColor = clampedColor;" << std::endl;

	}
	else {
		ssShader << "  fragColor = clampedColor;" << std::endl;
		m_legacyBlender->write(ssShader);
	}

	_strShader = std::move(ssShader.str());
	return inputs;
}

graphics::CombinerProgram * CombinerProgramBuilder::buildCombinerProgram(Combiner & _color,
																		Combiner & _alpha,
																		const CombinerKey & _key)
{
	g_cycleType = _key.getCycleType();
	g_textureConvert.setMode(_key.getBilerp());

	std::string strCombiner;
	CombinerInputs combinerInputs(compileCombiner(_key, _color, _alpha, strCombiner));

	const bool bUseLod = combinerInputs.usesLOD();
	const bool bUseTextures = combinerInputs.usesTexture();
	const bool bIsRect = _key.isRectKey();
	const bool bUseHWLight = !bIsRect && // Rects not use lighting
							 isHWLightingAllowed() &&
							 combinerInputs.usesShadeColor();

	if (bUseHWLight)
		combinerInputs.addInput(G_GCI_HW_LIGHT);

	std::stringstream ssShader;

	/* Write headers */
	m_fragmentHeader->write(ssShader);

	if (bUseTextures) {
		m_fragmentGlobalVariablesTex->write(ssShader);

		if (g_cycleType == G_CYC_2CYCLE && config.generalEmulation.enableLegacyBlending == 0)
			ssShader << "uniform lowp ivec4 uBlendMux2;" << std::endl << "uniform lowp int uForceBlendCycle2;" << std::endl;

		if (g_cycleType <= G_CYC_2CYCLE)
			m_fragmentHeaderDither->write(ssShader);
		m_fragmentHeaderNoise->write(ssShader);
		m_fragmentHeaderWriteDepth->write(ssShader);
		m_fragmentHeaderDepthCompare->write(ssShader);
		m_fragmentHeaderReadMSTex->write(ssShader);
		m_fragmentHeaderTextureEngine->write(ssShader);
		if (bUseLod)
			m_fragmentHeaderMipMap->write(ssShader);
		else if (g_cycleType < G_CYC_COPY)
			m_fragmentHeaderReadTex->write(ssShader);
		else
			m_fragmentHeaderReadTexCopyMode->write(ssShader);
	} else {
		m_fragmentGlobalVariablesNotex->write(ssShader);

		if (g_cycleType == G_CYC_2CYCLE && config.generalEmulation.enableLegacyBlending == 0)
			ssShader << "uniform lowp ivec4 uBlendMux2;" << std::endl << "uniform lowp int uForceBlendCycle2;" << std::endl;

		if (g_cycleType <= G_CYC_2CYCLE)
			m_fragmentHeaderDither->write(ssShader);
		m_fragmentHeaderNoise->write(ssShader);
		m_fragmentHeaderWriteDepth->write(ssShader);
		m_fragmentHeaderDepthCompare->write(ssShader);
	}

	if (bUseHWLight)
		m_fragmentHeaderCalcLight->write(ssShader);

	/* Write body */
	if (g_cycleType == G_CYC_2CYCLE)
		m_fragmentMain2Cycle->write(ssShader);
	else
		m_fragmentMain->write(ssShader);

	if (g_cycleType <= G_CYC_2CYCLE)
		m_fragmentBlendMux->write(ssShader);

	if (bUseTextures) {
		if (combinerInputs.usesTile(0))
		{
			m_fragmentTextureEngineTex0->write(ssShader);
		}
		if (combinerInputs.usesTile(1))
		{
			m_fragmentTextureEngineTex1->write(ssShader);
		}

		if (bUseLod) {
			m_fragmentReadTexMipmap->write(ssShader);
		} else {
			if (g_cycleType < G_CYC_COPY) {
				if (combinerInputs.usesTile(0))
					m_fragmentReadTex0->write(ssShader);
				else
					ssShader << "  lowp vec4 readtex0;" << std::endl;

				if (combinerInputs.usesTile(1))
					m_fragmentReadTex1->write(ssShader);
			} else
				m_fragmentReadTexCopyMode->write(ssShader);
		}
	}

	if (bUseHWLight)
		ssShader << "  calc_light(vNumLights, shadeColor.rgb, input_color);" << std::endl;
	else
		ssShader << "  input_color = shadeColor.rgb;" << std::endl;

	ssShader << "  vec_color = vec4(input_color, shadeColor.a);" << std::endl;
	ssShader << strCombiner << std::endl;

	if (config.frameBufferEmulation.N64DepthCompare != Config::dcDisable)
		m_fragmentCallN64Depth->write(ssShader);
	else
		m_fragmentRenderTarget->write(ssShader);

	// End of Main() function
	m_shaderFragmentMainEnd->write(ssShader);

	/* Write other functions */
	if (bUseHWLight)
		m_shaderCalcLight->write(ssShader);

	if (bUseTextures) {
		m_shaderTextureEngine->write(ssShader);
		if (bUseLod)
			m_shaderMipmap->write(ssShader);
		else {
			if (g_cycleType < G_CYC_COPY)
				m_shaderReadtex->write(ssShader);
			else
				m_shaderReadtexCopyMode->write(ssShader);
		}
	}

	m_shaderNoise->write(ssShader);

	if (g_cycleType <= G_CYC_2CYCLE)
		m_shaderDither->write(ssShader);

	m_shaderWriteDepth->write(ssShader);

	m_shaderN64DepthCompare->write(ssShader);

	m_shaderN64DepthRender->write(ssShader);

	const std::string strFragmentShader(std::move(ssShader.str()));

	/* Create shader program */

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar * strShaderData = strFragmentShader.data();
	glShaderSource(fragmentShader, 1, &strShaderData, nullptr);
	glCompileShader(fragmentShader);
	if (!Utils::checkShaderCompileStatus(fragmentShader))
	Utils::logErrorShader(GL_FRAGMENT_SHADER, strFragmentShader);

	GLuint program = glCreateProgram();
	Utils::locateAttributes(program, bIsRect, bUseTextures);
	if (bIsRect)
		glAttachShader(program, bUseTextures ? m_vertexShaderTexturedRect : m_vertexShaderRect);
	else
		glAttachShader(program, bUseTextures ? m_vertexShaderTexturedTriangle : m_vertexShaderTriangle);
	glAttachShader(program, fragmentShader);
	if (CombinerInfo::get().isShaderCacheSupported()) {
		if (IS_GL_FUNCTION_VALID(ProgramParameteri))
			glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
	}
	glLinkProgram(program);
	assert(Utils::checkProgramLinkStatus(program));
	glDeleteShader(fragmentShader);

	UniformGroups uniforms;
	m_uniformFactory->buildUniforms(program, combinerInputs, _key, uniforms);

	return new CombinerProgramImpl(_key, program, m_useProgram, combinerInputs, std::move(uniforms));
}

const ShaderPart * CombinerProgramBuilder::getVertexShaderHeader() const
{
	return m_vertexHeader.get();
}

const ShaderPart * CombinerProgramBuilder::getFragmentShaderHeader() const
{
	return m_fragmentHeader.get();
}

const ShaderPart * CombinerProgramBuilder::getFragmentShaderEnd() const
{
	return m_shaderFragmentMainEnd.get();
}

static
GLuint _createVertexShader(ShaderPart * _header, ShaderPart * _body, ShaderPart * _footer)
{
	std::stringstream ssShader;
	_header->write(ssShader);
	_body->write(ssShader);
	_footer->write(ssShader);
	const std::string strShader(std::move(ssShader.str()));
	const GLchar * strShaderData = strShader.data();

	GLuint shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_object, 1, &strShaderData, nullptr);
	glCompileShader(shader_object);
	if (!Utils::checkShaderCompileStatus(shader_object))
		Utils::logErrorShader(GL_VERTEX_SHADER, strShaderData);
	return shader_object;
}

CombinerProgramBuilder::CombinerProgramBuilder(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram)
: m_blender1(new ShaderBlender1(_glinfo))
, m_blender2(new ShaderBlender2(_glinfo))
, m_blenderAlpha (new ShaderBlenderAlpha(_glinfo))
, m_legacyBlender(new ShaderLegacyBlender)
, m_clamp(new ShaderClamp)
, m_signExtendColorC(new ShaderSignExtendColorC)
, m_signExtendAlphaC(new ShaderSignExtendAlphaC)
, m_signExtendColorABD(new ShaderSignExtendColorABD)
, m_signExtendAlphaABD(new ShaderSignExtendAlphaABD)
, m_alphaTest(new ShaderAlphaTest)
, m_callDither(new ShaderDithering(_glinfo))
, m_vertexHeader(new VertexShaderHeader(_glinfo))
, m_vertexEnd(new VertexShaderEnd(_glinfo))
, m_vertexRect(new VertexShaderRect(_glinfo))
, m_vertexTexturedRect(new VertexShaderTexturedRect(_glinfo))
, m_vertexTriangle(new VertexShaderTriangle(_glinfo))
, m_vertexTexturedTriangle(new VertexShaderTexturedTriangle(_glinfo))
, m_fragmentHeader(new FragmentShaderHeader(_glinfo))
, m_fragmentGlobalVariablesTex(new ShaderFragmentGlobalVariablesTex(_glinfo))
, m_fragmentGlobalVariablesNotex(new ShaderFragmentGlobalVariablesNotex(_glinfo))
, m_fragmentHeaderNoise(new ShaderFragmentHeaderNoise(_glinfo))
, m_fragmentHeaderWriteDepth(new ShaderFragmentHeaderWriteDepth(_glinfo))
, m_fragmentHeaderCalcLight(new ShaderFragmentHeaderCalcLight(_glinfo))
, m_fragmentHeaderMipMap(new ShaderFragmentHeaderMipMap(_glinfo))
, m_fragmentHeaderTextureEngine(new ShaderFragmentHeaderTextureEngine(_glinfo))
, m_fragmentHeaderReadMSTex(new ShaderFragmentHeaderReadMSTex(_glinfo))
, m_fragmentHeaderDither(new ShaderFragmentHeaderDither(_glinfo))
, m_fragmentHeaderDepthCompare(new ShaderFragmentHeaderDepthCompare(_glinfo))
, m_fragmentHeaderReadTex(new ShaderFragmentHeaderReadTex(_glinfo))
, m_fragmentHeaderReadTexCopyMode(new ShaderFragmentHeaderReadTexCopyMode(_glinfo))
, m_fragmentMain(new ShaderFragmentMain(_glinfo))
, m_fragmentMain2Cycle(new ShaderFragmentMain2Cycle(_glinfo))
, m_fragmentBlendMux(new ShaderFragmentBlendMux(_glinfo))
, m_fragmentReadTex0(new ShaderFragmentReadTex0(_glinfo))
, m_fragmentReadTex1(new ShaderFragmentReadTex1(_glinfo))
, m_fragmentTextureEngineTex0(new ShaderFragmentTextureEngineTex0(_glinfo))
, m_fragmentTextureEngineTex1(new ShaderFragmentTextureEngineTex1(_glinfo))
, m_fragmentReadTexCopyMode(new ShaderFragmentReadTexCopyMode(_glinfo))
, m_fragmentReadTexMipmap(new ShaderFragmentReadTexMipmap(_glinfo))
, m_fragmentCallN64Depth(new ShaderFragmentCallN64Depth(_glinfo))
, m_fragmentRenderTarget(new ShaderFragmentRenderTarget(_glinfo))
, m_shaderFragmentMainEnd(new ShaderFragmentMainEnd(_glinfo))
, m_shaderNoise(new ShaderNoise(_glinfo))
, m_shaderDither(new ShaderDither(_glinfo))
, m_shaderWriteDepth(new ShaderWriteDepth(_glinfo))
, m_shaderMipmap(new ShaderMipmap(_glinfo))
, m_shaderCalcLight(new ShaderCalcLight(_glinfo))
, m_shaderReadtex(new ShaderReadtex(_glinfo))
, m_shaderReadtexCopyMode(new ShaderReadtexCopyMode(_glinfo))
, m_shaderN64DepthCompare(new ShaderN64DepthCompare(_glinfo))
, m_shaderN64DepthRender(new ShaderN64DepthRender(_glinfo))
, m_shaderTextureEngine(new ShaderTextureEngine(_glinfo))
, m_useProgram(_useProgram)
, m_combinerOptionsBits(graphics::CombinerProgram::getShaderCombinerOptionsBits())
{
	m_vertexShaderRect = _createVertexShader(m_vertexHeader.get(), m_vertexRect.get(), m_vertexEnd.get());
	m_vertexShaderTriangle = _createVertexShader(m_vertexHeader.get(), m_vertexTriangle.get(), m_vertexEnd.get());
	m_vertexShaderTexturedRect = _createVertexShader(m_vertexHeader.get(), m_vertexTexturedRect.get(), m_vertexEnd.get());
	m_vertexShaderTexturedTriangle = _createVertexShader(m_vertexHeader.get(), m_vertexTexturedTriangle.get(), m_vertexEnd.get());
	m_uniformFactory.reset(new CombinerProgramUniformFactory(_glinfo));
}

CombinerProgramBuilder::~CombinerProgramBuilder()
{
	glDeleteShader(m_vertexShaderRect);
	glDeleteShader(m_vertexShaderTriangle);
	glDeleteShader(m_vertexShaderTexturedRect);
	glDeleteShader(m_vertexShaderTexturedTriangle);
}

bool CombinerProgramBuilder::isObsolete() const
{
	return m_combinerOptionsBits != graphics::CombinerProgram::getShaderCombinerOptionsBits();
}
