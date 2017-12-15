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
				unsigned int cvgXAlpha : 1;
				unsigned int alphaCvgSel : 1;
				unsigned int alphaDither : 2;
				unsigned int colorDither : 2;
				unsigned int renderTarget : 2;
				unsigned int textureFilter : 2;
				unsigned int fbMonochromeMode0 : 3;
				unsigned int fbMonochromeMode1 : 3;
				unsigned int fbFixedAlpha0 : 1;
				unsigned int fbFixedAlpha1 : 1;
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
	void setCvgXAlpha(unsigned int _cvgXAlpha);
	void setAlphaCvgSel(unsigned int _alphaCvgSel);
	void setAlphaDither(unsigned int _alphaDither);
	void setColorDither(unsigned int _colorDither);
	void setRenderTarget(unsigned int _renderTarget);
	void setTextureFilter(unsigned int _textureFilter);
	void setFbMonochromeMode0(unsigned int _fbMonochromeMode0);
	void setFbMonochromeMode1(unsigned int _fbMonochromeMode1);
	void setFbFixedAlpha0(unsigned int _fbFixedAlpha0);
	void setFbFixedAlpha1(unsigned int _fbFixedAlpha1);

	unsigned int getBiLerp0(void) const;
	unsigned int getBiLerp1(void) const;
	unsigned int getEnableAlphaTest(void) const;
	unsigned int getAlphaCompareMode(void) const;
	unsigned int getCvgXAlpha(void) const;
	unsigned int getAlphaCvgSel(void) const;
	unsigned int getAlphaDither(void) const;
	unsigned int getColorDither(void) const;
	unsigned int getRenderTarget(void) const;
	unsigned int getTextureFilter(void) const;
	unsigned int getFbMonochromeMode0(void) const;
	unsigned int getFbMonochromeMode1(void) const;
	unsigned int getFbFixedAlpha0(void) const;
	unsigned int getFbFixedAlpha1(void) const;

	void read(std::istream & _is);

private:
	gDPCombine m_key;
	SecondaryShaderParams m_secondaryFlags;
};

struct CombinerKeyHash {
public:
	std::size_t operator()(const CombinerKey &x) const
	{
		return std::hash<u64>()(x.getMux()) ^ std::hash<u64>()(x.getSecondaryParams());
	}
};
