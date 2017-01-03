#pragma once
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
	u64 getMux() const { return m_key.mux; }

private:
	gDPCombine m_key;
};
