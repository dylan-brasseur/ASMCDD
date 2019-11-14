//
// Created by drake on 24/10/19.
//

#include "include/utils.h"
#include <fstream>

std::string load(const std::string &filename) {
    std::ifstream input_src_file(filename, std::ios::in);
    std::string ligne;
    std::string file_content;
    if (input_src_file.fail()) {
        std::cerr << "FAIL\n";
        return "";
    }
    while(getline(input_src_file, ligne)) {
        file_content+=ligne + "\n";
    }
    //file_content += '\0';
    input_src_file.close();
    return file_content;
}

static const char* errorMessage[] = {"GL_INVALID_ENUM", "GL_INVALID_VALUE", "GL_INVALID_OPERATION", "GL_STACK_OVERFLOW", "GL_STACK_UNDERFLOW", "GL_OUT_OF_MEMORY", "GL_INVALID_FRAMEBUFFER_OPERATION", "GL_CONTEXT_LOST"};


const char *getErrorName(GLenum err){
    const int n = err-GL_INVALID_ENUM;
    return (n < 0 || n >= 8) ? "UNKNOWN_ERROR" : errorMessage[n];
}

const char *getShaderName(GLenum type){
    switch(type){
        case GL_VERTEX_SHADER:
            return "Vertex Shader";
        case GL_FRAGMENT_SHADER:
            return "Fragment Shader";
        case GL_TESS_CONTROL_SHADER:
            return "Tesselation control Shader";
        case GL_TESS_EVALUATION_SHADER:
            return "Tesselation evaluation Shader";
        case GL_GEOMETRY_SHADER:
            return "Geometry Shader";
        case GL_COMPUTE_SHADER:
            return "Compute Shader";
        default:
            return "Not a shader";
    }
}
