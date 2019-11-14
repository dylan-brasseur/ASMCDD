//
// Created by Dylan Brasseur on 23/10/19.
//

#include "include/Shader.h"
#include "include/utils.h"

std::vector<std::shared_ptr<Shader>> Shader::shaderList;

Shader::Shader(GLenum type, std::string const & filename){
    compile_status=GL_FALSE;
    this->filename = filename;
    this->type=type;
    id = glCreateShader(type);TEST_OPENGL_ERROR();
    std::cout << "Added shader " << id << std::endl;
}

bool Shader::compile()
{
    if(id <= 0) //Erreur lors de la création du shader
        return false;
    std::string content = load(filename);
    const char* source = content.c_str();
    glShaderSource(id, 1, (const GLchar**)&(source), nullptr);TEST_OPENGL_ERROR();
    glCompileShader(id);TEST_OPENGL_ERROR();
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
    if(compile_status != GL_TRUE)
    {
        GLint log_size;
        char *shader_log;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_size);
        shader_log = (char*)std::malloc(log_size+1);
        if(shader_log != nullptr) {
            glGetShaderInfoLog(id, log_size, &log_size, shader_log);
            std::cerr << getShaderName(type) << " @ " << filename << " : " << shader_log << std::endl;
            std::free(shader_log);
        }
        return false;
    }
    return true;
}

Shader::~Shader()
{
    if(id > 0)
    {
        glDeleteShader(id);
        std::cout << "Deleted shader " << id << std::endl;
    }

}

GLuint Shader::getId() const{
    return id;
}

GLint Shader::getCompileStatus() const{
    return compile_status;
}

const std::string &Shader::getFilename() const{
    return filename;
}

GLenum Shader::getType() const{
    return type;
}

void Shader::setFilename(const std::string &_filename){
    filename = _filename;
}

std::shared_ptr<Shader>& Shader::createShader(GLenum type, const std::string &filename){
    shaderList.push_back(std::make_unique<Shader>(type, filename));
    return shaderList[shaderList.size()-1];
}

std::vector<std::shared_ptr<Shader>>& Shader::getShaderList(){
    return shaderList;
}

bool Shader::compileAll(){
    for(auto & s : Shader::getShaderList())
    {
        if(!s->compile())
            return false;
    }
    return true;
}

bool Shader::operator==(const Shader &s){
    return id==s.id;
}
