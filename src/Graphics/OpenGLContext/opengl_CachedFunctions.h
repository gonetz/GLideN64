#pragma once
#include <array>
#include <unordered_map>
#include <Graphics/ObjectHandle.h>
#include <Graphics/Parameter.h>
#include "opengl_GLInfo.h"
#include "opengl_Attributes.h"

namespace opengl {

//#define CACHED_USE_CACHE

	template<class T>
	class Cached1
	{
	public:
		bool update(T _param)
		{
#ifdef CACHED_USE_CACHE
			if (_param == m_cached)
				return false;
#endif
			m_cached = _param;
			return true;
		}

		void reset()
		{
			m_cached.reset();
		}

	protected:
		T m_cached;
	};

	class CachedEnable : public Cached1<graphics::Parameter>
	{
	public:
		CachedEnable(graphics::Parameter _parameter);

		void enable(bool _enable);

	private:
		const graphics::Parameter m_parameter;
	};


	template<typename Bind>
	class CachedBind : public Cached1<graphics::ObjectHandle>
	{
	public:
		CachedBind(Bind _bind) : m_bind(_bind) {}

		void bind(graphics::Parameter _target, graphics::ObjectHandle _name) {
			if (update(_name))
				m_bind(GLenum(_target), GLuint(_name));
		}

	private:
		Bind m_bind;
	};

	typedef CachedBind<decltype(glBindFramebuffer)> CachedBindFramebuffer;

	typedef CachedBind<decltype(glBindRenderbuffer)> CachedBindRenderbuffer;

	typedef CachedBind<decltype(glBindBuffer)> CachedBindBuffer;

	class CachedBindTexture : public Cached1<graphics::ObjectHandle>
	{
	public:
		void bind(graphics::Parameter _target, graphics::ObjectHandle _name);
	};

	class CachedActiveTexture : public Cached1<graphics::Parameter>
	{
	public:
		void setActiveTexture(graphics::Parameter _index);
	};

	class CachedCullFace : public Cached1<graphics::Parameter>
	{
	public:
		void setCullFace(graphics::Parameter _mode);
	};

	class CachedDepthMask : public Cached1<graphics::Parameter>
	{
	public:
		void setDepthMask(bool _enable);
	};

	class CachedDepthCompare : public Cached1<graphics::Parameter>
	{
	public:
		void setDepthCompare(graphics::Parameter m_mode);
	};

	class Cached2
	{
	public:
		bool update(graphics::Parameter _p1,
			graphics::Parameter _p2)
		{
#ifdef CACHED_USE_CACHE
			if (_p1 == m_p1 &&
				_p2 == m_p2)
				return false;
#endif
			m_p1 = _p1;
			m_p2 = _p2;
			return true;
		}

		void reset()
		{
			m_p1.reset();
			m_p2.reset();
		}

	protected:
		graphics::Parameter m_p1, m_p2;
	};

	class Cached4
	{
	public:
		bool update(graphics::Parameter _p1,
			graphics::Parameter _p2,
			graphics::Parameter _p3,
			graphics::Parameter _p4)
		{
#ifdef CACHED_USE_CACHE
			if (_p1 == m_p1 &&
				_p2 == m_p2 &&
				_p3 == m_p3 &&
				_p4 == m_p4)
			return false;
#endif
			m_p1 = _p1;
			m_p2 = _p2;
			m_p3 = _p3;
			m_p4 = _p4;
			return true;
		}

		void reset()
		{
			m_p1.reset();
			m_p2.reset();
			m_p3.reset();
			m_p4.reset();
		}

	protected:
		graphics::Parameter m_p1, m_p2, m_p3, m_p4;
	};

	class CachedViewport : public Cached4
	{
	public:
		void setViewport(s32 _x, s32 _y, s32 _width, s32 _height);
	};

	class CachedScissor : public Cached4
	{
	public:
		void setScissor(s32 _x, s32 _y, s32 _width, s32 _height);
	};

	class CachedBlending : public Cached2
	{
	public:
		void setBlending(graphics::Parameter _sfactor, graphics::Parameter _dfactor);
	};

	class CachedBlendColor : public Cached4
	{
	public:
		void setBlendColor(f32 _red, f32 _green, f32 _blue, f32 _alpha);
	};

	class CachedClearColor : public Cached4
	{
	public:
		void setClearColor(f32 _red, f32 _green, f32 _blue, f32 _alpha);
	};

	class CachedVertexAttribArray {
	public:
		void enableVertexAttribArray(u32 _index, bool _enable);
		void reset();

	private:
		std::array<graphics::Parameter, MaxAttribIndex> m_attribs;
	};

	class CachedUseProgram : public Cached1<graphics::ObjectHandle>
	{
	public:
		void useProgram(graphics::ObjectHandle _program);
	};

	class CachedTextureUnpackAlignment : public Cached1<s32>
	{
	public:
		void setTextureUnpackAlignment(s32 _param);
	};

	/*---------------CachedFunctions-------------*/

	class CachedFunctions
	{
	public:
		CachedFunctions(const GLInfo & _glinfo);
		~CachedFunctions();

		void reset();

		CachedEnable * getCachedEnable(graphics::Parameter _parameter);

		CachedBindTexture * getCachedBindTexture();

		CachedActiveTexture * getCachedActiveTexture();

		CachedBindFramebuffer * getCachedBindFramebuffer();

		CachedBindRenderbuffer * getCachedBindRenderbuffer();

		CachedBindBuffer * getCachedBindBuffer();

		CachedCullFace * getCachedCullFace();

		CachedDepthMask * getCachedDepthMask();

		CachedDepthCompare * getCachedDepthCompare();

		CachedViewport * getCachedViewport();

		CachedScissor * getCachedScissor();

		CachedBlending * getCachedBlending();

		CachedBlendColor * getCachedBlendColor();

		CachedClearColor * getCachedClearColor();

		CachedVertexAttribArray * getCachedVertexAttribArray();

		CachedUseProgram * getCachedUseProgram();

		CachedTextureUnpackAlignment * getCachedTextureUnpackAlignment();

	private:
		typedef std::unordered_map<u32, CachedEnable> EnableParameters;

		EnableParameters m_enables;
		CachedBindTexture m_bindTexture;
		CachedActiveTexture m_activeTexture;
		CachedBindFramebuffer m_bindFramebuffer;
		CachedBindRenderbuffer m_bindRenderbuffer;
		CachedBindBuffer m_bindBuffer;
		CachedCullFace m_cullFace;
		CachedDepthMask m_depthMask;
		CachedDepthCompare m_depthCompare;
		CachedViewport m_viewport;
		CachedScissor m_scissor;
		CachedBlending m_blending;
		CachedBlendColor m_blendColor;
		CachedClearColor m_clearColor;
		CachedVertexAttribArray m_attribArray;
		CachedUseProgram m_useProgram;
		CachedTextureUnpackAlignment m_unpackAlignment;
	};

}
