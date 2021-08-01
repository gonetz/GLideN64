#ifndef TXHIRESNOCACHE_H
#define TXHIRESNOCACHE_H

#include "TxHiResLoader.h"

class TxHiResNoCache : public TxHiResLoader
{
	private:
		bool _createFileIndex(bool update);
		bool _createFileIndexInDir(tx_wstring directory, bool update);
		void _clear();

		struct fileIndexEntry_t
		{
			char fname[MAX_PATH];
			tx_wstring directory;
			uint32 siz;
			uint32 fmt;
		};

		tx_wstring _fullTexPath;
		tx_wstring _ident;
		char _identc[MAX_PATH];
		std::map<uint64, fileIndexEntry_t> _filesIndex;
		std::map<uint64, GHQTexInfo> _loadedTex;
		dispInfoFuncExt _callback;
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
