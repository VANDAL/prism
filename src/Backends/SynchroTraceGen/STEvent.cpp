#include "STEvent.hpp"

namespace STGen
{

////////////////////////////////////////////////////////////
// SynchroTrace - Compute Event
////////////////////////////////////////////////////////////
void STCompEvent::updateWrites(const Addr begin, const Addr size)
{
    isActive = true;
    uniqueWriteAddrs.insert(std::make_pair(begin, begin + size - 1));
}

void STCompEvent::updateReads(const Addr begin, const Addr size)
{
    isActive = true;
    uniqueReadAddrs.insert(std::make_pair(begin, begin + size - 1));
}

void STCompEvent::incWrites()
{
    isActive = true;
    ++writes;
}

void STCompEvent::incReads()
{
    isActive = true;
    ++reads;
}

void STCompEvent::incIOP()
{
    isActive = true;
    ++iops;
}

void STCompEvent::incFLOP()
{
    isActive = true;
    ++flops;
}

void STCompEvent::reset()
{
    isActive = false;
    iops = 0;
    flops = 0;
    writes = 0;
    reads = 0;
    uniqueWriteAddrs.clear();
    uniqueReadAddrs.clear();
}


////////////////////////////////////////////////////////////
// SynchroTrace - Communication Event
////////////////////////////////////////////////////////////
void STCommEvent::addEdge(const TID writer, const EID writer_event, const Addr addr)
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


void STCommEvent::reset()
{
    isActive = false;
    comms.clear();
}


////////////////////////////////////////////////////////////
// SynchroTrace - Context Event (Instruction)
////////////////////////////////////////////////////////////
void STInstrEvent::append(Addr addr)
{
    isActive = true;
    addrs.push_back(addr);
}

void STInstrEvent::reset()
{
    isActive = false;
    addrs.clear();
}

}; //end namespace STGen
