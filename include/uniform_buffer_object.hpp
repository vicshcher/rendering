#ifndef UNIFORM_BUFFER_OBJECT_HPP
#define UNIFORM_BUFFER_OBJECT_HPP

#include <glm/glm.hpp>

#include "constants.h"

struct UNIFORM_BUFFER_OBJECT
{
    alignas(16) glm::mat4 model = glm::mat4(1.0f);
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

using Position = glm::vec3;
using Color    = glm::vec4;

struct Vertex
{
    Position pos;
    Color    color;
};

#endif // UNIFORM_BUFFER_OBJECT_HPP