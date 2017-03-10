#ifndef STGEN_TEXT_LOGGER_H
#define STGEN_TEXT_LOGGER_H

#include "STLogger.hpp"
#include "BarrierMerge.hpp"
#include "Sigil2/SigiLog.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/ostream_sink.h"
#include "zfstream.h"
#include <fstream>
#include <sstream>

using SigiLog::info;
namespace STGen
{

class TextLogger : public STLogger
{
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */
  public:
    TextLogger(TID tid, std::string outputPath);
    TextLogger(const TextLogger& other) = delete;
    ~TextLogger() override final;

    auto flush(const STCompEvent& ev, EID eid, TID tid) -> void override final;
    auto flush(const STCommEvent& ev, EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

    static auto flushPthread(std::string filePath,
                             ThreadList newThreadsInOrder,
                             SpawnList threadSpawns,
                             BarrierList barrierParticipants) -> void;

    static auto flushStats(std::string filePath, ThreadStatMap allThreadsStats) -> void;

  private:
    /* XXX REMEMBER: the file stream needs to be returned
     * with the logger to extend the life of the stream */
    static auto getFileLogger(std::string filePath)
        -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<std::ofstream>>;

    static auto getGzLogger(std::string filePath)
        -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<gzofstream>>;

    static auto blockingLoggerFlush(std::shared_ptr<spdlog::logger> &logger) -> void;

    std::string logMsg; // reuse to save on heap allocations space
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<gzofstream> gzfile;
};

}; //end namespace STGen

#endif
