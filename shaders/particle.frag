#version 330 core

in float vSpeed;
in float vDensity;

out vec4 FragColor;

void main() {
    // Create circular particle
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    // Discard pixels outside the circle
    if (dist > 0.5) discard;
    
    // Smooth edge falloff
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    
    // Color based on speed — calm blue to energetic cyan/white
    float speedNorm = clamp(vSpeed * 2.0, 0.0, 1.0);
    
    // Deep ocean blue base
    vec3 calmColor = vec3(0.05, 0.15, 0.6);
    // Bright cyan for fast particles
    vec3 fastColor = vec3(0.2, 0.8, 1.0);
    // White for very fast particles
    vec3 veryFastColor = vec3(0.85, 0.95, 1.0);
    
    vec3 color;
    if (speedNorm < 0.5) {
        color = mix(calmColor, fastColor, speedNorm * 2.0);
    } else {
        color = mix(fastColor, veryFastColor, (speedNorm - 0.5) * 2.0);
    }
    
    // Inner glow — brighter in the center
    float glow = 1.0 - dist * 1.5;
    color += vec3(0.1, 0.2, 0.3) * glow;
    
    // Density-based opacity boost (denser areas look more opaque)
    float densityFactor = clamp(vDensity / 2000.0, 0.3, 1.0);
    alpha *= densityFactor;
    
    FragColor = vec4(color, alpha * 0.85);
}
