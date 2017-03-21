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

class TextLoggerCompressed : public STLoggerCompressed
{
    using Base = STLoggerCompressed;
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */
  public:
    TextLoggerCompressed(TID tid, std::string outputPath);
    TextLoggerCompressed(const TextLoggerCompressed& other) = delete;
    ~TextLoggerCompressed() override final;

    auto flush(const STCompEventCompressed &ev, EID eid, TID tid) -> void override final;
    auto flush(const STCommEventCompressed &ev, EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    std::string logMsg; // reuse to save on heap allocations space
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<gzofstream> gzfile;
};


class TextLoggerUncompressed : public STLoggerUncompressed
{
    using Base = STLoggerCompressed;
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */
  public:
    TextLoggerUncompressed(TID tid, std::string outputPath);
    TextLoggerUncompressed(const TextLoggerUncompressed& other) = delete;
    ~TextLoggerUncompressed() override final;

    auto flush(StatCounter iops, StatCounter flops,
                       STCompEventUncompressed::MemType type, Addr start, Addr end,
                       EID eid, TID tid) -> void override final;
    auto flush(EID producerEID, TID producerTID, Addr start, Addr end,
                       EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, Addr syncAddr, EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    std::string logMsg; // reuse to save on heap allocations space
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<gzofstream> gzfile;
};


/* Helpers */
auto flushPthread(std::string filePath,
                  ThreadList newThreadsInOrder,
                  SpawnList threadSpawns,
                  BarrierList barrierParticipants) -> void;

auto flushStats(std::string filePath, ThreadStatMap allThreadsStats) -> void;

/* XXX REMEMBER: the file stream needs to be returned
 * with the logger to extend the life of the stream */
auto getFileLogger(std::string filePath)
    -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<std::ofstream>>;

auto getGzLogger(std::string filePath)
    -> std::pair<std::shared_ptr<spdlog::logger>, std::shared_ptr<gzofstream>>;

auto blockingLoggerFlush(std::shared_ptr<spdlog::logger> &logger) -> void;

}; //end namespace STGen

#endif
