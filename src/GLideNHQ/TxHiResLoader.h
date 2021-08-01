#ifndef TXHIRESLOADER_H
#define TXHIRESLOADER_H

/* support hires textures
 *   0: disable
 *   1: enable
 */
#define HIRES_TEXTURE 1

#ifdef OS_WINDOWS
#define CORRECTFILENAME(str)  for (uint32 i = 0; i < strlen(str); i++) str[i] = tolower(str[i])
#else
#define CORRECTFILENAME(str)
#endif /* OS_WINDOWS */

#include "TxCache.h"
#include "TxQuantize.h"
#include "TxImage.h"
#include "TxReSample.h"

class TxHiResLoader
{
protected:
	uint32_t checkFileName(char* ident, char* fname, uint32_t* pChksum, uint32_t* pPalchksum, uint32_t* pFmt, uint32_t* pSiz);
	uint8_t* loadFileInfoTex(char* fname, int siz, int* pWidth, int* pHeight, uint32_t fmt, ColorFormat* pFormat);
 	
	std::unique_ptr<TxImage> _txImage;
	std::unique_ptr<TxQuantize> _txQuantize;
	std::unique_ptr<TxReSample> _txReSample;

	int _maxwidth;
	int _maxheight;
	int _maxbpp;
	int _options;

public:
	TxHiResLoader(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options);
	virtual ~TxHiResLoader(){}

	virtual bool empty() const = 0;
	virtual bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) = 0;
	virtual bool get(Checksum checksum, GHQTexInfo *info) = 0;
	virtual bool reload() = 0;
	virtual void dump() = 0;
};

#endif /* TXHIRESLOADER_H */
