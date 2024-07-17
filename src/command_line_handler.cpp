#include <ranges>

#include "command_line_handler.hpp"

DLL_EXPORT CommandLineHandler::CommandLineHandler(int argc, char* argv[])
{
    const auto args = getArgs(argc, argv);
    assert((args.size() - 1) % 2 == 0);
    for (const auto& [param, value] : args | std::views::drop(1) | std::views::pairwise) {
        if (param.starts_with("--")) {
            args_[param.substr(2)] = value;
        }
    }
}

DLL_EXPORT std::optional<std::string_view> CommandLineHandler::operator[](std::string_view param_name) const
{
    try {
        return args_.at(param_name);
    }
    catch (...) {
        return std::nullopt;
    }
}

DLL_EXPORT std::vector<std::string_view> CommandLineHandler::getArgs(int argc, char* argv[])
{
    std::vector<std::string_view> args{};
    args.reserve(argc);
    for (auto i = 0; i != argc; ++i) {
        if (std::strncmp(argv[i], "--", 2) == 0 || std::strncmp(argv[i], "-", 1) == 0) {
            continue;
        }
        args.emplace_back(argv[i]);
    }
    return args;
}