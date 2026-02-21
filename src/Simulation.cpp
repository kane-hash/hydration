#include "Simulation.h"
#include <cmath>
#include <random>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Simulation::Simulation(int numParticles) {
    // Precompute kernel coefficients
    float h = smoothingRadius;
    poly6Coeff = 4.0f / (static_cast<float>(M_PI) * std::pow(h, 8.0f));
    spikyGradCoeff = -10.0f / (static_cast<float>(M_PI) * std::pow(h, 5.0f));
    viscLaplCoeff = 40.0f / (static_cast<float>(M_PI) * std::pow(h, 5.0f));
    
    // Compute particle mass from rest density
    // Approximate: mass = restDensity * volume / numParticles
    float volume = (DOMAIN_MAX - DOMAIN_MIN) * (DOMAIN_MAX - DOMAIN_MIN);
    particleMass = restDensity * volume / static_cast<float>(numParticles);
    
    particles.resize(numParticles);
    reset();
}

void Simulation::reset() {
    int n = static_cast<int>(particles.size());
    int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(n) * 0.8f)));
    int rows = (n + cols - 1) / cols;
    
    // Place particles in a block in the upper portion of the domain
    float startX = 0.15f;
    float startY = 0.45f;
    float spacingX = 0.7f / static_cast<float>(cols);
    float spacingY = 0.5f / static_cast<float>(rows);
    
    // Small jitter for natural initialization
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> jitter(-0.002f, 0.002f);
    
    for (int i = 0; i < n; i++) {
        int col = i % cols;
        int row = i / cols;
        
        particles[i].position = glm::vec2(
            startX + col * spacingX + jitter(rng),
            startY + row * spacingY + jitter(rng)
        );
        particles[i].velocity = glm::vec2(0.0f);
        particles[i].force = glm::vec2(0.0f);
        particles[i].density = restDensity;
        particles[i].pressure = 0.0f;
    }
}

Simulation::CellKey Simulation::getCellKey(const glm::vec2& pos) const {
    return {
        static_cast<int>(std::floor(pos.x / smoothingRadius)),
        static_cast<int>(std::floor(pos.y / smoothingRadius))
    };
}

void Simulation::buildGrid() {
    grid.clear();
    for (int i = 0; i < static_cast<int>(particles.size()); i++) {
        CellKey key = getCellKey(particles[i].position);
        grid[key].push_back(i);
    }
}

void Simulation::computeDensityPressure() {
    float h2 = smoothingRadius * smoothingRadius;
    
    for (auto& p : particles) {
        p.density = 0.0f;
    }
    
    for (int i = 0; i < static_cast<int>(particles.size()); i++) {
        CellKey myCell = getCellKey(particles[i].position);
        
        // Search 3x3 neighborhood
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                CellKey neighborCell = {myCell.x + dx, myCell.y + dy};
                auto it = grid.find(neighborCell);
                if (it == grid.end()) continue;
                
                for (int j : it->second) {
                    glm::vec2 diff = particles[i].position - particles[j].position;
                    float r2 = glm::dot(diff, diff);
                    
                    if (r2 < h2) {
                        // Poly6 kernel
                        float w = poly6Coeff * std::pow(h2 - r2, 3.0f);
                        particles[i].density += particleMass * w;
                    }
                }
            }
        }
        
        // Ensure minimum density
        particles[i].density = std::max(particles[i].density, restDensity * 0.1f);
        
        // Tait equation of state
        float ratio = particles[i].density / restDensity;
        particles[i].pressure = gasConstant * (ratio * ratio * ratio * ratio * ratio * ratio * ratio - 1.0f);
    }
}

void Simulation::computeForces() {
    float h = smoothingRadius;
    float h2 = h * h;
    
    for (auto& p : particles) {
        p.force = glm::vec2(0.0f);
    }
    
    for (int i = 0; i < static_cast<int>(particles.size()); i++) {
        CellKey myCell = getCellKey(particles[i].position);
        
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                CellKey neighborCell = {myCell.x + dx, myCell.y + dy};
                auto it = grid.find(neighborCell);
                if (it == grid.end()) continue;
                
                for (int j : it->second) {
                    if (i == j) continue;
                    
                    glm::vec2 diff = particles[i].position - particles[j].position;
                    float r2 = glm::dot(diff, diff);
                    
                    if (r2 < h2 && r2 > 1e-12f) {
                        float r = std::sqrt(r2);
                        glm::vec2 dir = diff / r;
                        
                        // Pressure force (Spiky kernel gradient)
                        float pressureForce = -particleMass * 
                            (particles[i].pressure + particles[j].pressure) / 
                            (2.0f * particles[j].density) *
                            spikyGradCoeff * std::pow(h - r, 2.0f);
                        
                        particles[i].force += pressureForce * dir;
                        
                        // Viscosity force (Viscosity kernel Laplacian)
                        float viscForce = viscosity * particleMass *
                            (1.0f / particles[j].density) *
                            viscLaplCoeff * (h - r);
                        
                        particles[i].force += viscForce * (particles[j].velocity - particles[i].velocity);
                    }
                }
            }
        }
        
        // Gravity
        particles[i].force += glm::vec2(0.0f, gravity) * particles[i].density;
    }
}

void Simulation::integrate(float dt) {
    for (auto& p : particles) {
        // Semi-implicit Euler
        p.velocity += dt * p.force / p.density;
        
        // Clamp velocity for stability
        float speed = glm::length(p.velocity);
        if (speed > 5.0f) {
            p.velocity = (p.velocity / speed) * 5.0f;
        }
        
        p.position += dt * p.velocity;
    }
}

void Simulation::enforceBoundary() {
    float margin = 0.005f;
    
    for (auto& p : particles) {
        // Bottom
        if (p.position.y < DOMAIN_MIN + margin) {
            p.position.y = DOMAIN_MIN + margin;
            p.velocity.y *= boundaryDamping;
        }
        // Top
        if (p.position.y > DOMAIN_MAX - margin) {
            p.position.y = DOMAIN_MAX - margin;
            p.velocity.y *= boundaryDamping;
        }
        // Left
        if (p.position.x < DOMAIN_MIN + margin) {
            p.position.x = DOMAIN_MIN + margin;
            p.velocity.x *= boundaryDamping;
        }
        // Right
        if (p.position.x > DOMAIN_MAX - margin) {
            p.position.x = DOMAIN_MAX - margin;
            p.velocity.x *= boundaryDamping;
        }
    }
}

void Simulation::update(float dt) {
    // Sub-step for stability
    int substeps = 4;
    float subDt = dt / static_cast<float>(substeps);
    
    for (int s = 0; s < substeps; s++) {
        buildGrid();
        computeDensityPressure();
        computeForces();
        integrate(subDt);
        enforceBoundary();
    }
}

void Simulation::addForce(float x, float y, float radius, float strength) {
    for (auto& p : particles) {
        glm::vec2 diff = p.position - glm::vec2(x, y);
        float dist = glm::length(diff);
        if (dist < radius && dist > 1e-6f) {
            float factor = 1.0f - dist / radius;
            p.velocity += (diff / dist) * strength * factor;
        }
    }
}

void Simulation::applyCursorForce(float x, float y, bool attract) {
    glm::vec2 cursorPos(x, y);
    float radius = CURSOR_RADIUS;
    
    for (auto& p : particles) {
        glm::vec2 diff = p.position - cursorPos;
        float dist = glm::length(diff);
        
        if (dist < radius && dist > 1e-6f) {
            glm::vec2 dir = diff / dist;
            // Smooth cubic falloff
            float t = 1.0f - dist / radius;
            float factor = t * t * t;
            
            if (attract) {
                // Click: pull particles toward cursor
                p.velocity -= dir * factor * 3.0f;
            } else {
                // Hover: gently repel particles
                p.velocity += dir * factor * 5.0f;
            }
        }
    }
}

void Simulation::toggleGravity() {
    gravity = -gravity;
}
