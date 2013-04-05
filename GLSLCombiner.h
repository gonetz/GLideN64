#ifndef GLSL_COMBINER_H
#define GLSL_COMBINER_H

struct GLSLCombiner {
  GLhandleARB vertex_shader_object;
  GLhandleARB fragment_shader_object;
  GLhandleARB program_object;
};

void InitGLSLCombiner();
GLSLCombiner * CompileGLSLCominer(Combiner *_color, Combiner *_alpha);
void SetGLSLCombiner(GLSLCombiner *_Combiner);
void UpdateGLSLCombinerColors(GLSLCombiner*);

#endif //GLSL_COMBINER_H
