#include <Combiner.h>
#include "glsl_CombinerProgramImpl.h"

using namespace glsl;

CombinerProgramImpl::CombinerProgramImpl(GLuint _program, const CombinerInputs & _inputs, UniformGroups && _uniforms)
: m_bNeedUpdate(true)
, m_program(_program)
, m_inputs(_inputs)
, m_uniforms(std::move(_uniforms))
{
}


CombinerProgramImpl::~CombinerProgramImpl()
{
}

void CombinerProgramImpl::activate()
{
	glUseProgram(m_program);
}

void CombinerProgramImpl::update(bool _force)
{
	_force |= m_bNeedUpdate;
	m_bNeedUpdate = false;
	glUseProgram(m_program);
	for (auto it = m_uniforms.begin(); it != m_uniforms.end(); ++it)
		(*it)->update(_force);
}

CombinerKey CombinerProgramImpl::getKey() const
{
	return CombinerKey();
}

bool CombinerProgramImpl::usesTexture() const
{
	return m_inputs.usesTexture();
}

bool CombinerProgramImpl::usesTile(u32 _t) const {
	return m_inputs.usesTile(_t);
}

bool CombinerProgramImpl::usesShade() const {
	return m_inputs.usesShade();
}

bool CombinerProgramImpl::usesLOD() const {
	return m_inputs.usesLOD();
}

namespace graphics {

	// TODO implement
	std::ostream & operator<< (std::ostream & _os, const CombinerProgram & _combiner)
	{
		return _os;
	}

	std::istream & operator>> (std::istream & _is, CombinerProgram & _combiner)
	{
		return _is;
	}

}