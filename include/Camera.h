//
// Created by "Dylan Brasseur" on 14/11/2019.
//

#ifndef DISKSPROJECT_CAMERA_H
#define DISKSPROJECT_CAMERA_H
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <vector>

#define IDENTITY {1, 0, 0, 0, 0, 1, 0 ,0, 0, 0, 1, 0, 0, 0, 0, 1}
#define NULLVECTOR {0,0,0}

struct CameraState{
    glm::mat4 V, P, VP;
    glm::vec3 pos, lookat, up;
    float aspect, fov, near, far;
    bool P_good, V_good, VP_good;
    GLint VP_location;
};

/**
 * This class is used to ease the computation of camera matrices
 */
class Camera{
public:
    explicit Camera(GLint VP_location);
    void setAspect(float aspect);
    void setDepthLimits(float near, float far);
    void setFOV(float fov);
    void computeProjection();

    void setPosition(float x, float y, float z);
    void lookAt(float x, float y, float z);
    void setUp(float x, float y, float z);
    void computeView();

    void computeVP();
    float* getVP();
    float* getP();
    float* getV();

    void push_state();
    void pop_state();

    void apply();
private:
    glm::mat4 V IDENTITY, P IDENTITY, VP IDENTITY;
    glm::vec3 pos NULLVECTOR, lookat NULLVECTOR, up NULLVECTOR;
    float aspect, fov, near, far;
    bool P_good, V_good, VP_good;
    GLint VP_location;
    std::vector<CameraState> states;
};



#endif //DISKSPROJECT_CAMERA_H
