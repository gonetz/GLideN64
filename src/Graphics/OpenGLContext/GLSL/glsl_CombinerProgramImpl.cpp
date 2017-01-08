#include <Combiner.h>
#include "glsl_CombinerProgramImpl.h"

using namespace glsl;

/*---------------CombinerInputs-------------*/

bool CombinerInputs::usesTile(u32 _t) const
{
	if (_t == 0)
		return (m_inputs & ((1 << TEXEL0) | (1 << TEXEL0_ALPHA))) != 0;
	return (m_inputs & ((1 << TEXEL1) | (1 << TEXEL1_ALPHA))) != 0;
}

bool CombinerInputs::usesTexture() const
{
	return (m_inputs & ((1 << TEXEL1) | (1 << TEXEL1_ALPHA) | (1 << TEXEL0) | (1 << TEXEL0_ALPHA))) != 0;
}

bool CombinerInputs::usesLOD() const
{
	return (m_inputs & (1 << LOD_FRACTION)) != 0;
}

bool CombinerInputs::usesShade() const
{
	return (m_inputs & ((1 << SHADE) | (1 << SHADE_ALPHA))) != 0;
}

bool CombinerInputs::usesShadeColor() const
{
	return (m_inputs & (1 << SHADE)) != 0;
}

bool CombinerInputs::usesHwLighting() const
{
	return (m_inputs & (1 << HW_LIGHT)) != 0;
}

void CombinerInputs::addInput(int _input)
{
	m_inputs |= 1 << _input;
}


/*---------------CombinerProgramImpl-------------*/


CombinerProgramImpl::CombinerProgramImpl()
{
}


CombinerProgramImpl::~CombinerProgramImpl()
{
}

void CombinerProgramImpl::activate()
{
}

void CombinerProgramImpl::update(bool _force)
{

}

CombinerKey CombinerProgramImpl::getKey() const
{
	return CombinerKey();
}
