static const char* vertex_shader =
#ifdef SHADER_PRECISION
"#version 330 core										\n"
"in highp vec4 aPosition;						\n"
"in lowp vec4 aColor;							\n"
"in highp vec2 aTexCoord0;						\n"
"in highp vec2 aTexCoord1;						\n"
"in lowp float aNumLights;						\n"
"														\n"
"uniform int uRenderState;								\n"
"uniform int uTexturePersp;								\n"
"														\n"
"uniform int uFogMode;									\n"
"uniform lowp int uFogUsage;							\n"
"uniform lowp float uFogAlpha;							\n"
"uniform mediump float uFogMultiplier, uFogOffset;		\n"
"														\n"
"uniform mediump vec2 uTexScale;						\n"
"uniform mediump vec2 uTexOffset[2];					\n"
"uniform mediump vec2 uTexMask[2];						\n"
"uniform mediump vec2 uCacheShiftScale[2];				\n"
"uniform mediump vec2 uCacheScale[2];					\n"
"uniform mediump vec2 uCacheOffset[2];					\n"
"uniform lowp ivec2 uCacheFrameBuffer;					\n"
"														\n"
"out lowp vec4 vShadeColor;							\n"
"out mediump vec2 vTexCoord0;						\n"
"out mediump vec2 vTexCoord1;						\n"
"out mediump vec2 vLodTexCoord;						\n"
"out lowp float vNumLights;							\n"
"out mediump float vFogFragCoord;					\n"
#else
"#version 330 core										\n"
"in vec4	aPosition;								\n"
"in vec4	aColor;									\n"
"in vec2	aTexCoord0;								\n"
"in vec2	aTexCoord1;								\n"
"in float aNumLights;							\n"
"														\n"
"uniform int uRenderState;								\n"
"uniform int uTexturePersp;								\n"
"														\n"
"uniform int uFogMode;									\n"
"uniform int uFogUsage;									\n"
"uniform vec4 uFogColor;								\n"
"uniform float uFogMultiplier, uFogOffset;				\n"
"														\n"
"uniform vec2 	uTexScale;								\n"
"uniform vec2 	uTexOffset[2];							\n"
"uniform vec2 	uTexMask[2];							\n"
"uniform vec2 	uCacheShiftScale[2];					\n"
"uniform vec2 	uCacheScale[2];							\n"
"uniform vec2	uCacheOffset[2];						\n"
"uniform ivec2	uCacheFrameBuffer;						\n"
"														\n"
"out vec4 vShadeColor;								\n"
"out vec2 vTexCoord0;								\n"
"out vec2 vTexCoord1;								\n"
"out vec2 vLodTexCoord;								\n"
"out float vNumLights;								\n"
"out float vFogFragCoord;							\n"
#endif
"mediump vec2 calcTexCoord(in vec2 texCoord, in int idx)		\n"
"{																\n"
"    vec2 texCoordOut = texCoord*uCacheShiftScale[idx];			\n"
"    if (uCacheFrameBuffer[idx] != 0) {							\n"
"      if (uTexMask[idx] != vec2(0.0, 0.0))						\n"
"        texCoordOut -= mod(uTexOffset[idx], uTexMask[idx]);	\n"
"      else														\n"
"        texCoordOut -= uTexOffset[idx];						\n"
"      texCoordOut.t = -texCoordOut.t;							\n"
"    } else														\n"
"        texCoordOut -= uTexOffset[idx];						\n"
"    return (uCacheOffset[idx] + texCoordOut)* uCacheScale[idx];\n"
"}																\n"
"																\n"
"void main()													\n"
"{																\n"
"  gl_Position = aPosition;										\n"
"  vFogFragCoord = 0.0;											\n"
"  vShadeColor = aColor;										\n"
"  if (uRenderState == 1) {										\n"
"    vec2 texCoord = aTexCoord0;								\n"
"    texCoord *= uTexScale;										\n"
"    if (uTexturePersp == 0) texCoord *= 0.5;					\n"
"    vTexCoord0 = calcTexCoord(texCoord, 0);					\n"
"    vTexCoord1 = calcTexCoord(texCoord, 1);					\n"
"    vLodTexCoord = texCoord * uCacheShiftScale[0];				\n"
"    vNumLights = aNumLights;									\n"
"    switch (uFogMode) {										\n"
"      case 0:													\n"
"        if (aPosition.z < -aPosition.w)						\n"
"          vFogFragCoord = -uFogMultiplier + uFogOffset;		\n"
"        else													\n"
"          vFogFragCoord = aPosition.z/aPosition.w*uFogMultiplier \n"
"	                   + uFogOffset;							\n"
"      break;													\n"
"      case 1:													\n"
"          vFogFragCoord = uFogAlpha;							\n"
"      break;													\n"
"      case 2:													\n"
"          vFogFragCoord = 1.0 - uFogAlpha;						\n"
"      break;													\n"
"    }															\n"
"    vFogFragCoord = clamp(vFogFragCoord, 0.0, 1.0);			\n"
"    if ((uFogUsage&255) == 1 && uFogMode == 0)					\n"
"       vShadeColor.a = vFogFragCoord;							\n"
"  } else {														\n"
"    vTexCoord0 = aTexCoord0;									\n"
"    vTexCoord1 = aTexCoord1;									\n"
"    vNumLights = 0.0;											\n"
"    switch (uFogMode) {										\n"
"      case 1:													\n"
"          vFogFragCoord = uFogAlpha;							\n"
"      break;													\n"
"      case 2:													\n"
"          vFogFragCoord = 1.0 - uFogAlpha;						\n"
"      break;													\n"
"    }															\n"
"  }															\n"
"  gl_ClipDistance[0] = gl_Position.w - gl_Position.z;			\n"
"}																\n"
;

static const char* fragment_shader_header_common_variables =
#ifdef SHADER_PRECISION
"#version 330 core				\n"
"uniform sampler2D uTex0;		\n"
"uniform sampler2D uTex1;		\n"
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
"uniform lowp int uAlphaCompareMode;	\n"
"uniform lowp int uAlphaDitherMode;	\n"
"uniform lowp int uColorDitherMode;	\n"
"uniform lowp int uGammaCorrectionEnabled;	\n"
"uniform lowp int uFogUsage;	\n"
"uniform lowp int uFb8Bit;		\n"
"uniform lowp int uFbFixedAlpha;\n"
"uniform lowp int uSpecialBlendMode;\n"
"uniform mediump vec2 uDepthScale;	\n"
"in lowp vec4 vShadeColor;	\n"
"in mediump vec2 vTexCoord0;\n"
"in mediump vec2 vTexCoord1;\n"
"in mediump vec2 vLodTexCoord;\n"
"in lowp float vNumLights;	\n"
"in mediump float vFogFragCoord;\n"
"lowp vec3 input_color;			\n"
"out lowp vec4 fragColor;		\n"
#else
"#version 330 core										\n"
"uniform sampler2D uTex0;		\n"
"uniform sampler2D uTex1;		\n"
"uniform vec4 uPrimColor;		\n"
"uniform vec4 uEnvColor;		\n"
"uniform vec4 uCenterColor;		\n"
"uniform vec4 uScaleColor;		\n"
"uniform vec4 uBlendColor;		\n"
"uniform vec4 uFogColor;		\n"
"uniform float uK4;				\n"
"uniform float uK5;				\n"
"uniform float uPrimLod;		\n"
"uniform int uAlphaCompareMode;	\n"
"uniform int uAlphaDitherMode;	\n"
"uniform int uColorDitherMode;	\n"
"uniform int uGammaCorrectionEnabled;\n"
"uniform int uFogUsage;		\n"
"uniform int uFb8Bit;		\n"
"uniform int uFbFixedAlpha;	\n"
"uniform int uSpecialBlendMode;\n"
"in vec4 vShadeColor;		\n"
"in vec2 vTexCoord0;		\n"
"in vec2 vTexCoord1;		\n"
"in vec2 vLodTexCoord;		\n"
"in float vNumLights;		\n"
"in float vFogFragCoord;	\n"
"vec3 input_color;			\n"
"out vec4 fragColor;		\n"
#endif
;

static const char* fragment_shader_header_common_functions =
#ifdef SHADER_PRECISION
"															\n"
"lowp float snoise();						\n"
"void calc_light(in lowp float fLights, in lowp vec3 input_color, out lowp vec3 output_color);\n"
"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1);		\n"
"lowp vec4 readTex(in sampler2D tex, in mediump vec2 texCoord, in bool fb8bit, in bool fbFixedAlpha);	\n"
"bool depth_compare();										\n"
"bool alpha_test(in lowp float alphaValue);						\n"
"void colorNoiseDither(in float _noise, inout vec3 _color);	\n"
"void alphaNoiseDither(in float _noise, inout float _alpha);\n"
#else
"															\n"
"float snoise();										\n"
"float calc_light(in float fLights, in vec3 input_color, out vec3 output_color);\n"
"float mipmap(out vec4 readtex0, out vec4 readtex1);		\n"
"bool depth_compare();										\n"
"bool alpha_test(in float alphaValue);						\n"
#endif
#ifdef USE_TOONIFY
"void toonify(in mediump float intensity);	\n"
#endif
;

static const char* fragment_shader_calc_light =
#ifdef SHADER_PRECISION
"#version 330 core						\n"
"uniform mediump vec3 uLightDirection[8];	\n"
"uniform lowp vec3 uLightColor[8];	\n"
"																\n"
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
#else
"#version 330 core										\n"
"uniform vec3 uLightDirection[8];	\n"
"uniform vec3 uLightColor[8];	\n"
"																\n"
"float calc_light(in float fLights, in vec3 input_color, out vec3 output_color) {\n"
"  output_color = input_color;									\n"
"  int nLights = int(floor(fLights + 0.5));						\n"
"  if (nLights == 0)											\n"
"     return 1.0;												\n"
"  float full_intensity = 0.0;									\n"
"  output_color = uLightColor[nLights];							\n"
"  vec3 lightColor;												\n"
"  float intensity;												\n"
"  vec3 n = normalize(input_color);								\n"
"  for (int i = 0; i < nLights; i++)	{						\n"
"    intensity = max(dot(n, uLightDirection[i]), 0.0);			\n"
"    full_intensity += intensity;								\n"
"    lightColor = intensity*uLightColor[i];						\n"
"    output_color += lightColor;								\n"
"  };															\n"
"  clamp(output_color, 0.0, 1.0);								\n"
"  return full_intensity;										\n"
"}																\n"
#endif
;

static const char* alpha_test_fragment_shader =
#ifdef SHADER_PRECISION
"#version 330 core								\n"
"uniform lowp int uEnableAlphaTest;				\n"
"uniform lowp float uAlphaTestValue;			\n"
"bool alpha_test(in lowp float alphaValue)		\n"
#else
"#version 330 core										\n"
"uniform int uEnableAlphaTest;				\n"
"uniform float uAlphaTestValue;				\n"
"bool alpha_test(in float alphaValue)		\n"
#endif
"{											\n"
"  if (uEnableAlphaTest == 0) return true;	\n"
"  if (uAlphaTestValue > 0.0) return alphaValue >= uAlphaTestValue;\n"
"  return alphaValue > 0.0;					\n"
"}											\n"
;

static const char* fragment_shader_header_main =
#ifdef SHADER_PRECISION
"									\n"
"void main()						\n"
"{									\n"
"  gl_FragDepth = clamp((gl_FragCoord.z * 2.0 - 1.0) * uDepthScale.s + uDepthScale.t, 0.0, 1.0);   \n"
"  if (uAlphaCompareMode == 3) {//dither \n"
"    if (snoise() < 0.0) discard; \n"
"  }								\n"
"  lowp vec4 vec_color, combined_color;	\n"
"  lowp float alpha1, alpha2;			\n"
"  lowp vec3 color1, color2;				\n"
;
#else
"									\n"
"void main()						\n"
"{									\n"
"  if (uAlphaCompareMode == 3) {//dither \n"
"    if (snoise() < 0.0) discard; \n"
"  }								\n"
"  vec4 vec_color, combined_color;	\n"
"  float alpha1, alpha2;			\n"
"  vec3 color1, color2;				\n"
;
#endif

static const char* fragment_shader_dither =
"#version 330 core					\n"
"void colorNoiseDither(in float _noise, inout vec3 _color)	\n"
"{															\n"
"    mediump vec3 tmpColor = _color*255.0;					\n"
"    mediump ivec3 iColor = ivec3(tmpColor);				\n"
//"    iColor &= 248;										\n" // does not work with HW lighting enabled (why?!)
"    iColor |= ivec3(tmpColor*_noise)&7;					\n"
"    _color = vec3(iColor)/255.0;							\n"
"}															\n"
"void alphaNoiseDither(in float _noise, inout float _alpha)	\n"
"{															\n"
"    mediump float tmpAlpha = _alpha*255.0;					\n"
"    mediump int iAlpha = int(tmpAlpha)&248;				\n"
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

#ifdef SHADER_PRECISION
#if 0
static const char* fragment_shader_blender =
"  switch (uSpecialBlendMode) {							\n"
"	case 1:												\n"
// Mace
"		color1 = color1 * alpha1 + uBlendColor.rgb * (1.0 - alpha1); \n"
"	break;												\n"
// Donald Duck
"	case 2:												\n"
"		color1 = uFogColor.rgb*uFogColor.a + color1*(1.0-alpha1);\n"
"	break;												\n"
"  }													\n"
;
#else
static const char* fragment_shader_blender =
"  switch (uSpecialBlendMode) {	\n"
"    case 1:					\n"
// Mace
"		color1 = color1 * alpha1 + uBlendColor.rgb * (1.0 - alpha1); \n"
"    break;						\n"
"    case 2:					\n"
// Bomberman2
"		color1 = uBlendColor.rgb * uFogColor.a + color1 * (1.0 - uFogColor.a); \n"
"    break;						\n"
"    case 3:					\n"
// Conker BFD
"		color1 = color1 * uFogColor.a + uFogColor.rgb * (1.0 - uFogColor.a); \n"
"    break;						\n"
"  }							\n"
;
#endif

static const char* fragment_shader_end =
"}                               \n"
#else
static const char* fragment_shader_blender =
"  switch (uSpecialBlendMode) {	\n"
"    case 1:					\n"
// Mace
"		color1 = color1 * alpha1 + uBlendColor.rgb * (1.0 - alpha1); \n"
"    break;						\n"
"    case 2:					\n"
// Bomberman2
"		color1 = uBlendColor.rgb * uFogColor.a + color1.rgb * (1.0 - uFogColor.a); \n"
"    break;						\n"
"  }							\n"
;

static const char* fragment_shader_end =
"}                               \n"
#endif
;

static const char* fragment_shader_mipmap =
#ifdef SHADER_PRECISION
"#version 330 core					\n"
"in mediump vec2 vTexCoord0;	\n"
"in mediump vec2 vTexCoord1;	\n"
"in mediump vec2 vLodTexCoord;	\n"
"uniform sampler2D uTex0;			\n"
"uniform sampler2D uTex1;			\n"
"uniform lowp float uPrimitiveLod;		\n"
"uniform lowp int uEnableLod;		\n"
"uniform mediump vec2 uScreenScale;	\n"
"uniform mediump float uMinLod;		\n"
"uniform lowp int uMaxTile;			\n"
"uniform lowp int uTextureDetail;	\n"
"														\n"
"mediump float mipmap(out lowp vec4 readtex0, out lowp vec4 readtex1) {	\n"
"  if (uEnableLod == 0) {								\n"
"    readtex0 = texture(uTex0, vTexCoord0);			\n"
"    readtex1 = texture(uTex1, vTexCoord1);			\n"
"    return uPrimitiveLod;									\n"
"  }													\n"
"  if (uMaxTile == 0) {									\n"
"    readtex0 = texture(uTex0, vTexCoord0);				\n"
"    readtex1 = readtex0;								\n"
"    return uMinLod;									\n"
"  }													\n"
"  mediump vec2 dx = dFdx(vLodTexCoord);				\n"
"  dx *= uScreenScale;									\n"
"  mediump vec2 dy = dFdy(vLodTexCoord);				\n"
"  dy *= uScreenScale;									\n"
"  mediump float lod = max(length(dx), length(dy));		\n"
"  mediump float lod_tile, lod_frac;					\n"
"  bool magnifying = false;								\n"
"  if (lod < 1.0) {										\n"
"    magnifying = true;									\n"
"    lod_tile = 0.0;									\n"
"    lod_frac = lod;									\n"
"    if (uTextureDetail != 0)							\n"
"      lod_frac = max(lod, uMinLod);					\n"
"    if (uTextureDetail == 1)							\n"
"      lod_frac = 1.0 - lod_frac;						\n"
"  } else {												\n"
"    lod_tile = min(float(uMaxTile), floor(log2(floor(lod)))); \n"
"    lod_frac = fract(lod/pow(2.0, lod_tile));			\n"
"  }													\n"
"  if (lod_tile < 1.0) {								\n"
"    if (magnifying) {									\n"
"      readtex0 = texture(uTex0, vTexCoord0);			\n"
//     !sharpen && !detail
"      if (uTextureDetail == 0) readtex1 = readtex0;	\n"
"      else readtex1 = texture(uTex1, vTexCoord1);	\n"
"    } else {											\n"
//     detail
"      if (uTextureDetail == 2) {						\n"
"        readtex0 = textureLod(uTex1, vTexCoord1, 0.0);\n"
"        readtex1 = textureLod(uTex1, vTexCoord1, 1.0);\n"
"      } else {											\n"
"        readtex0 = texture(uTex0, vTexCoord0);		\n"
"        readtex1 = texture(uTex1, vTexCoord1);		\n"
"      }												\n"
"    }													\n"
"  } else {												\n"
"    if (uTextureDetail == 2) {							\n"
"      readtex0 = textureLod(uTex1, vTexCoord1, lod_tile);		\n"
"      readtex1 = textureLod(uTex1, vTexCoord1, lod_tile + 1.0);	\n"
"    } else {														\n"
"      readtex0 = textureLod(uTex1, vTexCoord1, lod_tile - 1.0);	\n"
"      readtex1 = textureLod(uTex1, vTexCoord1, lod_tile);		\n"
"    }																\n"
"  }																\n"
"  return lod_frac;													\n"
"}																	\n"
#else
"#version 330 core										\n"
"in vec2 vTexCoord0;		\n"
"in vec2 vTexCoord1;		\n"
"in vec2 vLodTexCoord;		\n"
"uniform sampler2D uTex0;		\n"
"uniform sampler2D uTex1;		\n"
"uniform float uPrimitiveLod;		\n"
"uniform int uEnableLod;		\n"
"uniform vec2 uScreenScale;	\n"
"uniform float uMinLod;			\n"
"uniform int uMaxTile;			\n"
"uniform int uTextureDetail;	\n"
"														\n"
"float mipmap(out vec4 readtex0, out vec4 readtex1) {	\n"
"  if (uEnableLod == 0) {								\n"
"    readtex0 = texture(uTex0, vTexCoord0);			\n"
"    readtex1 = texture(uTex1, vTexCoord1);			\n"
"    return uPrimitiveLod;									\n"
"  }													\n"
"  vec2 dx = dFdx(vLodTexCoord);						\n"
"  dx *= uScreenScale;									\n"
"  vec2 dy = dFdy(vLodTexCoord);						\n"
"  dy *= uScreenScale;									\n"
"  float lod = max(length(dx), length(dy));				\n"
//" float lod = max(length(dx), length(dy)) * max(uLodXScale, uLodYScale);\n"
"  float lod_tile, lod_frac;							\n"
"  bool magnifying = false;								\n"
"  if (lod < 1.0) {										\n"
"    magnifying = true;									\n"
"    lod_tile = 0.0;									\n"
"    lod_frac = lod;									\n"
"    if (uTextureDetail != 0)							\n"
"      lod_frac = max(lod, uMinLod);					\n"
"    if (uTextureDetail == 1)							\n"
"      lod_frac = 1.0 - lod_frac;						\n"
"  } else {												\n"
"    lod_tile = min(float(uMaxTile), floor(log2(floor(lod)))); \n"
"    lod_frac = fract(lod/pow(2.0, lod_tile));			\n"
"  }													\n"
"  if (lod_tile < 1.0) {								\n"
"    if (magnifying) {									\n"
"      readtex0 = texture(uTex0, vTexCoord0);			\n"
//     !sharpen && !detail
"      if (uTextureDetail == 0) readtex1 = readtex0;	\n"
"      else readtex1 = texture(uTex1, vTexCoord1);	\n"
"    } else {											\n"
//     detail
"      if (uTextureDetail == 2) {						\n"
"        readtex0 = textureLod(uTex1, vTexCoord1, 0.0);\n"
"        readtex1 = textureLod(uTex1, vTexCoord1, 1.0);\n"
"      } else {											\n"
"        readtex0 = texture(uTex0, vTexCoord0);		\n"
"        readtex1 = texture(uTex1, vTexCoord1);		\n"
"      }												\n"
"    }													\n"
"  } else {												\n"
"    if (uTextureDetail == 2) {							\n"
"      readtex0 = textureLod(uTex1, vTexCoord1, lod_tile);		\n"
"      readtex1 = textureLod(uTex1, vTexCoord1, lod_tile + 1.0);	\n"
"    } else {														\n"
"      readtex0 = textureLod(uTex1, vTexCoord1, lod_tile - 1.0);	\n"
"      readtex1 = textureLod(uTex1, vTexCoord1, lod_tile);		\n"
"    }																\n"
"  }																\n"
"  return lod_frac;													\n"
"}																	\n"
#endif
;

static const char* fragment_shader_readtex =
"#version 330 core													\n"
"uniform lowp int uTextureFilterMode;								\n"
"lowp vec4 filterNearest(in sampler2D tex, in mediump vec2 texCoord)\n"
"{																	\n"
"  return textureLod(tex, texCoord, 0.0);							\n"
"}																	\n"
// 3 point texture filtering.
// Original author: ArthurCarvalho
// GLSL implementation: twinaphex, mupen64plus-libretro project.
"#define TEX_OFFSET(off) textureLod(tex, texCoord - (off)/texSize, 0.0)	\n"
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
"lowp vec4 readTex(in sampler2D tex, in mediump vec2 texCoord, in bool fb8bit, in bool fbFixedAlpha)	\n"
"{																			\n"
"  lowp vec4 texColor = uTextureFilterMode == 0 ? filterNearest(tex, texCoord) : filter3point(tex, texCoord); \n"
"  if (fb8bit) texColor = vec4(texColor.r);									\n"
"  if (fbFixedAlpha) texColor.a = 0.825;									\n"
"  return texColor;															\n"
"}																			\n"
;

static const char* fragment_shader_noise =
"#version 330 core					\n"
"uniform sampler2D uTexNoise;		\n"
"uniform mediump vec2 uScreenScale;	\n"
"lowp float snoise()									\n"
"{														\n"
"  ivec2 coord = ivec2(gl_FragCoord.xy/uScreenScale);	\n"
"  return (texelFetch(uTexNoise, coord, 0).r - 0.5)*2.0;\n"
"}														\n"
;

static const char* fragment_shader_dummy_noise =
"						\n"
"lowp float snoise()	\n"
"{						\n"
"  return 1.0;			\n"
"}						\n"
;

#ifdef GL_IMAGE_TEXTURES_SUPPORT
static const char* depth_compare_shader_float =
"#version 430								\n"
//"uniform int uEnableDepth;				\n"
"uniform lowp int uDepthMode;				\n"
"uniform lowp int uDepthSource;				\n"
"uniform lowp int uEnableDepthCompare;		\n"
"uniform lowp int uEnableDepthUpdate;		\n"
"uniform mediump float uDeltaZ;				\n"
"layout(binding = 2, rg32f) uniform coherent image2D uDepthImage;\n"
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
"  const bool bInfront = curZ < bufZ;					\n"
"  const bool bFarther = (curZ + dzMin) >= bufZ;		\n"
"  const bool bNearer = (curZ - dzMin) <= bufZ;			\n"
"  const bool bMax = bufZ == 1.0;						\n"
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

static const char* default_vertex_shader =
"#version 420 core												\n"
"in highp vec4 	aPosition;								\n"
"void main()                                                    \n"
"{                                                              \n"
"  gl_Position = aPosition;										\n"
"}                                                              \n"
;

static const char* shadow_map_fragment_shader_float =
"#version 420 core											\n"
"layout(binding = 0) uniform sampler2D uDepthImage;		\n"
"layout(binding = 0, r16ui) uniform readonly uimage2D uZlutImage;\n"
"layout(binding = 1, r16ui) uniform readonly uimage1D uTlutImage;\n"
"uniform lowp vec4 uFogColor;								\n"
"out lowp vec4 fragColor;									\n"
"lowp float get_alpha()										\n"
"{															\n"
"  ivec2 coord = ivec2(gl_FragCoord.xy);					\n"
"  highp float bufZ = texelFetch(uDepthImage,coord, 0).r;	\n"
"  const highp int iZ = bufZ > 0.999 ? 262143 : int(floor(bufZ * 262143.0));\n"
"  int y0 = clamp(iZ/512, 0, 511);							\n"
"  int x0 = iZ - 512*y0;									\n"
"  uint iN64z = imageLoad(uZlutImage,ivec2(x0,y0)).r;		\n"
"  highp float n64z = clamp(float(iN64z)/65532.0, 0.0, 1.0);\n"
"  int index = min(255, int(n64z*255.0));					\n"
"  uint iAlpha = imageLoad(uTlutImage,index).r;			\n"
"  return float(iAlpha/256)/255.0;						\n"
"}														\n"
"void main()											\n"
"{														\n"
"  fragColor = vec4(uFogColor.rgb, get_alpha());		\n"
"}														\n"
;
#endif // GL_IMAGE_TEXTURES_SUPPORT

#if 0 // Do palette based monochrome image. Exactly as N64 does
#ifdef GL_IMAGE_TEXTURES_SUPPORT
static const char* zelda_monochrome_fragment_shader =
"#version 420 core											\n"
"layout(binding = 0) uniform sampler2D uColorImage;			\n"
"layout(binding = 1, r16ui) uniform readonly uimage1D uTlutImage;\n"
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
"  uint iAlpha = imageLoad(uTlutImage,index).r;			\n"
"  memoryBarrier();										\n"
"  return clamp(float((iAlpha&255) + index)/255.0, 0.0, 1.0); \n"
"}														\n"
"void main()											\n"
"{														\n"
"  fragColor = vec4(vec3(get_color()), 1.0);			\n"
"}														\n"
;
#endif // GL_IMAGE_TEXTURES_SUPPORT
#else // Cheat it
static const char* zelda_monochrome_fragment_shader =
"#version 420 core										\n"
"layout(binding = 0) uniform sampler2D uColorImage;		\n"
"out lowp vec4 fragColor;								\n"
"void main()											\n"
"{														\n"
"  ivec2 coord = ivec2(gl_FragCoord.xy);				\n"
"  vec4 tex = texelFetch(uColorImage, coord, 0);		\n"
//"  lowp float c = (tex.r + tex.g + tex.b) / 3.0f;		\n"
"  lowp float c = 0.2126 * tex.r + 0.7152 * tex.g + 0.0722 * tex.b; \n"
"  fragColor = vec4(c, c, c, 1.0);						\n"
"}														\n"
;
#endif
