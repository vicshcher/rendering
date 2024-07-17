#ifndef COMMAND_LINE_HANDLER_HPP
#define COMMAND_LINE_HANDLER_HPP

#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include "util.hpp"

class CommandLineHandler
{
public:
    DLL_EXPORT CommandLineHandler(int argc, char* argv[]);

    template<typename To>
    constexpr std::optional<To> get(std::string_view param_name) const
    {
        const auto v = operator[](param_name);
        if (!v.has_value()) {
            return std::nullopt;
        }
        return util::string_to<To>(*v);
    }

    DLL_EXPORT std::optional<std::string_view> operator[](std::string_view param_name) const;

private:
    DLL_EXPORT static std::vector<std::string_view> getArgs(int argc, char* argv[]);

private:
    std::map<std::string_view, std::string_view> args_;
};

#endif // COMMAND_LINE_HANDLER_HPP