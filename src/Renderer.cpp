#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

Renderer::Renderer() {}

Renderer::~Renderer() {
    if (particleVAO) glDeleteVertexArrays(1, &particleVAO);
    if (particleVBO) glDeleteBuffers(1, &particleVBO);
    if (boxVAO) glDeleteVertexArrays(1, &boxVAO);
    if (boxVBO) glDeleteBuffers(1, &boxVBO);
    if (bgVAO) glDeleteVertexArrays(1, &bgVAO);
    if (bgVBO) glDeleteBuffers(1, &bgVBO);
}

void Renderer::setupParticleBuffers() {
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    
    // Position (vec2) + Velocity (vec2) + Density (float) = 5 floats per particle
    size_t stride = 5 * sizeof(float);
    
    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Velocity
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Density
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Renderer::setupBoxBuffers() {
    float min = Simulation::DOMAIN_MIN;
    float max = Simulation::DOMAIN_MAX;
    
    float boxVertices[] = {
        min, min,
        max, min,
        max, min,
        max, max,
        max, max,
        min, max,
        min, max,
        min, min
    };
    
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    
    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Renderer::setupBackground() {
    // Full-screen quad in NDC
    float bgVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), bgVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

bool Renderer::init(const std::string& shaderDir) {
    if (!particleShader.load(shaderDir + "/particle.vert", shaderDir + "/particle.frag")) {
        std::cerr << "Failed to load particle shaders!" << std::endl;
        return false;
    }
    
    // Create a simple line shader inline (minimal GLSL)
    // We'll reuse the particle shader's projection but with a solid color
    
    // For the box and background, we create minimal inline shaders
    std::string lineVert = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(aPos, 0.0, 1.0);
        }
    )";
    
    std::string lineFrag = R"(
        #version 330 core
        uniform vec3 lineColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(lineColor, 0.8);
        }
    )";
    
    // Compile line shader manually
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char* vsSrc = lineVert.c_str();
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fsSrc = lineFrag.c_str();
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    
    lineShader.ID = glCreateProgram();
    glAttachShader(lineShader.ID, vs);
    glAttachShader(lineShader.ID, fs);
    glLinkProgram(lineShader.ID);
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    // Background shader
    std::string bgVert = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        out vec2 uv;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            uv = aPos * 0.5 + 0.5;
        }
    )";
    
    std::string bgFrag = R"(
        #version 330 core
        in vec2 uv;
        out vec4 FragColor;
        void main() {
            // Dark gradient background (deep navy to dark blue)
            vec3 topColor = vec3(0.02, 0.03, 0.08);
            vec3 bottomColor = vec3(0.05, 0.07, 0.15);
            vec3 color = mix(bottomColor, topColor, uv.y);
            
            // Subtle vignette
            float vignette = 1.0 - length(uv - 0.5) * 0.5;
            color *= vignette;
            
            FragColor = vec4(color, 1.0);
        }
    )";
    
    GLuint bgvs = glCreateShader(GL_VERTEX_SHADER);
    const char* bgvsSrc = bgVert.c_str();
    glShaderSource(bgvs, 1, &bgvsSrc, nullptr);
    glCompileShader(bgvs);
    
    GLuint bgfs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* bgfsSrc = bgFrag.c_str();
    glShaderSource(bgfs, 1, &bgfsSrc, nullptr);
    glCompileShader(bgfs);
    
    bgShader.ID = glCreateProgram();
    glAttachShader(bgShader.ID, bgvs);
    glAttachShader(bgShader.ID, bgfs);
    glLinkProgram(bgShader.ID);
    glDeleteShader(bgvs);
    glDeleteShader(bgfs);
    
    setupParticleBuffers();
    setupBoxBuffers();
    setupBackground();
    
    // Enable point sprites
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    return true;
}

void Renderer::render(const Simulation& sim, int windowWidth, int windowHeight) {
    glViewport(0, 0, windowWidth, windowHeight);
    
    // Draw background
    glDisable(GL_BLEND);
    bgShader.use();
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    // Compute projection that maps [0,1]x[0,1] to clip space with some padding
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float padding = 0.05f;
    
    glm::mat4 projection;
    if (aspect >= 1.0f) {
        float halfW = (0.5f + padding) * aspect;
        float halfH = 0.5f + padding;
        projection = glm::ortho(0.5f - halfW, 0.5f + halfW, 
                                0.5f - halfH, 0.5f + halfH);
    } else {
        float halfW = 0.5f + padding;
        float halfH = (0.5f + padding) / aspect;
        projection = glm::ortho(0.5f - halfW, 0.5f + halfW, 
                                0.5f - halfH, 0.5f + halfH);
    }
    
    // Enable blending for particles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw bounding box
    lineShader.use();
    lineShader.setMat4("projection", projection);
    lineShader.setVec3("lineColor", glm::vec3(0.15f, 0.35f, 0.65f));
    
    glBindVertexArray(boxVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 8);
    
    // Upload particle data
    const auto& particles = sim.getParticles();
    int count = sim.getParticleCount();
    
    std::vector<float> data(count * 5);
    for (int i = 0; i < count; i++) {
        data[i * 5 + 0] = particles[i].position.x;
        data[i * 5 + 1] = particles[i].position.y;
        data[i * 5 + 2] = particles[i].velocity.x;
        data[i * 5 + 3] = particles[i].velocity.y;
        data[i * 5 + 4] = particles[i].density;
    }
    
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    
    // Draw particles with additive blending for glow effect
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    particleShader.use();
    particleShader.setMat4("projection", projection);
    
    // Point size relative to window
    float pointSize = std::max(4.0f, static_cast<float>(windowHeight) * 0.012f);
    particleShader.setFloat("pointSize", pointSize);
    
    glDrawArrays(GL_POINTS, 0, count);
    
    // Reset blend mode
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}
