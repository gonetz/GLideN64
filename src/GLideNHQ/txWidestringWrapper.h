#ifndef ___TXWIDESCREENWRAPPER_H__
#define ___TXWIDESCREENWRAPPER_H__

#include <string>
#include <algorithm>

#define tx_swprintf	swprintf
#define wst(A) L##A
#define wccmp(A, B) A[0] == B[0]

typedef std::wstring tx_wstring;

inline
void removeColon(tx_wstring& _s)
{
	std::replace(_s.begin(), _s.end(), L':', L'-');
}

#endif // ___TXWIDESCREENWRAPPER_H__
