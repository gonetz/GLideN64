#pragma once
#include <unordered_map>
#include <Graphics/ObjectHandle.h>
#include <Graphics/Parameter.h>

namespace opengl {

	class CachedEnable
	{
	public:
		CachedEnable();
		CachedEnable(graphics::Parameter _parameter);

		void reset();

		void enable(bool _enable);

	private:
		const graphics::Parameter m_parameter;
		bool m_enabled;
	};


	template<typename Bind>
	class CachedBind
	{
	public:
		CachedBind(Bind * _bind) : m_bind(_bind), m_name(0U) {}

		void bind(graphics::Parameter _target, graphics::ObjectHandle _name) {
			// TODO make cacheble
			m_bind(GLenum(_target), GLuint(_name));
		}

	private:
		graphics::ObjectHandle m_name;
		Bind * m_bind;
	};

	class CachedBindTexture
	{
	public:
		CachedBindTexture();

		void reset();

		void bind(graphics::Parameter _target, graphics::ObjectHandle _name);

	private:
		graphics::ObjectHandle m_name;
	};

	class CachedActiveTexture
	{
	public:
		CachedActiveTexture();

		void reset();

		void setActiveTexture(u32 _index);

	private:
		static const u32 m_invalidIndex;
		u32 m_index;
	};

	class CachedFunctions
	{
	public:
		CachedFunctions();
		~CachedFunctions();

		void reset();

		CachedEnable * getCachedEnable(graphics::Parameter _parameter);

		CachedBindTexture * getCachedBindTexture();

		CachedActiveTexture * geCachedActiveTexture();

	private:
		typedef std::unordered_map<u32, CachedEnable> EnableParameters;

		EnableParameters m_enables;
		CachedBindTexture m_bindTexture;
		CachedActiveTexture m_activeTexture;
	};

}
