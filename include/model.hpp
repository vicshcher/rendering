#ifndef MODEL_HPP
#define MODEL_HPP

#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "uniform_buffer_object.hpp"
#include "util.hpp"

constexpr Color White = { 1.0, 1.0, 1.0, 1.0 };

struct Model
{
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
};

namespace off {

DLL_EXPORT Model fromFile(std::string_view path);

} // namespace off

#endif // MODEL_HPP