#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include "include/utils.h"
#include "include/Mesh.h"
#include "include/Shader.h"
#include "include/Program.h"
#include "include/Scene.h"

std::shared_ptr<Mesh> monkey, m2;

GLuint program_id;
static std::unique_ptr<Scene> scene;
float aspect_ratio=1;

void window_resize(int width, int height) {
    glViewport(0,0,width,height);TEST_OPENGL_ERROR();
    aspect_ratio = float(width)/float(height);
}

float lightpos[] = {10.0, 10.0, 50.0};
glm::mat4 MVP;
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    GLuint anim_time_location = glGetUniformLocation(program_id, "light_position");TEST_OPENGL_ERROR();
    glUniform3fv(anim_time_location, 1, lightpos);TEST_OPENGL_ERROR();
    anim_time_location = glGetUniformLocation(program_id, "MVP");TEST_OPENGL_ERROR();
    glUniformMatrix4fv(anim_time_location, 1, GL_FALSE, &(MVP[0][0]));TEST_OPENGL_ERROR();
    scene->draw();
    glutSwapBuffers();
}

void computeMVP(glm::mat4 & m, const float camerapos[3], const float centerpos[3], float fovy, float aspect, float zNear, float zFar){
    m = glm::perspective(glm::radians(fovy), aspect, zNear, zFar) * glm::lookAt(glm::vec3(camerapos[0], camerapos[1], camerapos[2]), glm::vec3(centerpos[0], centerpos[1], centerpos[2]), glm::vec3(0, 1, 0));
}

void anim(){
    static float anim=0;
    anim+=0.05;
    lightpos[0] = 10*std::cos(anim);
    lightpos[1] = 0;
    lightpos[2] = 10*std::sin(anim);
    constexpr float center[] = {0,0,0};
    float camera[] = {3,3,3};
    computeMVP(MVP, camera, center, 75, aspect_ratio, 0.1, 100);
    scene->getMesh(0).modifyInstance(1, {2*std::sin(0.3f*anim), 2*std::cos(0.3f*anim), 0.1f+0.02f*std::sin(anim), 0.7f*anim});
    glutPostRedisplay();
}

void timer(int value){
    anim();
    glutTimerFunc(15, timer, 0);
}

void init_glut(int &argc, char *argv[]) {
    //glewExperimental = GL_TRUE;
    glutInit(&argc, argv);
    glutInitContextVersion(4,5);
    glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition ( 100, 100 );
    glutCreateWindow("3D View");
    glutDisplayFunc(display);
    glutReshapeFunc(window_resize);
    glutTimerFunc(15, timer, 0);
}

bool init_glew() {
    if (glewInit()) {
        std::cerr << " Error while initializing glew";
        return false;
    }
    return true;
}

void init_GL() {
    glEnable(GL_DEPTH_TEST);TEST_OPENGL_ERROR();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);TEST_OPENGL_ERROR();
    glEnable(GL_CULL_FACE);TEST_OPENGL_ERROR();
    glCullFace(GL_BACK);TEST_OPENGL_ERROR();
    glClearColor(0.4,0.4,0.4,1.0);TEST_OPENGL_ERROR();
}



bool init_shaders() {
    auto vertex_shader_3D = Shader::createShader(GL_VERTEX_SHADER, "shaders/3D.vert");
    auto fragment_shader_3D = Shader::createShader(GL_FRAGMENT_SHADER, "shaders/3D.frag");
    if(!Shader::compileAll())
    {
        Shader::getShaderList().clear();
        return false;
    }
    auto program = Program::createProgram("3D program");
    program->attach(vertex_shader_3D);
    program->attach(fragment_shader_3D);
    if(!Program::linkAll())
    {
        Program::getProgramList().clear();
        return false;
    }
    program_id = program->use();
    return true;
}


int main(int argc, char *argv[]) {
    monkey = Mesh::createmesh("models/champi.ply", true);
    m2 = Mesh::createmesh("models/grass.ply", true);
    monkey->centerAndScaleToUnit(true, false, true);
    m2->centerAndScaleToUnit(true, false, true);
    m2->restOnY(0);

    monkey->restOnY(0);
    monkey->buildRawArrays();
    m2->buildRawArrays();
    init_glut(argc, argv);
    if (!init_glew())
        std::exit(-1);
    init_GL();
    if(!init_shaders())
        return EXIT_FAILURE;
    auto p = Program::getProgramList().at(0);
    if(!p->addAttribLocation("position") || !p->addAttribLocation("normal") || !p->addAttribLocation("color") ||!p->addAttribLocation("offset_scale_rot")){
        std::cerr << "Bad attribute location" << std::endl;
        exit(EXIT_FAILURE);
    }

    monkey->buildMeshVBOs(p->getAttribLocation("position"), p->getAttribLocation("normal"), p->getAttribLocation("color"));
    m2->buildMeshVBOs(p->getAttribLocation("position"), p->getAttribLocation("normal"), p->getAttribLocation("color"));
    scene = std::make_unique<Scene>(p->getAttribLocation("offset_scale_rot"));
    auto & mi = scene->addMesh(monkey);
    mi.addInstance({0,0,0.1, 0});
    mi.addInstance({1,0, 0.1, 0.5});
    auto & mi2 = scene->addMesh(m2);
    mi2.addInstance({-1, -0.5f, 0.1, 0.5f});
    glutMainLoop();
}
