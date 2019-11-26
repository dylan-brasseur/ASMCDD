//
// Created by Dylab Brasseur on 01/10/19.
//

#ifndef UTILS_H
#define UTILS_H

#include <GL/glew.h>
#include <iostream>
#include <cstring>
#include <memory>

const char* getErrorName(GLenum err);
const char* getShaderName(GLenum type);

#define __FILENAME__ (std::strrchr(__FILE__, '/') ? std::strrchr(__FILE__, '/') + 1 : __FILE__)

#define TEST_OPENGL_ERROR()                                                             \
  do {		  							\
    GLenum err = glGetError(); 					                        \
    if (err != GL_NO_ERROR) std::cerr << "OpenGL ERROR! " << __FILENAME__ << ' ' <<__LINE__ << " : "<< getErrorName(err) << std::endl;      \
  } while(0)

std::string load(const std::string &filename);

class implementation_error : public std::logic_error
{
public:
    implementation_error() : std::logic_error("Function not yet implemented") { };
};

#endif //UTILS_H
