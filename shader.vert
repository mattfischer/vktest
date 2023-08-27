#version 450

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformData { uint frame; } uniformData;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    uint frame = uniformData.frame;
    float theta = 2*3.14*frame / 100;
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    
    vec2 pos = positions[gl_VertexIndex];
    vec2 posTransformed = vec2(pos.x * cosTheta - pos.y * sinTheta, pos.y * cosTheta + pos.x * sinTheta);
    gl_Position = vec4(posTransformed, 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}