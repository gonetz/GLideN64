#ifndef TXHIRESNOCACHE_H
#define TXHIRESNOCACHE_H

#include "TxHiResLoader.h"
#include <unordered_map>

class TxHiResNoCache : public TxHiResLoader
{
	private:
		bool _createFileIndex(bool update);
		bool _createFileIndexInDir(tx_wstring directory, bool update);
		void _freeOldestTexture(u64 currentNum);
		void _clear();
		u64 _getCounterNum();

		struct fileIndexEntry_t
		{
			char fname[MAX_PATH];
			tx_wstring directory;
			uint32 siz;
			uint32 fmt;
		};

		struct loadedTexture_t
		{
			u64 counterNum;
			GHQTexInfo info;
		};

		tx_wstring _fullTexPath;
		tx_wstring _ident;
		char _identc[MAX_PATH];
		std::unordered_map<uint64, fileIndexEntry_t> _filesIndex;
		std::unordered_map<uint64, loadedTexture_t> _loadedTex;
		dispInfoFuncExt _callback;
		u64 _loadedTexSize = 0;

	public:
		~TxHiResNoCache();
  		TxHiResNoCache(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options,
			   const wchar_t *cachePath,
			   const wchar_t *texPackPath,
			   const wchar_t *fullTexPath,
			   const wchar_t *ident,
			   dispInfoFuncExt callback);
  		bool empty() const override;
  		bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) override { return false; }
  		bool get(Checksum checksum, GHQTexInfo *info) override;
  		bool reload() override;
  		void dump() override { };
};

#endif /* TXHIRESNOCACHE_H */
