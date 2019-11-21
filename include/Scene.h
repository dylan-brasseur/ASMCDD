#ifndef SCENE_H
#define SCENE_H

#include <utility>
#include <vector>
#include <string>
#include "Mesh.h"
#include "Camera.h"

#include <GL/glut.h>

struct InstanceCoordinates{
    //Coordinates
    float x;
    float y;
    //Scale
    float s;
    //Rotation
    float r;
};

class MeshInstances{
    std::shared_ptr<Mesh> base;
    std::vector<InstanceCoordinates> instances;
    std::vector<unsigned int> changed_indices;
    bool instancesAmountChanged;
    GLuint instanceVBO_id;
    float color_code[3];
    GLuint instance_VAO_id;
public:
    explicit MeshInstances(std::shared_ptr<Mesh>  m);
    void addInstance(InstanceCoordinates const & instance);
    void modifyInstance(unsigned int index, InstanceCoordinates const & instance);
    InstanceCoordinates & getInstance(unsigned int index);
    void draw(GLint instanceVBO_location);
    void drawAsDisks(GLint offset_scale_rot_location, GLint color_uniform_location);
    void setColor(float r, float g, float b);
};

class Scene {
private:
    std::vector<MeshInstances> mesh_instances;
    Camera camera;
    GLint instanceVBO_location;
public:
    explicit Scene(GLint instance_location, GLint VP_location):camera(VP_location), instanceVBO_location(instance_location){};
    MeshInstances & addMesh(std::shared_ptr<Mesh> const & m);
    MeshInstances & getMesh(unsigned int i);
    void draw();
    void drawAsDisks(GLint offset_scale_rot_location, GLint color_location, GLint bounds_location, float* bounds);

    Camera &getCamera();
};



#endif
