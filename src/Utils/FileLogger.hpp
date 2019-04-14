#ifndef PRISM_FILE_LOGGER_H
#define PRISM_FILE_LOGGER_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "zfstream.h"
#include <fstream>

#include "Utils/PrismLog.hpp"
using PrismLog::info;
using PrismLog::fatal;

/* Convenience functions for setting up 'spdlog' loggers */

namespace prism
{

template<typename Factory = spdlog::default_factory, typename Stream = std::ofstream>
inline auto getFileLogger(std::string filePath)
    -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<Stream>>
{
    /* Create a file logger from a file path name
     *
     * Usage:
     * getFileLogger(filepath) -> returns a normal file logger
     * getFileLogger<spdlog::async_factory>(filepath) -> returns an asynchornous logger
     * getFileLogger<spdlog::async_factory, gzofstream>(filepath) -> returns
     *      a gzipped async logger
     * TODO(someday): tag dispatch to make customization a bit easier
     *
     *
     * NOTE: the file stream needs to be returned
     * with the logger to extend the life of the stream */
    auto file = std::make_shared<Stream>(filePath.c_str(), std::ios::trunc | std::ios::out);
    if (file->fail() == true)
        fatal("Failed to open: " + filePath);
    auto logger = Factory::template create<spdlog::sinks::ostream_sink_st>(filePath, *file);
    logger->set_pattern("%v");
    return std::make_pair(logger, file);
}

inline auto blockingFlushAndDeleteLogger(std::shared_ptr<spdlog::logger> &logger) -> void
{
    /* This function should be called on a logger when all logging is complete,
     * and the caller wants to clean up */

    logger->flush();

    /* NOTE: Make sure all other shared_ptrs are destroyed.
     * This will HANG otherwise. It's difficult to detect a hang due to the way
     * spdlog handles asynchronous loggers.
     * Normal synchronous loggers can just call flush normally and block.
     *
     * Async loggers push a message AND a shared_ptr of the logger to a thread
     * pool, for each log. As such, we wait for all messages to flush, detected
     * by the number of reference counts.
     *
     * Expect 2 reference counts in the end:
     * - the passed in reference
     * - spdlog global registry  */
    while (logger.use_count() > 2);

    /* Destructor calls a blocking flush in case of asynchronous logging. */
    spdlog::drop(logger->name()); // remove from global registry
    logger.reset(); // be explicit for clarity

    assert(logger.use_count() == 0);
}

}; //end namespace STGen

#endif
