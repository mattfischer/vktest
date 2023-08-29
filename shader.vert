#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformData { uint frame; } uniformData;

void main() {
    uint frame = uniformData.frame;
    float theta = 2*3.14*frame / 100;
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    
    vec3 pos = position;
    vec3 posTransformed = vec3(pos.x * cosTheta - pos.y * sinTheta, pos.y * cosTheta + pos.x * sinTheta, pos.z);
    gl_Position = vec4(posTransformed, 1.0);
    fragColor = color;
}