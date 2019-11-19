#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include "include/Shader.h"
#include "include/Program.h"
#include "include/Scene.h"
#include "include/Camera.h"

struct WindowHolder{
    int window_id=-1;
    std::shared_ptr<Program> program;
    std::unique_ptr<Scene> scene;
};

std::shared_ptr<Mesh> monkey, m2;

GLuint program_id;
static std::unique_ptr<Scene> scene;
static WindowHolder view3D, graphs;

void window_resize_3D(int width, int height) {
    glViewport(0,0,width,height);TEST_OPENGL_ERROR();
    scene->getCamera().setAspect(float(width)/float(height));
}

float lightpos[] = {10.0, 10.0, 50.0};
void display_3D() {
    glutSetWindow(view3D.window_id);
    view3D.program->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    GLuint anim_time_location = glGetUniformLocation(program_id, "light_position");TEST_OPENGL_ERROR();
    glUniform3fv(anim_time_location, 1, lightpos);TEST_OPENGL_ERROR();
    //scene->getCamera().push_state();
    //scene->getCamera().lookAt(0, 0, 0);
    scene->draw();
    //scene->getCamera().pop_state();
    glutSwapBuffers();
}

void anim(){
    static float anim=0;
    anim+=0.05;
    lightpos[0] = 10*std::cos(anim);
    lightpos[1] = 0;
    lightpos[2] = 10*std::sin(anim);
    scene->getMesh(0).modifyInstance(1, {2*std::sin(0.3f*anim), 2*std::cos(0.3f*anim), 0.1f+0.02f*std::sin(anim), 0.7f*anim});
    glutSetWindow(view3D.window_id);
    glutPostRedisplay();
    glutSetWindow(graphs.window_id);
    glutPostRedisplay();
}

void timer(int value){
    auto start = std::chrono::high_resolution_clock::now();
    anim();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end-start;
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    glutTimerFunc(std::max(16L-ms.count(), 1L), timer, 0);
}

void display_graph(){
    glutSetWindow(graphs.window_id);
    graphs.program->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    GLuint anim_time_location = glGetUniformLocation(program_id, "light_position");TEST_OPENGL_ERROR();
    glUniform3fv(anim_time_location, 1, lightpos);TEST_OPENGL_ERROR();
    auto & ch = scene->getMesh(0).getInstance(1);
    auto & camera = scene->getCamera();
    camera.push_state();
    camera.lookAt(ch.x, 0, ch.y);
    scene->draw();
    camera.pop_state();
    //glutSolidCube(1.0);
    glutSwapBuffers();
}

void window_resize_graph(int width, int height) {
    glViewport(0,0,width,height);TEST_OPENGL_ERROR();
    //scene->getCamera().setAspect(float(width)/float(height));
}

void init_glut(int &argc, char *argv[]) {
    //glewExperimental = GL_TRUE;
    glutInit(&argc, argv);
    glutSetOption(GLUT_RENDERING_CONTEXT ,GLUT_USE_CURRENT_CONTEXT);
    glutInitContextVersion(4,5);
    glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);

    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(640, 640);
    glutInitWindowPosition ( 800, 100 );
    graphs.window_id = glutCreateWindow("Graph View");
    glEnable(GL_DEPTH_TEST);TEST_OPENGL_ERROR();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);TEST_OPENGL_ERROR();
    glEnable(GL_CULL_FACE);TEST_OPENGL_ERROR();
    glCullFace(GL_BACK);TEST_OPENGL_ERROR();
    glClearColor(1.0,1.0,1.0,1.0);TEST_OPENGL_ERROR();
    glutDisplayFunc(display_graph);
    glutReshapeFunc(window_resize_graph);

    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(640, 640);
    glutInitWindowPosition ( 100, 100 );
    view3D.window_id = glutCreateWindow("3D View");
    glEnable(GL_DEPTH_TEST);TEST_OPENGL_ERROR();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);TEST_OPENGL_ERROR();
    glEnable(GL_CULL_FACE);TEST_OPENGL_ERROR();
    glCullFace(GL_BACK);TEST_OPENGL_ERROR();
    glClearColor(0.4,0.4,0.4,1.0);TEST_OPENGL_ERROR();
    glutDisplayFunc(display_3D);
    glutReshapeFunc(window_resize_3D);
    glutTimerFunc(10, timer, 0);
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
    view3D.program=program;
    graphs.program=program;
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
    //init_GL();
    if(!init_shaders())
        return EXIT_FAILURE;
    auto p = Program::getProgramList().at(0);
    if(!p->addAttribLocation("position") || !p->addAttribLocation("normal") || !p->addAttribLocation("color") ||!p->addAttribLocation("offset_scale_rot") ||!p->addUniformLocation("MVP")){
        std::cerr << "Bad attribute location" << std::endl;
        exit(EXIT_FAILURE);
    }
    monkey->buildMeshVBOs(p->getAttribLocation("position"), p->getAttribLocation("normal"), p->getAttribLocation("color"));
    m2->buildMeshVBOs(p->getAttribLocation("position"), p->getAttribLocation("normal"), p->getAttribLocation("color"));
    scene = std::make_unique<Scene>(p->getAttribLocation("offset_scale_rot"), p->getUniformLocation("MVP"));
    auto & camera = scene->getCamera();
    camera.setAspect(1);
    camera.setFOV(75);
    camera.lookAt(0,0,0);
    camera.setPosition(3,3,3);
    camera.setUp(0,1,0);
    camera.setDepthLimits(0.1, 100);
    auto & mi = scene->addMesh(monkey);
    mi.addInstance({0,0,0.1, 0});
    mi.addInstance({1,0, 0.1, 0.5});
    auto & mi2 = scene->addMesh(m2);
    mi2.addInstance({-1, 0.0f, 0.1, 0.5f});
    glutMainLoop();
}
