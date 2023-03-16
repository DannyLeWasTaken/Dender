#version 450
#pragma shader_stage(vertex)

out gl_PerVertex 
{
    vec4 gl_Position;
};

layout(binding = 0) uniform VP {
    mat4 view;
    mat4 projection;
};

layout(binding = 1) uniform Model {
    mat4 model;
};

layout (location = 0) out vec3 color;
layout (location = 0) in vec3 ipos;

void main() {
    gl_Position = projection * view * model * vec4(ipos, 1.0);
    color = vec3(1, 0, 0);
}
