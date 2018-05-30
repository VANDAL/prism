#include "Utils/PrismLog.hpp"
#include "StgenCapnpParser.hpp"
#include "argparse/argparse.hpp"

using Event = EventStreamCompressed::Event;
using SyncType = EventStreamCompressed::Event::SyncType;


auto parseStgenCapnp(std::string fpath) {
    capnp::ReaderOptions options;
    options.traversalLimitInWords = (1UL << 63);

    for (auto message : PackedMultipleMessageGenerator(fpath, options)) {
        auto events = message->getRoot<EventStreamCompressed>();
        for (auto event : events.getEvents()) {
            switch (event.which()) {
            case Event::COMP:
                {
                    auto comp = event.getComp();

                    auto iops [[maybe_unused]]   = comp.getIops();
                    auto flops [[maybe_unused]]  = comp.getFlops();
                    auto reads [[maybe_unused]]  = comp.getReads();
                    auto writes [[maybe_unused]] = comp.getWrites();

                    if (comp.hasReadAddrs()) {
                        for (auto addr : comp.getReadAddrs()) {
                            (void)addr;
                        }
                    }

                    if (comp.hasWriteAddrs()) {
                        for (auto addr : comp.getWriteAddrs()) {
                            (void)addr;
                        }
                    }
                }
                break;
            case Event::COMM:
                if (event.getComm().hasEdges()) {
                    for (auto comm : event.getComm().getEdges()) {
                        auto producerThread [[maybe_unused]] = comm.getProducerThread();
                        auto producerEvent [[maybe_unused]]  = comm.getProducerEvent();
                        for (auto addr : comm.getAddrs()) {
                            auto start [[maybe_unused]] = addr.getStart();
                            auto end [[maybe_unused]]   = addr.getEnd();
                        }
                    }
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
