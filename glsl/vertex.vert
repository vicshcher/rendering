#include "../include/constants.h"

layout(location = VERTEX_POSITION_LOCATION) in vec3 vPosition;
layout(location = VERTEX_COLOR_LOCATION) in vec4 vColor;

layout(location = VERTEX_POSITION_LOCATION) out vec4 out_pos;
layout(location = VERTEX_COLOR_LOCATION) out vec4 out_color;

layout(binding = UNIFORM_BLOCK_BINDING) uniform UNIFORM_BUFFER_OBJECT
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

void main()
{
    out_color = vColor;
    out_pos = proj * view * model * vec4(vPosition, 1.0);
    gl_Position = out_pos;
}