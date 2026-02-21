#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aVel;
layout (location = 2) in float aDensity;

uniform mat4 projection;
uniform float pointSize;

out float vSpeed;
out float vDensity;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    
    vSpeed = length(aVel);
    vDensity = aDensity;
    
    gl_PointSize = pointSize;
}
