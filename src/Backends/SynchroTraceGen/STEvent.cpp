#include "STEvent.hpp"

namespace STGen
{

//-----------------------------------------------------------------------------
/** Compute Event **/
auto STCompEventCompressed::updateWrites(const Addr begin, const Addr size) -> void
{
    isActive = true;
    uniqueWriteAddrs.insert(std::make_pair(begin, begin + size - 1));
}

auto STCompEventCompressed::updateReads(const Addr begin, const Addr size) -> void
{
    isActive = true;
    uniqueReadAddrs.insert(std::make_pair(begin, begin + size - 1));
}

auto STCompEventCompressed::incWrites() -> void
{
    isActive = true;
    ++writes;
}

auto STCompEventCompressed::incReads() -> void
{
    isActive = true;
    ++reads;
}

auto STCompEventCompressed::incIOP() -> void
{
    isActive = true;
    ++iops;
}

auto STCompEventCompressed::incFLOP() -> void
{
    isActive = true;
    ++flops;
}

auto STCompEventCompressed::reset() -> void
{
    isActive = false;
    iops = 0;
    flops = 0;
    writes = 0;
    reads = 0;
    uniqueWriteAddrs.clear();
    uniqueReadAddrs.clear();
}


auto STCompEventUncompressed::incIOP() -> void
{
    isActive = true;
    ++iops;
}

auto STCompEventUncompressed::incFLOP() -> void
{
    isActive = true;
    ++flops;
}

auto STCompEventUncompressed::reset() -> void
{
    isActive = false;
    iops = 0;
    flops = 0;
}



//-----------------------------------------------------------------------------
/** Communication Event **/
auto STCommEventCompressed::addEdge(const TID writer, const EID writer_event,
                                    const Addr addr) -> void
{
    isActive = true;

    if (comms.empty())
    {
        comms.push_back(std::make_tuple(writer, writer_event, AddrSet(std::make_pair(addr, addr))));
    }
    else
    {
        for (auto &edge : comms)
        {
            if (std::get<0>(edge) == writer && std::get<1>(edge) == writer_event)
            {
                std::get<2>(edge).insert(std::make_pair(addr, addr));
                return;
            }
        }

        comms.push_back(std::make_tuple(writer, writer_event, AddrSet(std::make_pair(addr, addr))));
    }
}


auto STCommEventCompressed::reset() -> void
{
    isActive = false;
    comms.clear();
}

}; //end namespace STGen
