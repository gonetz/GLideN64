#pragma once
#include <istream>
#include "gDP.h"

class CombinerKey {

	struct SecondaryShaderParams
	{
		union
		{
			struct
			{
				unsigned int bi_lerp0 : 1;
				unsigned int bi_lerp1 : 1;
				unsigned int enableAlphaTest : 1;
				unsigned int alphaCompareMode : 2;

			};

			u64				mux;
		};
	};

public:
	CombinerKey() {
		m_key.mux = 0;
	}
	explicit CombinerKey(u64 _mux, bool _setModeBits = true);
	explicit CombinerKey(u64 _mux, u64 _secondaryFlags, bool _setModeBits = true);
	CombinerKey(const CombinerKey & _other);

	void operator=(u64 _mux);
	void operator=(const CombinerKey & _other);

	bool operator==(const CombinerKey & _other) const;
	bool operator<(const CombinerKey & _other) const;

	bool isRectKey() const;

	u32 getCycleType() const;

	u64 getMux() const { return m_key.mux; }

	u64 getSecondaryParams() const { return m_secondaryFlags.mux; }

	void setBiLerp0(unsigned int _bilerp0);
	void setBiLerp1(unsigned int _bilerp1);
	void setEnableAlphaTest(unsigned int _enableAlphaTest);
	void setAlphaCompareMode(unsigned int _alphaCompareMode);

	unsigned int getBiLerp0(void);
	unsigned int getBiLerp1(void);
	unsigned int getEnableAlphaTest(void);
	unsigned int getAlphaCompareMode(void);

	void read(std::istream & _is);

private:
	gDPCombine m_key;
	SecondaryShaderParams m_secondaryFlags;
};