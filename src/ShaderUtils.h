#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "OpenGL.h"
#include "Combiner.h"

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment);
bool checkShaderCompileStatus(GLuint obj);
bool checkProgramLinkStatus(GLuint obj);
void logErrorShader(GLenum _shaderType, const std::string & _strShader);
int compileCombiner(const gDPCombine & _combine, Combiner & _color, Combiner & _alpha, std::string & _strShader);

bool needClampColor();
bool combinedColorC(const gDPCombine & _combine);
bool combinedAlphaC(const gDPCombine & _combine);
bool combinedColorABD(const gDPCombine & _combine);
bool combinedAlphaABD(const gDPCombine & _combine);

extern const char* fragment_shader_header_clamp;
extern const char* fragment_shader_header_sign_extend_color_c;
extern const char* fragment_shader_header_sign_extend_alpha_c;
extern const char* fragment_shader_header_sign_extend_color_abd;
extern const char* fragment_shader_header_sign_extend_alpha_abd;
extern const char* fragment_shader_clamp;
extern const char* fragment_shader_sign_extend_color_c;
extern const char* fragment_shader_sign_extend_alpha_c;
extern const char* fragment_shader_sign_extend_color_abd;
extern const char* fragment_shader_sign_extend_alpha_abd;


#endif // SHADER_UTILS_H
