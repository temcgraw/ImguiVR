#ifndef __INITSHADER_H__
#define __INITSHADER_H__

#include <windows.h>
#include <GL/GL.h>

GLuint InitShader( const char* computeShaderFile);
GLuint InitShader( const char* vertexShaderFile, const char* fragmentShaderFile );
GLuint InitShader( const char* vertexShaderFile, const char* geometryShader, const char* fragmentShaderFile );
GLuint InitShader(const char* vertexShaderFile, const char* tessControlShader, const char* tessEvalShader, const char* fragmentShaderFile);
GLuint InitShader(const char* vertexShaderFile, const char* tessControlShader, const char* tessEvalShader, const char* geometryShader, const char* fragmentShaderFile);

#endif