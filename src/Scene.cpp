//
// Created by "Dylan Brasseur" on 11/11/2019.
//

#include "../include/Scene.h"

#include <utility>

std::vector<std::shared_ptr<Scene>> Scene::scene_list;

MeshInstances::MeshInstances(std::shared_ptr<Mesh> m){
    base = std::move(m);
    instancesAmountChanged = true;
    glGenBuffers(1, &instanceVBO_id);TEST_OPENGL_ERROR();
    glGenVertexArrays(1, &instance_VAO_id);TEST_OPENGL_ERROR();
    color_code[0] = 0;
    color_code[1] = 0;
    color_code[2] = 0;

}

void MeshInstances::addInstance(InstanceCoordinates const & instance)
{
    instances.push_back(instance);
    instancesAmountChanged=true;
}

void MeshInstances::modifyInstance(unsigned int index, InstanceCoordinates const & instance){
    instances[index] = instance;
    changed_indices.push_back(index);
}
void MeshInstances::draw(GLint instanceVBO_location){
    if(!instances.empty())
    {
        glBindVertexArray(base->vertexArray_id);TEST_OPENGL_ERROR();
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_id);TEST_OPENGL_ERROR();

        if(instancesAmountChanged){
            //Reallocate
            glBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(InstanceCoordinates),instances.data(), GL_DYNAMIC_DRAW);TEST_OPENGL_ERROR();
            instancesAmountChanged=false;
            changed_indices.clear(); //We copied the whole buffer anyway
        }else if(!changed_indices.empty())
        {
            //Modify content
            for(unsigned int i : changed_indices)
            {
                glBufferSubData(GL_ARRAY_BUFFER, i*sizeof(InstanceCoordinates),sizeof(InstanceCoordinates), &(instances[i]));TEST_OPENGL_ERROR();
            }
            changed_indices.clear();
        }
        glVertexAttribDivisor(instanceVBO_location, 1);TEST_OPENGL_ERROR();
        glVertexAttribPointer(instanceVBO_location, 4, GL_FLOAT, GL_FALSE, 0, nullptr);TEST_OPENGL_ERROR();
        glEnableVertexAttribArray(instanceVBO_location);TEST_OPENGL_ERROR();
        glDrawElementsInstanced(GL_TRIANGLES, base->triangleArray.size(), GL_UNSIGNED_INT, base->triangleArray.data(), instances.size());TEST_OPENGL_ERROR();
    }
    glBindVertexArray(0);
}

InstanceCoordinates &MeshInstances::getInstance(unsigned int index){
    return instances.at(index);
}

void MeshInstances::setColor(float r, float g, float b){
    color_code[0] = r;
    color_code[1] = g;
    color_code[2] = b;
}

void MeshInstances::drawAsDisks(GLint offset_scale_rot_location, GLint color_uniform_location){
    if(!instances.empty())
    {
        glUniform3fv(color_uniform_location, 1, color_code);TEST_OPENGL_ERROR();
        glBindVertexArray(instance_VAO_id);TEST_OPENGL_ERROR();
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_id);TEST_OPENGL_ERROR();

        if(instancesAmountChanged){
            //Reallocate
            glBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(InstanceCoordinates),instances.data(), GL_DYNAMIC_DRAW);TEST_OPENGL_ERROR();
            instancesAmountChanged=false;
            changed_indices.clear(); //We copied the whole buffer anyway
        }else if(!changed_indices.empty())
        {
            //Modify content
            for(unsigned int i : changed_indices)
            {
                glBufferSubData(GL_ARRAY_BUFFER, i*sizeof(InstanceCoordinates),sizeof(InstanceCoordinates), &(instances[i]));TEST_OPENGL_ERROR();
            }
            changed_indices.clear();
        }
        glVertexAttribPointer(offset_scale_rot_location, 4, GL_FLOAT, GL_FALSE, 0, nullptr);TEST_OPENGL_ERROR();
        glEnableVertexAttribArray(offset_scale_rot_location);TEST_OPENGL_ERROR();
        glDrawArrays(GL_POINTS, 0, instances.size());TEST_OPENGL_ERROR();
    }
    glBindVertexArray(0);
}

unsigned int Scene::addMesh(std::shared_ptr<Mesh> const & m) {
    mesh_instances.emplace_back(m);
    return mesh_instances.size()-1;
}

MeshInstances & Scene::getMesh(unsigned int i)
{
    return mesh_instances[i];
}

void Scene::draw() {
    //Appliquer la caméra
    camera.apply();
    // iterer sur l'ensemble des objets, et faire leur rendu.
    for(auto & mi : mesh_instances) {
        mi.draw(instanceVBO_location);
    }
}

Camera &Scene::getCamera(){
    return camera;
}

void Scene::drawAsDisks(GLint offset_scale_rot_location, GLint color_location, GLint bounds_location){
    glUniform4fv(bounds_location, 1, bounds);TEST_OPENGL_ERROR();
    for(auto & mi : mesh_instances)    {
        mi.drawAsDisks(offset_scale_rot_location, color_location);
    }
}

std::shared_ptr<Scene> &Scene::createScene(GLint instance_location, GLint VP_location){
    scene_list.push_back(std::make_shared<Scene>(instance_location, VP_location));
    return scene_list.back();
}

Scene::Scene(GLint instance_location, GLint VP_location) : camera(VP_location), instanceVBO_location(instance_location), bounds{-1, -1, 1, 1}{
}

void Scene::setBounds(float min_x, float min_y, float max_x, float max_y){
    bounds[0] = min_x;
    bounds[1] = min_y;
    bounds[2] = max_x;
    bounds[3] = max_y;
}

void Scene::addMeshInstance(unsigned int index, InstanceCoordinates ic){
    mesh_instances.at(index).addInstance(ic);
}
