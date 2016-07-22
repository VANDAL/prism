#ifndef SGL_LOG_H
#define SGL_LOG_H

#define SGL_DEBUG

#include "spdlog.h"
/* TODO verify static initialization fiasco won't be a problem. 
 * What happens when spdlog's registry is destructed? */

namespace
{
static constexpr char header[] = "[Sigil2]";
std::string color(const char *text, const char *color)
{
    static std::map<std::string, std::string> ANSIcolors_fg =
    {
        {"black", "\033[30m"},
        {"red", "\033[31m"},
        {"green", "\033[32m"},
        {"yellow", "\033[33m"},
        {"blue", "\033[34m"},
        {"magenta", "\033[35m"},
        {"cyan", "\033[36m"},
        {"white", "\033[37m"},
        {"end", "\033[0m"},
    };

    std::string ret(text);

    if (isatty(fileno(stdout)))
    {
        ret = std::string(ANSIcolors_fg[color]).append(text).append(ANSIcolors_fg["end"]);
    }

    return ret;
};
};

struct SigiLog
{
    static void info(const std::string &msg)
    {
        static std::mutex mutex_;
        std::lock_guard<std::mutex> lock(mutex_);

        static std::shared_ptr<spdlog::logger> info_ = nullptr;
        if (info_ == nullptr)
        {
            spdlog::set_sync_mode();
            info_ = spdlog::stderr_logger_st("sigil2-console");
            auto info = "[" + color("INFO", "blue") + "]";
            info_->set_pattern(header + info + "  %v");
        }
    
        info_->info(msg);
    }
    
    
    static void warn(const std::string &msg)
    {
        static std::mutex mutex_;
        std::lock_guard<std::mutex> lock(mutex_);

        static std::shared_ptr<spdlog::logger> warn_ = nullptr;
        if (warn_ == nullptr)
        {
            spdlog::set_sync_mode();
            warn_ = spdlog::stderr_logger_st("sigil2-warn");
            auto warn = "[" + color("WARN", "yellow") + "]";
            warn_->set_pattern(header + warn + "  %v");
        }
    
        warn_->warn(msg);
    }
    
    
    static void error(const std::string &msg)
    {
        static std::mutex mutex_;
        std::lock_guard<std::mutex> lock(mutex_);

        static std::shared_ptr<spdlog::logger> error_ = nullptr;
        if (error_ == nullptr)
        {
            spdlog::set_sync_mode();
            error_ = spdlog::stderr_logger_st("sigil2-error");
            auto error = "[" + color("ERROR", "red") + "]";
            error_->set_pattern(header + error + "  %v");
        }
    
        error_->error(msg);
    }
    
    
    static void debug(const std::string &msg)
    {
    #ifdef SGL_DEBUG
        static std::mutex mutex_;
        std::lock_guard<std::mutex> lock(mutex_);

        static std::shared_ptr<spdlog::logger> debug_ = nullptr;
        if (debug_ == nullptr)
        {
            spdlog::set_sync_mode();
            debug_ = spdlog::stderr_logger_st("sigil2-debug");
            auto debug = "[" + color("DEBUG", "magenta") + "]";
            debug_->set_pattern(header + debug + "  %v");
            debug_->set_level(spdlog::level::debug);
        }
    
        debug_->debug(msg);
    #endif
    }
    
    [[noreturn]] static void fatal(const std::string &msg)
    {
        error(std::string("Fatal: ").append(msg));
        std::exit(EXIT_FAILURE);
    }
};

#endif
