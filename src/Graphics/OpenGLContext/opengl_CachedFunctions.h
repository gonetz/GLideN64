#pragma once
#include <array>
#include <unordered_map>
#include <Graphics/ObjectHandle.h>
#include <Graphics/Parameter.h>
#include "opengl_GLInfo.h"
#include "opengl_Attributes.h"

namespace opengl {

#define CACHED_USE_CACHE1
#define CACHED_USE_CACHE2
#define CACHED_USE_CACHE4

	template<class T>
	class Cached1
	{
	public:
		bool update(T _param)
		{
#ifdef CACHED_USE_CACHE1
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

	template<class T1, class T2>
	class Cached2
	{
	public:
		bool update(T1 _p1, T2 _p2)
		{
#ifdef CACHED_USE_CACHE2
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
		T1 m_p1;
		T2 m_p2;
	};

	class Cached4
	{
	public:
		bool update(graphics::Parameter _p1,
			graphics::Parameter _p2,
			graphics::Parameter _p3,
			graphics::Parameter _p4)
		{
#ifdef CACHED_USE_CACHE4
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

	class CachedEnable : public Cached1<graphics::Parameter>
	{
	public:
		CachedEnable(graphics::Parameter _parameter);

		void enable(bool _enable);

		u32 get();

	private:
		const graphics::Parameter m_parameter;
	};


	class CachedBindFramebuffer : public Cached2<graphics::Parameter, graphics::ObjectHandle>
	{
	public:
		void bind(graphics::Parameter _target, graphics::ObjectHandle _name);
	};

	class CachedBindRenderbuffer : public Cached2<graphics::Parameter, graphics::ObjectHandle>
	{
	public:
		void bind(graphics::Parameter _target, graphics::ObjectHandle _name);
	};

	class CachedBindBuffer : public Cached2<graphics::Parameter, graphics::ObjectHandle>
	{
	public:
		void bind(graphics::Parameter _target, graphics::ObjectHandle _name);
	};

	class CachedBindTexture : public Cached2<graphics::Parameter, graphics::ObjectHandle>
	{
	public:
		void bind(graphics::Parameter _tmuIndex, graphics::Parameter _target, graphics::ObjectHandle _name);
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

	class CachedBlending : public Cached2<graphics::Parameter, graphics::Parameter>
	{
	public:
		void setBlending(graphics::Parameter _sfactor, graphics::Parameter _dfactor);
	};

	class CachedBlendingSeparate : public Cached4 //<graphics::Parameter, graphics::Parameter, graphics::Parameter, graphics::Parameter>
	{
	public:
		void setBlendingSeparate(graphics::Parameter _sfactorcolor, graphics::Parameter _dfactorcolor, graphics::Parameter _sfactoralpha, graphics::Parameter _dfactoralpha);
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
		CachedTextureUnpackAlignment() { m_cached = -1; }
		void setTextureUnpackAlignment(s32 _param);
	};

	struct texture_params {
		GLint magFilter;
		GLint minFilter;
		GLint wrapS;
		GLint wrapT;
		GLint maxMipmapLevel;
		GLfloat maxAnisotropy;
	};

	typedef std::unordered_map<u32, texture_params> TextureParams;

	/*---------------CachedFunctions-------------*/

	class CachedFunctions
	{
	public:
		CachedFunctions(const GLInfo & _glinfo);
		~CachedFunctions();

		void reset();

		CachedEnable * getCachedEnable(graphics::Parameter _parameter);

		CachedBindTexture * getCachedBindTexture();

		CachedBindFramebuffer * getCachedBindFramebuffer();

		CachedBindRenderbuffer * getCachedBindRenderbuffer();

		CachedBindBuffer * getCachedBindBuffer();

		CachedCullFace * getCachedCullFace();

		CachedDepthMask * getCachedDepthMask();

		CachedDepthCompare * getCachedDepthCompare();

		CachedViewport * getCachedViewport();

		CachedScissor * getCachedScissor();

		CachedBlending * getCachedBlending();
		
		CachedBlendingSeparate * getCachedBlendingSeparate();

		CachedBlendColor * getCachedBlendColor();

		CachedClearColor * getCachedClearColor();

		CachedVertexAttribArray * getCachedVertexAttribArray();

		CachedUseProgram * getCachedUseProgram();

		CachedTextureUnpackAlignment * getCachedTextureUnpackAlignment();

		TextureParams * getTexParams();

	private:
		typedef std::unordered_map<u32, CachedEnable> EnableParameters;

		TextureParams m_texparams;
		EnableParameters m_enables;
		CachedBindTexture m_bindTexture;
		CachedBindFramebuffer m_bindFramebuffer;
		CachedBindRenderbuffer m_bindRenderbuffer;
		CachedBindBuffer m_bindBuffer;
		CachedCullFace m_cullFace;
		CachedDepthMask m_depthMask;
		CachedDepthCompare m_depthCompare;
		CachedViewport m_viewport;
		CachedScissor m_scissor;
		CachedBlending m_blending;
		CachedBlendingSeparate m_blendingseparate;
		CachedBlendColor m_blendColor;
		CachedClearColor m_clearColor;
		CachedVertexAttribArray m_attribArray;
		CachedUseProgram m_useProgram;
		CachedTextureUnpackAlignment m_unpackAlignment;
	};

}
