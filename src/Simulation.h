#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>
#include <cstdint>

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 force;
    float density;
    float pressure;
};

class Simulation {
public:
    Simulation(int numParticles = 2000);
    
    void update(float dt);
    void reset();
    void addForce(float x, float y, float radius, float strength);
    void applyCursorForce(float x, float y, bool attract);
    void toggleGravity();
    
    static constexpr float CURSOR_RADIUS = 0.18f;
    
    const std::vector<Particle>& getParticles() const { return particles; }
    int getParticleCount() const { return static_cast<int>(particles.size()); }
    
    // Simulation domain [0, 1] x [0, 1]
    static constexpr float DOMAIN_MIN = 0.0f;
    static constexpr float DOMAIN_MAX = 1.0f;

private:
    std::vector<Particle> particles;
    
    // SPH parameters
    float smoothingRadius = 0.04f;        // h
    float restDensity = 1000.0f;          // ρ₀
    float gasConstant = 2000.0f;          // k
    float viscosity = 250.0f;             // μ
    float gravity = -9.81f;               // g (sign indicates direction)
    float boundaryDamping = -0.5f;
    float particleMass = 1.0f;
    
    // Kernel precomputed constants
    float poly6Coeff;
    float spikyGradCoeff;
    float viscLaplCoeff;
    
    // Spatial hashing
    struct CellKey {
        int x, y;
        bool operator==(const CellKey& other) const {
            return x == other.x && y == other.y;
        }
    };
    
    struct CellKeyHash {
        std::size_t operator()(const CellKey& k) const {
            return std::hash<int64_t>()(static_cast<int64_t>(k.x) * 73856093 ^ static_cast<int64_t>(k.y) * 19349663);
        }
    };
    
    std::unordered_map<CellKey, std::vector<int>, CellKeyHash> grid;
    
    void buildGrid();
    CellKey getCellKey(const glm::vec2& pos) const;
    void computeDensityPressure();
    void computeForces();
    void integrate(float dt);
    void enforceBoundary();
};
