#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR::SHADER: Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertSrc = readFile(vertexPath);
    std::string fragSrc = readFile(fragmentPath);
    
    if (vertSrc.empty() || fragSrc.empty()) return false;
    
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    
    if (!vert || !frag) return false;
    
    ID = glCreateProgram();
    glAttachShader(ID, vert);
    glAttachShader(ID, frag);
    glLinkProgram(ID);
    
    GLint success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(ID, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::LINKING\n" << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vert);
    glDeleteShader(frag);
    
    return true;
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &vec[0]);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

Shader::~Shader() {
    if (ID) glDeleteProgram(ID);
}
