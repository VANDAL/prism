#ifndef SIGIL2_LOG_H
#define SIGIL2_LOG_H

#include "spdlog/spdlog.h"
#include <atomic>

namespace
{
/* TODO(cleanup) : singleton is used for cleaner interface to spdlog,
 * but it's a bit messy */
class SigilLogger
{
  public:
    static std::shared_ptr<spdlog::logger>& instance()
    {
        static SigilLogger* l = new SigilLogger();
        return l->logger;
    }

  private:
    SigilLogger()
    {
        if ((logger = spdlog::get("SIGIL2")) == nullptr)
        {
            spdlog::set_sync_mode();
            if (isatty(fileno(stdout)))
                logger = spdlog::stderr_color_mt("SIGIL2");
            else
                logger = spdlog::stderr_logger_mt("SIGIL2");
            logger->set_pattern("[%n] [%l] %v");
        }
    }

    std::shared_ptr<spdlog::logger> logger;
};
}; // end namespace


namespace SigiLog
{
    inline auto enableDebug() -> void
    {
        SigilLogger::instance()->set_level(spdlog::level::debug);
    }

    inline auto info(const std::string &msg) -> void
    {
        SigilLogger::instance()->info(msg);
    }
    
    inline auto warn(const std::string &msg) -> void
    {
        SigilLogger::instance()->warn(msg);
    }
    
    inline auto error(const std::string& msg) -> void
    {
        SigilLogger::instance()->error(msg);
    }
    
    inline auto debug(const std::string &msg) -> void
    {
        SigilLogger::instance()->debug(msg);
    }
    
    [[noreturn]]
    inline auto fatal(const std::string &msg) -> void
    {
        SigilLogger::instance()->critical(msg);
        std::exit(EXIT_FAILURE);
    }
}; //end namespace SigiLog

#endif
