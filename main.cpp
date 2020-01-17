#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include "include/Shader.h"
#include "include/Program.h"
#include "include/Scene.h"
#include "include/LinePlot.h"
#include "include/ASMCDD.h"

std::mutex draw_lock;

std::string WINDOW_TITLE = "Accurate Synthesis of Multi-Class Disk Distributions - ";
std::string TARGET_STRING = "Computing target...";
std::string INIT_STRING = "Initializing : ";
std::string TOTAL_SIZE;
std::string CURRENT_SIZE;
std::string REFINE_STRING = "Refining...";
std::string ALGO_DONE = "Done in ";

unsigned long duration;

bool targetDone = false, initDone = false, refineDone= false;
bool initDisplayed = false;
ASMCDD algo;
float domainLength = 1;
std::vector<unsigned long> currentSizes;
std::vector<unsigned long> finalSizes;
unsigned long totalSize;

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

float rand_angle()
{
    static std::random_device rand_device;
    static std::mt19937_64 rand_gen(rand_device());
    static std::uniform_real_distribution<float> uni(0,  2*M_PI);
    return uni(rand_gen);
}

void addNewInstances(std::vector<Disk> const & allDisks, unsigned long index, std::shared_ptr<Scene> const & scene, float domainLength)
{
    for(unsigned long count = scene->getInstanceCount(index);count<allDisks.size(); count++)
    {
        Disk const & d = allDisks[count];
        scene->addMeshInstance(index, {d.x/domainLength, d.y/domainLength, d.r/domainLength, rand_angle()});
    }
}

/**
 * Updates the disks and plots;
 */

void update(){
    unsigned long current_size=0;
    bool localinit = initDone;
    if(!localinit || !initDisplayed)
    {
        auto plots = algo.getPrettyPCFplot(domainLength);
        for(auto const & p : plots.second)
        {
            if(currentSizes[p.first.first] == finalSizes[p.first.first] && currentSizes[p.first.second] == finalSizes[p.first.second]) continue;
            draw_lock.lock();
            windows[PCF_CURRENT].plot->replacePoints(windows[PCF_CURRENT].plot->getIdFromRelation(p.first), p.second);
            draw_lock.unlock();
        }
        for(unsigned long id=0; id<plots.first.size(); id++)
        {
            current_size+=plots.first[id].size();
            if(currentSizes[id] == finalSizes[id]) continue;
            draw_lock.lock();
            addNewInstances(plots.first[id], id, windows[DISKS_CURRENT].scene, domainLength);
            currentSizes[id] = plots.first[id].size();
            draw_lock.unlock();
        }
        if(localinit)
        {
            initDisplayed=true;
        }
        CURRENT_SIZE=std::to_string(current_size);
    }
}

void anim(){
    static float anim=0;
    anim+=0.05;
    lightpos[0] = 10*std::cos(anim);
    lightpos[1] = 0;
    lightpos[2] = 10*std::sin(anim);
    glutPostRedisplay();
}
std::chrono::time_point start = std::chrono::high_resolution_clock::now();
unsigned long long int nbOfFrames=0; unsigned long long int nbOfMs=0;
void timer(int value){
    if(nbOfFrames % 10 == 0)
    {
        update();
        std::string status;
        if(!targetDone)
        {
            status=TARGET_STRING;
        }else if(!initDone)
        {
            status=INIT_STRING + CURRENT_SIZE + "/" + TOTAL_SIZE;
        }else if(!refineDone)
        {
            status=REFINE_STRING;
        }else{
            char seconds [64];
            std::sprintf(seconds, "%.2lf", double(duration)/1000.0);
            status=ALGO_DONE + seconds + "s";
        }
        glutSetWindowTitle((WINDOW_TITLE+status).c_str());
    }

    anim();
    glutTimerFunc(1, timer, 0);
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
    draw_lock.lock();
    display_pcf(PCF_ORIGINAL);
    display_disks(DISKS_ORIGINAL);
    display_pcf(PCF_CURRENT);
    display_disks(DISKS_CURRENT);
    glViewport(0, 0, window_width, window_height);
    glutSwapBuffers();
    draw_lock.unlock();
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
}

void init_windows()
{
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
    init_windows();
    init_GL();
    if (!init_glew())
        std::exit(EXIT_FAILURE);
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
    windows[DISKS_ORIGINAL].scene = Scene::createScene(0,0);
    windows[PCF_CURRENT].plot = LinePlot::createLinePlot();
    windows[PCF_CURRENT].plot->setBounds(0,0,5, 2);
    windows[PCF_ORIGINAL].plot = LinePlot::createLinePlot();
    windows[PCF_ORIGINAL].plot->setBounds(0,0,5, 2);

    glLineWidth(2);
    glEnable(GL_LINE_SMOOTH);

    char text[16] = "";
    float3 colors[5] = {{0,0,1}, {1, 0.8, 0}, {0.5, 0.2, 0}, {0, 0.5, 0}, {1, 0, 0}};

    algo.loadFile("examples/forest_small.txt");
    finalSizes = algo.getFinalSizes(domainLength);
    currentSizes.resize(finalSizes.size(), 0);
    totalSize = std::accumulate(finalSizes.begin(), finalSizes.end(), 0UL);
    TOTAL_SIZE=std::to_string(totalSize);
    ASMCDD_params params;
    params.step=0.1;
    params.limit=5;
    params.sigma=0.25;
    algo.setParams(params);
    unsigned long tree_id = 0;
    unsigned long grass_id = 1;
    unsigned long mush_id = 2;
    algo.addDependency(tree_id, grass_id);
    algo.addDependency(tree_id, mush_id);
    std::map<std::pair<unsigned long, unsigned long>, float3> colormap;
    colormap.insert_or_assign(std::make_pair(tree_id, tree_id), colors[0]);
    colormap.insert_or_assign(std::make_pair(tree_id, grass_id), colors[1]);
    colormap.insert_or_assign(std::make_pair(tree_id, mush_id), colors[2]);
    colormap.insert_or_assign(std::make_pair(grass_id, grass_id), colors[3]);
    colormap.insert_or_assign(std::make_pair(mush_id, mush_id), colors[4]);

    windows[DISKS_ORIGINAL].scene->addMesh(tree);
    windows[DISKS_ORIGINAL].scene->addMesh(grass);
    windows[DISKS_ORIGINAL].scene->addMesh(mushroom);

    windows[DISKS_ORIGINAL].scene->getMesh(tree_id).setColor(0.7, 0.4, 0);
    windows[DISKS_ORIGINAL].scene->getMesh(grass_id).setColor(0, 1.0, 0);
    windows[DISKS_ORIGINAL].scene->getMesh(mush_id).setColor(1.0, 0, 0);

    windows[DISKS_CURRENT].scene->addMesh(tree);
    windows[DISKS_CURRENT].scene->addMesh(grass);
    windows[DISKS_CURRENT].scene->addMesh(mushroom);

    windows[DISKS_CURRENT].scene->getMesh(tree_id).setColor(0.7, 0.4, 0);
    windows[DISKS_CURRENT].scene->getMesh(grass_id).setColor(0, 1.0, 0);
    windows[DISKS_CURRENT].scene->getMesh(mush_id).setColor(1.0, 0, 0);

    windows[DISKS_ORIGINAL].scene->setBounds(0,0 , 1, 1);
    windows[DISKS_CURRENT].scene->setBounds(0,0 , 1, 1);


    for(auto & c : colormap)
    {
        std::sprintf(text, "%lu %lu", c.first.first, c.first.second);
        unsigned int id = windows[PCF_CURRENT].plot->addPlot(text, c.first.first, c.first.second);
        windows[PCF_CURRENT].plot->setPlotColor(id, c.second);
        id = windows[PCF_ORIGINAL].plot->addPlot(text, c.first.first, c.first.second);
        windows[PCF_ORIGINAL].plot->setPlotColor(id, c.second);
    }

    const unsigned char* renderer = glGetString(GL_RENDERER);
    start = std::chrono::high_resolution_clock::now();
    glutTimerFunc(1, timer, 0);
    std::thread renderThread(glutMainLoop);
    std::cout << "Running " << (argc > 0 ? argv[0] : "program") << " on " << renderer << std::endl;

    algo.computeTarget();
    draw_lock.lock();
    targetDone = true;
    draw_lock.unlock();
    auto plots = algo.getPrettyTargetPCFplot(domainLength);

    draw_lock.lock();
    for(auto const & p : plots.second)
    {
        windows[PCF_ORIGINAL].plot->addDataPoints(windows[PCF_ORIGINAL].plot->getIdFromRelation(p.first), p.second);
    }
    addNewInstances(plots.first[tree_id], tree_id, windows[DISKS_ORIGINAL].scene, 1);
    addNewInstances(plots.first[grass_id], grass_id, windows[DISKS_ORIGINAL].scene, 1);
    addNewInstances(plots.first[mush_id], mush_id, windows[DISKS_ORIGINAL].scene, 1);
    draw_lock.unlock();


    algo.initialize(domainLength, 0.0001);
    plots = algo.getPrettyPCFplot(domainLength);

    draw_lock.lock();
    initDone = true;
    draw_lock.unlock();

    draw_lock.lock();
    //TODO refining
    auto end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    refineDone = true;
    draw_lock.unlock();

    renderThread.join();
}
