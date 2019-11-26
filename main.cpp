#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>
#include "include/Shader.h"
#include "include/Program.h"
#include "include/Scene.h"
#include "include/LinePlot.h"

struct WindowHolder{
    WindowHolder()=default;
    WindowHolder(int _id, std::string  _title): id(_id), title(std::move(_title)){};
    int id=0;
    std::string title;
    std::shared_ptr<Program> program;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<LinePlot> plot;
};

enum WindowIndex : unsigned int{VIEW3D=0, GRAPHS=1, DISKS_ORIGINAL=2, DISKS_CURRENT=3, PCF_ORIGINAL=4, PCF_CURRENT=5};
static WindowHolder windows[6];

void window_resize_3D(int width, int height) {
    glViewport(0,0,width,height);TEST_OPENGL_ERROR();
    windows[VIEW3D].scene->getCamera().setAspect(float(width)/float(height));
}

float lightpos[] = {10.0, 10.0, 50.0};
void display_3D() {
    glutSetWindow(windows[VIEW3D].id);
    glClearColor(0.4,0.4,0.4,1.0);TEST_OPENGL_ERROR();
    GLint program_id = windows[VIEW3D].program->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    GLuint anim_time_location = glGetUniformLocation(program_id, "light_position");TEST_OPENGL_ERROR();
    glUniform3fv(anim_time_location, 1, lightpos);TEST_OPENGL_ERROR();
    windows[VIEW3D].scene->draw();
    glutSwapBuffers();
}

void anim(){
    static float anim=0;
    anim+=0.05;
    lightpos[0] = 10*std::cos(anim);
    lightpos[1] = 0;
    lightpos[2] = 10*std::sin(anim);
    windows[VIEW3D].scene->getMesh(0).modifyInstance(1, {2*std::sin(0.3f*anim), 2*std::cos(0.3f*anim), 0.1f+0.02f*std::sin(anim), 0.7f*anim});
    glutSetWindow(windows[VIEW3D].id);
    glutPostRedisplay();
    glutSetWindow(windows[GRAPHS].id);
    glutPostRedisplay();
}
std::chrono::time_point start = std::chrono::high_resolution_clock::now();
long long int frameTimes[3] = {17,17,16};
unsigned char frameTimeIndex=0;
unsigned long long int nbOfFrames=0; unsigned long long int nbOfMs=0;
void timer(int value){
    auto end = std::chrono::high_resolution_clock::now();
    glutTimerFunc(frameTimes[frameTimeIndex++], timer, 0);
    anim();
    if(frameTimeIndex >= 3) frameTimeIndex=0;
    auto duration = end-start;
    std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    nbOfFrames++;
    nbOfMs+=ms.count()/1000;
    if(nbOfMs >= 1000)
    {
        static char window_title[64];
        std::sprintf(window_title,"3D view - FPS : %.1f", float(nbOfFrames*1000)/float(nbOfMs));
        glutSetWindow(windows[VIEW3D].id);
        glutSetWindowTitle(window_title);
        std::sprintf(window_title,"Graph view - FPS : %.1f", float(nbOfFrames*1000)/float(nbOfMs));
        glutSetWindow(windows[DISKS_CURRENT].id);
        glutSetWindowTitle(window_title);
        nbOfFrames=0;
        nbOfMs=0;
    }
    start = end;
}
float bounds[4] = {-3, -3, 3, 3};
void display_current_disks(){
    glutSetWindow(windows[DISKS_CURRENT].id);
    windows[DISKS_CURRENT].program->use();
    glClearColor(1.0,1.0,1.0,1.0);TEST_OPENGL_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    GLuint color_location = windows[DISKS_CURRENT].program->getUniformLocation("color");
    GLuint bounds_location = windows[DISKS_CURRENT].program->getUniformLocation("bounds");
    windows[DISKS_CURRENT].scene->drawAsDisks(windows[DISKS_CURRENT].program->getAttribLocation("offset_scale_rot"), color_location, bounds_location, bounds);
    glutSwapBuffers();
}

void reshape_disks(int width, int height) {
    glViewport(0,0,width,height);TEST_OPENGL_ERROR();
}

void init_GL() {
    glEnable(GL_DEPTH_TEST);TEST_OPENGL_ERROR();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);TEST_OPENGL_ERROR();
    glEnable(GL_CULL_FACE);TEST_OPENGL_ERROR();
    glCullFace(GL_BACK);TEST_OPENGL_ERROR();
}

void display_current_pcf()
{
    glutSetWindow(windows[PCF_CURRENT].id);
    windows[PCF_CURRENT].program->use();
    glClearColor(1.0,1.0,1.0,1.0);TEST_OPENGL_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    GLint color_location = windows[PCF_CURRENT].program->getUniformLocation("color");
    GLint bounds_location = windows[PCF_CURRENT].program->getUniformLocation("bounds");
    GLint VBO_location = windows[PCF_CURRENT].program->getAttribLocation("point");
    windows[PCF_CURRENT].plot->draw(VBO_location, bounds_location, bounds, color_location);
    glutSwapBuffers();
}

void display_graphs(){
    glClearColor(0.0,0.0,0.0,1.0);TEST_OPENGL_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    glutSetWindow(windows[DISKS_CURRENT].id);
    glutPostRedisplay();
    glutSetWindow(windows[PCF_CURRENT].id);
    glutPostRedisplay();
}

void reshape_graphs(int width, int height)
{
    //TODO
}

WindowHolder init_window(std::string const & title, int parent_id, int width, int height, int position_x, int position_y, void(* display_func)(void), void(* reshape_func)(int, int)=nullptr)
{
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);

    int id=0;
    if(parent_id <= 0)
    {
        glutInitWindowSize(width, height);
        glutInitWindowPosition (position_x, position_y);
        id = glutCreateWindow(title.c_str());
    }else
    {
        id = glutCreateSubWindow(parent_id, position_x, position_y, width, height);
    }
    init_GL();
    glutDisplayFunc(display_func);
    glutReshapeFunc(reshape_func);
    return WindowHolder(id, title);
}

WindowHolder init_window(std::string const & title, int width, int height, void(* display_func)(void), void(* reshape_func)(int, int)=nullptr)
{
    return init_window(title, 0, width, height, -1, -1, display_func, reshape_func);

}

void init_glut(int &argc, char *argv[]) {
    glutInit(&argc, argv);
    glutSetOption(GLUT_RENDERING_CONTEXT ,GLUT_USE_CURRENT_CONTEXT);
    glutInitContextVersion(4,5);
    glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);

    windows[VIEW3D] = init_window("Current 3D View",640, 640, display_3D, window_resize_3D);
    windows[GRAPHS] = init_window("Graphs", 1281, 640, display_graphs, reshape_graphs);
    windows[DISKS_CURRENT] = init_window("Current Disks", windows[GRAPHS].id, 640, 640, 0,0, display_current_disks, reshape_disks);
    windows[PCF_CURRENT] = init_window("Current PCF", windows[GRAPHS].id, 640, 640, 641, 0, display_current_pcf, nullptr);
}

bool init_glew() {
    if (glewInit()) {
        std::cerr << " Error while initializing glew";
        return false;
    }
    return true;
}



bool init_shaders() {
    auto vertex_shader_3D = Shader::createShader(GL_VERTEX_SHADER, "shaders/3D.vert");
    auto fragment_shader_3D = Shader::createShader(GL_FRAGMENT_SHADER, "shaders/3D.frag");

    auto vertex_shader_disks = Shader::createShader(GL_VERTEX_SHADER, "shaders/Disks.vert");
    auto fragment_shader_disks = Shader::createShader(GL_FRAGMENT_SHADER, "shaders/Disks.frag");
    auto geometry_shader_disks = Shader::createShader(GL_GEOMETRY_SHADER, "shaders/Disks.geom");

    geometry_shader_disks->replace_in_shader("NB_V_DISK", "32");

    auto vertex_shader_plot = Shader::createShader(GL_VERTEX_SHADER, "shaders/Plot.vert");
    auto fragment_shader_plot = Shader::createShader(GL_FRAGMENT_SHADER, "shaders/Plot.frag");

    if(!Shader::compileAll())
    {
        Shader::getShaderList().clear();
        return false;
    }

    auto program_3D = Program::createProgram("3D program");
    program_3D->attach(vertex_shader_3D);
    program_3D->attach(fragment_shader_3D);

    auto program_Disks = Program::createProgram("Disk program");
    program_Disks->attach(vertex_shader_disks);
    program_Disks->attach(fragment_shader_disks);
    program_Disks->attach(geometry_shader_disks);

    windows[PCF_CURRENT].program = Program::createProgram("Plot program");
    windows[PCF_CURRENT].program->attach(vertex_shader_plot);
    windows[PCF_CURRENT].program->attach(fragment_shader_plot);

    if(!Program::linkAll())
    {
        Program::getProgramList().clear();
        return false;
    }

    windows[VIEW3D].program=program_3D;
    windows[DISKS_CURRENT].program=program_Disks;
    return true;
}

int main(int argc, char *argv[]) {
    auto monkey = Mesh::createmesh("models/champi.ply", true);
    auto m2 = Mesh::createmesh("models/grass.ply", true);
    monkey->centerAndScaleToUnit(true, false, true);
    m2->centerAndScaleToUnit(true, false, true);
    m2->restOnY(0);

    monkey->restOnY(0);
    monkey->buildRawArrays();
    m2->buildRawArrays();
    init_glut(argc, argv);
    if (!init_glew())
        std::exit(-1);
    if(!init_shaders())
        return EXIT_FAILURE;
    auto p = Program::getProgramList().at(0);
    if(!p->addAttribLocation("position") || !p->addAttribLocation("normal") || !p->addAttribLocation("color") ||!p->addAttribLocation("offset_scale_rot") ||!p->addUniformLocation("MVP")){
        std::cerr << "Bad attribute location 3D program" << std::endl;
        exit(EXIT_FAILURE);
    }
    monkey->buildMeshVBOs(p->getAttribLocation("position"), p->getAttribLocation("normal"), p->getAttribLocation("color"));
    m2->buildMeshVBOs(p->getAttribLocation("position"), p->getAttribLocation("normal"), p->getAttribLocation("color"));
    windows[VIEW3D].scene = std::make_shared<Scene>(p->getAttribLocation("offset_scale_rot"), p->getUniformLocation("MVP"));
    auto & camera = windows[VIEW3D].scene->getCamera();
    camera.setAspect(1);
    camera.setFOV(75);
    camera.lookAt(0,0,0);
    camera.setPosition(3,3,3);
    camera.setUp(0,1,0);
    camera.setDepthLimits(0.1, 100);
    auto & mi = windows[VIEW3D].scene->addMesh(monkey);
    mi.addInstance({0,0,0.1, 0});
    mi.addInstance({1,0, 0.1, 0.5});
    mi.setColor(1.0, 0.0, 0.0);
    auto & mi2 = windows[VIEW3D].scene->addMesh(m2);
    mi2.addInstance({-1, 0.0f, 0.1, 0.5f});
    mi2.setColor(0.0, 1.0, 0.0);
    if(!windows[DISKS_CURRENT].program->addAttribLocation("offset_scale_rot") || ! windows[DISKS_CURRENT].program->addUniformLocation("bounds") || !windows[DISKS_CURRENT].program->addUniformLocation("color"))
    {
        std::cerr << "Bad attribute location Disks program" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(!windows[PCF_CURRENT].program->addAttribLocation("point") || !windows[PCF_CURRENT].program->addUniformLocation("color") || !windows[PCF_CURRENT].program->addUniformLocation("bounds"))
    {
        std::cerr << "Bad attribute location Line program" << std::endl;
        exit(EXIT_FAILURE);
    }
    windows[DISKS_CURRENT].scene = windows[VIEW3D].scene;
    glLineWidth(2);
    glEnable(GL_LINE_SMOOTH);
    const unsigned char* vendor = glGetString(GL_VENDOR);
    const unsigned char* renderer = glGetString(GL_RENDERER);

    std::cout << "Running " << (argc > 0 ? argv[0] : "program") << " on " << renderer << "from " << vendor << std::endl;

    auto & plot = windows[PCF_CURRENT].plot = std::make_shared<LinePlot>();
    unsigned int id = plot->addPlot("test");
    plot->addDataPoint(id, std::make_pair<float, float>(0, 1));
    plot->addDataPoint(id, std::make_pair<float, float>(1, 2));
    plot->addDataPoint(id, std::make_pair<float, float>(2, 3));
    plot->setPlotColor(id, {0, 1, 0});
    id = plot->addPlot("test2");
    plot->addDataPoint(id, std::make_pair<float, float>(0, 3));
    plot->addDataPoint(id, std::make_pair<float, float>(1, 2));
    plot->addDataPoint(id, std::make_pair<float, float>(2, 1));
    plot->setPlotColor(id, {1, 0, 0});
    start = std::chrono::high_resolution_clock::now();
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
}
