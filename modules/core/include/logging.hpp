#ifndef FADE_CORE_LOGGING_HPP_
#define FADE_CORE_LOGGING_HPP_

#include "core/include/type_definitions.hpp"

#include <iostream>
#include <string>

namespace fade {

enum class LogLevel : fade::uint8
{
    kDebug,
    kInfo,
    kWarning,
    kError,
    kCritical,
    kVerbose
};

template <LogLevel log_level, typename... Args>
void Log(std::string in_format_str, Args&&... in_args)
{
    auto get_log_level_str = [](LogLevel in_log_level) -> std::string
    {
        if constexpr (log_level == LogLevel::kDebug) return "[\033[47mDebug\033[0m]: ";
        else if constexpr (log_level == LogLevel::kInfo) return "[\033[32mInfo\033[0m]: ";
        else if constexpr (log_level == LogLevel::kWarning) return "[\033[33mInfo\033[0m]: ";
        else if constexpr (log_level == LogLevel::kError) return "[\033[41mError\033[0m]: ";
        else if constexpr (log_level == LogLevel::kCritical) return "[\033[41mCritical\033[0m]: ";
        else if constexpr (log_level == LogLevel::kVerbose) return "[\033[34mVerbose\033[0m]: ";
        else return "Unknown";
    };
    std::string combined_format_str = get_log_level_str(log_level) + in_format_str + "\n";
    std::string out = std::vformat(combined_format_str, std::make_format_args(in_args...));
    std::cout << out;
}

}

#endif