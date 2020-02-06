//
// Created by drake on 24/10/19.
//

#include "../include/utils.h"
#include "../include/Scene.h"
#include <fstream>
#include <memory>

std::string load(const std::string &filename) {
    std::ifstream input_src_file(filename, std::ios::in);
    std::string ligne;
    std::string file_content;
    if (input_src_file.fail()) {
        std::cerr << "FAIL : " << filename << std::endl;
        return "";
    }
    while(getline(input_src_file, ligne)) {
        file_content+=ligne + "\n";
    }
    input_src_file.close();
    return file_content;
}

static const char* errorMessage[] = {"GL_INVALID_ENUM", "GL_INVALID_VALUE", "GL_INVALID_OPERATION", "GL_STACK_OVERFLOW", "GL_STACK_UNDERFLOW", "GL_OUT_OF_MEMORY", "GL_INVALID_FRAMEBUFFER_OPERATION", "GL_CONTEXT_LOST"};


const char *getErrorName(GLenum err){
    const unsigned int n = err-GL_INVALID_ENUM;
    return (n >= 8) ? "UNKNOWN_ERROR" : errorMessage[n];
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

//Returns the proportion of the circle perimeter that is in the [0, 1] domain
float perimeter_weight(double x, double y, double r)
{
    //We assume the domain is [0, 1] on all sides
    //Assuming the center x, y is between [0, 1]

    double full_angle = 2*M_PI;
    const std::pair<double, double> deltas[] = {{x, y}, {1-x, y}, {y, x}, {1-y, x}};
    for(const auto & delta : deltas)
    {
        double dx = delta.first;
        if(dx < r)
        {
            double dy = delta.second;
            double alpha =std::acos(dx/r);
            full_angle-= std::min(alpha, std::atan2(dy, dx)) + std::min(alpha, std::atan2((1-dy), dx));
        }
    }
    return clip(full_angle/(2*M_PI), 0.0, 1.0);
}

float perimeter_weight(float x, float y, float r, float diskfact)
{
    return perimeter_weight(x*diskfact, y*diskfact, r*diskfact);
}

float diskDistance(Disk const & a, Disk const & b, float rmax)
{
    float r1, r2;
    if(a.r > b.r)
    {
        r1 = a.r;
        r2 = b.r;
    }else
    {
        r1 = b.r;
        r2 = a.r;
    }
    r1/=rmax;
    r2/=rmax;
    float d = euclidian(a,b)/rmax;
    float extent = std::max(d+r1+r2, 2*r1);
    float overlap = clip(r1+r2-d, 0.0f, 2*r2);
    float f = (extent-overlap+d+r1-r2);
    if(d <= r1-r2)
    {
        return f/(4*r1 - 4*r2);
    }else if(d <= r1+r2)
    {
        return (f - 4*r1 + 7*r2)/(3*r2);
    }else{
        return f - 4*r1 - 2*r2 + 3;
    }
}