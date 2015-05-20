#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "OpenGL.h"
#include "Combiner.h"

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment);
bool checkShaderCompileStatus(GLuint obj);
bool checkProgramLinkStatus(GLuint obj);
int compileCombiner(Combiner & _color, Combiner & _alpha, char * _strShader);

#endif // SHADER_UTILS_H
