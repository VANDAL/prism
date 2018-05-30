#include "Utils/PrismLog.hpp"
#include "StgenCapnpParser.hpp"
#include "argparse/argparse.hpp"

using Event = EventStreamUncompressed::Event;
using MemType = EventStreamUncompressed::Event::MemType;
using SyncType = EventStreamUncompressed::Event::SyncType;


auto parseStgenCapnp(std::string fpath) {
    capnp::ReaderOptions options;
    options.traversalLimitInWords = (1UL << 63);

    for (auto message : PackedMultipleMessageGenerator(fpath, options)) {
        auto events = message->getRoot<EventStreamUncompressed>();
        for (auto event : events.getEvents()) {
            switch (event.which()) {
            case Event::COMP:
                {
                    auto comp = event.getComp();

                    auto iops [[maybe_unused]]      = comp.getIops();
                    auto flops [[maybe_unused]]     = comp.getFlops();
                    auto startAddr [[maybe_unused]] = comp.getStartAddr();
                    auto endAddr [[maybe_unused]]   = comp.getEndAddr();

                    switch (comp.getMem()) {
                    case MemType::READ:
                        break;
                    case MemType::WRITE:
                        break;
                    case MemType::NONE:
                        break;
                    }
                }
                break;
            case Event::COMM:
                {
                    auto comm = event.getComm();

                    auto producerThread [[maybe_unused]] = comm.getProducerThread();
                    auto producerEvent [[maybe_unused]]  = comm.getProducerEvent();
                    auto startAddr [[maybe_unused]]      = comm.getStartAddr();
                    auto endAddr [[maybe_unused]]        = comm.getEndAddr();
                }
                break;
            case Event::SYNC:
                {
                    auto sync = event.getSync();
                    auto args [[maybe_unused]] = sync.getArgs();

                    switch (sync.getType()) {
                    case SyncType::SPAWN:
                        break;
                    case SyncType::JOIN:
                        break;
                    case SyncType::BARRIER:
                        break;
                    default: // rest of cases...
                        break;
                    }
                }
                break;
            case Event::MARKER:
                {
                    auto marker = event.getMarker();
                    auto count [[maybe_unused]] = marker.getCount();
                }
                break;
            default:
                assert(false);
                break;
            }
        }
    }
}


int main(int argc, const char* argv[]) {
    ArgumentParser argparser;
    argparser.addFinalArgument("tracepath");
    argparser.parse(argc, argv);

    parseStgenCapnp(argparser.retrieve<std::string>("tracepath"));
}
