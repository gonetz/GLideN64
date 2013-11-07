#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

class GLSLCombiner : public OGLCombiner {
public:
	GLSLCombiner(Combiner *_color, Combiner *_alpha);
	virtual ~GLSLCombiner();
	virtual void Set();
	virtual void UpdateColors();
	virtual void UpdateFBInfo();
	virtual void UpdateDepthInfo();
	virtual void UpdateAlphaTestInfo();

private:
	GLuint m_aShaders[8];
	GLuint m_program;
	int m_nInputs;
};

void InitGLSLCombiner();
void DestroyGLSLCombiner();
void GLSL_CalcLOD();
void GLSL_PostCalcLOD();

#endif //GLSL_COMBINER_H
