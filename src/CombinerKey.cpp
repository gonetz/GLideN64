#include "Combiner.h"
#include "CombinerKey.h"


/*---------------CombinerKey-------------*/

CombinerKey::CombinerKey(u64 _mux, bool _setModeBits)
{
	m_key.mux = _mux;
	m_secondaryFlags.mux = 0;
	if (!_setModeBits)
		return;

	// High byte of muxs0 is zero. We can use it for addtional combiner flags:
	// [0 - 0] polygon type: 0 - triangle, 1 - rect
	// [1 - 2] cycle type
	u32 flags = CombinerInfo::get().isRectMode() ? 1U : 0U;
	flags |= (gDP.otherMode.cycleType << 1);

	m_key.muxs0 |= (flags << 24);
}

CombinerKey::CombinerKey(u64 _mux, u64 secondaryParams, bool _setModeBits)
{
	m_key.mux = _mux;
	m_secondaryFlags.mux = secondaryParams;
	if (!_setModeBits)
		return;

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
	m_secondaryFlags = _other.m_secondaryFlags;
}

void CombinerKey::operator=(u64 _mux)
{
	m_key.mux = _mux;
	m_secondaryFlags.mux = 0;
}

void CombinerKey::operator=(const CombinerKey & _other)
{
	m_key.mux = _other.m_key.mux;
	m_secondaryFlags = _other.m_secondaryFlags;
}

bool CombinerKey::operator==(const CombinerKey & _other) const
{
	return m_key.mux == _other.m_key.mux && m_secondaryFlags.mux == _other.m_secondaryFlags.mux;
}

bool CombinerKey::operator<(const CombinerKey & _other) const
{
	return (m_key.mux < _other.m_key.mux) ||
		(m_key.mux == _other.m_key.mux && m_secondaryFlags.mux < _other.m_secondaryFlags.mux);
}

u32 CombinerKey::getCycleType() const
{
	return (m_key.muxs0 >> 25) & 3;
}

bool CombinerKey::isRectKey() const
{
	return ((m_key.muxs0 >> 24) & 1) != 0;
}

void CombinerKey::setBiLerp0(unsigned int _bilerp0)
{
	m_secondaryFlags.bi_lerp0 = _bilerp0;
}

void CombinerKey::setBiLerp1(unsigned int _bilerp1)
{
	m_secondaryFlags.bi_lerp1 = _bilerp1;
}

void CombinerKey::setEnableAlphaTest(unsigned int _enableAlphaTest)
{
	m_secondaryFlags.enableAlphaTest = _enableAlphaTest;
}

void CombinerKey::setAlphaCompareMode(unsigned int _alphaCompareMode)
{
	m_secondaryFlags.alphaCompareMode = _alphaCompareMode;
}

void CombinerKey::setCvgXAlpha(unsigned int _cvgXAlpha)
{
	m_secondaryFlags.cvgXAlpha = _cvgXAlpha;
}

void CombinerKey::setAlphaCvgSel(unsigned int _alphaCvgSel)
{
	m_secondaryFlags.alphaCvgSel = _alphaCvgSel;
}

void CombinerKey::setAlphaDither(unsigned int _alphaDither)
{
	m_secondaryFlags.alphaDither = _alphaDither;
}

void CombinerKey::setColorDither(unsigned int _colorDither)
{
	m_secondaryFlags.colorDither = _colorDither;
}

void CombinerKey::setRenderTarget(unsigned int _renderTarget)
{
	m_secondaryFlags.renderTarget = _renderTarget;
}

void CombinerKey::setTextureFilter(unsigned int _textureFilter)
{
	m_secondaryFlags.textureFilter = _textureFilter;
}

void CombinerKey::setFbMonochromeMode0(unsigned int _fbMonochromeMode0)
{
	m_secondaryFlags.fbMonochromeMode0 = _fbMonochromeMode0;
}

void CombinerKey::setFbMonochromeMode1(unsigned int _fbMonochromeMode1)
{
	m_secondaryFlags.fbMonochromeMode1 = _fbMonochromeMode1;
}

void CombinerKey::setFbFixedAlpha0(unsigned int _fbFixedAlpha0)
{
	m_secondaryFlags.fbFixedAlpha0 = _fbFixedAlpha0;
}

void CombinerKey::setFbFixedAlpha1(unsigned int _fbFixedAlpha1)
{
	m_secondaryFlags.fbFixedAlpha1 = _fbFixedAlpha1;
}

unsigned int CombinerKey::getBiLerp0(void) const
{
	return m_secondaryFlags.bi_lerp0;
}

unsigned int CombinerKey::getBiLerp1(void) const
{
	return m_secondaryFlags.bi_lerp1;
}

unsigned int CombinerKey::getEnableAlphaTest(void) const
{
	return m_secondaryFlags.enableAlphaTest;
}

unsigned int CombinerKey::getAlphaCompareMode(void) const
{
	return m_secondaryFlags.alphaCompareMode;
}

unsigned int CombinerKey::getCvgXAlpha(void) const
{
	return m_secondaryFlags.cvgXAlpha;
}

unsigned int CombinerKey::getAlphaCvgSel(void) const
{
	return m_secondaryFlags.alphaCvgSel;
}

unsigned int CombinerKey::getAlphaDither(void) const
{
	return m_secondaryFlags.alphaDither;
}

unsigned int CombinerKey::getColorDither(void) const
{
	return m_secondaryFlags.colorDither;
}

unsigned int CombinerKey::getRenderTarget(void) const
{
	return m_secondaryFlags.renderTarget;
}

unsigned int CombinerKey::getTextureFilter(void) const
{
	return m_secondaryFlags.textureFilter;
}

unsigned int CombinerKey::getFbMonochromeMode0(void) const
{
	return m_secondaryFlags.fbMonochromeMode0;
}

unsigned int CombinerKey::getFbMonochromeMode1(void) const
{
	return m_secondaryFlags.fbMonochromeMode1;
}

unsigned int CombinerKey::getFbFixedAlpha0(void) const
{
	return m_secondaryFlags.fbFixedAlpha0;
}

unsigned int CombinerKey::getFbFixedAlpha1(void) const
{
	return m_secondaryFlags.fbFixedAlpha1;
}

void CombinerKey::read(std::istream & _is) {
	_is.read((char*)&m_key.mux, sizeof(m_key.mux));
	_is.read((char*)&m_secondaryFlags.mux, sizeof(m_secondaryFlags.mux));
}
