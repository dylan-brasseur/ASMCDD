//
// Created by Dylan Brasseur on 25/10/19.
//
#include <algorithm>
#include "include/Program.h"
#include "include/utils.h"

std::vector<std::shared_ptr<Program>> Program::programList;

Program::Program(std::string const & name){
    this->name = name;
    linkStatus=GL_FALSE;
    id=glCreateProgram();TEST_OPENGL_ERROR();
    std::cout << "Added program " <<id << ' '<< name << std::endl;
}

GLuint Program::use() const{
    glUseProgram(id);
    return id;
}

void Program::attach(std::shared_ptr<Shader> & s){
    glAttachShader(id, s->getId());TEST_OPENGL_ERROR();
    attachedShaders.push_back(s);
}

void Program::attach(unsigned int shaderlist_id){
    auto s = Shader::getShaderList().at(shaderlist_id);
    attach(s);
}

void Program::detach(std::shared_ptr<Shader> & s){
    glDetachShader(id, s->getId());TEST_OPENGL_ERROR();
    auto place = std::find(attachedShaders.begin(), attachedShaders.end(), s);
    if(place != attachedShaders.end()) attachedShaders.erase(place);
}

void Program::detach(unsigned int shaderlist_id){
    auto s = Shader::getShaderList().at(shaderlist_id);
    detach(s);
}

bool Program::link(){
    glLinkProgram(id);TEST_OPENGL_ERROR();
    glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
    if (linkStatus!=GL_TRUE) {
        GLint log_size;
        char *program_log;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_size);
        program_log = (char*)std::malloc(log_size+1); /* +1 pour le caractere de fin de chaine '\0' */
        if(program_log != nullptr) {
            glGetProgramInfoLog(id, log_size, &log_size, program_log);
            std::cerr << "Program " << name << " : " << program_log << std::endl;
            std::free(program_log);
        }
        return false;
    }
    return true;
}

Program::~Program(){
    if(id > 0)
    {
        for(auto & s : attachedShaders)
        {
            glDetachShader(id, s->getId());
        }
        glDeleteProgram(id);
        std::cout << "Deleted program "<<id << ' ' << name << std::endl;
    }
}

std::shared_ptr<Program> &Program::createProgram(const std::string &_name){
    programList.push_back(std::make_shared<Program>(_name));
    return programList[programList.size()-1];
}

std::vector<std::shared_ptr<Program>> &Program::getProgramList(){
    return programList;
}

bool Program::linkAll(){
    for(auto & p : programList)
    {
        if(!p->link())
            return false;
    }
    return true;
}

bool Program::addAttribLocation(const std::string& attrib_name){
    const GLint location = glGetAttribLocation(id, attrib_name.c_str());TEST_OPENGL_ERROR();
    attrib_locations.insert_or_assign(attrib_name, location);
    return location!=-1;
}

GLint Program::getAttribLocation(const std::string &attrib_name) const{
    return attrib_locations.at(attrib_name);
}

bool Program::addUniformLocation(const std::string &attrib_name){
    const GLint location = glGetUniformLocation(id, attrib_name.c_str());TEST_OPENGL_ERROR();
    uniform_locations.insert_or_assign(attrib_name, location);
    return location!=-1;
}

GLint Program::getUniformLocation(const std::string &attrib_name) const{
    return uniform_locations.at(attrib_name);
}

