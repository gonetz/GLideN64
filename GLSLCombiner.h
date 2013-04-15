#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

class GLSLCombiner : public OGLCombiner {
public:
	GLSLCombiner(Combiner *_color, Combiner *_alpha);
	virtual void Set();
	virtual void UpdateColors();

private:
	GLhandleARB m_vertexShaderObject;
	GLhandleARB m_fragmentShaderObject;
	GLhandleARB m_programObject;
	int m_nInputs;
};

void InitGLSLCombiner();

#endif //GLSL_COMBINER_H
