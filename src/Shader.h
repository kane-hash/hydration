#pragma once

#include <OpenGL/gl3.h>
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    GLuint ID;

    Shader() : ID(0) {}
    
    bool load(const std::string& vertexPath, const std::string& fragmentPath);
    
    void use() const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;
    void setFloat(const std::string& name, float value) const;
    
    ~Shader();

private:
    GLuint compileShader(GLenum type, const std::string& source);
    std::string readFile(const std::string& path);
};
