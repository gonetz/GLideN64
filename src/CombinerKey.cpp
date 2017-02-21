#include "Combiner.h"
#include "CombinerKey.h"


/*---------------CombinerKey-------------*/

CombinerKey::CombinerKey(u64 _mux)
{
	m_key.mux = _mux;

	// High byte of muxs0 is zero. We can use it for addtional combiner flags:
	// [0 - 0] polygon type: 0 - triangle, 1 - rect
	// [1 - 2] cycle type
	u32 flags = CombinerInfo::get().isRectMode() ? 1U : 0U;
	flags |= (gDP.otherMode.cycleType << 1);

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

bool CombinerKey::isRectKey() const
{
	return ((m_key.muxs0 >> 24) & 1) != 0;
}

void CombinerKey::read(std::istream & _is) {
	_is.read((char*)&m_key.mux, sizeof(m_key.mux));
}
