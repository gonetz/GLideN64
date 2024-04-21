/*
 * Texture Filtering
 * Version:  1.0
 *
 * Copyright (C) 2007  Hiroshi Morii   All Rights Reserved.
 * Email koolsmoky(at)users.sourceforge.net
 * Web   http://www.3dfxzone.it/koolsmoky
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef __MSC__
#pragma warning(disable: 4786)
#endif

#include "TxFilter.h"
#include "TxFilterExport.h"

TxFilter *txFilter = nullptr;

#ifdef __cplusplus
extern "C"{
#endif

TAPI boolean TAPIENTRY
txfilter_init(int maxwidth, int maxheight, int maxbpp, int options, int cachesize,
	const wchar_t * txCachePath, const wchar_t* txDumpPath, const wchar_t * texPackPath, const wchar_t * ident,
	dispInfoFuncExt callback)
{
  if (txFilter) return 0;

  txFilter = new TxFilter(maxwidth, maxheight, maxbpp, options, cachesize,
	  txCachePath, txDumpPath, texPackPath, ident, callback);

  return 1;
}

TAPI void TAPIENTRY
txfilter_shutdown(void)
{
  if (txFilter) delete txFilter;

  txFilter = nullptr;
}

TAPI boolean TAPIENTRY
txfilter_filter(uint8 *src, int srcwidth, int srcheight, uint16 srcformat,
		 uint64 g64crc, N64FormatSize n64FmtSz, GHQTexInfo *info)
{
  if (txFilter)
	return txFilter->filter(src, srcwidth, srcheight, ColorFormat(u32(srcformat)),
							   g64crc, n64FmtSz, info);

  return 0;
}

TAPI boolean TAPIENTRY
txfilter_hirestex(uint64 g64crc, Checksum r_crc64, uint16 *palette, N64FormatSize n64FmtSz, GHQTexInfo *info)
{
  if (txFilter)
	return txFilter->hirestex(g64crc, r_crc64, palette, n64FmtSz, info);

  return 0;
}

TAPI uint64 TAPIENTRY
txfilter_checksum(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette)
{
  if (txFilter)
	return txFilter->checksum64(src, width, height, size, rowStride, palette);

  return 0;
}

TAPI uint64 TAPIENTRY
txfilter_checksum_strong(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette)
{
	if (txFilter)
		return txFilter->checksum64strong(src, width, height, size, rowStride, palette);

	return 0;
}

TAPI boolean TAPIENTRY
txfilter_dmptx(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, N64FormatSize n64FmtSz, Checksum r_crc64)
{
  if (txFilter)
	return txFilter->dmptx(src, width, height, rowStridePixel, ColorFormat(u32(gfmt)), n64FmtSz, r_crc64, FALSE);

  return 0;
}

TAPI boolean TAPIENTRY
txfilter_dmptx_strong(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, N64FormatSize n64FmtSz, Checksum r_crc64)
{
	if (txFilter)
		return txFilter->dmptx(src, width, height, rowStridePixel, ColorFormat(u32(gfmt)), n64FmtSz, r_crc64, TRUE);

	return 0;
}

TAPI boolean TAPIENTRY
txfilter_dmptx_mipmap(uint8 *src, int width, int height, int rowStridePixel, uint16 gfmt, N64FormatSize n64FmtSz, Checksum crc64, Checksum crc64base, boolean strongCRC)
{
	if (txFilter)
		return txFilter->dmptxMipmap(src, width, height, rowStridePixel, ColorFormat(u32(gfmt)), n64FmtSz, crc64, crc64base, strongCRC);

	return 0;
}

TAPI boolean TAPIENTRY
txfilter_dmp_mipmap(GHQDumpTexInfo* infos, int numLevels)
{
	for (int i = 0; i < numLevels; ++i) {

	}

	return 0;
}

TAPI boolean TAPIENTRY
txfilter_reloadhirestex()
{
  if (txFilter)
	return txFilter->reloadhirestex();

  return 0;
}

TAPI void TAPIENTRY
txfilter_dumpcache(void)
{
	if (txFilter)
	  txFilter->dumpcache();
}


#ifdef __cplusplus
}
#endif
