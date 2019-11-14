//
// Created by "Dylan Brasseur" on 11/11/2019.
//

#include "include/Scene.h"

MeshInstances::MeshInstances(std::shared_ptr<Mesh> m){
    base = std::move(m);
    instancesAmountChanged = true;
    glGenBuffers(1, &instanceVBO_id);TEST_OPENGL_ERROR();
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
            glBufferData(GL_ARRAY_BUFFER, instances.size()*sizeof(InstanceCoordinates),instances.data(), GL_STATIC_DRAW);TEST_OPENGL_ERROR();
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

MeshInstances & Scene::addMesh(std::shared_ptr<Mesh> const & m) {
    mesh_instances.emplace_back(m);
    return mesh_instances[mesh_instances.size()-1];
}

MeshInstances & Scene::getMesh(unsigned int i)
{
    return mesh_instances[i];
}

void Scene::draw() {
    // iterer sur l'ensemble des objets, et faire leur rendu.
    for(auto & mi : mesh_instances) {
        mi.draw(instanceVBO_location);
    }
}