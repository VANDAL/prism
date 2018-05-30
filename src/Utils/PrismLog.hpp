#ifndef PRISM_LOG_H
#define PRISM_LOG_H

#include "spdlog/fmt/fmt.h"
#include <atomic>

#define FMT_LOG(name) \
    auto name(const std::string& msg) -> void;\
    template<typename... Args> \
    auto name(const char* s, const Args&... args) -> void { \
        name(fmt::format(s, args...)); \
    }

namespace PrismLog
{
    auto enableDebug() -> void;
    FMT_LOG(info);
    FMT_LOG(warn);
    FMT_LOG(error);
    FMT_LOG(debug);

    // to avoid noreturn warnings
    [[noreturn]] auto fatal(const std::string& msg) -> void;
    template<typename... Args>
    [[noreturn]] auto fatal(const char* s, const Args&... args) -> void {
        fatal(fmt::format(s, args...));
    }
};

#endif
