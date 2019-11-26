//
// Created by "Dylan Brasseur" on 14/11/2019.
//

#include "../include/Camera.h"
#include "../include/utils.h"

Camera::Camera(GLint _VP_location){
    VP_location = _VP_location;
    V = IDENTITY;
    P = IDENTITY;
    VP = IDENTITY;
    pos = {0,0,0};
    lookat = {0,0,0};
    aspect = 1;
    fov = 75;
    near = 0.1;
    far = 1000;
    P_good = V_good = VP_good = false;
}

//PROJECTTION

void Camera::setAspect(float _aspect){
    aspect = _aspect;
    P_good = false;
    VP_good = false;
}

void Camera::setDepthLimits(float _near, float _far){
    near = _near;
    far = _far;
    P_good = false;
    VP_good = false;

}


void Camera::setFOV(float _fov){
    fov= _fov;
    P_good = false;
    VP_good = false;
}

void Camera::computeProjection(){
    P = glm::perspective(glm::radians(fov), aspect, near, far);
    P_good = true;
}

//VIEW

void Camera::lookAt(float x, float y, float z){
    lookat = {x,y,z};
    V_good = false;
    VP_good = false;
}

void Camera::setPosition(float x, float y, float z){
    pos = {x,y,z};
    V_good = false;
    VP_good = false;
}

void Camera::setUp(float x, float y, float z){
    up = {x,y,z};
    V_good = false;
    VP_good = false;
}

void Camera::computeView(){
    V = glm::lookAt(pos, lookat, up);
    V_good = true;
}

//VP
void Camera::computeVP(){
    if(!P_good) computeProjection();
    if(!V_good) computeView();
    VP = P*V;
    VP_good=true;
}

float *Camera::getVP(){
    if(!VP_good) computeVP();
    return &(VP[0][0]);
}

float *Camera::getP(){
    if(!P_good) computeProjection();
    return &P[0][0];
}

float *Camera::getV(){
    if(!V_good) computeView();
    return &V[0][0];
}

void Camera::apply(){
    if(!VP_good) computeVP();
    glUniformMatrix4fv(VP_location, 1, GL_FALSE, getVP());TEST_OPENGL_ERROR();
}

void Camera::push_state(){
    CameraState state{V, P, VP,
    pos, lookat, up,
    aspect, fov, near, far,
    P_good, V_good, VP_good,
    VP_location};
    states.push_back(state);
}

void Camera::pop_state(){
    if(!states.empty())
    {
        auto & s = states.back();
        far = s.far;
        P_good = s.P_good;
        P = s.P;
        VP_location = s.VP_location;
        V_good = s.V_good;
        V = s.V;
        lookat= s.lookat;
        aspect= s.aspect;
        pos = s.pos;
        fov = s.fov;
        near = s.near;
        up = s.up;
        VP_good = s.VP_good;
        VP = s.VP;
        states.pop_back();
    }
}







