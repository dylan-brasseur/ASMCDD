//
// Created by Dylan Brasseur on 25/10/19.
//

#ifndef DISKSPROJECT_PROGRAM_H
#define DISKSPROJECT_PROGRAM_H

#include <string>
#include <GL/glew.h>
#include <map>
#include <iostream>
#include "Shader.h"

/**
 * This class manages a program for OpenGL
 */
class Program{
public:
    explicit Program(std::string const & name="");
    ~Program();
    static std::shared_ptr<Program>& createProgram(const std::string & name);
    static std::vector<std::shared_ptr<Program>>& getProgramList();
    static bool linkAll();
    GLuint use() const;
    void attach(std::shared_ptr<Shader> & s);
    void attach(unsigned int shaderlist_id);
    void detach(std::shared_ptr<Shader> & s);
    void detach(unsigned int shaderlist_id);
    bool link();

    bool addAttribLocation(const std::string& attrib_name);
    template<class = void>
    static bool addAttribLocations(){
        return true;
    }
    template<typename Arg>
    bool addAttribLocations(Arg value){
        return addAttribLocation(value);
    }
    template<typename Arg, typename ... Args>
    bool addAttribLocations(Arg head, Args ... args){
        return addAttribLocation(head) && addAttribLocations(args...);
    }
    [[nodiscard]] GLint getAttribLocation(const std::string& attrib_name) const;

    bool addUniformLocation(std::string const & attrib_name);
    /*template<class = void>
    static bool addUniformLocations(){
        return true;
    }*/
    template<typename Arg>
    bool addUniformLocations(Arg value){
        return addUniformLocation(value);
    }
    template<typename Arg, typename ... Args>
    bool addUniformLocations(Arg head, Args ... args){
        return addUniformLocation(head) && addUniformLocations(args...);
    }
    [[nodiscard]] GLint getUniformLocation(const std::string& attrib_name) const;

private:
    std::string name;
    GLuint id;
    GLint linkStatus;
    std::vector<std::shared_ptr<Shader>> attachedShaders;
    static std::vector<std::shared_ptr<Program>> programList;
    std::map<std::string, GLint> attrib_locations;
    std::map<std::string, GLint> uniform_locations;
};

#endif //DISKSPROJECT_PROGRAM_H
