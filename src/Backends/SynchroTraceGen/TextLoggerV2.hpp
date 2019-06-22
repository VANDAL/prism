#ifndef STGEN_TEXT_LOGGER_V2_H
#define STGEN_TEXT_LOGGER_V2_H

#include "Utils/PrismLog.hpp"
#include "Utils/FileLogger.hpp"
#include "STLogger.hpp"
#include "spdlog/spdlog.h"

using PrismLog::info;
namespace STGen
{

/**
 * XXX The spacing in the format is MANDATORY.
 * Parsers expect the spacing to be exact. This includes any trailing spaces.
 * This is to ensure efficiency in parsing, which becomes important in large
 * trace files. Any modifications will require additional modifications in
 * supporting parsers.
 */

class TextLoggerV2Compressed : public STLoggerCompressed
{
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */

    using Base = STLoggerCompressed;
  public:
    TextLoggerV2Compressed(TID tid, const std::string& outputPath);
    TextLoggerV2Compressed(const TextLoggerV2Compressed& other) = delete;
    ~TextLoggerV2Compressed() override final;

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


class TextLoggerV2Uncompressed : public STLoggerUncompressed
{
    /* Uses spdlog logging library to asynchronously log to a text file.
     * The format is a custom format.
     * Each new logger writes to a new file */

    using Base = STLoggerCompressed;
  public:
    TextLoggerV2Uncompressed(TID tid, const std::string& outputPath);
    TextLoggerV2Uncompressed(const TextLoggerV2Uncompressed& other) = delete;
    ~TextLoggerV2Uncompressed() override final;

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

}; //end namespace STGen

#endif
