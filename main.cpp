#include <iostream>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include "src/Simulation.h"
#include "src/Renderer.h"

// --- Globals for callbacks ---
static Simulation* g_sim = nullptr;
static bool g_mouseDown = false;
static double g_mouseX = 0.0, g_mouseY = 0.0;
static int g_winW = 1200, g_winH = 800;

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;
    
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_SPACE:
            if (g_sim) g_sim->reset();
            std::cout << "[Hydration] Simulation reset" << std::endl;
            break;
        case GLFW_KEY_G:
            if (g_sim) g_sim->toggleGravity();
            std::cout << "[Hydration] Gravity toggled" << std::endl;
            break;
    }
}

void mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        g_mouseDown = (action == GLFW_PRESS);
    }
}

void cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    g_mouseX = xpos;
    g_mouseY = ypos;
}

void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    g_winW = width;
    g_winH = height;
}

// Convert screen coords to simulation domain [0,1]x[0,1]
glm::vec2 screenToSim(double sx, double sy) {
    // Account for the same padding/aspect used in Renderer
    float aspect = static_cast<float>(g_winW) / static_cast<float>(g_winH);
    float padding = 0.05f;
    
    float normX = static_cast<float>(sx) / static_cast<float>(g_winW);
    float normY = 1.0f - static_cast<float>(sy) / static_cast<float>(g_winH); // flip Y
    
    float simX, simY;
    if (aspect >= 1.0f) {
        float halfW = (0.5f + padding) * aspect;
        float halfH = 0.5f + padding;
        simX = (normX - 0.5f) * 2.0f * halfW + 0.5f;
        simY = (normY - 0.5f) * 2.0f * halfH + 0.5f;
    } else {
        float halfW = 0.5f + padding;
        float halfH = (0.5f + padding) / aspect;
        simX = (normX - 0.5f) * 2.0f * halfW + 0.5f;
        simY = (normY - 0.5f) * 2.0f * halfH + 0.5f;
    }
    
    return glm::vec2(simX, simY);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Request OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
    
    GLFWwindow* window = glfwCreateWindow(g_winW, g_winH, "Hydration Physics", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync
    
    // Set callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    // Get actual framebuffer size (Retina)
    glfwGetFramebufferSize(window, &g_winW, &g_winH);
    
    // Enable multisampling
    glEnable(GL_MULTISAMPLE);
    
    // Create simulation & renderer
    Simulation sim(2000);
    g_sim = &sim;
    
    Renderer renderer;
    if (!renderer.init("shaders")) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "=== Hydration Physics Simulation ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Mouse click/drag - Push particles" << std::endl;
    std::cout << "  Space            - Reset simulation" << std::endl;
    std::cout << "  G                - Toggle gravity" << std::endl;
    std::cout << "  Escape           - Quit" << std::endl;
    std::cout << "====================================" << std::endl;
    
    double lastTime = glfwGetTime();
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        
        // Clamp dt for stability (e.g., when window is being moved)
        dt = std::min(dt, 0.02f);
        
        // Compute cursor position in simulation space
        glm::vec2 cursorSim = screenToSim(g_mouseX, g_mouseY);
        
        // Cursor always repels particles wherever it touches
        sim.applyCursorForce(cursorSim.x, cursorSim.y, false);
        
        // Update simulation
        sim.update(dt);
        
        // Get framebuffer size for rendering
        glfwGetFramebufferSize(window, &g_winW, &g_winH);
        
        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.render(sim, g_winW, g_winH);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    g_sim = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
