#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <map>
#include <string_view>

#include "util.hpp"

class Config
{
public:
    DLL_EXPORT static std::shared_ptr<Config> create(std::string_view cfg_path) noexcept;
    DLL_EXPORT static Config& instance();

    template<typename To = std::string_view>
    constexpr std::optional<To> get(std::string_view param_name) const
    {
        try {
            return util::string_to<To>(params_.at(param_name));
        }
        catch (...) {
            return std::nullopt;
        }
    }

    template<template<typename...> class Container, typename T = std::string_view>
    constexpr std::optional<Container<T>> get(std::string_view param_name) const
    {
        const auto v = get(param_name);
        if (!v.has_value()) {
            return std::nullopt;
        }
        Container<T> c{};
        c.reserve(std::ranges::count(*v, ',') + 1);
        for (const auto& elem : util::comma_iterator_adaptor(*v)) {
            c.emplace_back(util::string_to<T>(elem));
        }
        return c;
    }

private:
    DLL_EXPORT Config() = default;

private:
    bool is_initialized_ = false;
    std::string config_;
    std::map<std::string_view, std::string_view> params_;
};

#endif // CONFIG_HPP
