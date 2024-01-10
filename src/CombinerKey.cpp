#include "Combiner.h"
#include "CombinerKey.h"


/*---------------CombinerKey-------------*/

CombinerKey::CombinerKey(u64 _mux, bool _setModeBits)
{
	m_key.mux = _mux;
	if (!_setModeBits)
		return;

	// High byte of muxs0 is zero. We can use it for addtional combiner flags:
	// [0 - 0] polygon type: 0 - triangle, 1 - rect
	// [1 - 2] cycle type
	// [3 - 3] bi_lerp1
	// [4 - 4] bi_lerp0
	u32 flags = CombinerInfo::get().isRectMode() ? 1U : 0U;
	const u32 cycleType = gDP.otherMode.cycleType;
	const u32 bilerp = (gDP.otherMode.h >> 10) & 3;

	u32 wantAlpha = 0;
	if (gDP.otherMode.cycleType == G_CYC_FILL) {
		wantAlpha = 0;
	}
	else if (gDP.otherMode.cycleType == G_CYC_COPY) {
		if (gDP.otherMode.alphaCompare & G_AC_THRESHOLD) {
			wantAlpha = 1;
		}
		else {
			wantAlpha = 0;
		}
	}
	else if ((gDP.otherMode.alphaCompare & G_AC_THRESHOLD) != 0) {
		wantAlpha = 1;
	}
	else {
		wantAlpha = 0;
	}

	wantAlpha |= !!(gDP.otherMode.cvgXAlpha);
	u32 noAlpha = !wantAlpha;

	flags |= (cycleType << 1);
	flags |= (bilerp << 3);
	flags |= (noAlpha << 5);

	m_key.muxs0 |= (flags << 24);
}

CombinerKey::CombinerKey(const CombinerKey & _other)
{
	m_key.mux = _other.m_key.mux;
}

void CombinerKey::operator=(u64 _mux)
{
	m_key.mux = _mux;
}

void CombinerKey::operator=(const CombinerKey & _other)
{
	m_key.mux = _other.m_key.mux;
}

bool CombinerKey::operator==(const CombinerKey & _other) const
{
	return m_key.mux == _other.m_key.mux;
}

bool CombinerKey::operator<(const CombinerKey & _other) const
{
	return m_key.mux < _other.m_key.mux;
}

u32 CombinerKey::getCycleType() const
{
	return (m_key.muxs0 >> 25) & 3;
}

u32 CombinerKey::getBilerp() const
{
	return (m_key.muxs0 >> 27) & 3;
}

bool CombinerKey::isRectKey() const
{
	return ((m_key.muxs0 >> 24) & 1) != 0;
}

bool CombinerKey::noAlpha() const
{
	return ((m_key.muxs0 >> 29) & 1) != 0;
}

void CombinerKey::read(std::istream & _is)
{
	_is.read((char*)&m_key.mux, sizeof(m_key.mux));
}

const CombinerKey & CombinerKey::getEmpty()
{
	static CombinerKey emptyKey;
	return emptyKey;
}
