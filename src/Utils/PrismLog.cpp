#include "PrismLog.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include "spdlog/fmt/fmt.h"
#include <atomic>
#include <iostream>

class PrismLogger
{
  public:
    static auto instance() -> std::shared_ptr<spdlog::logger>&
    {
        static auto l = std::make_unique<PrismLogger>();
        return l->spdlogger;
    }

    PrismLogger()
    {
        constexpr auto loggerName = "PRISM";
        if ((spdlogger = spdlog::get(loggerName)) == nullptr)
        {
            spdlogger = spdlog::stderr_color_mt(loggerName);
            spdlogger->set_pattern("[%n] [%^%l%$] %v");
        }
    }

    std::shared_ptr<spdlog::logger> spdlogger;
};


namespace PrismLog
{
    auto enableDebug() -> void
    {
        PrismLogger::instance()->set_level(spdlog::level::debug);
    }

    auto info(const std::string &msg) -> void
    {
        PrismLogger::instance()->info(msg);
    }
    
    auto warn(const std::string &msg) -> void
    {
        PrismLogger::instance()->warn(msg);
    }
    
    auto error(const std::string& msg) -> void
    {
        PrismLogger::instance()->error(msg);
    }
    
    auto debug(const std::string &msg) -> void
    {
        PrismLogger::instance()->debug(msg);
    }
    
    auto fatal(const std::string &msg) -> void
    {
        PrismLogger::instance()->critical(msg);
        std::exit(EXIT_FAILURE);
    }

}; //end namespace PrismLog
