#pragma once
#include <unordered_map>
#include <Graphics/ObjectName.h>
#include <Graphics/Parameter.h>

namespace opengl {

	class CachedEnable {
	public:
		CachedEnable();
		CachedEnable(graphics::Parameter _parameter);
		void enable(bool _enable);

	private:
		const graphics::Parameter m_parameter;
		bool m_enabled;
	};


	template<typename Bind>
	class CachedBind {
	public:
		CachedBind(Bind * _bind) : m_bind(_bind), m_name(0U) {}

		void bind(graphics::Parameter _target, graphics::ObjectName _name) {
			// TODO make cacheble
			m_bind(GLenum(_target), GLuint(_name));
		}

	private:
		graphics::ObjectName m_name;
		Bind * m_bind;
	};

	class CachedBindTexture {
	public:
		CachedBindTexture();
		void bind(graphics::Parameter _target, graphics::ObjectName _name);

	private:
		graphics::ObjectName m_name;
	};


	class CachedFunctions
	{
	public:
		CachedFunctions();
		~CachedFunctions();
		CachedEnable * getCachedEnable(graphics::Parameter _parameter);
		CachedBindTexture * getCachedBindTexture();

	private:
		typedef std::unordered_map<u32, CachedEnable> EnableParameters;

		EnableParameters m_enables;
		CachedBindTexture m_bindTexture;
	};

}