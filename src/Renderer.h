#pragma once

#include <OpenGL/gl3.h>
#include "Shader.h"
#include "Simulation.h"
#include <glm/glm.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool init(const std::string& shaderDir);
    void render(const Simulation& sim, int windowWidth, int windowHeight);
    
private:
    Shader particleShader;
    Shader lineShader;
    
    // Particle rendering
    GLuint particleVAO = 0;
    GLuint particleVBO = 0;
    
    // Box rendering
    GLuint boxVAO = 0;
    GLuint boxVBO = 0;
    
    // Background rendering
    GLuint bgVAO = 0;
    GLuint bgVBO = 0;
    Shader bgShader;
    
    void setupParticleBuffers();
    void setupBoxBuffers();
    void setupBackground();
};
