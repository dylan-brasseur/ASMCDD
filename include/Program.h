//
// Created by Dylan Brasseur on 25/10/19.
//

#ifndef DISKSPROJECT_PROGRAM_H
#define DISKSPROJECT_PROGRAM_H

#include <string>
#include <GL/glew.h>
#include <map>
#include "Shader.h"

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
    [[nodiscard]] GLint getAttribLocation(const std::string& attrib_name) const;

private:
    std::string name;
    GLuint id;
    GLint linkStatus;
    std::vector<std::shared_ptr<Shader>> attachedShaders;
    static std::vector<std::shared_ptr<Program>> programList;
    std::map<std::string, GLint> attrib_locations;
};

#endif //DISKSPROJECT_PROGRAM_H
