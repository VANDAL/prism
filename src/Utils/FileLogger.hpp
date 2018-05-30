#ifndef PRISM_FILE_LOGGER_H
#define PRISM_FILE_LOGGER_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/ostream_sink.h"
#include "zfstream.h"
#include <fstream>

#include "Utils/PrismLog.hpp"
using PrismLog::info;
using PrismLog::fatal;

/* Convenience functions for setting up 'spdlog' loggers */

namespace prism
{

inline auto getFileLogger(std::string filePath)
    -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<std::ofstream>>
{
    /* Create a text file logger from a file path name
     *
     * XXX: the file stream needs to be returned
     * with the logger to extend the life of the stream */
    auto file = std::make_shared<std::ofstream>(filePath, std::ios::trunc | std::ios::out);
    if (file->fail() == true)
        fatal("Failed to open: " + filePath);
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(*file);
    auto logger = spdlog::create(filePath, {sink});
    logger->set_pattern("%v");
    return std::make_pair(logger, file);
}

inline auto getGzLogger(std::string filePath)
    -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<gzofstream>>
{
    /* Create a gzipped text file logger from a file path name
     *
     * XXX: the file stream needs to be returned
     * with the logger to extend the life of the stream */
    auto gzfile = std::make_shared<gzofstream>(filePath.c_str(), std::ios::trunc | std::ios::out);
    if (gzfile->fail() == true)
        fatal("Failed to open: " + filePath);
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(*gzfile);
    auto logger = spdlog::create(filePath, {sink});
    logger->set_pattern("%v");
    return std::make_pair(logger, gzfile);
}

inline auto blockingFlushAndDeleteLogger(std::shared_ptr<spdlog::logger> &logger) -> void
{
    /* This function should be called on a logger when all logging is complete,
     * and the caller wants to clean up */

    /* Make sure all other shared_ptrs are destroyed, as well.
     * Might be annoying to throw here, but it'll save debugging headaches.
     * Expect 2 reference counts:
     * - the passed in reference
     * - spdlog global registry  */
    if (logger.use_count() > 2)
        throw std::invalid_argument("tried to flush spdlog logger "
                                    "while still in use somewhere: " + logger->name());

    /* Destructor calls a blocking flush in case of asynchronous logging. */
    logger->flush();
    spdlog::drop(logger->name()); // remove from global registry
    logger.reset(); // be explicit for clarity
}

}; //end namespace STGen

#endif
