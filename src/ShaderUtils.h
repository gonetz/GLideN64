#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "OpenGL.h"
#include "Combiner.h"

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment);
bool checkShaderCompileStatus(GLuint obj);
bool checkProgramLinkStatus(GLuint obj);
void logErrorShader(GLenum _shaderType, const std::string & _strShader);
int compileCombiner(const gDPCombine & _combine, Combiner & _color, Combiner & _alpha, std::string & _strShader);

#endif // SHADER_UTILS_H
