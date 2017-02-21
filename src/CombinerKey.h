#pragma once
#include <istream>
#include "gDP.h"

class CombinerKey {
public:
	CombinerKey() = default;
	explicit CombinerKey(u64 _mux);
	CombinerKey(const CombinerKey & _other);

	void operator=(u64 _mux);
	void operator=(const CombinerKey & _other);

	bool operator==(const CombinerKey & _other) const;
	bool operator<(const CombinerKey & _other) const;

	bool isRectKey() const;

	u32 getCycleType() const;

	u64 getMux() const { return m_key.mux; }

	void read(std::istream & _is);

private:
	gDPCombine m_key;
};
