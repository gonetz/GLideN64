#include <assert.h>
#include <memory.h>
#include <algorithm>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include "OpenGL.h"
#include "Textures.h"
#include "GLSLCombiner.h"
#include "GBI.h"
#include "RSP.h"
#include "gDP.h"
#include "gSP.h"
#include "N64.h"
#include "convert.h"
#include "FrameBuffer.h"
#include "Config.h"
#include "Keys.h"
#include "GLideNHQ/Ext_TxFilter.h"

using namespace std;

const GLuint g_noiseTexIndex = 2;
const GLuint g_MSTex0Index = g_noiseTexIndex + 1;

inline u32 GetNone( u64 *src, u16 x, u16 i, u8 palette )
{
	return 0x00000000;
}

inline u32 GetCI4IA_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return IA88_RGBA4444( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return IA88_RGBA4444( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetCI4IA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return IA88_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return IA88_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetCI4RGBA_RGBA5551( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return RGBA5551_RGBA5551( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return RGBA5551_RGBA5551( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetCI4RGBA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return RGBA5551_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
	else
		return RGBA5551_RGBA8888( *(u16*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

inline u32 GetIA31_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return IA31_RGBA8888( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetIA31_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return IA31_RGBA4444( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetI4_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return I4_RGBA8888( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetI4_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	u8 color4B;

	color4B = ((u8*)src)[(x>>1)^(i<<1)];

	return I4_RGBA4444( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

inline u32 GetCI8IA_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA4444( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetCI8IA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA8888( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetCI8RGBA_RGBA5551( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA5551_RGBA5551( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetCI8RGBA_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA5551_RGBA8888( *(u16*)&TMEM[256 + ((u8*)src)[x^(i<<1)]] );
}

inline u32 GetIA44_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA44_RGBA8888(((u8*)src)[x^(i<<1)]);
}

inline u32 GetIA44_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA44_RGBA4444(((u8*)src)[x^(i<<1)]);
}

inline u32 GetI8_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return I8_RGBA8888(((u8*)src)[x^(i<<1)]);
}

inline u32 GetI8_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return I8_RGBA4444(((u8*)src)[x^(i<<1)]);
}

inline u32 GetCI16IA_RGBA8888(u64 *src, u16 x, u16 i, u8 palette)
{
	const u16 tex = ((u16*)src)[x^i];
	const u16 col = (*(u16*)&TMEM[256 + (tex >> 8)]);
	const u16 c = col >> 8;
	const u16 a = col & 0xFF;
	return (a << 24) | (c << 16) | (c << 8) | c;
}

inline u32 GetCI16IA_RGBA4444(u64 *src, u16 x, u16 i, u8 palette)
{
	const u16 tex = ((u16*)src)[x^i];
	const u16 col = (*(u16*)&TMEM[256 + (tex >> 8)]);
	const u16 c = col >> 12;
	const u16 a = col & 0x0F;
	return (a << 12) | (c << 8) | (c << 4) | c;
}

inline u32 GetCI16RGBA_RGBA8888(u64 *src, u16 x, u16 i, u8 palette)
{
	const u16 tex = (((u16*)src)[x^i])&0xFF;
	return RGBA5551_RGBA8888(((u16*)&TMEM[256])[tex << 2]);
}

inline u32 GetCI16RGBA_RGBA5551(u64 *src, u16 x, u16 i, u8 palette)
{
	const u16 tex = (((u16*)src)[x^i]) & 0xFF;
	return RGBA5551_RGBA5551(((u16*)&TMEM[256])[tex << 2]);
}

inline u32 GetRGBA5551_RGBA8888(u64 *src, u16 x, u16 i, u8 palette)
{
	u16 tex = ((u16*)src)[x^i];
	return RGBA5551_RGBA8888(tex);
}

inline u32 GetRGBA5551_RGBA5551( u64 *src, u16 x, u16 i, u8 palette )
{
	u16 tex = ((u16*)src)[x^i];
	return RGBA5551_RGBA5551(tex);
}

inline u32 GetIA88_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA8888(((u16*)src)[x^i]);
}

inline u32 GetIA88_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return IA88_RGBA4444(((u16*)src)[x^i]);
}

inline u32 GetRGBA8888_RGBA8888( u64 *src, u16 x, u16 i, u8 palette )
{
	return ((u32*)src)[x^i];
}

inline u32 GetRGBA8888_RGBA4444( u64 *src, u16 x, u16 i, u8 palette )
{
	return RGBA8888_RGBA4444(((u32*)src)[x^i]);
}

u32 YUV_RGBA8888(u8 y, u8 u, u8 v)
{
	s32 r = (s32)(y + (1.370705f * (v - 128)));
	s32 g = (s32)((y - (0.698001f * (v - 128)) - (0.337633f * (u - 128))));
	s32 b = (s32)(y + (1.732446f * (u - 128)));
	//clipping the result
	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;

	return (0xff << 24) | (b << 16) | (g << 8) | r;
}

u16 YUV_RGBA4444(u8 y, u8 u, u8 v)
{
	return RGBA8888_RGBA4444(YUV_RGBA8888(y, u, v));
}

inline void GetYUV_RGBA8888(u64 * src, u32 * dst, u16 x)
{
	const u32 t = (((u32*)src)[x]);
	u8 y1 = (u8)t & 0xFF;
	u8 v = (u8)(t >> 8) & 0xFF;
	u8 y0 = (u8)(t >> 16) & 0xFF;
	u8 u = (u8)(t >> 24) & 0xFF;
	u32 c = YUV_RGBA8888(y0, u, v);
	*(dst++) = c;
	c = YUV_RGBA8888(y1, u, v);
	*(dst++) = c;
}

inline void GetYUV_RGBA4444(u64 * src, u16 * dst, u16 x)
{
	const u32 t = (((u32*)src)[x]);
	u8 y1 = (u8)t & 0xFF;
	u8 v = (u8)(t >> 8) & 0xFF;
	u8 y0 = (u8)(t >> 16) & 0xFF;
	u8 u = (u8)(t >> 24) & 0xFF;
	u16 c = YUV_RGBA4444(y0, u, v);
	*(dst++) = c;
	c = YUV_RGBA4444(y1, u, v);
	*(dst++) = c;
}

const struct TextureLoadParameters
{
	GetTexelFunc	Get16;
	GLenum			glType16;
	GLint			glInternalFormat16;
	GetTexelFunc	Get32;
	GLenum			glType32;
	GLint			glInternalFormat32;
	u32				autoFormat, lineShift, maxTexels;
} imageFormat[4][4][5] =
{ // G_TT_NONE
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetI4_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI4_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // RGBA as I
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetI4_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI4_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // CI without palette
			{ GetIA31_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA31_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // IA
			{ GetI4_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI4_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // I
		},
		{ // 8-bit
			{ GetI8_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI8_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 3, 4096 }, // RGBA as I
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetI8_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI8_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 3, 4096 }, // CI without palette
			{ GetIA44_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA44_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 4096 }, // IA
			{ GetI8_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI8_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 3, 4096 }, // I
		},
		{ // 16-bit
			{ GetRGBA5551_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetRGBA5551_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetIA88_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA88_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // CI as IA
			{ GetIA88_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA88_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	},
	// DUMMY
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI (Banjo-Kazooie uses this, doesn't make sense, but it works...)
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // IA as CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // I as CI
		},
		{ // 8-bit
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // IA as CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // I as CI
		},
		{ // 16-bit
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetRGBA5551_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // CI
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI16RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // IA as CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	},
	// G_TT_RGBA16
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI (Banjo-Kazooie uses this, doesn't make sense, but it works...)
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // IA as CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // I as CI
		},
		{ // 8-bit
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // IA as CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // I as CI
		},
		{ // 16-bit
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetRGBA5551_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // CI
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI16RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // IA as CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	},
	// G_TT_IA16
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // CI
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // IA as CI
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // I as CI
		},
		{ // 8-bit
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // CI
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // IA as CI
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // I as CI
		},
		{ // 16-bit
			{ GetCI16IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI16IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // CI
			{ GetCI16IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI16IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	}
};

/** cite from RiceVideo */
inline u32 CalculateDXT(u32 txl2words)
{
	if (txl2words == 0) return 1;
	else return (2048 + txl2words - 1) / txl2words;
}

u32 sizeBytes[4] = {0, 1, 2, 4};

inline u32 Txl2Words(u32 width, u32 size)
{
	if (size == 0)
		return max(1U, width / 16);
	else
		return max(1U, width*sizeBytes[size] / 8);
}

inline u32 ReverseDXT(u32 val, u32 lrs, u32 width, u32 size)
{
	if (val == 0x800) return 1;

	int low = 2047 / val;
	if (CalculateDXT(low) > val)	low++;
	int high = 2047 / (val - 1);

	if (low == high)	return low;

	for (int i = low; i <= high; i++) {
		if (Txl2Words(width, size) == (u32)i)
			return i;
	}

	return	(low + high) / 2;
}
/** end RiceVideo cite */

TextureCache & TextureCache::get() {
	static TextureCache cache;
	return cache;
}

void TextureCache::_initDummyTexture(CachedTexture * _pDummy)
{
	_pDummy->address = 0;
	_pDummy->clampS = 1;
	_pDummy->clampT = 1;
	_pDummy->clampWidth = 2;
	_pDummy->clampHeight = 2;
	_pDummy->crc = 0;
	_pDummy->format = 0;
	_pDummy->size = 0;
	_pDummy->frameBufferTexture = CachedTexture::fbNone;
	_pDummy->width = 2;
	_pDummy->height = 2;
	_pDummy->realWidth = 2;
	_pDummy->realHeight = 2;
	_pDummy->maskS = 0;
	_pDummy->maskT = 0;
	_pDummy->scaleS = 0.5f;
	_pDummy->scaleT = 0.5f;
	_pDummy->shiftScaleS = 1.0f;
	_pDummy->shiftScaleT = 1.0f;
	_pDummy->textureBytes = 2 * 2 * 4;
	_pDummy->tMem = 0;
}

void TextureCache::init()
{
	m_maxBytes = config.texture.maxBytes;
	m_curUnpackAlignment = 0;

	u32 dummyTexture[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	m_pDummy = addFrameBufferTexture(); // we don't want to remove dummy texture
	_initDummyTexture(m_pDummy);

	glBindTexture(GL_TEXTURE_2D, m_pDummy->glName);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummyTexture );

	m_cachedBytes = m_pDummy->textureBytes;
	activateDummy( 0 );
	activateDummy( 1 );
	current[0] = current[1] = NULL;

#ifdef GL_MULTISAMPLING_SUPPORT
	m_pMSDummy = addFrameBufferTexture(); // we don't want to remove dummy texture
	_initDummyTexture(m_pMSDummy);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pMSDummy->glName);

#if defined(GLES3_1)
	glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling,
				GL_RGBA8, m_pMSDummy->realWidth, m_pMSDummy->realHeight, false);
#else
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.video.multisampling,
				GL_RGBA8, m_pMSDummy->realWidth, m_pMSDummy->realHeight, false);
#endif

	activateMSDummy(0);
	activateMSDummy(1);
#else
	m_pMSDummy = NULL;
#endif
}

void TextureCache::destroy()
{
	current[0] = current[1] = NULL;

	for (Textures::const_iterator cur = m_textures.cbegin(); cur != m_textures.cend(); ++cur)
		glDeleteTextures( 1, &cur->second.glName );
	m_textures.clear();

	for (Textures::const_iterator cur = m_fbTextures.cbegin(); cur != m_fbTextures.cend(); ++cur)
		glDeleteTextures( 1, &cur->second.glName );
	m_fbTextures.clear();

	m_cachedBytes = 0;
}

void TextureCache::_checkCacheSize()
{
	if (m_cachedBytes <= m_maxBytes)
		return;
	Textures::const_iterator iter = m_textures.cend();
	do {
		--iter;
		m_cachedBytes -= iter->second.textureBytes;
		glDeleteTextures( 1, &iter->second.glName );
	} while (m_cachedBytes > m_maxBytes && iter != m_textures.cbegin());
	m_textures.erase(iter, m_textures.cend());
}

CachedTexture * TextureCache::_addTexture(u32 _crc32)
{
	if (m_curUnpackAlignment == 0)
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &m_curUnpackAlignment);
	_checkCacheSize();
	GLuint glName;
	glGenTextures(1, &glName);
	m_textures.emplace(_crc32, glName);
	CachedTexture & texture = m_textures.at(_crc32);
	texture.crc = _crc32;
	return &texture;
}

void TextureCache::removeFrameBufferTexture(CachedTexture * _pTexture)
{
	Textures::const_iterator iter = m_fbTextures.find(_pTexture->glName);
	assert(iter != m_fbTextures.end());
	m_cachedBytes -= iter->second.textureBytes;
	glDeleteTextures( 1, &iter->second.glName );
	m_fbTextures.erase(iter);
}

CachedTexture * TextureCache::addFrameBufferTexture()
{
	_checkCacheSize();
	GLuint glName;
	glGenTextures(1, &glName);
	m_fbTextures.emplace(glName, glName);
	return &m_fbTextures.at(glName);
}

struct TileSizes
{
	u32 maskWidth, clampWidth, width, realWidth;
	u32 maskHeight, clampHeight, height, realHeight;
};

static
void _calcTileSizes(u32 _t, TileSizes & _sizes, gDPTile * _pLoadTile)
{
	gDPTile * pTile = _t < 2 ? gSP.textureTile[_t] : &gDP.tiles[_t];

	const TextureLoadParameters & loadParams = imageFormat[gDP.otherMode.textureLUT][pTile->size][pTile->format];
	const u32 maxTexels = loadParams.maxTexels;
	const u32 tileWidth = ((pTile->lrs - pTile->uls) & 0x03FF) + 1;
	const u32 tileHeight = ((pTile->lrt - pTile->ult) & 0x03FF) + 1;

	const bool bUseLoadSizes = _pLoadTile != NULL && _pLoadTile->loadType == LOADTYPE_TILE &&
		(pTile->tmem == _pLoadTile->tmem);

	u32 loadWidth = 0, loadHeight = 0;
	if (bUseLoadSizes) {
		loadWidth = ((_pLoadTile->lrs - _pLoadTile->uls) & 0x03FF) + 1;
		loadHeight = ((_pLoadTile->lrt - _pLoadTile->ult) & 0x03FF) + 1;
	}

	const u32 lineWidth = pTile->line << loadParams.lineShift;
	const u32 lineHeight = lineWidth != 0 ? min(maxTexels / lineWidth, tileHeight) : 0;

	u32 maskWidth = 1 << pTile->masks;
	u32 maskHeight = 1 << pTile->maskt;
	u32 width, height;

	gDPLoadTileInfo &info = gDP.loadInfo[pTile->tmem];
	if (info.loadType == LOADTYPE_TILE) {
		if (pTile->masks && ((maskWidth * maskHeight) <= maxTexels))
			width = maskWidth; // Use mask width if set and valid
		else {
			width = min(info.width, info.texWidth);
			if (info.size > pTile->size)
				width <<= info.size - pTile->size;
		}
		if (pTile->maskt && ((maskWidth * maskHeight) <= maxTexels))
			height = maskHeight;
		else
			height = info.height;
	} else {
		if (pTile->masks && ((maskWidth * maskHeight) <= maxTexels))
			width = maskWidth; // Use mask width if set and valid
		else if ((tileWidth * tileHeight) <= maxTexels)
			width = tileWidth; // else use tile width if valid
		else
			width = lineWidth; // else use line-based width

		if (pTile->maskt && ((maskWidth * maskHeight) <= maxTexels))
			height = maskHeight;
		else if ((tileWidth * tileHeight) <= maxTexels)
			height = tileHeight;
		else
			height = lineHeight;
	}

	_sizes.clampWidth = (pTile->clamps && gDP.otherMode.cycleType != G_CYC_COPY) ? tileWidth : width;
	_sizes.clampHeight = (pTile->clampt && gDP.otherMode.cycleType != G_CYC_COPY) ? tileHeight : height;

	if (_sizes.clampWidth > 256)
		pTile->clamps = 0;
	if (_sizes.clampHeight > 256)
		pTile->clampt = 0;

	// Make sure masking is valid
	if (maskWidth > width) {
		pTile->masks = powof(width);
		maskWidth = 1 << pTile->masks;
	}

	if (maskHeight > height) {
		pTile->maskt = powof(height);
		maskHeight = 1 << pTile->maskt;
	}

	_sizes.maskWidth = maskWidth;
	_sizes.maskHeight = maskHeight;
	_sizes.width = width;
	_sizes.height = height;

	if (pTile->clamps != 0)
		_sizes.realWidth = _sizes.clampWidth;
	else if (pTile->masks != 0)
		_sizes.realWidth = _sizes.maskWidth;
	else
		_sizes.realWidth = _sizes.width;

	if (pTile->clampt != 0)
		_sizes.realHeight = _sizes.clampHeight;
	else if (pTile->maskt != 0)
		_sizes.realHeight = _sizes.maskHeight;
	else
		_sizes.realHeight = _sizes.height;

	if (gSP.texture.level > gSP.texture.tile) {
		_sizes.realWidth = pow2(_sizes.realWidth);
		_sizes.realHeight = pow2(_sizes.realHeight);
	}
}

inline
void _updateCachedTexture(const GHQTexInfo & _info, CachedTexture *_pTexture)
{
	_pTexture->textureBytes = _info.width * _info.height;
	switch (_info.format) {
		case GL_RGB:
		case GL_RGBA4:
		case GL_RGB5_A1:
		_pTexture->textureBytes <<= 1;
		break;
		default:
		_pTexture->textureBytes <<= 2;
	}
	_pTexture->realWidth = _info.width;
	_pTexture->realHeight = _info.height;
	/*
	_pTexture->scaleS = 1.0f / (f32)(_pTexture->realWidth);
	_pTexture->scaleT = 1.0f / (f32)(_pTexture->realHeight);
	*/
}

bool TextureCache::_loadHiresBackground(CachedTexture *_pTexture)
{
	if (!TFH.isInited())
		return false;

	u8 * addr = (u8*)(RDRAM + gSP.bgImage.address);
	int tile_width = gSP.bgImage.width;
	int tile_height = gSP.bgImage.height;
	int bpl = tile_width << gSP.bgImage.size >> 1;

	u8 * paladdr = NULL;
	u16 * palette = NULL;
	if ((gSP.bgImage.size < G_IM_SIZ_16b) && (gDP.otherMode.textureLUT != G_TT_NONE || gSP.bgImage.format == G_IM_FMT_CI)) {
		if (gSP.bgImage.size == G_IM_SIZ_8b)
			paladdr = (u8*)(gDP.TexFilterPalette);
		else if (config.textureFilter.txHresAltCRC)
			paladdr = (u8*)(gDP.TexFilterPalette + (gSP.bgImage.palette << 5));
		else
			paladdr = (u8*)(gDP.TexFilterPalette + (gSP.bgImage.palette << 4));
		// TODO: fix palette load
		//			palette = (rdp.pal_8 + (gSP.textureTile[_t]->palette << 4));
	}

	u64 ricecrc = txfilter_checksum(addr, tile_width,
						tile_height, (unsigned short)(gSP.bgImage.format << 8 | gSP.bgImage.size),
						bpl, paladdr);
	GHQTexInfo ghqTexInfo;
	if (txfilter_hirestex(_pTexture->crc, ricecrc, palette, &ghqTexInfo)) {
		glTexImage2D(GL_TEXTURE_2D, 0, ghqTexInfo.format,
			ghqTexInfo.width, ghqTexInfo.height, 0, ghqTexInfo.texture_format,
			ghqTexInfo.pixel_type, ghqTexInfo.data);
		assert(!isGLError());
		_updateCachedTexture(ghqTexInfo, _pTexture);
		return true;
	}
	return false;
}

void TextureCache::_loadBackground(CachedTexture *pTexture)
{
	if (_loadHiresBackground(pTexture))
		return;

	u32 *pDest;

	u8 *pSwapped, *pSrc;
	u32 numBytes, bpl;
	u32 x, y, j, tx, ty;
	u16 clampSClamp;
	u16 clampTClamp;
	GetTexelFunc GetTexel;
	GLuint glInternalFormat;
	GLenum glType;

	const TextureLoadParameters & loadParams = imageFormat[pTexture->format == 2 ? G_TT_RGBA16 : G_TT_NONE][pTexture->size][pTexture->format];
	if (loadParams.autoFormat == GL_RGBA) {
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 2;
		GetTexel = loadParams.Get32;
		glInternalFormat = loadParams.glInternalFormat32;
		glType = loadParams.glType32;
	} else {
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 1;
		GetTexel = loadParams.Get16;
		glInternalFormat = loadParams.glInternalFormat16;
		glType = loadParams.glType16;
	}

	bpl = gSP.bgImage.width << gSP.bgImage.size >> 1;
	numBytes = bpl * gSP.bgImage.height;
	pSwapped = (u8*)malloc(numBytes);
	assert(pSwapped != NULL);
	UnswapCopyWrap(RDRAM, gSP.bgImage.address, pSwapped, 0, RDRAMSize, numBytes);
	pDest = (u32*)malloc(pTexture->textureBytes);
	assert(pDest != NULL);

	clampSClamp = pTexture->width - 1;
	clampTClamp = pTexture->height - 1;

	j = 0;
	for (y = 0; y < pTexture->realHeight; y++) {
		ty = min(y, (u32)clampTClamp);

		pSrc = &pSwapped[bpl * ty];

		for (x = 0; x < pTexture->realWidth; x++) {
			tx = min(x, (u32)clampSClamp);

			if (glInternalFormat == GL_RGBA)
				((u32*)pDest)[j++] = GetTexel((u64*)pSrc, tx, 0, pTexture->palette);
			else
				((u16*)pDest)[j++] = GetTexel((u64*)pSrc, tx, 0, pTexture->palette);
		}
	}

	bool bLoaded = false;
	if ((config.textureFilter.txEnhancementMode | config.textureFilter.txFilterMode) != 0 &&
			config.textureFilter.txFilterIgnoreBG == 0 &&
			TFH.isInited()) {
		GHQTexInfo ghqTexInfo;
		if (txfilter_filter((u8*)pDest, pTexture->realWidth, pTexture->realHeight,
				glInternalFormat, (uint64)pTexture->crc, &ghqTexInfo) != 0 &&
				ghqTexInfo.data != NULL) {
			if (ghqTexInfo.width % 2 != 0 &&
					ghqTexInfo.format != GL_RGBA &&
					m_curUnpackAlignment > 1)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexImage2D(GL_TEXTURE_2D, 0, ghqTexInfo.format,
					ghqTexInfo.width, ghqTexInfo.height, 0,
					ghqTexInfo.texture_format, ghqTexInfo.pixel_type,
					ghqTexInfo.data);
			_updateCachedTexture(ghqTexInfo, pTexture);
			bLoaded = true;
		}
	}
	if (!bLoaded) {
		if (pTexture->realWidth % 2 != 0 && glInternalFormat != GL_RGBA)
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
#ifdef GLES2
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pTexture->realWidth,
				pTexture->realHeight, 0, GL_RGBA, glType, pDest);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, pTexture->realWidth,
				pTexture->realHeight, 0, GL_RGBA, glType, pDest);
#endif
	}
	if (m_curUnpackAlignment > 1)
		glPixelStorei(GL_UNPACK_ALIGNMENT, m_curUnpackAlignment);
	free(pDest);
}

bool TextureCache::_loadHiresTexture(u32 _tile, CachedTexture *_pTexture, u64 & _ricecrc)
{
	if (config.textureFilter.txHiresEnable == 0 || !TFH.isInited())
		return false;

	gDPLoadTileInfo & info = gDP.loadInfo[_pTexture->tMem];

	int bpl;
	u8 * addr = (u8*)(RDRAM + info.texAddress);
	int tile_width = _pTexture->width;
	int tile_height = _pTexture->height;
	if (info.loadType == LOADTYPE_TILE) {
		bpl = info.texWidth << info.size >> 1;
		addr += (info.ult * bpl) + (((info.uls << info.size) + 1) >> 1);
	}
	else {
		if (gSP.textureTile[_tile]->size == G_IM_SIZ_32b)
			bpl = gSP.textureTile[_tile]->line << 4;
		else if (info.dxt == 0)
			bpl = gSP.textureTile[_tile]->line << 3;
		else {
			u32 dxt = info.dxt;
			if (dxt > 1)
				dxt = ReverseDXT(dxt, info.width, _pTexture->width, _pTexture->size);
			bpl = dxt << 3;
		}
	}

	u8 * paladdr = NULL;
	u16 * palette = NULL;
	if ((_pTexture->size < G_IM_SIZ_16b) && (gDP.otherMode.textureLUT != G_TT_NONE || _pTexture->format == G_IM_FMT_CI)) {
		if (_pTexture->size == G_IM_SIZ_8b)
			paladdr = (u8*)(gDP.TexFilterPalette);
		else if (config.textureFilter.txHresAltCRC)
			paladdr = (u8*)(gDP.TexFilterPalette + (_pTexture->palette << 5));
		else
			paladdr = (u8*)(gDP.TexFilterPalette + (_pTexture->palette << 4));
		// TODO: fix palette load
		//			palette = (rdp.pal_8 + (gSP.textureTile[_t]->palette << 4));
	}

	_ricecrc = txfilter_checksum(addr, tile_width, tile_height, (unsigned short)(_pTexture->format << 8 | _pTexture->size), bpl, paladdr);
	GHQTexInfo ghqTexInfo;
	if (txfilter_hirestex(_pTexture->crc, _ricecrc, palette, &ghqTexInfo)) {
#ifdef GLES2
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ghqTexInfo.width, ghqTexInfo.height, 0, GL_RGBA, ghqTexInfo.pixel_type, ghqTexInfo.data);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, ghqTexInfo.format, ghqTexInfo.width, ghqTexInfo.height, 0, ghqTexInfo.texture_format, ghqTexInfo.pixel_type, ghqTexInfo.data);
#endif
		assert(!isGLError());
		_updateCachedTexture(ghqTexInfo, _pTexture);
		return true;
	}

	return false;
}

/*
 * Worker function for _load
*/
void TextureCache::_getTextureDestData(CachedTexture& tmptex,
						u32* pDest,
						GLuint glInternalFormat,
						GetTexelFunc GetTexel,
						u16* pLine)
{
	u16 mirrorSBit, maskSMask, clampSClamp;
	u16 mirrorTBit, maskTMask, clampTClamp;
	u16 x, y, i, j, tx, ty;
	u64 *pSrc;
	if (tmptex.maskS > 0) {
		clampSClamp = tmptex.clampS ? tmptex.clampWidth - 1 : (tmptex.mirrorS ? (tmptex.width << 1) - 1 : tmptex.width - 1);
		maskSMask = (1 << tmptex.maskS) - 1;
		mirrorSBit = (tmptex.mirrorS != 0 || tmptex.realWidth/tmptex.width == 2) ? 1 << tmptex.maskS : 0;
	} else {
		clampSClamp = min(tmptex.clampWidth, tmptex.width) - 1;
		maskSMask = 0xFFFF;
		mirrorSBit = 0x0000;
	}

	if (tmptex.maskT > 0) {
		clampTClamp = tmptex.clampT ? tmptex.clampHeight - 1 : (tmptex.mirrorT ? (tmptex.height << 1) - 1 : tmptex.height - 1);
		maskTMask = (1 << tmptex.maskT) - 1;
		mirrorTBit = (tmptex.mirrorT != 0 || tmptex.realHeight/tmptex.height == 2) ? 1 << tmptex.maskT : 0;
	} else {
		clampTClamp = min(tmptex.clampHeight, tmptex.height) - 1;
		maskTMask = 0xFFFF;
		mirrorTBit = 0x0000;
	}

	if (tmptex.size == G_IM_SIZ_32b) {
		const u16 * tmem16 = (u16*)TMEM;
		const u32 tbase = tmptex.tMem << 2;

		int wid_64 = (tmptex.clampWidth) << 2;
		if (wid_64 & 15) {
			wid_64 += 16;
		}
		wid_64 &= 0xFFFFFFF0;
		wid_64 >>= 3;
		int line32 = tmptex.line << 1;
		line32 = (line32 - wid_64) << 3;
		if (wid_64 < 1) {
			wid_64 = 1;
		}
		int width = wid_64 << 1;
		line32 = width + (line32 >> 2);

		u16 gr, ab;

		j = 0;
		for (y = 0; y < tmptex.realHeight; ++y) {
			ty = min(y, clampTClamp) & maskTMask;
			if (y & mirrorTBit) {
				ty ^= maskTMask;
			}

			u32 tline = tbase + line32 * ty;
			u32 xorval = (ty & 1) ? 3 : 1;

			for (x = 0; x < tmptex.realWidth; ++x) {
				tx = min(x, clampSClamp) & maskSMask;
				if (x & mirrorSBit) {
					tx ^= maskSMask;
				}

				u32 taddr = ((tline + tx) ^ xorval) & 0x3ff;
				gr = swapword(tmem16[taddr]);
				ab = swapword(tmem16[taddr | 0x400]);
				pDest[j++] = (ab << 16) | gr;
			}
		}
	} else if (tmptex.format == G_IM_FMT_YUV) {
		j = 0;
		*pLine <<= 1;
		for (y = 0; y < tmptex.realHeight; ++y) {
			pSrc = &TMEM[tmptex.tMem] + *pLine * y;
			for (x = 0; x < tmptex.realWidth / 2; x++) {
				if (glInternalFormat == GL_RGBA) {
					GetYUV_RGBA8888(pSrc, pDest + j, x);
				} else {
					GetYUV_RGBA4444(pSrc, (u16*)pDest + j, x);
				}
				j += 2;
			}
		}
	} else {
		j = 0;
		for (y = 0; y < tmptex.realHeight; ++y) {
			ty = min(y, clampTClamp) & maskTMask;

			if (y & mirrorTBit)
			ty ^= maskTMask;

			pSrc = &TMEM[(tmptex.tMem + *pLine * ty) & 0x1FF];

			i = (ty & 1) << 1;
			for (x = 0; x < tmptex.realWidth; ++x) {
				tx = min(x, clampSClamp) & maskSMask;

				if (x & mirrorSBit) {
					tx ^= maskSMask;
				}

				if (glInternalFormat == GL_RGBA) {
					pDest[j++] = GetTexel(pSrc, tx, i, tmptex.palette);
				} else {
					((u16*)pDest)[j++] = GetTexel(pSrc, tx, i, tmptex.palette);
				}
			}
		}
	}
}

void TextureCache::_load(u32 _tile, CachedTexture *_pTexture)
{
	u64 ricecrc = 0;
	if (_loadHiresTexture(_tile, _pTexture, ricecrc))
		return;

	u32 *pDest;

	u16 line;
	GetTexelFunc GetTexel;
	GLuint glInternalFormat;
	GLenum glType;
	u32 sizeShift;

	const TextureLoadParameters & loadParams = imageFormat[gDP.otherMode.textureLUT][_pTexture->size][_pTexture->format];
	if (loadParams.autoFormat == GL_RGBA) {
		sizeShift = 2;
		_pTexture->textureBytes = (_pTexture->realWidth * _pTexture->realHeight) << sizeShift;
		GetTexel = loadParams.Get32;
		glInternalFormat = loadParams.glInternalFormat32;
		glType = loadParams.glType32;
	} else {
		sizeShift = 1;
		_pTexture->textureBytes = (_pTexture->realWidth * _pTexture->realHeight) << sizeShift;
		GetTexel = loadParams.Get16;
		glInternalFormat = loadParams.glInternalFormat16;
		glType = loadParams.glType16;
	}

	pDest = (u32*)malloc(_pTexture->textureBytes);
	assert(pDest != NULL);

	GLint mipLevel = 0, maxLevel = 0;
#ifndef GLES2
	if (config.generalEmulation.enableLOD != 0 && gSP.texture.level > gSP.texture.tile + 1)
		maxLevel = _tile == 0 ? 0 : gSP.texture.level - gSP.texture.tile - 1;
#endif

	_pTexture->max_level = maxLevel;

	CachedTexture tmptex(0);
	memcpy(&tmptex, _pTexture, sizeof(CachedTexture));

	line = tmptex.line;

	while (true) {
		_getTextureDestData(tmptex, pDest, glInternalFormat, GetTexel, &line);

		bool bLoaded = false;
		if (m_toggleDumpTex &&
				config.textureFilter.txHiresEnable != 0 &&
				config.textureFilter.txDump != 0) {
			txfilter_dmptx((u8*)pDest, tmptex.realWidth, tmptex.realHeight,
					tmptex.realWidth, glInternalFormat,
					(unsigned short)(_pTexture->format << 8 | _pTexture->size),
					ricecrc);
		}
		else if ((config.textureFilter.txEnhancementMode | config.textureFilter.txFilterMode) != 0 &&
				maxLevel == 0 &&
				(config.textureFilter.txFilterIgnoreBG == 0 || (RSP.cmd != G_TEXRECT && RSP.cmd != G_TEXRECTFLIP)) &&
				TFH.isInited())
		{
			GHQTexInfo ghqTexInfo;
			if (txfilter_filter((u8*)pDest, tmptex.realWidth, tmptex.realHeight,
							glInternalFormat, (uint64)_pTexture->crc,
							&ghqTexInfo) != 0 && ghqTexInfo.data != NULL) {
#ifdef GLES2
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
						ghqTexInfo.width, ghqTexInfo.height,
						0, GL_RGBA, ghqTexInfo.pixel_type,
						ghqTexInfo.data);
#else
				glTexImage2D(GL_TEXTURE_2D, 0, ghqTexInfo.format,
						ghqTexInfo.width, ghqTexInfo.height,
						0, ghqTexInfo.texture_format, ghqTexInfo.pixel_type,
						ghqTexInfo.data);
#endif
				_updateCachedTexture(ghqTexInfo, _pTexture);
				bLoaded = true;
			}
		}
		if (!bLoaded) {
			if (tmptex.realWidth % 2 != 0 &&
					glInternalFormat != GL_RGBA &&
					m_curUnpackAlignment > 1)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
#ifdef GLES2
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tmptex.realWidth,
					tmptex.realHeight, 0, GL_RGBA, glType, pDest);
#else
			glTexImage2D(GL_TEXTURE_2D, mipLevel, glInternalFormat, tmptex.realWidth,
					tmptex.realHeight, 0, GL_RGBA, glType, pDest);
#endif
		}
		if (mipLevel == maxLevel)
			break;
		++mipLevel;
		const u32 tileMipLevel = gSP.texture.tile + mipLevel + 1;
		gDPTile & mipTile = gDP.tiles[tileMipLevel];
		line = mipTile.line;
		tmptex.tMem = mipTile.tmem;
		tmptex.palette = mipTile.palette;
		tmptex.maskS = mipTile.masks;
		tmptex.maskT = mipTile.maskt;
		TileSizes sizes;
		_calcTileSizes(tileMipLevel, sizes, NULL);
		tmptex.width = sizes.width;
		tmptex.clampWidth = sizes.clampWidth;
		tmptex.height = sizes.height;
		tmptex.clampHeight = sizes.clampHeight;
		// Insure mip-map levels size consistency.
		if (tmptex.realWidth > 1)
			tmptex.realWidth >>= 1;
		if (tmptex.realHeight > 1)
			tmptex.realHeight >>= 1;
		_pTexture->textureBytes += (tmptex.realWidth * tmptex.realHeight) << sizeShift;
	}
	if (m_curUnpackAlignment > 1)
		glPixelStorei(GL_UNPACK_ALIGNMENT, m_curUnpackAlignment);
	free(pDest);
}

struct TextureParams
{
	u16 width;
	u16 height;
	u16 clampWidth;
	u16 clampHeight;
	u8 maskS;
	u8 maskT;
	u8 mirrorS;
	u8 mirrorT;
	u8 clampS;
	u8 clampT;
	u8 format;
	u8 size;
};

static
u32 _calculateCRC(u32 t, const TextureParams & _params)
{
	const u32 line = gSP.textureTile[t]->line;
	const u32 lineBytes = line << 3;

	const u64 *src = (u64*)&TMEM[gSP.textureTile[t]->tmem];
	u32 crc = 0xFFFFFFFF;
	crc = CRC_Calculate(crc, src, _params.height*lineBytes);

	if (gSP.textureTile[t]->size == G_IM_SIZ_32b) {
		src = (u64*)&TMEM[gSP.textureTile[t]->tmem + 256];
		crc = CRC_Calculate(crc, src, _params.height*lineBytes);
	}

	if (gDP.otherMode.textureLUT != G_TT_NONE || gSP.textureTile[t]->format == G_IM_FMT_CI) {
		if (gSP.textureTile[t]->size == G_IM_SIZ_4b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC16[gSP.textureTile[t]->palette], 4 );
		else if (gSP.textureTile[t]->size == G_IM_SIZ_8b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC256, 4 );
	}

	crc = CRC_Calculate(crc, &_params, sizeof(_params));

	return crc;
}

void TextureCache::activateTexture(u32 _t, CachedTexture *_pTexture)
{
#ifdef GL_MULTISAMPLING_SUPPORT
	if (config.video.multisampling > 0 && _pTexture->frameBufferTexture == CachedTexture::fbMultiSample) {
		glActiveTexture(GL_TEXTURE0 + g_MSTex0Index + _t);
		// Bind the cached texture
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _pTexture->glName);
	} else
#endif
	{
		glActiveTexture(GL_TEXTURE0 + _t);
		// Bind the cached texture
		glBindTexture(GL_TEXTURE_2D, _pTexture->glName);
	}

	const bool bUseBilinear = (gDP.otherMode.textureFilter | (gSP.objRendermode&G_OBJRM_BILERP)) != 0;
	const bool bUseLOD = currentCombiner()->usesLOD();
	const GLint texLevel = bUseLOD ? _pTexture->max_level : 0;

#ifndef GLES2
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, texLevel);
#endif
	if (config.texture.bilinearMode == BILINEAR_STANDARD) {
		if (bUseBilinear) {
			if (texLevel > 0)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			if (texLevel > 0)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	} else { // 3 point filter
		if (texLevel > 0) { // Apply standard bilinear to mipmap textures
			if (bUseBilinear) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			} else {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
		} else if (bUseBilinear && config.generalEmulation.enableLOD != 0 && bUseLOD) { // Apply standard bilinear to first tile of mipmap texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else { // Don't use texture filter. Texture will be filtered by 3 point filter shader
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}


	// Set clamping modes
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _pTexture->clampS ? GL_CLAMP_TO_EDGE : _pTexture->mirrorS ? GL_MIRRORED_REPEAT : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _pTexture->clampT ? GL_CLAMP_TO_EDGE : _pTexture->mirrorT ? GL_MIRRORED_REPEAT : GL_REPEAT);

	if (video().getRender().getRenderState() == OGLRender::rsTriangle && config.texture.maxAnisotropyF > 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, config.texture.maxAnisotropyF);

	_pTexture->lastDList = RSP.DList;

	current[_t] = _pTexture;
}

void TextureCache::activateDummy(u32 _t)
{
	glActiveTexture( GL_TEXTURE0 + _t );

	glBindTexture( GL_TEXTURE_2D, m_pDummy->glName );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

void TextureCache::activateMSDummy(u32 _t)
{
#ifdef GL_MULTISAMPLING_SUPPORT
	glActiveTexture(GL_TEXTURE0 + g_MSTex0Index + _t);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_pMSDummy->glName);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
#endif
}

void TextureCache::_updateBackground()
{
	u32 numBytes = gSP.bgImage.width * gSP.bgImage.height << gSP.bgImage.size >> 1;
	u32 crc;

	crc = CRC_Calculate( 0xFFFFFFFF, &RDRAM[gSP.bgImage.address], numBytes );

	if (gDP.otherMode.textureLUT != G_TT_NONE || gSP.bgImage.format == G_IM_FMT_CI) {
		if (gSP.bgImage.size == G_IM_SIZ_4b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC16[gSP.bgImage.palette], 4 );
		else if (gSP.bgImage.size == G_IM_SIZ_8b)
			crc = CRC_Calculate( crc, &gDP.paletteCRC256, 4 );
	}

	u32 params[4] = {gSP.bgImage.width, gSP.bgImage.height, gSP.bgImage.format, gSP.bgImage.size};
	crc = CRC_Calculate(crc, params, sizeof(u32)*4);

	Textures::iterator iter = m_textures.find(crc);
	if (iter != m_textures.end()) {
		CachedTexture & current = iter->second;
		assert(current.width == gSP.bgImage.width);
		assert(current.height == gSP.bgImage.height);
		assert(current.format == gSP.bgImage.format);
		assert(current.size == gSP.bgImage.size);

		activateTexture(0, &current);
		m_hits++;
		return;
	}

	m_misses++;

	glActiveTexture( GL_TEXTURE0 );
	CachedTexture * pCurrent = _addTexture(crc);

	glBindTexture( GL_TEXTURE_2D, pCurrent->glName );

	pCurrent->address = gSP.bgImage.address;

	pCurrent->format = gSP.bgImage.format;
	pCurrent->size = gSP.bgImage.size;

	pCurrent->width = gSP.bgImage.width;
	pCurrent->height = gSP.bgImage.height;

	pCurrent->clampWidth = gSP.bgImage.width;
	pCurrent->clampHeight = gSP.bgImage.height;
	pCurrent->palette = gSP.bgImage.palette;
	pCurrent->maskS = 0;
	pCurrent->maskT = 0;
	pCurrent->mirrorS = 0;
	pCurrent->mirrorT = 0;
	pCurrent->clampS = 0;
	pCurrent->clampT = 0;
	pCurrent->line = 0;
	pCurrent->tMem = 0;
	pCurrent->lastDList = RSP.DList;
	pCurrent->frameBufferTexture = CachedTexture::fbNone;

	pCurrent->realWidth = gSP.bgImage.width;
	pCurrent->realHeight = gSP.bgImage.height;

	pCurrent->scaleS = 1.0f / (f32)(pCurrent->realWidth);
	pCurrent->scaleT = 1.0f / (f32)(pCurrent->realHeight);

	pCurrent->shiftScaleS = 1.0f;
	pCurrent->shiftScaleT = 1.0f;

	pCurrent->offsetS = 0.5f;
	pCurrent->offsetT = 0.5f;

	_loadBackground(pCurrent);
	activateTexture(0, pCurrent);

	m_cachedBytes += pCurrent->textureBytes;
	current[0] = pCurrent;
}

void TextureCache::_clear()
{
	current[0] = current[1] = NULL;

	for (Textures::const_iterator cur = m_textures.cbegin(); cur != m_textures.cend(); ++cur) {
		m_cachedBytes -= cur->second.textureBytes;
		glDeleteTextures(1, &cur->second.glName);
	}
	m_textures.clear();
}

void TextureCache::update(u32 _t)
{
	if (config.textureFilter.txHiresEnable != 0 && config.textureFilter.txDump != 0) {
		/* Force reload hi-res textures. Useful for texture artists */
		if (isKeyPressed(G64_VK_R, 0x0001)) {
			if (txfilter_reloadhirestex()) {
				_clear();
			}
		}
		/* Turn on texture dump */
		else if (isKeyPressed(G64_VK_D, 0x0001)) {
			m_toggleDumpTex = !m_toggleDumpTex;
			if (m_toggleDumpTex) {
				displayLoadProgress(L"Texture dump - ON\n");
				_clear();
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			else {
				displayLoadProgress(L"Texture dump - OFF\n");
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	}


	switch(gSP.textureTile[_t]->textureMode) {
	case TEXTUREMODE_BGIMAGE:
		_updateBackground();
		return;
	case TEXTUREMODE_FRAMEBUFFER:
		FrameBuffer_ActivateBufferTexture( _t, gSP.textureTile[_t]->frameBuffer );
		return;
	case TEXTUREMODE_FRAMEBUFFER_BG:
		FrameBuffer_ActivateBufferTextureBG( _t, gSP.textureTile[_t]->frameBuffer );
		return;
	}

	if (gDP.otherMode.textureLOD == G_TL_LOD && gSP.texture.level == gSP.texture.tile && _t == 1) {
		current[1] = current[0];
		activateTexture(_t, current[_t]);
		return;
	}

	if (gSP.texture.tile == 7 &&
			_t == 0 &&
			gSP.textureTile[0] == gDP.loadTile &&
			gDP.loadTile->loadType == LOADTYPE_BLOCK &&
			gSP.textureTile[0]->tmem == gSP.textureTile[1]->tmem)
		gSP.textureTile[0] = gSP.textureTile[1];

	TileSizes sizes;
	_calcTileSizes(_t, sizes, gDP.loadTile);

	u32 crc;
	{
	TextureParams params;
	params.width = sizes.width;
	params.height = sizes.height;
	params.clampWidth = sizes.clampWidth;
	params.clampHeight = sizes.clampHeight;
	params.maskS = gSP.textureTile[_t]->masks;
	params.maskT = gSP.textureTile[_t]->maskt;
	params.mirrorS = gSP.textureTile[_t]->mirrors;
	params.mirrorT = gSP.textureTile[_t]->mirrort;
	params.clampS = gSP.textureTile[_t]->clamps;
	params.clampT = gSP.textureTile[_t]->clampt;
	params.format = gSP.textureTile[_t]->format;
	params.size = gSP.textureTile[_t]->size;

	crc = _calculateCRC( _t, params );
	}

	if (current[_t] != NULL && current[_t]->crc == crc) {
		activateTexture(_t, current[_t]);
		return;
	}

	Textures::iterator iter = m_textures.find(crc);
	if (iter != m_textures.end()) {
		CachedTexture & current = iter->second;
		assert(current.width == sizes.width);
		assert(current.height == sizes.height);
		assert(current.clampWidth == sizes.clampWidth);
		assert(current.clampHeight == sizes.clampHeight);
		assert(current.maskS == gSP.textureTile[_t]->masks);
		assert(current.maskT == gSP.textureTile[_t]->maskt);
		assert(current.mirrorS == gSP.textureTile[_t]->mirrors);
		assert(current.mirrorT == gSP.textureTile[_t]->mirrort);
		assert(current.clampS == gSP.textureTile[_t]->clamps);
		assert(current.clampT == gSP.textureTile[_t]->clampt);
		assert(current.format == gSP.textureTile[_t]->format);
		assert(current.size == gSP.textureTile[_t]->size);

		activateTexture(_t, &current);
		m_hits++;
		return;
	}

	m_misses++;

	glActiveTexture( GL_TEXTURE0 + _t );

	CachedTexture * pCurrent = _addTexture(crc);

	glBindTexture( GL_TEXTURE_2D, pCurrent->glName );

	pCurrent->address = gDP.loadInfo[gSP.textureTile[_t]->tmem].texAddress;

	pCurrent->format = gSP.textureTile[_t]->format;
	pCurrent->size = gSP.textureTile[_t]->size;

	pCurrent->width = sizes.width;
	pCurrent->height = sizes.height;

	pCurrent->clampWidth = sizes.clampWidth;
	pCurrent->clampHeight = sizes.clampHeight;

	pCurrent->palette = gSP.textureTile[_t]->palette;
/*	pCurrent->fulS = gSP.textureTile[t]->fulS;
	pCurrent->fulT = gSP.textureTile[t]->fulT;
	pCurrent->ulS = gSP.textureTile[t]->ulS;
	pCurrent->ulT = gSP.textureTile[t]->ulT;
	pCurrent->lrS = gSP.textureTile[t]->lrS;
	pCurrent->lrT = gSP.textureTile[t]->lrT;*/
	pCurrent->maskS = gSP.textureTile[_t]->masks;
	pCurrent->maskT = gSP.textureTile[_t]->maskt;
	pCurrent->mirrorS = gSP.textureTile[_t]->mirrors;
	pCurrent->mirrorT = gSP.textureTile[_t]->mirrort;
	pCurrent->clampS = gSP.textureTile[_t]->clamps;
	pCurrent->clampT = gSP.textureTile[_t]->clampt;
	pCurrent->line = gSP.textureTile[_t]->line;
	pCurrent->tMem = gSP.textureTile[_t]->tmem;
	pCurrent->lastDList = RSP.DList;
	pCurrent->frameBufferTexture = CachedTexture::fbNone;

	pCurrent->realWidth = sizes.realWidth;
	pCurrent->realHeight = sizes.realHeight;

	pCurrent->scaleS = 1.0f / (f32)(pCurrent->realWidth);
	pCurrent->scaleT = 1.0f / (f32)(pCurrent->realHeight);

	pCurrent->offsetS = 0.5f;
	pCurrent->offsetT = 0.5f;

	_load(_t, pCurrent);
	activateTexture( _t, pCurrent );

	m_cachedBytes += pCurrent->textureBytes;
	current[_t] = pCurrent;
}

void getTextureShiftScale(u32 t, const TextureCache & cache, f32 & shiftScaleS, f32 & shiftScaleT)
{
	if (gSP.textureTile[t]->textureMode != TEXTUREMODE_NORMAL) {
		shiftScaleS = cache.current[t]->shiftScaleS;
		shiftScaleT = cache.current[t]->shiftScaleT;
		return;
	}

	if (gDP.otherMode.textureLOD == G_TL_LOD && gSP.texture.level == gSP.texture.tile)
		t = 0;

	if (gSP.textureTile[t]->shifts > 10)
		shiftScaleS = (f32)(1 << (16 - gSP.textureTile[t]->shifts));
	else if (gSP.textureTile[t]->shifts > 0)
		shiftScaleS /= (f32)(1 << gSP.textureTile[t]->shifts);

	if (gSP.textureTile[t]->shiftt > 10)
		shiftScaleT = (f32)(1 << (16 - gSP.textureTile[t]->shiftt));
	else if (gSP.textureTile[t]->shiftt > 0)
		shiftScaleT /= (f32)(1 << gSP.textureTile[t]->shiftt);
}
