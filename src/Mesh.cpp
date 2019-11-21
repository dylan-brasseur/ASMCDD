#include "../include/Mesh.h"
#include <iostream>
#include <fstream>
#include <sstream>
#define TINYPLY_IMPLEMENTATION
#include "../include/tinyply.h"
#include "../include/utils.h"

std::vector<std::shared_ptr<Mesh>> Mesh::meshList;
void Mesh::loadOFF (const std::string & filename) {
    std::ifstream in (filename.c_str ());
    if (!in)
    {
        std::cerr << "Cannot open file : " << filename << std::endl;
        exit (EXIT_FAILURE);
    }
    std::stringstream ss;
    std::string line;
    std::getline(in, line); //OFF line
    std::getline(in, line); //Dimensions
    ss << line;
    unsigned int sizeV, sizeT, tmp;
    ss >> sizeV >> sizeT >> tmp;
    vertices.resize (sizeV);
    std::vector<unsigned int> verticesUsage(sizeV, 0U); //Vector for usage of vertex, for color
    triangles.resize (sizeT);
    for (unsigned int i = 0; i < sizeV; i++)
    {
        std::getline(in, line);
        ss.clear();
        ss << line;
        ss >> vertices[i].p;
    }

    int s;
    for (unsigned int t = 0; t < sizeT; t++) {
        std::getline(in, line);
        ss.clear();
        ss << line;
        ss >> s;
        for (unsigned int j = 0; j < 3; j++)
        {
            ss >> triangles[t][j];
            ++verticesUsage[triangles[t][j]];
        }

        //Check for color
        //Not really what you should expect, as the colors are per faces,
        //but it works for really simple meshes
        float r,g,b;
        ss >> r;
        if(ss.fail())
        {
            //Default color
            vertices[triangles[t][0]].c += Vec3(0.666, 0.666, 0.666);
            vertices[triangles[t][1]].c += Vec3(0.666, 0.666, 0.666);
            vertices[triangles[t][2]].c += Vec3(0.666, 0.666, 0.666);
        }else
        {
            ss >> g >> b;
            vertices[triangles[t][0]].c += Vec3((r/255), (g/255), (b/255));
            vertices[triangles[t][1]].c += Vec3((r/255), (g/255), (b/255));
            vertices[triangles[t][2]].c += Vec3((r/255), (g/255), (b/255));
        }
    }

    for(unsigned int v=0; v < sizeV; v++)
    {
        if(verticesUsage[v])
        {
            vertices[v].c[0] /= verticesUsage[v];
            vertices[v].c[1] /= verticesUsage[v];
            vertices[v].c[2] /= verticesUsage[v];
        }
    }
    in.close ();
}

bool Mesh::loadPLY(const std::string & filename)
{
    std::ifstream in (filename.c_str (), ios::binary);
    if (!in)
    {
        std::cerr << "Cannot open file : " << filename << std::endl;
        exit (EXIT_FAILURE);
    }
    vertices.clear();
    triangles.clear();
    PlyFile file;
    file.parse_header(in);
    std::shared_ptr<PlyData> plyvertices, plycolors, plyfaces, plynormals;
    bool hasColors=true, hasNormal=true;
    try{
        plyvertices = file.request_properties_from_element("vertex", {"x", "y", "z"});
    }catch(const exception & e){
        std::cerr << "File " << filename << " lacks vertices, aborting load." << std::endl;
        in.close();
        return false;
    }
    try{
        plynormals = file.request_properties_from_element("vertex", {"nx", "ny", "nz"});
    }catch(const exception & e)
    {
        std::cout << "File " << filename << " lacks normals, they will be computed." << std::endl;
        hasNormal=false;
    }
    try{
        plycolors = file.request_properties_from_element("vertex", {"red", "green", "blue"});
    }catch(const exception & e)
    {
        std::cout << "File " << filename << "lacks colors, will be defaulted to #A9A9A9." << std::endl;
        hasColors=false;
    }
    try{
        plyfaces = file.request_properties_from_element("face", {"vertex_indices"}, 3);
    }catch(const exception & e)
    {
        std::cerr << "File " << filename << " lacks faces, aborting." << std::endl;
        in.close();
        return false;
    }

    file.read(in);
    in.close();
    vertices.reserve(plyvertices->count);
    triangles.reserve(plyfaces->count);

    vector<float3> vertices_temp(plyvertices->count);
    std::memcpy(vertices_temp.data(), plyvertices->buffer.get(), plyvertices->buffer.size_bytes());
    vector<unsigned int> triangles_temp(plyfaces->count*3);
    std::memcpy(triangles_temp.data(), plyfaces->buffer.get(), plyfaces->buffer.size_bytes());
    vector<float3> normal_temp;
    vector<unsigned char> colors_temp;


    if(hasNormal)
    {
        normal_temp.reserve(plynormals->count);
        std::memcpy(normal_temp.data(), plynormals->buffer.get(), plynormals->buffer.size_bytes());
    }
    if(hasColors)
    {
        colors_temp.reserve(plycolors->count*3);
        std::memcpy(colors_temp.data(), plycolors->buffer.get(), plycolors->buffer.size_bytes());
    }

    std::memcpy(colors_temp.data(), plycolors->buffer.get(), plycolors->buffer.size_bytes());

    for(size_t i=0; i<vertices_temp.size(); i++)
    {
        vertices.emplace_back(Vec3(vertices_temp[i]), hasNormal ? Vec3(normal_temp[i]) : Vec3(), hasColors ? Vec3(colors_temp[i*3]/255.0f, colors_temp[i*3+1]/255.0f, colors_temp[i*3+2]/255.0f) : Vec3(0.666, 0.666, 0.666));
    }
    for(size_t i=0; i<triangles_temp.size()/3; i++)
    {
        triangles.emplace_back(triangles_temp[i*3], triangles_temp[i*3+1], triangles_temp[i*3+2]);
    }
    if(!hasNormal)
    {
        recomputeNormals();
    }
    return true;
}

void Mesh::recomputeNormals () {
    for (auto & vertice : vertices)
        vertice.n = Vec3 (0.0, 0.0, 0.0);
    for (auto & triangle : triangles) {
        Vec3 e01 = vertices[  triangle[1]  ].p -  vertices[  triangle[0]  ].p;
        Vec3 e02 = vertices[  triangle[2]  ].p -  vertices[  triangle[0]  ].p;
        Vec3 n = Vec3::cross (e01, e02);
        n.normalize ();
        for (unsigned int j = 0; j < 3; j++)
            vertices[  triangle[j]  ].n += n;
    }
    for (auto & vertice : vertices)
        vertice.n.normalize ();
}

void Mesh::centerAndScaleToUnit () {
    std::pair<Vec3, Vec3> bounds = getBounds();
    Vec3 c = (bounds.first+bounds.second)/2;
    float maxD = -INFINITY;
    for (auto & vertice : vertices){
        vertice.p-=c;
        float m = vertice.p.squareLength();
        if (m > maxD)
            maxD = m;
    }
    maxD = sqrt(maxD);
    for  (auto & vertice : vertices)
        vertice.p/=maxD;
}

void Mesh::buildRawArrays(){
    buildVertexArray();
    buildNormalArray();
    buildColorArray();
    buildTriangleArray();
}

void Mesh::buildVertexArray()
{
    vertexArray.clear();
    vertexArray.reserve(3*vertices.size());
    for(auto v : vertices)
    {
        vertexArray.push_back(v.p[0]);
        vertexArray.push_back(v.p[1]);
        vertexArray.push_back(v.p[2]);
    }
}

void Mesh::buildTriangleArray()
{
    triangleArray.clear();
    triangleArray.reserve(3*triangles.size());
    for(auto t : triangles)
    {
        triangleArray.push_back(t[0]);
        triangleArray.push_back(t[1]);
        triangleArray.push_back(t[2]);
    }
}

void Mesh::buildNormalArray()
{
    normalArray.clear();
    normalArray.reserve(3*vertices.size());
    for(auto v : vertices)
    {
        normalArray.push_back(v.n[0]);
        normalArray.push_back(v.n[1]);
        normalArray.push_back(v.n[2]);
    }
}
void Mesh::buildColorArray()
{
    colorArray.clear();
    colorArray.reserve(3*vertices.size());
    for(auto v : vertices)
    {
        colorArray.push_back(v.c[0]);
        colorArray.push_back(v.c[1]);
        colorArray.push_back(v.c[2]);
    }
}

std::shared_ptr<Mesh> &Mesh::createmesh(const std::string &filename, bool ply_type){
    auto m = std::make_shared<Mesh>();
    if(ply_type)
        m->loadPLY(filename);
    else
        m->loadOFF(filename);
    meshList.push_back(m);
    return meshList[meshList.size()-1];
}

void Mesh::buildMeshVBOs(GLint vertex_location, GLint normal_location, GLint color_location){
    glGenVertexArrays(1, &vertexArray_id);TEST_OPENGL_ERROR();
    glBindVertexArray(vertexArray_id);TEST_OPENGL_ERROR();
    GLuint vbo_ids[3] = {0, 0, 0};
    glGenBuffers((vertex_location!= -1) + (normal_location!= -1) + (color_location!= -1), vbo_ids);TEST_OPENGL_ERROR();
    uint8_t index_vbo=0;
    if(vertex_location != -1){
        vertexVBO_id =vbo_ids[index_vbo];
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[index_vbo++]);TEST_OPENGL_ERROR();
        glBufferData(GL_ARRAY_BUFFER, vertexArray.size()*sizeof(float), vertexArray.data(), GL_STATIC_DRAW);TEST_OPENGL_ERROR();
        glVertexAttribPointer(vertex_location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);TEST_OPENGL_ERROR();
        glEnableVertexAttribArray(vertex_location);TEST_OPENGL_ERROR();

    }
    if(normal_location != -1){
        normalVBO_id =vbo_ids[index_vbo];
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[index_vbo++]);TEST_OPENGL_ERROR();
        glBufferData(GL_ARRAY_BUFFER, normalArray.size()*sizeof(float), normalArray.data(), GL_STATIC_DRAW);TEST_OPENGL_ERROR();
        glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);TEST_OPENGL_ERROR();
        glEnableVertexAttribArray(normal_location);TEST_OPENGL_ERROR();
    }
    if(color_location != -1){
        colorVBO_id =vbo_ids[index_vbo];
        glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[index_vbo]);TEST_OPENGL_ERROR();
        glBufferData(GL_ARRAY_BUFFER, colorArray.size()*sizeof(float), colorArray.data(), GL_STATIC_DRAW);TEST_OPENGL_ERROR();
        glVertexAttribPointer(color_location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);TEST_OPENGL_ERROR();
        glEnableVertexAttribArray(color_location);TEST_OPENGL_ERROR();
    }

    glBindVertexArray(0);

}

void Mesh::centerAndScaleToUnit(bool onX, bool onY, bool onZ){
    if(onX && onY && onZ){
        centerAndScaleToUnit();
        return;
    }
    std::pair<Vec3, Vec3> bounds = getBounds();
    Vec3 c = (bounds.first+bounds.second)/2;
    Vec3 bias(onX ? 1 : 0, onY ? 1 : 0, onZ ? 1 : 0);
    if(!onX && !onY && !onZ)
    {
        for (auto & v : vertices)
        {
            v.p-=c;
        }
        return;
    }
    float maxD = -INFINITY;
    for (auto & v : vertices)
    {
        v.p-=c;
        float len = (v.p*bias).squareLength();
        if(len > maxD) maxD = len;
    }
    maxD = sqrt(maxD);
    for(auto & v : vertices)
    {
        v.p/=maxD;
    }
}

std::pair<Vec3, Vec3> Mesh::getBounds() const{
    Vec3 mn(INFINITY, INFINITY, INFINITY), mx(-INFINITY, -INFINITY, -INFINITY);
    for(auto & v : vertices)
    {
        for(int i=0; i<3; i++)
        {
            mn[i] = min(mn[i], v.p[i]);
            mx[i] = max(mx[i], v.p[i]);
        }
    }
    return {mn, mx};
}

void Mesh::restOnY(float y){
    float minY = INFINITY;
    for(auto & v : vertices)
    {
        minY = v.p[1] < minY ? v.p[1] : minY;
    }
    const float diff = y-minY;
    for(auto & v : vertices)
    {
        v.p[1]+=diff;
    }
}
