#include "../include/constants.h"

layout(location = VERTEX_POSITION_LOCATION) in vec4 fPos;
layout(location = VERTEX_COLOR_LOCATION) in vec4 fColor;

layout(location = 0) out vec4 color;

void main()
{
    vec4 zero = vec4(0.0, 0.0, 0.0, 1.0);
    color = (distance(fPos, zero) / 100.0) * fColor;
}