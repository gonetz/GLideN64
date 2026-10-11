#ifndef POST_PROCESSOR_H
#define POST_PROCESSOR_H

#include <functional>
#include <list>
#include <memory>
#include "Types.h"
#include "Textures.h"
#include "Graphics/ObjectHandle.h"

namespace graphics {
	class ShaderProgram;
}

struct FrameBuffer;

class PostProcessor {
public:
	void init();
	void destroy();

	using PostprocessingFunc = std::function<FrameBuffer*(PostProcessor&, FrameBuffer*)>;
	using PostprocessingList = std::list<PostprocessingFunc>;
	const PostprocessingList & getPostprocessingList() const;

	static PostProcessor & get();

	// Debug feature: draw content of the coverage buffer of _pBuffer as gray scale image,
	// like the G_RM_VISCVG render mode does on hardware. Returns the buffer with the coverage
	// image, or _pBuffer when coverage can't be displayed. The source buffer is not modified.
	FrameBuffer * doCoverageDisplay(FrameBuffer * _pBuffer);

private:
	PostProcessor();
	PostProcessor(const PostProcessor & _other) = delete;

	FrameBuffer * _doGammaCorrection(FrameBuffer * _pBuffer);
	FrameBuffer * _doFXAA(FrameBuffer * _pBuffer);

	void _createBuffer(const FrameBuffer * _pMainBuffer, std::unique_ptr<FrameBuffer> & _pBuffer);
	void _createResultBuffer(const FrameBuffer * _pMainBuffer);
	void _preDraw(FrameBuffer * _pBuffer);
	void _postDraw();
	FrameBuffer * _doPostProcessing(FrameBuffer * _pBuffer, graphics::ShaderProgram * _pShader);

	std::unique_ptr<graphics::ShaderProgram> m_gammaCorrectionProgram;
	std::unique_ptr<graphics::ShaderProgram> m_FXAAProgram;
	std::unique_ptr<graphics::ShaderProgram> m_coverageDisplayProgram;
	std::unique_ptr<FrameBuffer> m_pResultBuffer;
	std::unique_ptr<FrameBuffer> m_pCoverageBuffer;
	CachedTexture * m_pTextureOriginal;
	PostprocessingList m_postprocessingList;
};

#endif // POST_PROCESSOR_H
