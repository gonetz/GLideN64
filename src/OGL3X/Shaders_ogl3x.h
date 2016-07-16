#if defined(GLES3_1)
#define MAIN_SHADER_VERSION "#version 310 es \n"
#define AUXILIARY_SHADER_VERSION "\n"
#elif defined(GLES3)
#define MAIN_SHADER_VERSION "#version 300 es \n"
#define AUXILIARY_SHADER_VERSION "\n"
#else
#define MAIN_SHADER_VERSION "#version 330 core \n"
#define AUXILIARY_SHADER_VERSION "#version 330 core \n"
#endif

static const char* vertex_shader =
MAIN_SHADER_VERSION
"in highp vec4 aPosition;							\n"
"in lowp vec4 aColor;								\n"
"in highp vec2 aTexCoord0;							\n"
"in highp vec2 aTexCoord1;							\n"
"in lowp float aNumLights;							\n"
"in highp vec4 aModify;								\n"
"													\n"
"uniform int uRenderState;							\n"
"uniform int uTexturePersp;							\n"
"													\n"
"uniform lowp int uFogUsage;						\n"
"uniform mediump vec2 uFogScale;					\n"
"uniform mediump vec2 uScreenCoordsScale;			\n"
"													\n"
#ifdef GL_USE_UNIFORMBLOCK
"layout (std140) uniform TextureBlock {				\n"
"  mediump vec2 uTexScale;							\n"
"  mediump vec2 uTexOffset[2];						\n"
"  mediump vec2 uCacheScale[2];						\n"
"  mediump vec2 uCacheOffset[2];					\n"
"  mediump vec2 uCacheShiftScale[2];				\n"
"  lowp ivec2 uCacheFrameBuffer;					\n"
"};													\n"
#else
"uniform mediump vec2 uTexScale;					\n"
"uniform mediump vec2 uTexOffset[2];				\n"
"uniform mediump vec2 uCacheScale[2];				\n"
"uniform mediump vec2 uCacheOffset[2];				\n"
"uniform mediump vec2 uCacheShiftScale[2];			\n"
"uniform lowp ivec2 uCacheFrameBuffer;				\n"
#endif //  GL_USE_UNIFORMBLOCK
"out lowp vec4 vShadeColor;							\n"
"out mediump vec2 vTexCoord0;						\n"
"out mediump vec2 vTexCoord1;						\n"
"out mediump vec2 vLodTexCoord;						\n"
"out lowp float vNumLights;							\n"

"mediump vec2 calcTexCoord(in vec2 texCoord, in int idx)		\n"
"{																\n"
"    vec2 texCoordOut = texCoord*uCacheShiftScale[idx];			\n"
"    texCoordOut -= uTexOffset[idx];							\n"
"    if (uCacheFrameBuffer[idx] != 0)							\n"
"      texCoordOut.t = -texCoordOut.t;							\n"
"    return (uCacheOffset[idx] + texCoordOut)* uCacheScale[idx];\n"
"}																\n"
"																\n"
"void main()													\n"
"{																\n"
"  gl_Position = aPosition;										\n"
"  vShadeColor = aColor;										\n"
"  if (uRenderState < 3) {										\n"
"    vec2 texCoord = aTexCoord0;								\n"
"    texCoord *= uTexScale;										\n"
"    if (uTexturePersp == 0 && aModify[2] == 0.0) texCoord *= 0.5;\n"
"    vTexCoord0 = calcTexCoord(texCoord, 0);					\n"
"    vTexCoord1 = calcTexCoord(texCoord, 1);					\n"
"    vLodTexCoord = texCoord;									\n"
"    vNumLights = aNumLights;									\n"
"    if (aModify != vec4(0.0)) {								\n"
"      if ((aModify[0]) != 0.0) {								\n"
"        gl_Position.xy = gl_Position.xy * uScreenCoordsScale + vec2(-1.0, 1.0);	\n"
"        gl_Position.xy *= gl_Position.w;						\n"
"      }														\n"
"      if ((aModify[1]) != 0.0)									\n"
"        gl_Position.z *= gl_Position.w;						\n"
"      if ((aModify[3]) != 0.0)									\n"
"        vNumLights = 0.0;										\n"
"    }															\n"
"    if (uFogUsage == 1) {										\n"
"      lowp float fp;											\n"
"      if (aPosition.z < -aPosition.w && aModify[1] == 0.0)		\n"
"        fp = -uFogScale.s + uFogScale.t;						\n"
"      else														\n"
"        fp = aPosition.z/aPosition.w*uFogScale.s + uFogScale.t;\n"
"      vShadeColor.a = clamp(fp, 0.0, 1.0);						\n"
"    }															\n"
"  } else {														\n"
"    vTexCoord0 = aTexCoord0;									\n"
"    vTexCoord1 = aTexCoord1;									\n"
"    vNumLights = 0.0;											\n"
"  }															\n"
#ifndef GLESX
"  gl_ClipDistance[0] = gl_Position.w - gl_Position.z;			\n"
#endif
"}																\n"
;

static const char* vertex_shader_notex =
MAIN_SHADER_VERSION
"in highp vec4 aPosition;			\n"
"in lowp vec4 aColor;				\n"
"in lowp float aNumLights;			\n"
"in highp vec4 aModify;				\n"
"									\n"
"uniform int uRenderState;			\n"
"									\n"
"uniform lowp int uFogUsage;		\n"
"uniform mediump vec2 uFogScale;	\n"
"uniform mediump vec2 uScreenCoordsScale;\n"
"									\n"
"out lowp vec4 vShadeColor;			\n"
"out lowp float vNumLights;			\n"
"																\n"
"void main()													\n"
"{																\n"
"  gl_Position = aPosition;										\n"
"  vShadeColor = aColor;										\n"
"  if (uRenderState < 3) {										\n"
"    vNumLights = aNumLights;									\n"
"    if (aModify != vec4(0.0)) {								\n"
"      if ((aModify[0]) != 0.0) {								\n"
"        gl_Position.xy = gl_Position.xy * uScreenCoordsScale + vec2(-1.0, 1.0);	\n"
"        gl_Position.xy *= gl_Position.w;						\n"
"      }														\n"
"      if ((aModify[1]) != 0.0) 								\n"
"        gl_Position.z *= gl_Position.w;						\n"
"      if ((aModify[3]) != 0.0)									\n"
"        vNumLights = 0.0;										\n"
"    }															\n"
"    if (uFogUsage == 1) {										\n"
"      lowp float fp;											\n"
"      if (aPosition.z < -aPosition.w && aModify[1] == 0.0)		\n"
"        fp = -uFogScale.s + uFogScale.t;						\n"
"      else														\n"
"        fp = aPosition.z/aPosition.w*uFogScale.s + uFogScale.t;\n"
"      vShadeColor.a = clamp(fp, 0.0, 1.0);						\n"
"    }															\n"
"  } else {														\n"
"    vNumLights = 0.0;											\n"
"  }															\n"
#ifndef GLESX
"  gl_ClipDistance[0] = gl_Position.w - gl_Position.z;			\n"
#endif
"}																\n"
;

static const char* fragment_shader_header_common_variables =
MAIN_SHADER_VERSION
"uniform sampler2D uTex0;		\n"
"uniform sampler2D uTex1;		\n"
"uniform sampler2D uDepthTex;	\n"
#ifdef GL_USE_UNIFORMBLOCK
"layout (std140) uniform ColorsBlock {\n"
"  lowp vec4 uFogColor;			\n"
"  lowp vec4 uCenterColor;		\n"
"  lowp vec4 uScaleColor;		\n"
"  lowp vec4 uBlendColor;		\n"
"  lowp vec4 uEnvColor;			\n"
"  lowp vec4 uPrimColor;		\n"
"  lowp float uPrimLod;			\n"
"  lowp float uK4;				\n"
"  lowp float uK5;				\n"
"};								\n"
#else
"uniform lowp vec4 uFogColor;	\n"
"uniform lowp vec4 uCenterColor;\n"
"uniform lowp vec4 uScaleColor;	\n"
"uniform lowp vec4 uBlendColor;	\n"
"uniform lowp vec4 uEnvColor;	\n"
"uniform lowp vec4 uPrimColor;	\n"
"uniform lowp float uPrimLod;	\n"
"uniform lowp float uK4;		\n"
"uniform lowp float uK5;		\n"
#endif // GL_USE_UNIFORMBLOCK
#ifdef GLESX
"uniform mediump vec2 uScreenScale;	\n"
#endif
"uniform lowp int uAlphaCompareMode;\n"
"uniform lowp int uAlphaDitherMode;	\n"
"uniform lowp int uColorDitherMode;	\n"
"uniform lowp int uFogUsage;		\n"
"uniform lowp ivec2 uFbMonochrome;	\n"
"uniform lowp ivec2 uFbFixedAlpha;	\n"
"uniform lowp int uEnableAlphaTest;	\n"
"uniform lowp int uCvgXAlpha;		\n"
"uniform lowp int uAlphaCvgSel;		\n"
"uniform lowp int uRenderTarget;	\n"
"uniform lowp float uAlphaTestValue;\n"
"uniform mediump vec2 uDepthScale;	\n"
"uniform lowp ivec4 uBlendMux1;		\n"
"uniform lowp int uForceBlendCycle1;\n"
"in lowp vec4 vShadeColor;			\n"
"in mediump vec2 vTexCoord0;		\n"
"in mediump vec2 vTexCoord1;		\n"
"in mediump vec2 vLodTexCoord;		\n"
"in lowp float vNumLights;			\n"
"out lowp vec4 fragColor;			\n"
;

#ifdef GL_MULTISAMPLING_SUPPORT
static const char* fragment_shader_header_common_variables_ms_enabled =
	"uniform lowp ivec2 uMSTexEnabled;      \n";

static const char* fragment_shader_header_common_variables_ms_tex0 =
	"uniform lowp sampler2DMS uMSTex0;      \n";

static const char* fragment_shader_header_common_variables_ms_tex1 =
	"uniform lowp sampler2DMS uMSTex1;      \n";
#endif

static const char* fragment_shader_header_common_variables_notex =
MAIN_SHADER_VERSION
"uniform sampler2D uDepthTex;	\n"
#ifdef GL_USE_UNIFORMBLOCK
"layout (std140) uniform ColorsBlock {\n"
"  lowp vec4 uFogColor;			\n"
"  lowp vec4 uCenterColor;		\n"
"  lowp vec4 uScaleColor;		\n"
"  lowp vec4 uBlendColor;		\n"
"  lowp vec4 uEnvColor;			\n"
"  lowp vec4 uPrimColor;		\n"
"  lowp float uPrimLod;			\n"
"  lowp float uK4;				\n"
"  lowp float uK5;				\n"
"};								\n"
#else
"uniform lowp vec4 uFogColor;	\n"
"uniform lowp vec4 uCenterColor;\n"
"uniform lowp vec4 uScaleColor;	\n"
"uniform lowp vec4 uBlendColor;	\n"
"uniform lowp vec4 uEnvColor;	\n"
"uniform lowp vec4 uPrimColor;	\n"
"uniform lowp float uPrimLod;	\n"
"uniform lowp float uK4;		\n"
"uniform lowp float uK5;		\n"
#endif // GL_USE_UNIFORMBLOCK
#ifdef GLESX
"uniform mediump vec2 uScreenScale;	\n"
#endif
"uniform lowp int uAlphaCompareMode;\n"
"uniform lowp int uAlphaDitherMode;	\n"
"uniform lowp int uColorDitherMode;	\n"
"uniform lowp int uFogUsage;		\n"
"uniform lowp int uEnableAlphaTest;	\n"
"uniform lowp int uCvgXAlpha;		\n"
"uniform lowp int uAlphaCvgSel;		\n"
"uniform lowp int uRenderTarget;	\n"
"uniform lowp float uAlphaTestValue;\n"
"uniform mediump vec2 uDepthScale;	\n"
"uniform lowp ivec4 uBlendMux1;		\n"
"uniform lowp int uForceBlendCycle1;\n"
"in lowp vec4 vShadeColor;			\n"
"in lowp float vNumLights;			\n"
"out lowp vec4 fragColor;			\n"
;

static const char* fragment_shader_header_common_variables_blend_mux_2cycle =
"uniform lowp ivec4 uBlendMux2;			\n"
"uniform lowp int uForceBlendCycle2;	\n"
;

static const char* fragment_shader_header_noise =
	"lowp float snoise();\n";
static const char* fragment_shader_header_write_depth =
	"void writeDepth();\n";
static const char* fragment_shader_header_calc_light =
	"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color);\n";
static const char* fragment_shader_header_mipmap =
	"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1);\n";
static const char* fragment_shader_header_readTex =
	"lowp vec4 readTex(in sampler2D tex, in mediump vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha);\n";
#ifdef GL_MULTISAMPLING_SUPPORT
static const char* fragment_shader_header_readTexMS =
	"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in mediump vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha);\n";
#endif // GL_MULTISAMPLING_SUPPORT
#ifdef GL_IMAGE_TEXTURES_SUPPORT
static const char* fragment_shader_header_depth_compare =
	"bool depth_compare();\n";
#endif  // GL_IMAGE_TEXTURES_SUPPORT
static const char* fragment_shader_header_noise_dither =
	"void colorNoiseDither(in lowp float _noise, inout lowp vec3 _color);\n"
	"void alphaNoiseDither(in lowp float _noise, inout lowp float _alpha);\n";
#ifdef USE_TOONIFY
static const char* fragment_shader_header_alpha_noise_toonify =
	"void toonify(in mediump float intensity);\n"
#endif

static const char* fragment_shader_calc_light =
AUXILIARY_SHADER_VERSION
#ifdef GL_USE_UNIFORMBLOCK
"layout (std140) uniform LightBlock {		\n"
"  mediump vec3 uLightDirection[8];			\n"
"  lowp vec3 uLightColor[8];				\n"
"};											\n"
#else
"uniform mediump vec3 uLightDirection[8];	\n"
"uniform lowp vec3 uLightColor[8];			\n"
#endif // GL_USE_UNIFORMBLOCK
"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color) {\n"
"  output_color = input_color;									\n"
"  lowp int nLights = int(floor(fLights + 0.5));				\n"
"  if (nLights == 0)											\n"
"     return;													\n"
"  output_color = uLightColor[nLights];							\n"
"  mediump float intensity;										\n"
"  mediump vec3 n = normalize(input_color);						\n"
"  for (int i = 0; i < nLights; i++)	{						\n"
"    intensity = max(dot(n, uLightDirection[i]), 0.0);			\n"
"    output_color += intensity*uLightColor[i];					\n"
"  };															\n"
"  clamp(output_color, 0.0, 1.0);								\n"
"}																\n"
;

static const char* fragment_shader_header_main =
"									\n"
"void main()						\n"
"{									\n"
"  writeDepth();                                                                                    \n"
"  lowp mat4 muxPM = mat4(vec4(0.0), vec4(0.0), uBlendColor, uFogColor);							\n"
"  lowp vec4 muxA = vec4(0.0, uFogColor.a, vShadeColor.a, 0.0);										\n"
"  lowp vec4 muxB = vec4(0.0, 1.0, 1.0, 0.0);														\n"
"  lowp vec4 vec_color, combined_color;																\n"
"  lowp float alpha1, alpha2;																		\n"
"  lowp vec3 color1, color2, input_color;															\n"
;

static const char* fragment_shader_dither =
AUXILIARY_SHADER_VERSION
"void colorNoiseDither(in lowp float _noise, inout lowp vec3 _color)	\n"
"{															\n"
"    mediump vec3 tmpColor = _color*255.0;					\n"
"    mediump ivec3 iColor = ivec3(tmpColor);				\n"
//"    iColor &= 248;										\n" // does not work with HW lighting enabled (why?!)
"    iColor |= ivec3(tmpColor*_noise)&7;					\n"
"    _color = vec3(iColor)/255.0;							\n"
"}															\n"
"void alphaNoiseDither(in lowp float _noise, inout lowp float _alpha)	\n"
"{															\n"
"    mediump float tmpAlpha = _alpha*255.0;					\n"
"    mediump int iAlpha = int(tmpAlpha);					\n"
//"    iAlpha &= 248;											\n" // causes issue #518. need further investigation
"    iAlpha |= int(tmpAlpha*_noise)&7;						\n"
"    _alpha = float(iAlpha)/255.0;							\n"
"}															\n"
;

#ifdef USE_TOONIFY
static const char* fragment_shader_toonify =
"																	\n"
"void toonify(in mediump float intensity) {							\n"
"   if (intensity > 0.5)											\n"
"	   return;														\n"
"	else if (intensity > 0.125)										\n"
"		fragColor = vec4(vec3(fragColor)*0.5, fragColor.a);\n"
"	else															\n"
"		fragColor = vec4(vec3(fragColor)*0.2, fragColor.a);\n"
"}																	\n"
;
#endif

static const char* fragment_shader_end =
"}                               \n"
;

static const char* fragment_shader_mipmap =
#ifndef GLESX
AUXILIARY_SHADER_VERSION
"in mediump vec2 vTexCoord0;	\n"
"in mediump vec2 vTexCoord1;	\n"
"in mediump vec2 vLodTexCoord;	\n"
"uniform sampler2D uTex0;			\n"
"uniform sampler2D uTex1;			\n"
"uniform mediump vec2 uScreenScale;	\n"
#endif
"uniform lowp int uEnableLod;		\n"
"uniform mediump float uMinLod;		\n"
"uniform lowp int uMaxTile;			\n"
"uniform lowp int uTextureDetail;	\n"
"														\n"
"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
"  readtex0 = texture(uTex0, vTexCoord0);				\n"
"  readtex1 = textureLod(uTex1, vTexCoord1, 0.0);		\n"
"														\n"
"  mediump float fMaxTile = float(uMaxTile);			\n"
#if 1
"  mediump vec2 dx = abs(dFdx(vLodTexCoord));			\n"
"  dx *= uScreenScale;									\n"
"  mediump float lod = max(dx.x, dx.y);					\n"
#else
"  mediump vec2 dx = dFdx(vLodTexCoord);				\n"
"  dx *= uScreenScale;									\n"
"  mediump vec2 dy = dFdy(vLodTexCoord);				\n"
"  dy *= uScreenScale;									\n"
"  mediump float lod = max(length(dx), length(dy));		\n"
#endif
"  bool magnify = lod < 1.0;							\n"
"  mediump float lod_tile = magnify ? 0.0 : floor(log2(floor(lod))); \n"
"  bool distant = lod > 128.0 || lod_tile >= fMaxTile;	\n"
"  mediump float lod_frac = fract(lod/pow(2.0, lod_tile));	\n"
"  if (magnify) lod_frac = max(lod_frac, uMinLod);		\n"
"  if (uTextureDetail == 0)	{							\n"
"    if (distant) lod_frac = 1.0;						\n"
"    else if (magnify) lod_frac = 0.0;					\n"
"  }													\n"
"  if (magnify && ((uTextureDetail & 1) != 0))			\n"
"      lod_frac = 1.0 - lod_frac;						\n"
"  if (uMaxTile == 0) {									\n"
"    if (uEnableLod != 0 && (uTextureDetail & 2) == 0)	\n"
"      readtex1 = readtex0;								\n"
"    return lod_frac;									\n"
"  }													\n"
"  if (uEnableLod == 0) return lod_frac;				\n"
"														\n"
"  lod_tile = min(lod_tile, fMaxTile);					\n"
"  lowp float lod_tile_m1 = max(0.0, lod_tile - 1.0);	\n"
"  lowp vec4 lodT = textureLod(uTex1, vTexCoord1, lod_tile);	\n"
"  lowp vec4 lodT_m1 = textureLod(uTex1, vTexCoord1, lod_tile_m1);	\n"
"  lowp vec4 lodT_p1 = textureLod(uTex1, vTexCoord1, lod_tile + 1.0);	\n"
"  if (lod_tile < 1.0) {								\n"
"    if (magnify) {									\n"
//     !sharpen && !detail
"      if (uTextureDetail == 0) readtex1 = readtex0;	\n"
"    } else {											\n"
//     detail
"      if ((uTextureDetail & 2) != 0 ) {				\n"
"        readtex0 = lodT;								\n"
"        readtex1 = lodT_p1;							\n"
"      }												\n"
"    }													\n"
"  } else {												\n"
"    if ((uTextureDetail & 2) != 0 ) {							\n"
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

static const char* fragment_shader_fake_mipmap =
"uniform lowp int uMaxTile;			\n"
"uniform mediump float uMinLod;		\n"
"														\n"
"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
"  readtex0 = texture(uTex0, vTexCoord0);				\n"
"  readtex1 = texture(uTex1, vTexCoord1);				\n"
"  if (uMaxTile == 0) return 1.0;						\n"
"  return uMinLod;										\n"
"}														\n"
;

static const char* fragment_shader_readtex =
AUXILIARY_SHADER_VERSION
"lowp vec4 readTex(in sampler2D tex, in mediump vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
"{												\n"
"  lowp vec4 texColor = texture(tex, texCoord);	\n"
"  if (fbMonochrome == 1) texColor = vec4(texColor.r);	\n"
"  else if (fbMonochrome == 2) 						\n"
"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
"  if (fbFixedAlpha == 1) texColor.a = 0.825;		\n"
"  return texColor;								\n"
"}												\n"
;

static const char* fragment_shader_readtex_3point =
AUXILIARY_SHADER_VERSION
"uniform lowp int uTextureFilterMode;								\n"
// 3 point texture filtering.
// Original author: ArthurCarvalho
// GLSL implementation: twinaphex, mupen64plus-libretro project.
"#define TEX_OFFSET(off) texture(tex, texCoord - (off)/texSize)	\n"
"lowp vec4 filter3point(in sampler2D tex, in mediump vec2 texCoord)			\n"
"{																			\n"
"  mediump vec2 texSize = vec2(textureSize(tex,0));							\n"
"  mediump vec2 offset = fract(texCoord*texSize - vec2(0.5));	\n"
"  offset -= step(1.0, offset.x + offset.y);								\n"
"  lowp vec4 c0 = TEX_OFFSET(offset);										\n"
"  lowp vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));	\n"
"  lowp vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));	\n"
"  return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);				\n"
"}																			\n"
"lowp vec4 readTex(in sampler2D tex, in mediump vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
"{																			\n"
"  lowp vec4 texStandard = texture(tex, texCoord); 							\n"
"  lowp vec4 tex3Point = filter3point(tex, texCoord); 						\n"
"  lowp vec4 texColor = uTextureFilterMode == 0 ? texStandard : tex3Point;	\n"
"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
"  else if (fbMonochrome == 2) 												\n"
"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
"  if (fbFixedAlpha == 1) texColor.a = 0.825;									\n"
"  return texColor;															\n"
"}																			\n"
;

#ifdef GL_MULTISAMPLING_SUPPORT
static const char* fragment_shader_readtex_ms =
AUXILIARY_SHADER_VERSION
"uniform lowp int uMSAASamples;	\n"
"uniform lowp float uMSAAScale;	\n"
"lowp vec4 sampleMS(in lowp sampler2DMS mstex, in mediump ivec2 ipos)			\n"
"{																			\n"
"  lowp vec4 texel = vec4(0.0);												\n"
"  for (int i = 0; i < uMSAASamples; ++i)									\n"
"    texel += texelFetch(mstex, ipos, i);									\n"
"  return texel * uMSAAScale;												\n"
"}																			\n"
"																			\n"
"lowp vec4 readTexMS(in lowp sampler2DMS mstex, in mediump vec2 texCoord, in lowp int fbMonochrome, in lowp int fbFixedAlpha)	\n"
"{																			\n"
"  mediump vec2 msTexSize = vec2(textureSize(mstex));						\n"
"  mediump ivec2 itexCoord = ivec2(msTexSize * texCoord);					\n"
"  lowp vec4 texColor = sampleMS(mstex, itexCoord);							\n"
"  if (fbMonochrome == 1) texColor = vec4(texColor.r);						\n"
"  else if (fbMonochrome == 2) 												\n"
"    texColor.rgb = vec3(dot(vec3(0.2126, 0.7152, 0.0722), texColor.rgb));	\n"
"  if (fbFixedAlpha == 1) texColor.a = 0.825;									\n"
"  return texColor;															\n"
"}																			\n"
;
#endif // GL_MULTISAMPLING_SUPPORT

static const char* fragment_shader_noise =
AUXILIARY_SHADER_VERSION
#ifndef GLESX
"uniform mediump vec2 uScreenScale;	\n"
#endif
"uniform sampler2D uTexNoise;		\n"
"lowp float snoise()									\n"
"{														\n"
"  ivec2 coord = ivec2(gl_FragCoord.xy/uScreenScale);	\n"
"  return texelFetch(uTexNoise, coord, 0).r;			\n"
"}														\n"
;

static const char* fragment_shader_dummy_noise =
"						\n"
"lowp float snoise()	\n"
"{						\n"
"  return 1.0;			\n"
"}						\n"
;

static const char* fragment_shader_depth =
AUXILIARY_SHADER_VERSION
#ifndef GLESX
"uniform mediump vec2 uDepthScale;																	\n"
#endif
"void writeDepth()						        		\n"
"{														\n"
"  gl_FragDepth = clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);	\n"
"}														\n"
;

static const char* fragment_shader_dummy_depth =
"void writeDepth()	    \n"
"{						\n"
"}						\n"
;

#ifdef GL_IMAGE_TEXTURES_SUPPORT
static const char* depth_compare_shader_float =
#ifndef GLESX
"#version 430								\n"
"layout(binding = 2, rg32f) uniform coherent image2D uDepthImage;\n"
#else
"layout(binding = 2, rgba32f) highp uniform coherent image2D uDepthImage;\n"
#endif
//"uniform int uEnableDepth;				\n"
"uniform lowp int uDepthMode;				\n"
"uniform lowp int uDepthSource;				\n"
"uniform lowp int uEnableDepthCompare;		\n"
"uniform lowp int uEnableDepthUpdate;		\n"
"uniform mediump float uDeltaZ;				\n"
"bool depth_compare()									\n"
"{														\n"
//"  if (uEnableDepth == 0) return true;					\n"
"  ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
"  highp vec4 depth = imageLoad(uDepthImage,coord);		\n"
"  highp float bufZ = depth.r;							\n"
"  highp float curZ = gl_FragDepth;						\n"
"  highp float dz, dzMin;								\n"
"  if (uDepthSource == 1) {								\n"
"     dzMin = dz = uDeltaZ;								\n"
"  } else {												\n"
"    dz = 4.0*fwidth(gl_FragDepth);						\n"
"    dzMin = min(dz, depth.g);							\n"
"  }													\n"
"  bool bInfront = curZ < bufZ;							\n"
"  bool bFarther = (curZ + dzMin) >= bufZ;				\n"
"  bool bNearer = (curZ - dzMin) <= bufZ;				\n"
"  bool bMax = bufZ == 1.0;								\n"
"  bool bRes;											\n"
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
"     default:											\n"
"       bRes = bInfront;								\n"
"       break;											\n"
"  }													\n"
"  if (uEnableDepthUpdate != 0  && bRes) {				\n"
"    highp vec4 depth_out = vec4(gl_FragDepth, dz, 1.0, 1.0); \n"
"    imageStore(uDepthImage,coord, depth_out);			\n"
"  }													\n"
"  memoryBarrierImage();								\n"
"  if (uEnableDepthCompare != 0)						\n"
"    return bRes;										\n"
"  return true;											\n"
"}														\n"
;

static const char* shadow_map_fragment_shader_float =
#ifndef GLESX
"#version 420 core											\n"
"layout(binding = 0, r16ui) uniform readonly uimage2D uZlutImage;\n"
"layout(binding = 1, r16ui) uniform readonly uimage2D uTlutImage;\n"
#else
MAIN_SHADER_VERSION
"layout(binding = 0, r32ui) highp uniform readonly uimage2D uZlutImage;\n"
"layout(binding = 1, r32ui) highp uniform readonly uimage2D uTlutImage;\n"
#endif
"layout(binding = 0) uniform sampler2D uDepthImage;		\n"
"uniform lowp vec4 uFogColor;								\n"
"out lowp vec4 fragColor;									\n"
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
#endif // GL_IMAGE_TEXTURES_SUPPORT

static const char* default_vertex_shader =
MAIN_SHADER_VERSION
"in highp vec4 	aPosition;								\n"
"void main()                                                    \n"
"{                                                              \n"
"  gl_Position = aPosition;										\n"
"}                                                              \n"
;

#if 0 // Do palette based monochrome image. Exactly as N64 does
static const char* zelda_monochrome_fragment_shader =
"#version 420 core											\n"
"layout(binding = 0) uniform sampler2D uColorImage;			\n"
"layout(binding = 1, r16ui) uniform readonly uimage2D uTlutImage;\n"
"out lowp vec4 fragColor;									\n"
"lowp float get_color()										\n"
"{															\n"
"  ivec2 coord = ivec2(gl_FragCoord.xy);					\n"
"  vec4 color = 31.0*texelFetch(uColorImage, coord, 0);		\n"
"  int r = int(color.r); int g = int(color.g); int b = int(color.b);\n"
//"  int a = 0; if ((color.r + color.g + color.b) > 0) a = 32768;\n"
//"  int color16 = 32768 + r*1024 + g*32 + b;		\n"
"  int color16 = r*1024 + g*32 + b;						\n"
"  int index = min(255, color16/256);					\n"
"  uint iAlpha = imageLoad(uTlutImage,ivec2(index,0)).r;			\n"
"  memoryBarrier();										\n"
"  return clamp(float((iAlpha&255) + index)/255.0, 0.0, 1.0); \n"
"}														\n"
"void main()											\n"
"{														\n"
"  fragColor = vec4(vec3(get_color()), 1.0);			\n"
"}														\n"
;
#else // Cheat it
static const char* zelda_monochrome_fragment_shader =
MAIN_SHADER_VERSION
"uniform sampler2D uColorImage;							\n"
"out lowp vec4 fragColor;								\n"
"void main()											\n"
"{														\n"
"  mediump ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
"  lowp vec4 tex = texelFetch(uColorImage, coord, 0);		\n"
//"  lowp float c = (tex.r + tex.g + tex.b) / 3.0f;		\n"
"  lowp float c = dot(vec4(0.2126, 0.7152, 0.0722, 0.0), tex);\n"
"  fragColor = vec4(c, c, c, 1.0);						\n"
"}														\n"
;
#endif

static const char* depth_texture_fragment_shader =
MAIN_SHADER_VERSION
"uniform sampler2D uTex0;						\n"
"in mediump vec2 vTexCoord0;					\n"
"void main()									\n"
"{												\n"
"  gl_FragDepth = texture(uTex0, vTexCoord0).r;	\n"
"}												\n"
;

const char * strTexrectDrawerVertexShader =
MAIN_SHADER_VERSION
"in highp vec4 aPosition;		\n"
"in highp vec2 aTexCoord0;		\n"
"out mediump vec2 vTexCoord0;	\n"
"void main()					\n"
"{								\n"
"  gl_Position = aPosition;		\n"
"  vTexCoord0 = aTexCoord0;		\n"
"}								\n"
;

const char * strTexrectDrawerTex3PointFilter =
MAIN_SHADER_VERSION
"uniform mediump vec4 uTextureBounds;																			\n"
// 3 point texture filtering.
// Original author: ArthurCarvalho
// GLSL implementation: twinaphex, mupen64plus-libretro project.
"#define TEX_OFFSET(off) texture(tex, texCoord - (off)/texSize)													\n"
"lowp vec4 texFilter(in sampler2D tex, in mediump vec2 texCoord)												\n"
"{																												\n"
"  mediump vec2 texSize = vec2(textureSize(tex,0));																\n"
"  mediump vec2 texelSize = vec2(1.0) / texSize;																\n"
"  lowp vec4 c = texture(tex, texCoord);		 																\n"
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
"																											    \n"
;

const char * strTexrectDrawerTexBilinearFilter =
MAIN_SHADER_VERSION
"uniform mediump vec4 uTextureBounds;																			\n"
"#define TEX_OFFSET(off) texture(tex, texCoord - (off)/texSize)													\n"
"lowp vec4 texFilter(in sampler2D tex, in mediump vec2 texCoord)												\n"
"{																												\n"
"  mediump vec2 texSize = vec2(textureSize(tex,0));																\n"
"  mediump vec2 texelSize = vec2(1.0) / texSize;																\n"
"  lowp vec4 c = texture(tex, texCoord);																		\n"
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
"																												\n"
;

const char * strTexrectDrawerFragmentShaderTex =
"uniform sampler2D uTex0;																						\n"
"uniform lowp int uEnableAlphaTest;																				\n"
"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);												\n"
"in mediump vec2 vTexCoord0;																					\n"
"out lowp vec4 fragColor;																						\n"
"void main()																									\n"
"{																												\n"
"  fragColor = texFilter(uTex0, vTexCoord0);																	\n"
"  if (fragColor == uTestColor) discard;																		\n"
"  if (uEnableAlphaTest == 1 && !(fragColor.a > 0.0)) discard;													\n"
"}																												\n"
;

const char * strTexrectDrawerFragmentShaderClean =
MAIN_SHADER_VERSION
"lowp vec4 uTestColor = vec4(4.0/255.0, 2.0/255.0, 1.0/255.0, 0.0);	\n"
"out lowp vec4 fragColor;													\n"
"void main()																\n"
"{																			\n"
"  fragColor = uTestColor;													\n"
"}																			\n"
;

const char* strTextureCopyShader =
MAIN_SHADER_VERSION
"in mediump vec2 vTexCoord0;                            \n"
"uniform sampler2D uTex0;				                \n"
"out lowp vec4 fragColor;								\n"
"                                                       \n"
"void main()                                            \n"
"{                                                      \n"
"    fragColor = texture(uTex0, vTexCoord0);	        \n"
"}							                            \n"
;
