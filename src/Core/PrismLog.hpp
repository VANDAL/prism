#ifndef PRISM_LOG_H
#define PRISM_LOG_H

#include "spdlog/spdlog.h"
#include <atomic>

namespace
{
/* TODO(cleanup) : singleton is used for cleaner interface to spdlog,
 * but it's a bit messy */
class PrismLogger
{
  public:
    static auto& instance()
    {
        static PrismLogger* l = new PrismLogger();
        return l->logger;
    }

  private:
    PrismLogger()
    {
        if ((logger = spdlog::get("PRISM")) == nullptr)
        {
            spdlog::set_sync_mode();
            if (isatty(fileno(stdout)))
                logger = spdlog::stderr_color_mt("PRISM");
            else
                logger = spdlog::stderr_logger_mt("PRISM");
            logger->set_pattern("[%n] [%l] %v");
        }
    }

    std::shared_ptr<spdlog::logger> logger;
};
}; // end namespace


namespace PrismLog
{
    inline auto enableDebug()
    {
        PrismLogger::instance()->set_level(spdlog::level::debug);
    }

    inline auto info(const std::string &msg)
    {
        PrismLogger::instance()->info(msg);
    }
    
    inline auto warn(const std::string &msg)
    {
        PrismLogger::instance()->warn(msg);
    }
    
    inline auto error(const std::string& msg)
    {
        PrismLogger::instance()->error(msg);
    }
    
    inline auto debug(const std::string &msg)
    {
        PrismLogger::instance()->debug(msg);
    }
    
    [[noreturn]]
    inline auto fatal(const std::string &msg)
    {
        PrismLogger::instance()->critical(msg);
        std::exit(EXIT_FAILURE);
    }
}; //end namespace PrismLog

#endif
