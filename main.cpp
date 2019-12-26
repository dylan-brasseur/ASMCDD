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
#include "include/ASMCDD.h"

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
    //windows[VIEW3D].scene->getMesh(0).modifyInstance(1, {2*std::sin(0.3f*anim), 2*std::cos(0.3f*anim), 0.1f+0.02f*std::sin(anim), 0.7f*anim});
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
    auto tree = Mesh::createmesh("models/tree.ply", true);

    for(auto & m : Mesh::getMeshList()){
        m->centerAndScaleToUnit(true, false, true);
        m->restOnY(0);
        m->buildRawArrays();
        m->buildMeshVBOs(windows[VIEW3D].program->getAttribLocation("position"), windows[VIEW3D].program->getAttribLocation("normal"), windows[VIEW3D].program->getAttribLocation("color"));
    }

    windows[VIEW3D].scene = Scene::createScene(windows[VIEW3D].program->getAttribLocation("offset_scale_rot"), windows[VIEW3D].program->getUniformLocation("MVP"));
    windows[VIEW3D].scene->setBounds(0, 0, 1, 1);
    auto & camera = windows[VIEW3D].scene->getCamera();
    camera.setAspect(1);
    camera.setFOV(60);
    camera.lookAt(0.5,0,0.5);
    camera.setPosition(0.5,0.75,-0.5);
    camera.setUp(0,1,0);
    camera.setDepthLimits(0.1, 100);

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


    ASMCDD orig("examples/forest_small.txt");

    orig.save();
    auto & disks = orig.getSavedDisks();
    unsigned int grassInstances =  windows[DISKS_ORIGINAL].scene->addMesh(grass);
    unsigned int treeInstances =  windows[DISKS_ORIGINAL].scene->addMesh(tree);
    unsigned int champisInstances =  windows[DISKS_ORIGINAL].scene->addMesh(mushroom);
    windows[DISKS_ORIGINAL].scene->getMesh(treeInstances).setColor(0.7, 0.4, 0);
    std::for_each(disks[0].begin(), disks[0].end(), [&](Disk & ic){windows[DISKS_ORIGINAL].scene->addMeshInstance(treeInstances, {ic.x, ic.y, ic.r, 0});});
    windows[DISKS_ORIGINAL].scene->getMesh(grassInstances).setColor(0, 1.0, 0);
    std::for_each(disks[1].begin(), disks[1].end(), [&](Disk & ic){windows[DISKS_ORIGINAL].scene->addMeshInstance(grassInstances,{ic.x, ic.y, ic.r, 0});});
    windows[DISKS_ORIGINAL].scene->getMesh(champisInstances).setColor(1.0, 0, 0);
    std::for_each(disks[2].begin(), disks[2].end(), [&](Disk & ic){windows[DISKS_ORIGINAL].scene->addMeshInstance(champisInstances,{ic.x, ic.y, ic.r, 0});});
    windows[DISKS_ORIGINAL].scene->setBounds(0,0 , 1, 1);
    glLineWidth(2);
    glEnable(GL_LINE_SMOOTH);
    const unsigned char* renderer = glGetString(GL_RENDERER);

    orig.addClassDependency(0,0);
    orig.addClassDependency(0,1);
    orig.addClassDependency(0,2);
    orig.addClassDependency(1,1);
    orig.addClassDependency(2,2);
    orig.computePCF();
    orig.save();
    auto pcfs = orig.getSavedInteractions();

    std::cout << "Running " << (argc > 0 ? argv[0] : "program") << " on " << renderer << std::endl;

    windows[PCF_ORIGINAL].plot = LinePlot::createLinePlot();
    char text[16] = "";
    float3 colors[5] = {{0,0,1}, {1, 0.8, 0}, {0.5, 0.2, 0}, {0, 0.5, 0}, {1, 0, 0}};
    for(unsigned long k = 0; k<pcfs.size(); k++)
    {
        std::sprintf(text, "%d", k);
        unsigned int id = windows[PCF_ORIGINAL].plot->addPlot(text);
        for(unsigned long i=0; i<pcfs[k].meanPCF.size(); i++)
        {
            float a = pcfs[k].meanPCF[i];
            windows[PCF_ORIGINAL].plot->addDataPoint(id, std::make_pair(pcfs[k].radii[i], a));
        }
        windows[PCF_ORIGINAL].plot->setPlotColor(id, colors[k]);
    }

    windows[PCF_ORIGINAL].plot->setBounds(0,0,5, 2);


    ASMCDD_new current;
    unsigned long tree_id = current.addTargetClass(disks[0]);
    unsigned long grass_id = current.addTargetClass(disks[1]);
    unsigned long mush_id = current.addTargetClass(disks[2]);
    current.addDependency(tree_id, grass_id);
    current.addDependency(tree_id, mush_id);
    current.computeTarget();
    current.initialize(1, 0.005);
    auto plot = current.getCurrentPCFplot();
    std::map<std::pair<unsigned long, unsigned long>, float3> colormap;
    colormap.insert_or_assign(std::make_pair(tree_id, tree_id), colors[0]);
    colormap.insert_or_assign(std::make_pair(tree_id, grass_id), colors[1]);
    colormap.insert_or_assign(std::make_pair(tree_id, mush_id), colors[2]);
    colormap.insert_or_assign(std::make_pair(grass_id, grass_id), colors[3]);
    colormap.insert_or_assign(std::make_pair(mush_id, mush_id), colors[4]);


    windows[PCF_CURRENT].plot = LinePlot::createLinePlot();
    for(auto const & p : plot)
    {
        std::sprintf(text, "%lu %lu", p.first.first, p.first.second);
        unsigned int id = windows[PCF_CURRENT].plot->addPlot(text);
        windows[PCF_CURRENT].plot->addDataPoints(id, p.second);
        windows[PCF_CURRENT].plot->setPlotColor(id, colormap.at(p.first));
    }

    windows[PCF_CURRENT].plot->setBounds(0,0,5, 2);

    std::vector<Disk> currDisks;
    unsigned int mi = windows[VIEW3D].scene->addMesh(mushroom);
    windows[VIEW3D].scene->getMesh(mi).setColor(1.0, 0.0, 0.0);
    currDisks = current.getCurrentDisks(mush_id);
    std::for_each(currDisks.begin(), currDisks.end(), [&](Disk & ic){windows[VIEW3D].scene->addMeshInstance(mi, {ic.x, ic.y, ic.r, 0});});
    mi = windows[VIEW3D].scene->addMesh(grass);
    windows[VIEW3D].scene->getMesh(mi).setColor(0.0, 1.0, 0.0);
    currDisks = current.getCurrentDisks(grass_id);
    std::for_each(currDisks.begin(), currDisks.end(), [&](Disk & ic){windows[VIEW3D].scene->addMeshInstance(mi, {ic.x, ic.y, ic.r, 0});});
    mi = windows[VIEW3D].scene->addMesh(tree);
    windows[VIEW3D].scene->getMesh(mi).setColor(0.7f, 0.3f, 0.0f);
    currDisks = current.getCurrentDisks(tree_id);
    std::for_each(currDisks.begin(), currDisks.end(), [&](Disk & ic){windows[VIEW3D].scene->addMeshInstance(mi, {ic.x, ic.y, ic.r, 0});});

    start = std::chrono::high_resolution_clock::now();
    glutTimerFunc(16, timer, 0);
    std::thread renderThread(glutMainLoop);
    renderThread.join();
}
