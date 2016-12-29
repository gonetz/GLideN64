#include "GLFunctions.h"
#include "opengl_CachedFunctions.h"

using namespace opengl;

/*---------------CachedEnable-------------*/

CachedEnable::CachedEnable(graphics::Parameter _parameter)
: m_parameter(_parameter)
, m_enabled(false)
{
}

CachedEnable::CachedEnable()
: m_parameter(0U)
, m_enabled(false)
{
}

void CachedEnable::reset()
{
	m_enabled = false;
}

void CachedEnable::enable(bool _enable)
{
	// TODO make cacheable
	if (_enable) {
		glEnable(GLenum(m_parameter));
	} else {
		glDisable(GLenum(m_parameter));
	}
}

/*---------------CachedBindTexture-------------*/

CachedBindTexture::CachedBindTexture()
: m_name(0U) {
}

void CachedBindTexture::reset()
{
	m_name = graphics::ObjectName(0U);
}

void CachedBindTexture::bind(graphics::Parameter _target, graphics::ObjectName _name)
{
	m_name = _name;
	// TODO make cacheable
	glBindTexture(GLenum(_target), GLuint(_name));
}


/*---------------CachedFunctions-------------*/

CachedFunctions::CachedFunctions()
{
}


CachedFunctions::~CachedFunctions()
{
}

void CachedFunctions::reset() {
	for (auto it : m_enables)
		it.second.reset();

	m_bindTexture.reset();
}


CachedEnable * CachedFunctions::getCachedEnable(graphics::Parameter _parameter)
{
	const u32 key(_parameter);
	auto it = m_enables.find(key);
	if (it == m_enables.end()) {
		auto res = m_enables.emplace(key, _parameter);
		if (res.second)
			return &(res.first->second);
		return nullptr;
	}
	return &(it->second);
}

CachedBindTexture * CachedFunctions::getCachedBindTexture()
{
	return &m_bindTexture;
}
