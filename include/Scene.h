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
    unsigned long getCount();
};

class Scene {
private:
    std::vector<MeshInstances> mesh_instances;
    Camera camera;
    GLint instanceVBO_location;
    static std::vector<std::shared_ptr<Scene>> scene_list;
    float bounds[4];
public:
    static std::shared_ptr<Scene> & createScene(GLint instance_location, GLint VP_location);
    Scene(GLint instance_location, GLint VP_location);
    unsigned int addMesh(std::shared_ptr<Mesh> const & m);
    MeshInstances & getMesh(unsigned int i);
    void addMeshInstance(unsigned int index, InstanceCoordinates);
    void moveOrAddInstances(unsigned int index, std::vector<Disk> const & disks);
    void draw();
    void drawAsDisks(GLint offset_scale_rot_location, GLint color_location, GLint bounds_location);
    unsigned long getInstanceCount(unsigned int index);

    void setBounds(float min_x, float min_y, float max_x, float max_y);
    Camera &getCamera();
};



#endif
