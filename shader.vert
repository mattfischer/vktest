#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTex;

layout(binding = 0) uniform UniformData { mat3 transformation; } uniformData;

void main() {    
    gl_Position = vec4(uniformData.transformation * position, 1.0);
    fragColor = color;
    fragTex = tex;
}