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

#include <assert.h>
#include "TxCache.h"
#include "TxDbg.h"
#include <osal_files.h>
#include <zlib.h>
#include <memory.h>
#include <stdlib.h>

class TxCacheImpl
{
public:
	virtual ~TxCacheImpl() = default;

	virtual bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) = 0;
	virtual bool get(Checksum checksum, GHQTexInfo *info) = 0;
	virtual bool save(const wchar_t *path, const wchar_t *filename, const int config) = 0;
	virtual bool load(const wchar_t *path, const wchar_t *filename, const int config, boolean force) = 0;
	virtual bool del(Checksum checksum) = 0;
	virtual bool isCached(Checksum checksum) = 0;
	virtual void clear() = 0;
	virtual bool empty() const = 0;

	virtual uint64 size() const = 0;
	virtual uint64 totalSize() const = 0;
	virtual uint64 cacheLimit() const = 0;
};

class TxMemoryCache : public TxCacheImpl
{
public:
	TxMemoryCache(uint32 & _options, uint64 cacheLimit, dispInfoFuncExt callback);

	bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) override;
	bool get(Checksum checksum, GHQTexInfo *info) override;

	bool save(const wchar_t *path, const wchar_t *filename, const int config) override;
	bool load(const wchar_t *path, const wchar_t *filename, const int config, boolean force) override;
	bool del(Checksum checksum) override;
	bool isCached(Checksum checksum) override;
	void clear() override;
	bool empty() const  override { return _cache.empty(); }

	uint64 size() const  override { return _cache.size(); }
	uint64 totalSize() const  override { return _totalSize; }
	uint64 cacheLimit() const  override { return _cacheLimit; }

private:
	struct TXCACHE {
		int size;
		GHQTexInfo info;
		std::list<uint64>::iterator it;
	};

	uint32 & _options;
	dispInfoFuncExt _callback;
	uint64 _cacheLimit;
	uint64 _totalSize;

	std::map<uint64, TXCACHE*> _cache;
	std::list<uint64> _cachelist;

	uint8 *_gzdest0 = nullptr;
	uint8 *_gzdest1 = nullptr;
	uint32 _gzdestLen = 0;
};

TxMemoryCache::TxMemoryCache(uint32 & options,
							uint64 cacheLimit,
							dispInfoFuncExt callback)
	: _options(options)
	, _cacheLimit(cacheLimit)
	, _callback(callback)
	, _totalSize(0U)
{
	/* zlib memory buffers to (de)compress hires textures */
	if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE)) {
		_gzdest0 = TxMemBuf::getInstance()->get(0);
		_gzdest1 = TxMemBuf::getInstance()->get(1);
		_gzdestLen = (TxMemBuf::getInstance()->size_of(0) < TxMemBuf::getInstance()->size_of(1)) ?
			TxMemBuf::getInstance()->size_of(0) : TxMemBuf::getInstance()->size_of(1);

		if (!_gzdest0 || !_gzdest1 || !_gzdestLen) {
			_options &= ~(GZ_TEXCACHE | GZ_HIRESTEXCACHE);
			_gzdest0 = nullptr;
			_gzdest1 = nullptr;
			_gzdestLen = 0;
		}
	}
}

bool TxMemoryCache::add(Checksum checksum, GHQTexInfo *info, int dataSize)
{
	/* NOTE: dataSize must be provided if info->data is zlib compressed. */

	if (!checksum || !info->data || _cache.find(checksum) != _cache.end())
		return false;

	uint8 *dest = info->data;
	uint32 format = info->format;

	if (dataSize == 0) {
		dataSize = TxUtil::sizeofTx(info->width, info->height, info->format);

		if (!dataSize)
			return false;

		if (_options & (GZ_TEXCACHE | GZ_HIRESTEXCACHE)) {
			/* zlib compress it. compression level:1 (best speed) */
			uLongf destLen = _gzdestLen;
			dest = (dest == _gzdest0) ? _gzdest1 : _gzdest0;
			if (compress2(dest, &destLen, info->data, dataSize, 1) != Z_OK) {
				dest = info->data;
				DBG_INFO(80, wst("Error: zlib compression failed!\n"));
			}
			else {
				DBG_INFO(80, wst("zlib compressed: %.02fkb->%.02fkb\n"), (float)dataSize / 1000, (float)destLen / 1000);
				dataSize = destLen;
				format |= GL_TEXFMT_GZ;
			}
		}
	}

  
  /* if cache size exceeds limit, remove old cache */
	if (_cacheLimit != 0) {
		_totalSize += dataSize;
		if ((_totalSize > _cacheLimit) && !_cachelist.empty()) {
			/* _cachelist is arranged so that frequently used textures are in the back */
			std::list<uint64>::iterator itList = _cachelist.begin();
			while (itList != _cachelist.end()) {
				/* find it in _cache */
				auto itMap = _cache.find(*itList);
				if (itMap != _cache.end()) {
					/* yep we have it. remove it. */
					_totalSize -= (*itMap).second->size;
					free((*itMap).second->info.data);
					delete (*itMap).second;
					_cache.erase(itMap);
				}
				itList++;

				/* check if memory cache has enough space */
				if (_totalSize <= _cacheLimit)
					break;
			}
			/* remove from _cachelist */
			_cachelist.erase(_cachelist.begin(), itList);

			DBG_INFO(80, wst("+++++++++\n"));
		}
		_totalSize -= dataSize;
	}

	/* cache it */
	uint8 *tmpdata = (uint8*)malloc(dataSize);
	if (tmpdata == nullptr)
		return false;

	TXCACHE *txCache = new TXCACHE;
	/* we can directly write as we filter, but for now we get away
	* with doing memcpy after all the filtering is done.
	*/
	memcpy(tmpdata, dest, dataSize);

	/* copy it */
	memcpy(&txCache->info, info, sizeof(GHQTexInfo));
	txCache->info.data = tmpdata;
	txCache->info.format = format;
	txCache->size = dataSize;

	/* add to cache */
	if (_cacheLimit != 0) {
		_cachelist.push_back(checksum);
		txCache->it = --(_cachelist.end());
	}
	/* _cache[checksum] = txCache; */
	_cache.insert(std::map<uint64, TXCACHE*>::value_type(checksum, txCache));

#ifdef DEBUG
	DBG_INFO(80, wst("[%5d] added!! crc:%08X %08X %d x %d gfmt:%x total:%.02fmb\n"),
		_cache.size(), checksum._hi, checksum._low,
		info->width, info->height, info->format & 0xffff, (double)_totalSize / 1000000);

	if (_cacheLimit != 0) {
		DBG_INFO(80, wst("cache max config:%.02fmb\n"), (double)_cacheLimit / 1000000);

		if (_cache.size() != _cachelist.size()) {
			DBG_INFO(80, wst("Error: cache/cachelist mismatch! (%d/%d)\n"), _cache.size(), _cachelist.size());
		}
	}
#endif

	/* total cache size */
	_totalSize += dataSize;

	return true;
}

bool TxMemoryCache::get(Checksum checksum, GHQTexInfo *info)
{
	if (!checksum || _cache.empty())
		return false;

	/* find a match in cache */
	auto itMap = _cache.find(checksum);
	if (itMap != _cache.end()) {
		/* yep, we've got it. */
		memcpy(info, &(((*itMap).second)->info), sizeof(GHQTexInfo));

		/* push it to the back of the list */
		if (_cacheLimit != 0) {
			_cachelist.erase(((*itMap).second)->it);
			_cachelist.push_back(checksum);
			((*itMap).second)->it = --(_cachelist.end());
		}

		/* zlib decompress it */
		if (info->format & GL_TEXFMT_GZ) {
			uLongf destLen = _gzdestLen;
			uint8 *dest = (_gzdest0 == info->data) ? _gzdest1 : _gzdest0;
			if (uncompress(dest, &destLen, info->data, ((*itMap).second)->size) != Z_OK) {
				DBG_INFO(80, wst("Error: zlib decompression failed!\n"));
				return false;
			}
			info->data = dest;
			info->format &= ~GL_TEXFMT_GZ;
			DBG_INFO(80, wst("zlib decompressed: %.02fkb->%.02fkb\n"), (float)(((*itMap).second)->size) / 1000, (float)destLen / 1000);
		}

		return true;
	}

	return false;
}

bool TxMemoryCache::save(const wchar_t *path, const wchar_t *filename, int config)
{
	if (_cache.empty())
		return false;

	/* dump cache to disk */
	char cbuf[MAX_PATH];

	osal_mkdirp(path);

#ifdef OS_WINDOWS
	wchar_t curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	CHDIR(path);
#else
	char curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	wcstombs(cbuf, path, MAX_PATH);
	CHDIR(cbuf);
#endif

	wcstombs(cbuf, filename, MAX_PATH);

	gzFile gzfp = gzopen(cbuf, "wb1");
	DBG_INFO(80, wst("gzfp:%x file:%ls\n"), gzfp, filename);
	if (gzfp) {
		/* write header to determine config match */
		gzwrite(gzfp, &config, 4);

		auto itMap = _cache.begin();
		int total = 0;
		while (itMap != _cache.end()) {
			uint8 *dest = (*itMap).second->info.data;
			uint32 destLen = (*itMap).second->size;
			uint32 format = (*itMap).second->info.format;

			/* to keep things simple, we save the texture data in a zlib uncompressed state. */
			/* sigh... for those who cannot wait the extra few seconds. changed to keep
			* texture data in a zlib compressed state. if the GZ_TEXCACHE or GZ_HIRESTEXCACHE
			* option is toggled, the cache will need to be rebuilt.
			*/
			/*if (format & GL_TEXFMT_GZ) {
			dest = _gzdest0;
			destLen = _gzdestLen;
			if (dest && destLen) {
			if (uncompress(dest, &destLen, (*itMap).second->info.data, (*itMap).second->size) != Z_OK) {
			dest = nullptr;
			destLen = 0;
			}
			format &= ~GL_TEXFMT_GZ;
			}
			}*/

			if (dest && destLen) {
				/* texture checksum */
				gzwrite(gzfp, &((*itMap).first), 8);

				/* other texture info */
				gzwrite(gzfp, &((*itMap).second->info.width), 4);
				gzwrite(gzfp, &((*itMap).second->info.height), 4);
				gzwrite(gzfp, &format, 4);
				gzwrite(gzfp, &((*itMap).second->info.texture_format), 2);
				gzwrite(gzfp, &((*itMap).second->info.pixel_type), 2);
				gzwrite(gzfp, &((*itMap).second->info.is_hires_tex), 1);

				gzwrite(gzfp, &destLen, 4);
				gzwrite(gzfp, dest, destLen);
			}

			itMap++;

			if (_callback)
				(*_callback)(wst("Total textures saved to HDD: %d\n"), ++total);
		}
		gzclose(gzfp);
	}

	CHDIR(curpath);

	return !_cache.empty();
}

bool TxMemoryCache::load(const wchar_t *path, const wchar_t *filename, int config, boolean force)
{
	/* find it on disk */
	char cbuf[MAX_PATH];

#ifdef OS_WINDOWS
	wchar_t curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	CHDIR(path);
#else
	char curpath[MAX_PATH];
	GETCWD(MAX_PATH, curpath);
	wcstombs(cbuf, path, MAX_PATH);
	CHDIR(cbuf);
#endif

	wcstombs(cbuf, filename, MAX_PATH);

	gzFile gzfp = gzopen(cbuf, "rb");
	DBG_INFO(80, wst("gzfp:%x file:%ls\n"), gzfp, filename);
	if (gzfp) {
		/* yep, we have it. load it into memory cache. */
		int dataSize;
		uint64 checksum;
		int tmpconfig;
		/* read header to determine config match */
		gzread(gzfp, &tmpconfig, 4);

		if (tmpconfig == config || force) {
			do {
				GHQTexInfo tmpInfo;

				gzread(gzfp, &checksum, 8);

				gzread(gzfp, &tmpInfo.width, 4);
				gzread(gzfp, &tmpInfo.height, 4);
				gzread(gzfp, &tmpInfo.format, 4);
				gzread(gzfp, &tmpInfo.texture_format, 2);
				gzread(gzfp, &tmpInfo.pixel_type, 2);
				gzread(gzfp, &tmpInfo.is_hires_tex, 1);

				gzread(gzfp, &dataSize, 4);

				tmpInfo.data = (uint8*)malloc(dataSize);
				if (tmpInfo.data) {
					gzread(gzfp, tmpInfo.data, dataSize);

					/* add to memory cache */
					add(checksum, &tmpInfo, (tmpInfo.format & GL_TEXFMT_GZ) ? dataSize : 0);

					free(tmpInfo.data);
				}
				else {
					gzseek(gzfp, dataSize, SEEK_CUR);
				}

				/* skip in between to prevent the loop from being tied down to vsync */
				if (_callback && (!(_cache.size() % 100) || gzeof(gzfp)))
					(*_callback)(wst("[%d] total mem:%.02fmb - %ls\n"), _cache.size(), (float)_totalSize / 1000000, filename);

			} while (!gzeof(gzfp));
			gzclose(gzfp);
		}
	}

	CHDIR(curpath);

	return !_cache.empty();
}

bool TxMemoryCache::del(Checksum checksum)
{
	if (!checksum || _cache.empty())
		return false;

	auto itMap = _cache.find(checksum);
	if (itMap != _cache.end()) {

		/* for texture cache (not hi-res cache) */
		if (!_cachelist.empty())
			_cachelist.erase(((*itMap).second)->it);

		/* remove from cache */
		free((*itMap).second->info.data);
		_totalSize -= (*itMap).second->size;
		delete (*itMap).second;
		_cache.erase(itMap);

		DBG_INFO(80, wst("removed from cache: checksum = %08X %08X\n"), checksum._low, checksum._hi);

		return true;
	}

	return false;
}

bool TxMemoryCache::isCached(Checksum checksum)
{
	return _cache.find(checksum) != _cache.end();
}

void TxMemoryCache::clear()
{
	if (!_cache.empty()) {
		auto itMap = _cache.begin();
		while (itMap != _cache.end()) {
			free((*itMap).second->info.data);
			delete (*itMap).second;
			itMap++;
		}
		_cache.clear();
	}

	if (!_cachelist.empty()) _cachelist.clear();

	_totalSize = 0;
}

TxCache::~TxCache()
{
	/* free memory, clean up, etc */
	clear();
}

TxCache::TxCache(uint32 options,
	uint64 cachesize,
	const wchar_t *cachePath,
	const wchar_t *ident,
	dispInfoFuncExt callback)
	: _options(options)
	, _callback(callback)
{
	/* save path name */
	if (cachePath)
		_cachePath.assign(cachePath);

	/* save ROM name */
	if (ident)
		_ident.assign(ident);

	_pImpl.reset(new TxMemoryCache(_options, cachesize, _callback));
}

bool TxCache::add(Checksum checksum, GHQTexInfo *info, int dataSize)
{
	return _pImpl->add(checksum, info, dataSize);
}

bool TxCache::get(Checksum checksum, GHQTexInfo *info)
{
	return _pImpl->get(checksum, info);
}

uint64 TxCache::size() const
{
	return _pImpl->size();
}

uint64 TxCache::totalSize() const
{
	return _pImpl->totalSize();
}

uint64 TxCache::cacheLimit() const
{
	return _pImpl->cacheLimit();
}

boolean TxCache::save(const wchar_t *path, const wchar_t *filename, const int config)
{
	return _pImpl->save(path, filename, config);
}

boolean TxCache::load(const wchar_t *path, const wchar_t *filename, const int config, boolean force)
{
	return _pImpl->load(path, filename, config, force);
}

boolean TxCache::del(Checksum checksum)
{
	return _pImpl->del(checksum);
}

boolean TxCache::isCached(Checksum checksum)
{
	return _pImpl->isCached(checksum);
}

void TxCache::clear()
{
	_pImpl->clear();
}

bool TxCache::empty() const
{
	return _pImpl->empty();
}
