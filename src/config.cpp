#include "config.hpp"

DLL_EXPORT std::shared_ptr<Config> Config::create(std::string_view cfg_path) noexcept
{
    static std::shared_ptr<Config> config{ new Config{} };
    if (config && !config->is_initialized_) {
        config->config_ = util::read_file_contents(cfg_path);
        if (config->config_.empty()) {
            return config;
        }

        const auto& [b, e] = std::ranges::remove(config->config_, ' ');
        config->config_.erase(b, e);
        for (const auto& l : util::line_iterator_adaptor(config->config_)) {
            auto line = l.substr(0, l.find_first_of('#'));
            const auto splitter = std::ranges::find(line, '=');
            if (splitter == line.end()) {
                config.reset();
                return util::handle_error() << "missing '='";
            }
            if (splitter == line.end() - 1) {
                config.reset();
                return util::handle_error() << "missing value after '='";
            }
            config->params_[{ line.begin(), splitter }] = { splitter + 1, line.end() };
        }
        config->is_initialized_ = true;
    }
    if (!config) {
        return util::handle_error();
    }
    return config;
}

DLL_EXPORT Config& Config::instance()
{
    return *create("");
}