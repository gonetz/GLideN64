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

#ifndef __TXUTIL_H__
#define __TXUTIL_H__

/* maximum number of CPU cores allowed */
#define MAX_NUMCORE 8

#include "TxInternal.h"

/* extension for cache files */
#define TEXCACHE_EXT wst("htc")

class TxUtil
{
private:
	uint32 RiceCRC32(const uint8* src, int width, int height, int size, int rowStride);
	boolean RiceCRC32_CI4(const uint8* src, int width, int height, int rowStride,
						  uint32* crc32, uint32* cimax);
	boolean RiceCRC32_CI8(const uint8* src, int width, int height, int rowStride,
						  uint32* crc32, uint32* cimax);
public:
	TxUtil() { }
	~TxUtil() { }
	int sizeofTx(int width, int height, uint16 format);
	uint32 checksumTx(uint8 *data, int width, int height, uint16 format);
#if 0 /* unused */
	uint32 chkAlpha(uint32* src, int width, int height);
#endif
	uint32 checksum(uint8 *src, int width, int height, int size, int rowStride);
	uint64 checksum64(uint8 *src, int width, int height, int size, int rowStride, uint8 *palette);
	int getNumberofProcessors();
};

class TxMemBuf
{
private:
	uint8 *_tex[4];
	uint32 _size[4];
	uint32 _numBufs;
	TxMemBuf();
public:
	static TxMemBuf* getInstance() {
		static TxMemBuf txMemBuf;
		return &txMemBuf;
	}
	~TxMemBuf();
	boolean init(int maxwidth, int maxheight, boolean deposterize);
	void shutdown(void);
	uint8 *get(unsigned int num);
	uint32 size_of(unsigned int num);
};

void setTextureFormat(uint16 internalFormat, GHQTexInfo * info);

#endif /* __TXUTIL_H__ */
