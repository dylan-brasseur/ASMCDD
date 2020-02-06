//
// Created by Dylab Brasseur on 01/10/19.
//

#ifndef UTILS_H
#define UTILS_H

#include <GL/glew.h>
#include <iostream>
#include <cstring>
#include <memory>
#include <vector>
#include <cmath>

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

struct Disk{
    Disk(float _x, float _y, float _r): x(_x), y(_y), r(_r){};
    float x, y, r;
};

template<typename T>
T clip(T a, T min, T max)
{
    return std::min(std::max(a, min), max);
}

struct Target_pcf_type{
    Target_pcf_type(float _mean, float _min, float _max): mean(_mean), min(_min), max(_max){};
    float mean;
    float min;
    float max;
    float radius=0;
};

struct Compute_status{
    float rmax;
    std::vector<Disk> disks;
    std::vector<unsigned long> parents;
};

struct ASMCDD_params{
    float step = 0.1;
    float sigma = 0.25;
    float limit = 5;
    float domainLength = 1;
    unsigned long max_iter = 2000;
    float threshold = 0.001;
    float error_delta=0.0001;
    bool distanceThreshold = true;
    std::string example_filename;
};

struct Contribution{
    std::vector<float> weights;
    std::vector<float> contribution;
    std::vector<float> pcf;
};

struct Compare{
    float val;
    unsigned long i;
    unsigned long j;
};

inline float euclidian(Disk const & a, Disk const & b)
{
    return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

inline float gaussian_kernel(float sigma, float x)
{
    static const float sqrtpi = std::sqrt(M_PI);
    return std::exp(-((x*x)/(sigma*sigma)))/(sqrtpi*sigma);
}

inline float computeRmax(unsigned long n)
{
    return float(2.0*std::sqrt(1/(2*std::sqrt(3.0)*double(n))));
}

float perimeter_weight(double x, double y, double r);
float perimeter_weight(float x, float y, float r, float diskfact);
float diskDistance(Disk const & a, Disk const & b, float rmax);

#endif //UTILS_H
