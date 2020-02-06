#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>
#include "utils.h"

struct float3{
    float a;
    float b;
    float c;
};

/**
 * Base vector class, from gmini
 */
class Vec3 {
private:
    float mVals[3]{};
public:
    Vec3() = default;
    Vec3( float x , float y , float z ) {
       mVals[0] = x; mVals[1] = y; mVals[2] = z;
    }
    explicit Vec3(float3 f){mVals[0] = f.a; mVals[1] = f.b; mVals[2] = f.c;}
    float & operator [] (unsigned int c) { return mVals[c]; }
    float operator [] (unsigned int c) const { return mVals[c]; }
    Vec3& operator = (Vec3 const & other) {
       mVals[0] = other[0] ; mVals[1] = other[1]; mVals[2] = other[2];
       return *this;
    }
    [[nodiscard]] float squareLength() const {
       return mVals[0]*mVals[0] + mVals[1]*mVals[1] + mVals[2]*mVals[2];
    }
    [[nodiscard]] float length() const { return std::sqrt( squareLength() ); }
    void normalize() { float L = length(); mVals[0] /= L; mVals[1] /= L; mVals[2] /= L; }
    static float dot( Vec3 const & a , Vec3 const & b ) {
       return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    }
    static Vec3 cross( Vec3 const & a , Vec3 const & b ) {
       return Vec3( a[1]*b[2] - a[2]*b[1] ,
                    a[2]*b[0] - a[0]*b[2] ,
                    a[0]*b[1] - a[1]*b[0] );
    }
    void operator += (Vec3 const & other) {
        mVals[0] += other[0];
        mVals[1] += other[1];
        mVals[2] += other[2];
    }
    void operator -= (Vec3 const & other) {
        mVals[0] -= other[0];
        mVals[1] -= other[1];
        mVals[2] -= other[2];
    }
    void operator *= (float s) {
        mVals[0] *= s;
        mVals[1] *= s;
        mVals[2] *= s;
    }
    void operator /= (float s) {
        mVals[0] /= s;
        mVals[1] /= s;
        mVals[2] /= s;
    }
    Vec3 operator *(Vec3 const & o) const{
        return {o[0]*mVals[0],o[1]*mVals[1], o[2]*mVals[2] };
    }
};

static inline Vec3 operator + (Vec3 const & a , Vec3 const & b) {
   return Vec3(a[0]+b[0] , a[1]+b[1] , a[2]+b[2]);
}
static inline Vec3 operator - (Vec3 const & a , Vec3 const & b) {
   return Vec3(a[0]-b[0] , a[1]-b[1] , a[2]-b[2]);
}
static inline Vec3 operator * (float a , Vec3 const & b) {
   return Vec3(a*b[0] , a*b[1] , a*b[2]);
}
static inline Vec3 operator / (Vec3 const &  a , float b) {
   return Vec3(a[0]/b , a[1]/b , a[2]/b);
}
static inline std::ostream & operator << (std::ostream & s , Vec3 const & p) {
    s << p[0] << " " << p[1] << " " << p[2];
    return s;
}
static inline std::istream & operator >> (std::istream & s , Vec3 & p) {
    s >> p[0] >> p[1] >> p[2];
    return s;
}

#endif
