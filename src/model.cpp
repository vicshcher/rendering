#include <array>

#include "model.hpp"

namespace off {
namespace {

Position readVertex(std::string_view row)
{
    const auto delim1 = row.find_first_of(' ');
    const auto delim2 = delim1 + 1 + row.substr(delim1 + 1).find_first_of(' ');
    Position v{};
    v[0] = std::stof(row.substr(0, delim1).data());
    v[1] = std::stof(row.substr(delim1 + 1, delim2).data());
    v[2] = std::stof(row.substr(delim2 + 1).data());
    return v;
}

void readIndices(std::string_view row, std::vector<uint32_t>& indices)
{
    const auto delim1 = row.find_first_of(' ');
    const auto delim2 = delim1 + 1 + row.substr(delim1 + 1).find_first_of(' ');
    const auto delim3 = delim2 + 1 + row.substr(delim2 + 1).find_last_of(' ');

    indices.push_back(std::stoi(row.substr(delim1 + 1, delim2).data()));
    indices.push_back(std::stoi(row.substr(delim2 + 1, delim3).data()));
    indices.push_back(std::stoi(row.substr(delim3 + 1).data()));
}

} // namespace

DLL_EXPORT Model fromFile(std::string_view path)
{
    auto file_contains = util::read_file_contents(path);

    const auto begin_2nd_row = file_contains.find_first_of('\n') + 1;
    const auto end_2nd_row = file_contains.substr(begin_2nd_row + 1).find_first_of('\n') + 1;
    const auto second_row = file_contains.substr(begin_2nd_row, end_2nd_row);

    size_t vert_count = std::stoi(second_row.substr(0, second_row.find_first_of(' ')));
    size_t face_count = std::stoi(second_row.substr(second_row.find_first_of(' ') + 1, second_row.find_last_of(' ')));

    file_contains = file_contains.substr(begin_2nd_row + end_2nd_row + 1, std::string::npos);
    std::string_view file{ file_contains.begin(), file_contains.end() };

    Model result{};
    result.vertices.reserve(vert_count);
    result.indices.reserve(face_count * 3);

    size_t string_count = 0;
    for (size_t pos = 0; pos != std::string::npos; pos = file.find_first_of('\n'), ++string_count) {
        file = file.substr(pos + 1, std::string::npos);
        const auto row = file.substr(0, file.find_first_of('\n'));
        if (string_count < vert_count) {
            result.vertices.emplace_back(readVertex(row), White);
        }
        if (string_count >= vert_count && string_count < vert_count + face_count) {
            readIndices(row, result.indices);
        }
    }
    return result;
}

} // namespace off