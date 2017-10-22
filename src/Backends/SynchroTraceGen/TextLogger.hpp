#ifndef STGEN_TEXT_LOGGER_H
#define STGEN_TEXT_LOGGER_H

#include "Core/SigiLog.hpp"
#include "Utils/FileLogger.hpp"
#include "STLogger.hpp"
#include "BarrierMerge.hpp"
#include "spdlog/spdlog.h"
#include <sstream>

using SigiLog::info;
namespace STGen
{

class TextLoggerCompressed : public STLoggerCompressed
{
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */

    using Base = STLoggerCompressed;
  public:
    TextLoggerCompressed(TID tid, std::string outputPath);
    TextLoggerCompressed(const TextLoggerCompressed& other) = delete;
    ~TextLoggerCompressed() override final;

    auto flush(const STCompEventCompressed &ev, EID eid, TID tid) -> void override final;
    auto flush(const STCommEventCompressed &ev, EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
               EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    std::string logMsg; // reuse to save on heap allocations space
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<gzofstream> gzfile;
};


class TextLoggerUncompressed : public STLoggerUncompressed
{
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */

    using Base = STLoggerCompressed;
  public:
    TextLoggerUncompressed(TID tid, std::string outputPath);
    TextLoggerUncompressed(const TextLoggerUncompressed& other) = delete;
    ~TextLoggerUncompressed() override final;

    auto flush(StatCounter iops, StatCounter flops,
                       STCompEventUncompressed::MemType type, Addr start, Addr end,
                       EID eid, TID tid) -> void override final;
    auto flush(EID producerEID, TID producerTID, Addr start, Addr end,
                       EID eid, TID tid) -> void override final;
    auto flush(unsigned char syncType, unsigned numArgs, Addr *syncArgs,
               EID eid, TID tid) -> void override final;
    auto instrMarker(int limit) -> void override final;

  private:
    std::string logMsg; // reuse to save on heap allocations space
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<gzofstream> gzfile;
};


auto flushPthread(std::string filePath,
                  ThreadList newThreadsInOrder,
                  SpawnList threadSpawns,
                  BarrierList barrierParticipants) -> void;

auto flushStats(std::string filePath, ThreadStatMap allThreadsStats) -> void;

}; //end namespace STGen

#endif
