#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

/**
layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer ReadVec4
{
  vec4 values[];
};
**/

void main()
{
  outFragColor = vec4(inColor, 1.0);
}