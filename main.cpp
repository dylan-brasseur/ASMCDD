#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <thread>
#include "include/Shader.h"
#include "include/Program.h"
#include "include/Scene.h"
#include "include/LinePlot.h"

float euclidianDist(InstanceCoordinates const & a, InstanceCoordinates const & b)
{
    return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

struct WindowHolder{
    std::shared_ptr<Program> program;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<LinePlot> plot;
    int width=0, height=0, posx=0, posy=0;
};

int window_width=1280; int window_height=640;

enum WindowIndex : unsigned int{VIEW3D=0, DISKS_ORIGINAL=1, DISKS_CURRENT=2, PCF_ORIGINAL=3, PCF_CURRENT=4};
static WindowHolder windows[5];

void window_resize_3D(int width, int height) {
    windows[VIEW3D].width = width;
    windows[VIEW3D].height = height;
    windows[VIEW3D].scene->getCamera().setAspect(float(width)/float(height));
}

float lightpos[] = {10.0, 10.0, 50.0};
void display_3D() {
    if(windows[VIEW3D].width <=0 || windows[VIEW3D].height <=0)
        return;
    GLint program_id = windows[VIEW3D].program->use();
    glClearColor(0.4,0.4,0.4,1.0);TEST_OPENGL_ERROR();
    glScissor(windows[VIEW3D].posx,windows[VIEW3D].posy, windows[VIEW3D].width, windows[VIEW3D].height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    glViewport(windows[VIEW3D].posx,windows[VIEW3D].posy, windows[VIEW3D].width, windows[VIEW3D].height);
    GLuint anim_time_location = glGetUniformLocation(program_id, "light_position");TEST_OPENGL_ERROR();
    glUniform3fv(anim_time_location, 1, lightpos);TEST_OPENGL_ERROR();
    windows[VIEW3D].scene->draw();
}

void anim(){
    static float anim=0;
    anim+=0.05;
    lightpos[0] = 10*std::cos(anim);
    lightpos[1] = 0;
    lightpos[2] = 10*std::sin(anim);
    windows[VIEW3D].scene->getMesh(0).modifyInstance(1, {2*std::sin(0.3f*anim), 2*std::cos(0.3f*anim), 0.1f+0.02f*std::sin(anim), 0.7f*anim});
    glutPostRedisplay();
}
std::chrono::time_point start = std::chrono::high_resolution_clock::now();
unsigned long long int nbOfFrames=0; unsigned long long int nbOfMs=0;
void timer(int value){
    auto end = std::chrono::high_resolution_clock::now();
    anim();
    auto duration = end-start;
    std::chrono::microseconds ms = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    nbOfFrames++;
    nbOfMs+=ms.count()/1000;
    if(nbOfMs >= 1000)
    {
        static char window_title[128];
        std::sprintf(window_title,"Accurate Synthesis of Multi-Class Disk Distributions - FPS : %.1f", float(nbOfFrames*1000)/float(nbOfMs));
        glutSetWindowTitle(window_title);
        nbOfFrames=0;
        nbOfMs=0;
    }
    start = end;
    glutTimerFunc(17, timer, 0);
}
void display_disks(WindowIndex id){
    windows[id].program->use();
    glClearColor(1.0,1.0,1.0,1.0);TEST_OPENGL_ERROR();
    glScissor(windows[id].posx,windows[id].posy, windows[id].width, windows[id].height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    glViewport(windows[id].posx,windows[id].posy, windows[id].width, windows[id].height);
    GLuint color_location = windows[id].program->getUniformLocation("color");
    GLuint bounds_location = windows[id].program->getUniformLocation("bounds");
    windows[id].scene->drawAsDisks(windows[id].program->getAttribLocation("offset_scale_rot"), color_location, bounds_location);
}

void init_GL() {
    glEnable(GL_DEPTH_TEST);TEST_OPENGL_ERROR();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);TEST_OPENGL_ERROR();
    glEnable(GL_CULL_FACE);TEST_OPENGL_ERROR();
    glEnable(GL_SCISSOR_TEST);
    glCullFace(GL_BACK);TEST_OPENGL_ERROR();
}

void display_pcf(WindowIndex id)
{
    windows[id].program->use();
    glClearColor(0.95,0.95,0.95,1.0);TEST_OPENGL_ERROR();
    glScissor(windows[id].posx,windows[id].posy, windows[id].width, windows[id].height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    glViewport(windows[id].posx,windows[id].posy, windows[id].width, windows[id].height);
    GLint color_location = windows[id].program->getUniformLocation("color");
    GLint bounds_location = windows[id].program->getUniformLocation("bounds");
    GLint VBO_location = windows[id].program->getAttribLocation("point");
    windows[id].plot->draw(VBO_location, bounds_location, color_location);
}

WindowHolder init_window(int width, int height, int position_x, int position_y)
{
    WindowHolder win;
    win.width = width;
    win.height = height;
    win.posx = position_x;
    win.posy = position_y;
    return win;
}

void display_window()
{
    glScissor(0,0, window_width, window_height);
    glClearColor(0.0,0.0,0.0, 1.0);TEST_OPENGL_ERROR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);TEST_OPENGL_ERROR();
    display_3D();
    display_pcf(PCF_CURRENT);
    display_pcf(PCF_ORIGINAL);
    display_disks(DISKS_ORIGINAL);
    display_disks(DISKS_CURRENT);

    glViewport(0, 0, window_width, window_height);
    glutSwapBuffers();
}

void reshape_window(int width, int height)
{
    if(width<height)
    {
        glutReshapeWindow(height, height);
        return;
    }
    window_resize_3D(std::min(width/2, width-height/2), height);
    windows[DISKS_CURRENT].width=windows[DISKS_CURRENT].height = windows[DISKS_ORIGINAL].width = windows[DISKS_ORIGINAL].height =  height/2;
    windows[DISKS_CURRENT].posx = windows[DISKS_ORIGINAL].posx = windows[VIEW3D].width;
    windows[DISKS_CURRENT].posy = 0;
    windows[DISKS_ORIGINAL].posy = windows[DISKS_CURRENT].height;

    windows[PCF_CURRENT].width = windows[PCF_ORIGINAL].width = width - windows[VIEW3D].width - windows[DISKS_CURRENT].width;
    windows[PCF_CURRENT].height = windows[PCF_ORIGINAL].height = windows[DISKS_CURRENT].height;
    windows[PCF_CURRENT].posx = windows[PCF_ORIGINAL].posx = windows[DISKS_CURRENT].posx+ windows[DISKS_CURRENT].width;
    windows[PCF_CURRENT].posy=0;
    windows[PCF_ORIGINAL].posy = windows[DISKS_CURRENT].height;

    window_width = width;
    window_height = height;
}

void init_glut(int &argc, char *argv[]) {
    glutInit(&argc, argv);
    glutSetOption(GLUT_RENDERING_CONTEXT ,GLUT_USE_CURRENT_CONTEXT);
    glutInitContextVersion(4,5);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(1280, 640);
    glutCreateWindow("Accurate Synthesis of Multi-Class Disk Distributions");

    windows[VIEW3D] = init_window(640, 640, 0, 0);
    windows[DISKS_CURRENT] = init_window( 320, 320, 640, 0);
    windows[PCF_CURRENT] = init_window( 320, 320, 960, 0);
    windows[DISKS_ORIGINAL] = init_window( 320, 320, 640, 320);
    windows[PCF_ORIGINAL] = init_window( 320, 320, 960, 320);

    glutDisplayFunc(display_window);
    glutReshapeFunc(reshape_window);
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

    geometry_shader_disks->replace_in_shader("NB_V_DISK", "16");

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

    auto program_PCF = Program::createProgram("Plot program");
    program_PCF->attach(vertex_shader_plot);
    program_PCF->attach(fragment_shader_plot);

    if(!Program::linkAll())
    {
        Program::getProgramList().clear();
        return false;
    }

    windows[VIEW3D].program=program_3D;
    windows[DISKS_CURRENT].program=program_Disks;
    windows[PCF_CURRENT].program=program_PCF;
    windows[DISKS_ORIGINAL].program=program_Disks;
    windows[PCF_ORIGINAL].program=program_PCF;
    return true;
}

int main(int argc, char *argv[]) {
    srand(time(nullptr));
    init_glut(argc, argv);
    init_GL();
    if (!init_glew())
        std::exit(-1);
    if(!init_shaders())
        return EXIT_FAILURE;
    if(!windows[VIEW3D].program->addAttribLocations("position", "normal", "color", "offset_scale_rot") || !windows[VIEW3D].program->addUniformLocations("MVP")){
        std::cerr << "Bad attribute location 3D program" << std::endl;
        exit(EXIT_FAILURE);
    }

    auto mushroom = Mesh::createmesh("models/champi.ply", true);
    auto grass = Mesh::createmesh("models/grass.ply", true);
    auto tree = Mesh::createmesh("models/ptree.ply", true);

    for(auto & m : Mesh::getMeshList()){
        m->centerAndScaleToUnit(true, false, true);
        m->restOnY(0);
        m->buildRawArrays();
        m->buildMeshVBOs(windows[VIEW3D].program->getAttribLocation("position"), windows[VIEW3D].program->getAttribLocation("normal"), windows[VIEW3D].program->getAttribLocation("color"));
    }

    windows[VIEW3D].scene = Scene::createScene(windows[VIEW3D].program->getAttribLocation("offset_scale_rot"), windows[VIEW3D].program->getUniformLocation("MVP"));
    windows[VIEW3D].scene->setBounds(-3, -3, 3, 3);
    auto & camera = windows[VIEW3D].scene->getCamera();
    camera.setAspect(1);
    camera.setFOV(75);
    camera.lookAt(0,0,0);
    camera.setPosition(0,3,-3);
    camera.setUp(0,1,0);
    camera.setDepthLimits(0.1, 100);

    unsigned int mi = windows[VIEW3D].scene->addMesh(mushroom);
    windows[VIEW3D].scene->addMeshInstance(mi, {0,0,0.1, 0});
    windows[VIEW3D].scene->addMeshInstance(mi, {1,0, 0.1, 0.5});
    windows[VIEW3D].scene->getMesh(mi).setColor(1.0, 0.0, 0.0);

    mi = windows[VIEW3D].scene->addMesh(grass);
    windows[VIEW3D].scene->addMeshInstance(mi, {-1, 0.0f, 0.1, 0.5f});
    windows[VIEW3D].scene->getMesh(mi).setColor(0.0, 1.0, 0.0);

    mi = windows[VIEW3D].scene->addMesh(tree);
    windows[VIEW3D].scene->addMeshInstance(mi, {0.0f, 1.0f, 1.0f, 0.3f});
    windows[VIEW3D].scene->addMeshInstance(mi, {2.0f, 1.0f, 1.0f, 0.3f});
    windows[VIEW3D].scene->getMesh(mi).setColor(0.7f, 0.3f, 0.0f);

    if(!windows[DISKS_CURRENT].program->addAttribLocations("offset_scale_rot") || !windows[DISKS_CURRENT].program->addUniformLocations("bounds", "color")){
        std::cerr << "Bad attribute location Disks program" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(!windows[PCF_CURRENT].program->addAttribLocations("point") || !windows[PCF_CURRENT].program->addUniformLocations("color", "bounds"))
    {
        std::cerr << "Bad attribute location Line program" << std::endl;
        exit(EXIT_FAILURE);
    }
    windows[DISKS_CURRENT].scene = windows[VIEW3D].scene;
    windows[DISKS_ORIGINAL].program = windows[DISKS_CURRENT].program;
    windows[DISKS_ORIGINAL].scene = Scene::createScene(0,0);
    windows[DISKS_ORIGINAL].scene->setBounds(-3, -3, 3, 3);

    std::vector<InstanceCoordinates> grassTestInstances, treeTestInstances, champisTestInstances;
    //10 tree, 50 champis, 430 grass
#define randf(min, max) (float((max)-(min))*float(rand())/float(RAND_MAX) + float(min))
    float treebias = 0.05, grassbias = 0.001, champibias = 0.01;
    for(int i=0; i<1000; i++)
    {
        InstanceCoordinates coords{randf(-2.6, 2.6), randf(-2.6, 2.6), randf(0.45, 0.55), randf(0, 6)};
        if(std::none_of(treeTestInstances.begin(), treeTestInstances.end(), [&](InstanceCoordinates ic){return euclidianDist(ic, coords) <= ic.s+coords.s+treebias;}))
        {
            treeTestInstances.push_back(coords);
            if(treeTestInstances.size() == 10)
                break;
        }
    }
    for(int i=0; i<5000; i++)
    {
        int treeidx = rand()%treeTestInstances.size();
        float size = randf(0.04, 0.08);
        float theta = randf(0, 2*M_PI);
        float r = randf(0.2*treeTestInstances[treeidx].s+size, 0.95*treeTestInstances[treeidx].s-size);
        InstanceCoordinates coords{treeTestInstances[treeidx].x+r*std::cos(theta), treeTestInstances[treeidx].y+r*std::sin(theta), size, randf(0, 2*M_PI)};
        if(std::none_of(champisTestInstances.begin(), champisTestInstances.end(), [&](InstanceCoordinates ic){return euclidianDist(ic, coords) <= ic.s+coords.s+champibias;}))
        {
            champisTestInstances.push_back(coords);
            if(champisTestInstances.size() == 50)
                break;
        }
    }
    for(int i=0; i<430000; i++)
    {
        InstanceCoordinates coords{randf(-3, 3), randf(-3, 3), randf(0.04, 0.06), randf(0, 2*M_PI)};
        if(std::none_of(treeTestInstances.begin(), treeTestInstances.end(), [&](InstanceCoordinates ic){return euclidianDist(ic, coords) <= ic.s+coords.s+3*grassbias;})
        && std::none_of(grassTestInstances.begin(), grassTestInstances.end(), [&](InstanceCoordinates ic){return euclidianDist(ic, coords) <= ic.s+coords.s+grassbias;})){
            grassTestInstances.push_back(coords);
            if(grassTestInstances.size() == 430)
                break;
        }
    }
    unsigned int grassInstances =  windows[DISKS_ORIGINAL].scene->addMesh(grass);
    unsigned int treeInstances =  windows[DISKS_ORIGINAL].scene->addMesh(tree);
    unsigned int champisInstances =  windows[DISKS_ORIGINAL].scene->addMesh(mushroom);
    windows[DISKS_ORIGINAL].scene->getMesh(treeInstances).setColor(0.7, 0.4, 0);
    std::for_each(treeTestInstances.begin(), treeTestInstances.end(), [&](auto & ic){windows[DISKS_ORIGINAL].scene->addMeshInstance(treeInstances,ic);});
    windows[DISKS_ORIGINAL].scene->getMesh(champisInstances).setColor(1.0, 0, 0);
    std::for_each(champisTestInstances.begin(), champisTestInstances.end(), [&](auto & ic){windows[DISKS_ORIGINAL].scene->addMeshInstance(champisInstances,ic);});
    windows[DISKS_ORIGINAL].scene->getMesh(grassInstances).setColor(0, 1.0, 0);
    std::for_each(grassTestInstances.begin(), grassTestInstances.end(), [&](auto & ic){windows[DISKS_ORIGINAL].scene->addMeshInstance(grassInstances,ic);});

    glLineWidth(1);
    glEnable(GL_LINE_SMOOTH);
    const unsigned char* renderer = glGetString(GL_RENDERER);

    std::cout << "Running " << (argc > 0 ? argv[0] : "program") << " on " << renderer << std::endl;

    auto & plot = windows[PCF_CURRENT].plot = LinePlot::createLinePlot();
    auto & plot_o = windows[PCF_ORIGINAL].plot = LinePlot::createLinePlot();
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

    id = plot_o->addPlot("test3");
    plot_o->addDataPoint(id, std::make_pair<float, float>(-1, 1));
    plot_o->addDataPoint(id, std::make_pair<float, float>(-0, -5));
    plot_o->setPlotColor(id, {0,0,1});

    start = std::chrono::high_resolution_clock::now();
    glutTimerFunc(16, timer, 0);
    std::thread renderThread(glutMainLoop);
    plot->setBounds(-0.05, -0.05, 3, 3);
    renderThread.join();
}
