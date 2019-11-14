#ifndef MESH_H
#define MESH_H


#include <vector>
#include <string>
#include <memory>
#include "Vec3.h"

#include <GL/glut.h>


// -------------------------------------------
// Basic Mesh class
// Modified by Dylan Brasseur
// -------------------------------------------

struct MeshVertex {
    inline MeshVertex () {}
    MeshVertex (const Vec3 & _p, const Vec3 & _n, const Vec3 & _c) : p (_p), n (_n), c(_c){}
    inline MeshVertex (const MeshVertex & v) = default;
    inline virtual ~MeshVertex () = default;
    inline MeshVertex & operator = (const MeshVertex & v) = default;
    // membres :
    Vec3 p; // une position
    Vec3 n; // une normale
    Vec3 c; // Une couleur
};

struct MeshTriangle {
    inline MeshTriangle () {
        corners[0] = corners[1] = corners[2] = 0;
    }
    inline MeshTriangle (const MeshTriangle & t) {
        corners[0] = t[0];   corners[1] = t[1];   corners[2] = t[2];
    }
    inline MeshTriangle (unsigned int v0, unsigned int v1, unsigned int v2) {
        corners[0] = v0;   corners[1] = v1;   corners[2] = v2;
    }
    inline virtual ~MeshTriangle () {}
    inline MeshTriangle & operator = (const MeshTriangle & t) {
        corners[0] = t[0];   corners[1] = t[1];   corners[2] = t[2];
        return (*this);
    }

    unsigned int operator [] (unsigned int c) const { return corners[c]; }
    unsigned int & operator [] (unsigned int c) { return corners[c]; }

private:
    // membres :
    unsigned int corners[3];
};




class Mesh {
public:
    Mesh(){
        vertexArray_id = vertexVBO_id = normalVBO_id = colorVBO_id = 0;
    }

    void loadOFF (const std::string & filename);
    bool loadPLY(const std::string & filename);
    void buildRawArrays();
    void buildMeshVBOs(GLint vertex_location, GLint normal_location, GLint color_location);
    void recomputeNormals();
    void centerAndScaleToUnit();
    void centerAndScaleToUnit(bool fromX, bool fromY, bool fromZ);
    void restOnY(float y);
    void buildVertexArray();
    void buildTriangleArray();
    void buildNormalArray();
    void buildColorArray();
    [[nodiscard]] std::pair<Vec3, Vec3> getBounds() const;

    static std::shared_ptr<Mesh>& createmesh(const std::string & filename, bool ply_type);
    static std::vector<std::shared_ptr<Mesh>>& getMeshList(){
        return meshList;
    }
    static std::vector<std::shared_ptr<Mesh>> meshList;


    std::vector<MeshVertex>     vertices;
    std::vector<MeshTriangle>   triangles;
    std::vector<float>          vertexArray;
    std::vector<unsigned int>   triangleArray;
    std::vector<float>          normalArray;
    std::vector<float>          colorArray;
    GLuint vertexArray_id, vertexVBO_id, normalVBO_id, colorVBO_id;
};



#endif
