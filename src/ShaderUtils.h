#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "OpenGL.h"

GLuint createShaderProgram(const char * _strVertex, const char * _strFragment);
bool checkShaderCompileStatus(GLuint obj);
bool checkProgramLinkStatus(GLuint obj);

#endif // SHADER_UTILS_H
